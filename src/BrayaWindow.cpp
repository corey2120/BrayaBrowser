#include "BrayaWindow.h"
#include "BrayaTab.h"
#include "BrayaSettings.h"
#include "BrayaCustomization.h"
#include "BrayaHistory.h"
#include "BrayaDownloads.h"
#include "BrayaBookmarks.h"
#include "BrayaPasswordManager.h"
#include "TabGroup.h"
#include "extensions/BrayaExtensionManager.h"
#include "extensions/BrayaWebExtension.h"
#include "extensions/ExtensionInstaller.h"
#include "extensions/BrayaExtensionAPI.h"
#include "adblocker/BrayaAdBlocker.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <ctime>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#include <jsc/jsc.h>
#include <json-glib/json-glib.h>

// Structs for extension popup IPC
struct PopupMessageData {
    BrayaWindow* window;
    WebKitWebView* popupView;
    std::string extensionId;
};

struct PopupResponseCallbackData {
    WebKitWebView* popupView;
    int callbackId;
};

// Static callback for popup message response
static void on_popup_response_callback(GObject* object, GAsyncResult* result, gpointer user_data) {
    PopupResponseCallbackData* cbData = static_cast<PopupResponseCallbackData*>(user_data);

    GError* error = nullptr;
    JSCValue* value = webkit_web_view_evaluate_javascript_finish(
        WEBKIT_WEB_VIEW(object), result, &error);

    if (value && !error) {
        char* response = jsc_value_to_json(value, 0);

        // Send response back to popup
        std::string responseScript = "window.__handleExtensionMessageResponse(" +
            std::to_string(cbData->callbackId) + ", " + std::string(response) + ");";

        webkit_web_view_evaluate_javascript(cbData->popupView,
            responseScript.c_str(), -1, nullptr, nullptr, nullptr, nullptr, nullptr);

        g_free(response);
    } else if (error) {
        std::cerr << "✗ Error evaluating JS: " << error->message << std::endl;
        g_error_free(error);
    }

    delete cbData;
}

// Static callback for popup message handling
static void on_popup_message_received(WebKitUserContentManager* manager,
                                       JSCValue* value,
                                       gpointer user_data) {
    PopupMessageData* data = static_cast<PopupMessageData*>(user_data);

    if (!value || !jsc_value_is_object(value)) return;

    JSCValue* typeValue = jsc_value_object_get_property(value, "type");
    if (!typeValue) return;

    char* type = jsc_value_to_string(typeValue);
    if (!type) return;

    if (strcmp(type, "runtimeSendMessage") == 0) {
        // Get message details
        JSCValue* messageValue = jsc_value_object_get_property(value, "message");
        JSCValue* callbackIdValue = jsc_value_object_get_property(value, "callbackId");

        if (!messageValue || !callbackIdValue) {
            g_free(type);
            return;
        }

        char* messageJson = jsc_value_to_json(messageValue, 0);
        int callbackId = jsc_value_to_int32(callbackIdValue);

        std::cout << "🔌 Popup message: " << messageJson << std::endl;

        // Get the extension's background page
        auto* ext = data->window->extensionManager->getWebExtension(data->extensionId);
        if (ext && ext->getBackgroundPage()) {
            // Forward message to background page
            std::string script = "if (window.__messageListeners) { "
                "window.__messageListeners.forEach(function(listener) { "
                "listener(" + std::string(messageJson) + ", {}, function(response) { "
                "console.log('[Background] Sending response:', response); "
                "}); }); }";

            PopupResponseCallbackData* cbData = new PopupResponseCallbackData{data->popupView, callbackId};

            webkit_web_view_evaluate_javascript(ext->getBackgroundPage(),
                script.c_str(), -1, nullptr, nullptr, nullptr,
                on_popup_response_callback, cbData);
        }

        g_free(messageJson);
    }

    g_free(type);
}

// Static callback for extension button clicks
static void on_extension_button_clicked(GtkButton* button, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(g_object_get_data(G_OBJECT(button), "window"));
    const char* extensionId = (const char*)g_object_get_data(G_OBJECT(button), "extension-id");

    if (!window || !window->extensionManager || !extensionId) {
        return;
    }

    auto* extension = window->extensionManager->getWebExtension(extensionId);
    if (!extension) {
        return;
    }

    BrowserAction action = extension->getBrowserAction();
    std::cout << "🔌 Extension button clicked: " << extension->getName() << std::endl;

    // If there's a popup, show it
    if (!action.default_popup.empty()) {
        std::cout << "  Opening popup: " << action.default_popup << std::endl;

        // Create popup window
        GtkWidget* popupWindow = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(popupWindow), extension->getName().c_str());
        gtk_window_set_default_size(GTK_WINDOW(popupWindow), 400, 600);
        gtk_window_set_transient_for(GTK_WINDOW(popupWindow), GTK_WINDOW(window->getWindow()));
        gtk_window_set_modal(GTK_WINDOW(popupWindow), TRUE);

        // Get the WebKit context from the extension manager
        WebKitWebContext* context = window->extensionManager->getWebContext();

        // Create WebView with the same context as main browser
        WebKitWebView* popupView;
        if (context) {
            popupView = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW,
                                                      "web-context", context,
                                                      nullptr));
            std::cout << "  ✓ Created popup WebView with extension context" << std::endl;
        } else {
            popupView = WEBKIT_WEB_VIEW(webkit_web_view_new());
            std::cout << "  ⚠️  Created popup WebView with default context" << std::endl;
        }

        // Enable JavaScript and other necessary features
        WebKitSettings* settings = webkit_web_view_get_settings(popupView);
        webkit_settings_set_enable_javascript(settings, TRUE);
        webkit_settings_set_enable_write_console_messages_to_stdout(settings, TRUE);
        webkit_settings_set_allow_file_access_from_file_urls(settings, TRUE);
        webkit_settings_set_allow_universal_access_from_file_urls(settings, TRUE);

        // Inject browser APIs using UserScript to guarantee it runs BEFORE page scripts
        std::cout << "  → Setting up API injection for popup..." << std::endl;

        // Get the extension API JavaScript (popups use content script API, not background)
        std::string apiScript = BrayaExtensionAPI::getContentScriptAPI();

        // Create a user script that will inject at document start (before any page content)
        WebKitUserContentManager* popupContentManager = webkit_web_view_get_user_content_manager(popupView);

        // Add message handler for popup-background IPC
        PopupMessageData* popupData = new PopupMessageData{window, popupView, extensionId};
        g_signal_connect_data(popupContentManager, "script-message-received::extensionMessage",
                         G_CALLBACK(on_popup_message_received), popupData,
                         [](gpointer data, GClosure*) {
                             delete static_cast<PopupMessageData*>(data);
                         }, GConnectFlags(0));

        webkit_user_content_manager_register_script_message_handler(popupContentManager, "extensionMessage", nullptr);
        WebKitUserScript* userScript = webkit_user_script_new(
            apiScript.c_str(),
            WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
            WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,  // CRITICAL: Inject BEFORE page scripts
            nullptr,  // Allow list (nullptr = all pages)
            nullptr   // Block list
        );

        webkit_user_content_manager_add_script(popupContentManager, userScript);
        webkit_user_script_unref(userScript);

        // Inject the extension manifest
        std::string manifestScript = "window.__extensionManifest = " + extension->getManifestJson() + ";";
        WebKitUserScript* manifestUserScript = webkit_user_script_new(
            manifestScript.c_str(),
            WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
            WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
            nullptr,
            nullptr
        );
        webkit_user_content_manager_add_script(popupContentManager, manifestUserScript);
        webkit_user_script_unref(manifestUserScript);

        std::cout << "  ✓ Browser APIs will inject at document start (before page scripts)" << std::endl;

        // CRITICAL: Inject polyfills synchronously BEFORE loading popup
        // This ensures they're available when extension scripts load
        std::string polyfill = R"(
            // Polyfill for requestIdleCallback
            if (!window.requestIdleCallback) {
                window.requestIdleCallback = function(callback) {
                    const start = Date.now();
                    return setTimeout(function() {
                        callback({
                            didTimeout: false,
                            timeRemaining: function() {
                                return Math.max(0, 50 - (Date.now() - start));
                            }
                        });
                    }, 1);
                };
            }
            if (!window.cancelIdleCallback) {
                window.cancelIdleCallback = function(id) { clearTimeout(id); };
            }
            // Also set on self for worker-like contexts
            if (typeof self !== 'undefined') {
                self.requestIdleCallback = window.requestIdleCallback;
                self.cancelIdleCallback = window.cancelIdleCallback;
            }
        )";
        webkit_web_view_evaluate_javascript(popupView, polyfill.c_str(), -1, nullptr, nullptr, nullptr, nullptr, nullptr);

        // Load popup HTML using chrome-extension:// protocol
        // Extract numeric ID from extension ID (e.g., "extension_1763035889" -> "1763035889")
        std::string extensionId = extension->getId();
        size_t underscorePos = extensionId.find('_');
        std::string numericId = (underscorePos != std::string::npos) ?
                                extensionId.substr(underscorePos + 1) : extensionId;

        std::string popupUri = "chrome-extension://" + numericId + "/" + action.default_popup;

        std::cout << "  Loading popup from: " << popupUri << std::endl;
        webkit_web_view_load_uri(popupView, popupUri.c_str());

        // Add to window
        GtkWidget* scrolled = gtk_scrolled_window_new();
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), GTK_WIDGET(popupView));
        gtk_window_set_child(GTK_WINDOW(popupWindow), scrolled);

        gtk_window_present(GTK_WINDOW(popupWindow));
    } else {
        // No popup - just trigger the extension
        std::cout << "  No popup defined for this extension" << std::endl;
    }
}

BrayaWindow::BrayaWindow(GtkApplication* app)
    : activeTabIndex(-1), nextTabId(1), showBookmarksBar(true), nextGroupId(1),
      isSplitView(false), splitHorizontal(true), activeTabIndexPane2(-1),
      settings(std::make_unique<BrayaSettings>()),
      history(std::make_unique<BrayaHistory>()),
      downloads(std::make_unique<BrayaDownloads>()),
      bookmarksManager(std::make_unique<BrayaBookmarks>()),
      passwordManager(std::make_unique<BrayaPasswordManager>()),
      extensionManager(std::make_unique<BrayaExtensionManager>()),
      adBlocker(std::make_unique<BrayaAdBlocker>()),
      cssProvider(nullptr),
      statusLabel(nullptr),
      statusBar(nullptr),
      tabStack2(nullptr),
      splitPane(nullptr),
      window(nullptr),
      bookmarksBar(nullptr),
      memoryLabel(nullptr),
      memoryTimerSourceId(0) {
    
    g_print("Creating Braya window...\n");
    
    // Create main window - let GTK handle decorations with headerbar
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Braya Browser");
    gtk_window_set_default_size(GTK_WINDOW(window), 1400, 900);

    // Set window icon (will show in taskbar/window list)
    // In GTK4, the icon name is used by the window manager
    GtkIconTheme* icon_theme = gtk_icon_theme_get_for_display(gdk_display_get_default());

    // For development: Add local icon search paths first
    std::vector<std::string> iconDirs = {
        "../resources/icons",  // From build directory
        "resources/icons",     // From project root
        "/home/cobrien/Projects/braya-browser-cpp/resources/icons"  // Absolute path
    };

    bool iconPathRegistered = false;
    for (const auto& iconDir : iconDirs) {
        struct stat sb;
        if (stat(iconDir.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
            gtk_icon_theme_add_search_path(icon_theme, iconDir.c_str());
            g_print("✓ Added icon search path: %s\n", iconDir.c_str());
            iconPathRegistered = true;
        }
    }
    if (!iconPathRegistered) {
        g_warning("No icon search paths available – falling back to system theme only");
    }

    // Try setting icon based on application ID first (for development)
    const char* app_id = g_application_get_application_id(G_APPLICATION(app));
    bool iconSet = false;
    if (app_id && gtk_icon_theme_has_icon(icon_theme, app_id)) {
        gtk_window_set_icon_name(GTK_WINDOW(window), app_id);
        g_print("✓ Set window icon to application ID: %s\n", app_id);
        iconSet = true;
    }

    if (!iconSet && gtk_icon_theme_has_icon(icon_theme, "braya-browser")) {
        gtk_window_set_icon_name(GTK_WINDOW(window), "braya-browser");
        g_print("✓ Set window icon to: braya-browser\n");
        iconSet = true;
    }

    if (!iconSet) {
        gtk_window_set_icon_name(GTK_WINDOW(window), "application-x-executable");
        g_warning("No branded icon found, falling back to generic executable icon");
    }

    // Store BrayaWindow instance pointer on the GTK window for access in callbacks
    g_object_set_data(G_OBJECT(window), "braya-window-instance", this);

    // Don't remove decorations - we'll use headerbar for Firefox-style controls

    g_print("Window created\n");
    
    // Connect theme callback
    settings->setThemeCallback([this](int themeId) {
        this->applyTheme(themeId);
    });

    // Connect download status callback
    downloads->setDownloadStatusCallback([this](int activeCount) {
        this->updateDownloadsButton(activeCount);
    });

    // Setup CSS
    setupCSS();

    g_print("CSS loaded\n");

    // Apply saved customization theme (colors, presets, etc.)
    {
        static BrayaCustomization* customization = nullptr;
        if (!customization) {
            customization = new BrayaCustomization();
        }
        customization->applyTheme(window);
        g_print("✓ Applied saved customization theme\n");
    }

    // Build UI
    setupUI();

    g_print("UI built\n");

    // Initialize extension system BEFORE creating any tabs
    // This ensures web extensions are loaded before web views are created

    // 🎨 Favicon Fix: Ensure data directories exist for WebKit
    // WebKit will automatically use these for favicon storage
    const char* dataDir = g_build_filename(g_get_user_data_dir(), "braya-browser", "webkit", NULL);
    const char* cacheDir = g_build_filename(g_get_user_cache_dir(), "braya-browser", "webkit", NULL);
    const char* iconDbDir = g_build_filename(g_get_user_data_dir(), "braya-browser", "webkit", "icondatabase", NULL);

    g_mkdir_with_parents(dataDir, 0700);
    g_mkdir_with_parents(cacheDir, 0700);
    g_mkdir_with_parents(iconDbDir, 0700);

    std::cout << "🎨 Created WebKit data directories for favicon support" << std::endl;
    std::cout << "  Data: " << dataDir << std::endl;
    std::cout << "  Cache: " << cacheDir << std::endl;
    std::cout << "  Icon DB: " << iconDbDir << std::endl;

    WebKitWebContext* context = webkit_web_context_get_default();

    // Note: WebKit 2.50+ automatically manages favicon database
    // The directories we created above will be used automatically
    std::cout << "✓ WebKit will use favicon database at: " << iconDbDir << std::endl;

    // ⚡ PERFORMANCE: Optimize for smooth scrolling and responsiveness
    webkit_web_context_set_cache_model(context, WEBKIT_CACHE_MODEL_WEB_BROWSER);

    extensionManager->initialize(context);
    settings->setWebContext(context);

    // 🧹 Memory optimization: WebKit will handle automatic memory management
    // We'll implement session data limits to prevent unbounded growth

    // 💤 Phase 2: Tab suspension timer (check every 30 seconds for inactive tabs)
    g_timeout_add_seconds(30, [](gpointer data) -> gboolean {
        BrayaWindow* window = static_cast<BrayaWindow*>(data);
        time_t now = time(nullptr);

        // Suspend tabs that have been inactive for 10+ minutes (600 seconds)
        for (size_t i = 0; i < window->tabs.size(); i++) {
            // Don't suspend the active tab in main pane
            if ((int)i == window->activeTabIndex) continue;

            // 🔧 Don't suspend the active tab in split view pane 2
            if (window->isSplitView && (int)i == window->activeTabIndexPane2) continue;

            // Don't suspend pinned tabs
            if (window->tabs[i]->isPinned()) continue;

            // Don't suspend already suspended tabs
            if (window->tabs[i]->isSuspended()) continue;

            // Check if tab has been inactive for 10 minutes
            // For now, suspend all inactive tabs after 1 check cycle (30 seconds for testing)
            // TODO: Add proper time tracking per tab
            window->tabs[i]->suspend();
        }

        return G_SOURCE_CONTINUE;  // Keep running
    }, this);

    // 📊 Memory indicator timer (update every 10 seconds)
    // First update happens after 10 seconds to ensure all components are initialized
    // Store the source ID so we can remove it in the destructor
    memoryTimerSourceId = g_timeout_add_seconds(10, [](gpointer data) -> gboolean {
        BrayaWindow* window = static_cast<BrayaWindow*>(data);
        window->updateMemoryIndicator();
        return G_SOURCE_CONTINUE;  // Keep running
    }, this);

    // Pass extension manager to settings so it can manage extensions
    settings->setExtensionManager(extensionManager.get());

    // Initialize ad-blocker
    adBlocker->initialize();
    adBlocker->enable();  // Enable by default with STANDARD security level

    // Pass ad-blocker to settings so it can be controlled via UI
    settings->setAdBlocker(adBlocker.get());

    // Pass password manager to settings so it can be accessed via Passwords tab
    settings->setPasswordManager(passwordManager.get());

    // Update shield button with current blocked count
    updateAdBlockerShield();

    // Set callback to update extension buttons when extensions are installed/removed
    settings->setExtensionChangeCallback([this]() {
        std::cout << "🔄 Extension change detected, updating toolbar buttons..." << std::endl;
        updateExtensionButtons();
    });

    // Load web extension .so
    // Try installed location first, fall back to build directory for development
    std::string installedExtPath = "/usr/lib/braya-browser/web-extensions/libbraya-web-extension.so";
    std::string buildExtPath = "./build/libbraya-web-extension.so";
    
    if (g_file_test(installedExtPath.c_str(), G_FILE_TEST_EXISTS)) {
        extensionManager->loadNativeExtension(installedExtPath);
    } else {
        extensionManager->loadNativeExtension(buildExtPath);
    }

    // Auto-load saved extensions from config
    std::cout << "\n📦 Loading saved extensions..." << std::endl;
    settings->loadExtensionStates();

    // Load test extension if no extensions are installed
    if (extensionManager->getWebExtensions().empty()) {
        std::cout << "📦 Loading test WebExtension..." << std::endl;
        if (extensionManager->loadWebExtension("./extensions/hello-world")) {
            std::cout << "✓ Test extension loaded successfully!\n" << std::endl;
            settings->saveExtensionStates();  // Save it for next time
        } else {
            std::cerr << "⚠️  Failed to load test extension\n" << std::endl;
        }
    }

    // Update extension buttons in toolbar
    updateExtensionButtons();

    // Restore previous session, or create first tab if no session exists
    restoreSession();
    if (tabs.empty()) {
        // No saved session - create default tab
        createTab();
    }
    
    // Setup download handling using WebKitNetworkSession (WebKitGTK 6.0+)
    // In WebKitGTK 6.0, WebKitWebContext::download-started was removed
    // and replaced with WebKitNetworkSession::download-started
    if (!tabs.empty()) {
        std::cout << "🔽 Setting up download handler..." << std::endl;

        // Get the network session from the web view
        WebKitWebView* webView = tabs[0]->getWebView();
        WebKitNetworkSession* session = webkit_web_view_get_network_session(webView);

        if (session) {
            g_signal_connect(session, "download-started",
                G_CALLBACK(+[](WebKitNetworkSession* session, WebKitDownload* download, gpointer data) {
                    std::cout << "📥 Download started!" << std::endl;
                    BrayaWindow* window = static_cast<BrayaWindow*>(data);

                    // Get the download request to check the filename
                    WebKitURIRequest* request = webkit_download_get_request(download);
                    const char* uri = webkit_uri_request_get_uri(request);
                    std::string url(uri ? uri : "");

                    std::cout << "   Download URL: " << url << std::endl;

                    // Check if this is an extension file (.xpi or .crx)
                    bool isExtension = (url.find(".xpi") != std::string::npos ||
                                       url.find(".crx") != std::string::npos);

                    if (isExtension) {
                        std::cout << "🔌 Extension file detected! Auto-installing..." << std::endl;

                        // Cancel the download since we'll install directly
                        webkit_download_cancel(download);

                        // Validate window and extensionManager pointers
                        if (!window) {
                            std::cerr << "❌ Window pointer is null!" << std::endl;
                            return;
                        }

                        if (!window->extensionManager) {
                            std::cerr << "❌ ExtensionManager is null!" << std::endl;
                            return;
                        }

                        std::cout << "✓ Pointers validated, scheduling installation..." << std::endl;

                        // Copy URL for the idle callback
                        std::string* urlCopy = new std::string(url);

                        // Schedule installation on GTK main thread to avoid thread-safety issues
                        g_idle_add(+[](gpointer data) -> gboolean {
                            // Get URL from the pair
                            auto* pair = static_cast<std::pair<BrayaWindow*, std::string*>*>(data);
                            BrayaWindow* win = pair->first;
                            std::string* urlPtr = pair->second;
                            std::string url = *urlPtr;

                            delete urlPtr;
                            delete pair;

                            std::cout << "✓ Running on GTK main thread..." << std::endl;

                            // Validate window pointer is still valid
                            if (!win) {
                                std::cerr << "❌ Window pointer became null!" << std::endl;
                                return FALSE;
                            }

                            std::cout << "✓ Window pointer: " << (void*)win << std::endl;

                            // Try to validate the pointer is actually a BrayaWindow by checking window widget
                            if (!win->window || !GTK_IS_WINDOW(win->window)) {
                                std::cerr << "❌ Window pointer is INVALID - not pointing to a valid BrayaWindow!" << std::endl;
                                std::cerr << "   win->window = " << (void*)(win->window) << std::endl;
                                return FALSE;
                            }

                            std::cout << "✓ Window object validated via GTK window check" << std::endl;

                            // Validate extensionManager
                            if (!win->extensionManager) {
                                std::cerr << "❌ ExtensionManager became null!" << std::endl;
                                return FALSE;
                            }

                            std::cout << "✓ ExtensionManager valid" << std::endl;

                            // Show "Installing..." notification (now safe on main thread)
                            std::cout << "→ About to check statusLabel..." << std::endl;

                            GtkWidget* label = win->statusLabel;
                            std::cout << "→ Got statusLabel pointer: " << (void*)label << std::endl;

                            if (label) {
                                std::cout << "→ statusLabel is not NULL, checking if it's a GtkLabel..." << std::endl;
                                if (GTK_IS_LABEL(label)) {
                                    std::cout << "✓ Updating status label..." << std::endl;
                                    gtk_label_set_text(GTK_LABEL(label), "📥 Installing extension...");
                                } else {
                                    std::cout << "⚠️  statusLabel is not a GtkLabel" << std::endl;
                                }
                            } else {
                                std::cout << "⚠️  Status label is NULL" << std::endl;
                            }

                            // Install the extension directly from the URL
                            std::cout << "✓ Creating ExtensionInstaller..." << std::endl;
                            ExtensionInstaller installer(win->extensionManager.get());

                            std::cout << "✓ Calling installFromUrl..." << std::endl;
                            installer.installFromUrl(url, [win](bool success, const std::string& message) {
                                if (success) {
                                    std::cout << "✅ Extension auto-installed: " << message << std::endl;

                                    // Show success message (safely)
                                    if (win && win->statusLabel && GTK_IS_LABEL(win->statusLabel)) {
                                        gtk_label_set_text(GTK_LABEL(win->statusLabel), "✅ Extension installed successfully!");

                                        // Clear message after 3 seconds
                                        g_timeout_add_seconds(3, +[](gpointer data) -> gboolean {
                                            if (data && GTK_IS_LABEL(data)) {
                                                gtk_label_set_text(GTK_LABEL(data), "");
                                            }
                                            return FALSE;
                                        }, win->statusLabel);
                                    }

                                    win->updateExtensionButtons();
                                    win->settings->refreshExtensionsList();
                                    win->settings->saveExtensionStates();
                                } else {
                                    std::cerr << "❌ Extension auto-install failed: " << message << std::endl;

                                    // Show error message (safely)
                                    if (win && win->statusLabel && GTK_IS_LABEL(win->statusLabel)) {
                                        std::string errorMsg = "❌ Installation failed: " + message;
                                        gtk_label_set_text(GTK_LABEL(win->statusLabel), errorMsg.c_str());

                                        // Clear after 5 seconds
                                        g_timeout_add_seconds(5, +[](gpointer data) -> gboolean {
                                            if (data && GTK_IS_LABEL(data)) {
                                                gtk_label_set_text(GTK_LABEL(data), "");
                                            }
                                            return FALSE;
                                        }, win->statusLabel);
                                    }
                                }
                            });

                            return FALSE;  // Remove idle callback after running once
                        }, new std::pair<BrayaWindow*, std::string*>(window, urlCopy));
                    } else if (window->downloads) {
                        // Regular download - handle normally
                        window->downloads->handleDownload(download);
                    }
                }), this);

            std::cout << "✓ Download handler connected to NetworkSession" << std::endl;
        } else {
            std::cerr << "⚠️  Could not get NetworkSession for downloads" << std::endl;
        }

        // Connect user message handler for receiving messages from web extension
        if (!tabs.empty() && tabs[0]) {
            WebKitWebView* webView = tabs[0]->getWebView();
            g_signal_connect(webView, "user-message-received",
                G_CALLBACK(BrayaExtensionManager::onUserMessageReceived), this);
            g_print("✓ Extension message handler connected\n");
        }
    }

    g_print("First tab created\n");
    
    // Connect keyboard shortcuts
    GtkEventController* key_controller = gtk_event_controller_key_new();
    g_signal_connect(key_controller, "key-pressed", G_CALLBACK(onKeyPress), this);
    gtk_widget_add_controller(window, key_controller);

    // Connect to close-request signal to save session
    g_signal_connect(window, "close-request", G_CALLBACK(+[](GtkWindow* win, gpointer data) -> gboolean {
        BrayaWindow* window = static_cast<BrayaWindow*>(data);
        window->saveSession();
        return FALSE;  // Allow window to close
    }), this);

    g_print("Braya window initialization complete!\n");
}

BrayaWindow::~BrayaWindow() {
    // 📊 Remove memory indicator timer
    if (memoryTimerSourceId != 0) {
        g_source_remove(memoryTimerSourceId);
        memoryTimerSourceId = 0;
    }

    // Save session before closing
    saveSession();

    // 🛡️ MEMORY FIX: GObjectPtr handles cleanup automatically
    // No need to manually unref - destructors handle it
    faviconCache.clear();

    tabs.clear();
}

std::string BrayaWindow::getResourcePath(const std::string& filename) {
    // Try multiple locations for resources
    std::vector<std::string> searchPaths = {
        // Development build location
        std::string(g_get_current_dir()) + "/resources/" + filename,
        // Relative to current directory
        "resources/" + filename,
        // System installation location
        "/usr/share/braya-browser/resources/" + filename,
        // Local installation
        std::string(getenv("HOME") ? getenv("HOME") : "") + "/.local/share/braya-browser/resources/" + filename
    };
    
    // Check each path
    for (const auto& path : searchPaths) {
        struct stat buffer;
        if (stat(path.c_str(), &buffer) == 0) {
            return path;
        }
    }
    
    // Fallback to system path even if file doesn't exist
    return "/usr/share/braya-browser/resources/" + filename;
}

void BrayaWindow::setupCSS() {
    // Load theme based on settings
    int theme = settings->getTheme();
    std::string themePath;
    
    switch(theme) {
        case 0: // DARK
            themePath = getResourcePath("theme-dark.css");
            break;
        case 1: // LIGHT
            themePath = getResourcePath("theme-light.css");
            break;
        case 2: // INDUSTRIAL
            themePath = getResourcePath("theme-industrial.css");
            break;
        default:
            themePath = getResourcePath("theme-dark.css");
    }
    
    loadThemeCSS(themePath);
}

void BrayaWindow::loadThemeCSS(const std::string& themePath) {
    // Remove old provider if exists
    if (cssProvider) {
        gtk_style_context_remove_provider_for_display(
            gdk_display_get_default(),
            GTK_STYLE_PROVIDER(cssProvider)
        );
        g_object_unref(cssProvider);
    }
    
    // Create new provider
    cssProvider = gtk_css_provider_new();
    GFile* file = g_file_new_for_path(themePath.c_str());
    gtk_css_provider_load_from_file(cssProvider, file);
    g_object_unref(file);
    
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(cssProvider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    
    std::cout << "✓ Loaded theme: " << themePath << std::endl;

    // Load userChrome.css (Zen-style)
    loadUserChrome();
}

// Zen Browser-style userChrome.css support
void BrayaWindow::loadUserChrome() {
    std::string userChromePath = std::string(g_get_home_dir()) +
        "/.config/braya-browser/chrome/userChrome.css";

    // Check if userChrome.css exists
    if (!g_file_test(userChromePath.c_str(), G_FILE_TEST_EXISTS)) {
        std::cout << "ℹ️  No userChrome.css found (create one in ~/.config/braya-browser/chrome/)" << std::endl;
        return;
    }

    // Load userChrome.css
    GtkCssProvider* userChromeProvider = gtk_css_provider_new();
    GFile* file = g_file_new_for_path(userChromePath.c_str());

    GError* error = nullptr;
    gtk_css_provider_load_from_file(userChromeProvider, file);
    g_object_unref(file);

    if (error) {
        std::cerr << "❌ Error loading userChrome.css: " << error->message << std::endl;
        g_error_free(error);
        return;
    }

    // Apply with highest priority (like Zen Browser)
    GdkDisplay* display = gdk_display_get_default();
    gtk_style_context_add_provider_for_display(
        display,
        GTK_STYLE_PROVIDER(userChromeProvider),
        GTK_STYLE_PROVIDER_PRIORITY_USER  // Higher than theme priority
    );

    std::cout << "✅ Loaded userChrome.css (Zen-style customization)" << std::endl;
}

void BrayaWindow::applyTheme(int themeId) {
    std::string themePath;

    switch(themeId) {
        case 0: // DARK
            themePath = getResourcePath("theme-dark.css");
            break;
        case 1: // LIGHT
            themePath = getResourcePath("theme-light.css");
            break;
        case 2: // INDUSTRIAL
            themePath = getResourcePath("theme-industrial.css");
            break;
        default:
            themePath = getResourcePath("theme-dark.css");
    }

    loadThemeCSS(themePath);
}

// Downloads UI methods

void BrayaWindow::showDownloadsButton() {
    if (downloadsBtn && GTK_IS_WIDGET(downloadsBtn)) {
        gtk_widget_set_visible(downloadsBtn, TRUE);
    }
}

void BrayaWindow::hideDownloadsButton() {
    if (downloadsBtn && GTK_IS_WIDGET(downloadsBtn)) {
        gtk_widget_set_visible(downloadsBtn, FALSE);
    }
}

void BrayaWindow::updateDownloadsButton(int activeCount) {
    if (!downloadsBtn || !GTK_IS_WIDGET(downloadsBtn)) return;

    if (activeCount > 0) {
        // Show button with count badge
        showDownloadsButton();

        // Update tooltip with active download count
        std::string tooltip = "Downloads";
        if (activeCount == 1) {
            tooltip += " (1 active download)";
        } else {
            tooltip += " (" + std::to_string(activeCount) + " active downloads)";
        }
        gtk_widget_set_tooltip_text(downloadsBtn, tooltip.c_str());
    } else {
        // Hide button when no active downloads
        hideDownloadsButton();
    }
}

void BrayaWindow::updateAdBlockerShield() {
    if (!adBlockerShieldBtn || !GTK_IS_WIDGET(adBlockerShieldBtn)) return;
    if (!adBlocker) return;

    BlockingStats stats = adBlocker->getStats();
    int totalBlocked = stats.total_blocked;

    // Update button label — only show count when something has been blocked
    std::string label = totalBlocked > 0 ? "🛡️ " + std::to_string(totalBlocked) : "🛡️";
    gtk_button_set_label(GTK_BUTTON(adBlockerShieldBtn), label.c_str());

    // Update tooltip
    std::string tooltip = "Ad-Blocker";
    if (totalBlocked > 0) {
        tooltip += " (" + std::to_string(totalBlocked) + " blocked";
        if (stats.blocked_today > 0) {
            tooltip += ", " + std::to_string(stats.blocked_today) + " today";
        }
        tooltip += ")";
    }
    gtk_widget_set_tooltip_text(adBlockerShieldBtn, tooltip.c_str());
}

void BrayaWindow::setupUI() {
    // Main horizontal box
    mainBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), mainBox);
    
    // Create sidebar
    createSidebar();
    gtk_box_append(GTK_BOX(mainBox), sidebar);

    // Content area (right side)
    contentBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(mainBox), contentBox);
    gtk_widget_set_hexpand(contentBox, TRUE);
    gtk_widget_set_vexpand(contentBox, TRUE);

    // Create components
    createNavbar();
    // Don't append navbar - it's in the headerbar now!

    // Bookmark bar at top of content area
    createBookmarksBar();
    gtk_box_append(GTK_BOX(contentBox), bookmarksBar);

    // Create split pane container (GtkPaned)
    splitPane = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_wide_handle(GTK_PANED(splitPane), TRUE);
    gtk_widget_set_vexpand(splitPane, TRUE);

    // Primary tab stack for web views
    tabStack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(tabStack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(GTK_STACK(tabStack), 150);
    gtk_widget_set_vexpand(tabStack, TRUE);

    // Secondary tab stack for split view
    tabStack2 = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(tabStack2), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(GTK_STACK(tabStack2), 150);
    gtk_widget_set_vexpand(tabStack2, TRUE);

    // Set up split pane with both stacks
    gtk_paned_set_start_child(GTK_PANED(splitPane), tabStack);
    gtk_paned_set_end_child(GTK_PANED(splitPane), tabStack2);
    gtk_paned_set_resize_start_child(GTK_PANED(splitPane), TRUE);
    gtk_paned_set_resize_end_child(GTK_PANED(splitPane), TRUE);
    gtk_paned_set_shrink_start_child(GTK_PANED(splitPane), FALSE);
    gtk_paned_set_shrink_end_child(GTK_PANED(splitPane), FALSE);

    // Initially hide the second pane (not in split view mode)
    gtk_widget_set_visible(tabStack2, FALSE);

    gtk_box_append(GTK_BOX(contentBox), splitPane);
    
    // Find bar (hidden by default)
    findBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_add_css_class(findBar, "find-bar");
    gtk_widget_set_visible(findBar, FALSE);
    gtk_widget_set_margin_start(findBar, 10);
    gtk_widget_set_margin_end(findBar, 10);
    gtk_widget_set_margin_top(findBar, 5);
    gtk_widget_set_margin_bottom(findBar, 5);
    
    GtkWidget* findLabel = gtk_label_new("Find:");
    gtk_box_append(GTK_BOX(findBar), findLabel);
    
    findEntry = gtk_entry_new();
    gtk_widget_set_size_request(findEntry, 250, -1);
    g_signal_connect(findEntry, "changed", G_CALLBACK(onFindEntryChanged), this);
    g_signal_connect(findEntry, "activate", G_CALLBACK(onFindNextClicked), this);
    gtk_box_append(GTK_BOX(findBar), findEntry);
    
    findMatchLabel = gtk_label_new("");
    gtk_widget_add_css_class(findMatchLabel, "find-match-label");
    gtk_box_append(GTK_BOX(findBar), findMatchLabel);
    
    GtkWidget* findPrevBtn = gtk_button_new_with_label("◀");
    gtk_widget_set_tooltip_text(findPrevBtn, "Previous (Shift+Enter)");
    g_signal_connect(findPrevBtn, "clicked", G_CALLBACK(onFindPrevClicked), this);
    gtk_box_append(GTK_BOX(findBar), findPrevBtn);
    
    GtkWidget* findNextBtn = gtk_button_new_with_label("▶");
    gtk_widget_set_tooltip_text(findNextBtn, "Next (Enter)");
    g_signal_connect(findNextBtn, "clicked", G_CALLBACK(onFindNextClicked), this);
    gtk_box_append(GTK_BOX(findBar), findNextBtn);
    
    GtkWidget* findCloseBtn = gtk_button_new_with_label("✕");
    gtk_widget_set_tooltip_text(findCloseBtn, "Close (Esc)");
    g_signal_connect(findCloseBtn, "clicked", G_CALLBACK(onFindCloseClicked), this);
    gtk_box_append(GTK_BOX(findBar), findCloseBtn);
    
    gtk_box_append(GTK_BOX(contentBox), findBar);

    // 📊 Status bar at the bottom
    createStatusBar();
    gtk_box_append(GTK_BOX(contentBox), statusBar);
}

void BrayaWindow::createSidebar() {
    sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(sidebar, 56, -1);  // Zen-like width
    gtk_widget_add_css_class(sidebar, "sidebar");

    // Tabs scrolled window
    GtkWidget* tabsScroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tabsScroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    
    tabsBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(tabsScroll), tabsBox);
    gtk_box_append(GTK_BOX(sidebar), tabsScroll);
    gtk_widget_set_vexpand(tabsScroll, TRUE);

    // New tab button (compact)
    GtkWidget* newTabBtn = gtk_button_new_with_label("+");
    gtk_widget_set_tooltip_text(newTabBtn, "New Tab (Ctrl+T)");
    gtk_widget_add_css_class(newTabBtn, "new-tab-btn");
    g_signal_connect(newTabBtn, "clicked", G_CALLBACK(onNewTabClicked), this);
    gtk_box_append(GTK_BOX(sidebar), newTabBtn);
    
    // Downloads button - only visible during active downloads
    downloadsBtn = gtk_button_new_from_icon_name("folder-download-symbolic");
    gtk_widget_set_tooltip_text(downloadsBtn, "Downloads (Ctrl+J)");
    gtk_widget_add_css_class(downloadsBtn, "settings-btn");
    g_signal_connect(downloadsBtn, "clicked", G_CALLBACK(onDownloadsClicked), this);
    gtk_widget_set_visible(downloadsBtn, FALSE);  // Hidden by default
    gtk_box_append(GTK_BOX(sidebar), downloadsBtn);

    // Settings button at bottom
    GtkWidget* settingsBtn = gtk_button_new_with_label("⚙");
    gtk_widget_set_tooltip_text(settingsBtn, "Settings");
    gtk_widget_add_css_class(settingsBtn, "settings-btn");
    g_signal_connect(settingsBtn, "clicked", G_CALLBACK(onSettingsClicked), this);
    gtk_box_append(GTK_BOX(sidebar), settingsBtn);
}

void BrayaWindow::createNavbar() {
    // Create headerbar like Firefox - native window controls
    GtkWidget* headerbar = gtk_header_bar_new();
    gtk_widget_add_css_class(headerbar, "titlebar");
    gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(headerbar), TRUE);
    // Remove the default app icon from headerbar (we're using emoji instead)
    gtk_header_bar_set_decoration_layout(GTK_HEADER_BAR(headerbar), ":minimize,maximize,close");
    gtk_window_set_titlebar(GTK_WINDOW(window), headerbar);
    
    // Left side box for all left controls
    GtkWidget* leftBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(leftBox, "navbar-left");
    gtk_widget_set_halign(leftBox, GTK_ALIGN_START);

    // Sidebar toggle button
    GtkWidget* sidebarToggle = gtk_button_new_from_icon_name("view-left-pane-symbolic");
    gtk_widget_set_tooltip_text(sidebarToggle, "Toggle Sidebar (Ctrl+B)");
    gtk_widget_add_css_class(sidebarToggle, "nav-btn");
    g_signal_connect_swapped(sidebarToggle, "clicked", G_CALLBACK(+[](BrayaWindow* win) {
        gboolean visible = gtk_widget_get_visible(win->sidebar);
        gtk_widget_set_visible(win->sidebar, !visible);
    }), this);
    gtk_box_append(GTK_BOX(leftBox), sidebarToggle);

    // Back button - Firefox style
    backBtn = gtk_button_new_from_icon_name("go-previous-symbolic");
    gtk_widget_add_css_class(backBtn, "nav-btn");
    gtk_widget_set_tooltip_text(backBtn, "Back (Alt+Left)");
    gtk_widget_set_sensitive(backBtn, FALSE);
    gtk_widget_set_can_focus(backBtn, FALSE);
    gtk_widget_set_visible(backBtn, TRUE);
    g_signal_connect(backBtn, "clicked", G_CALLBACK(onBackClicked), this);
    gtk_box_append(GTK_BOX(leftBox), backBtn);
    g_print("✓ Back button created\n");
    
    // Forward button - Firefox style
    forwardBtn = gtk_button_new_from_icon_name("go-next-symbolic");
    gtk_widget_add_css_class(forwardBtn, "nav-btn");
    gtk_widget_set_tooltip_text(forwardBtn, "Forward (Alt+Right)");
    gtk_widget_set_sensitive(forwardBtn, FALSE);
    gtk_widget_set_can_focus(forwardBtn, FALSE);
    gtk_widget_set_visible(forwardBtn, TRUE);
    g_signal_connect(forwardBtn, "clicked", G_CALLBACK(onForwardClicked), this);
    gtk_box_append(GTK_BOX(leftBox), forwardBtn);
    g_print("✓ Forward button created\n");
    
    // Reload button
    reloadBtn = gtk_button_new_from_icon_name("view-refresh-symbolic");
    gtk_widget_add_css_class(reloadBtn, "nav-btn");
    gtk_widget_set_tooltip_text(reloadBtn, "Reload (Ctrl+R)");
    gtk_widget_set_can_focus(reloadBtn, FALSE);
    g_signal_connect(reloadBtn, "clicked", G_CALLBACK(onReloadClicked), this);
    gtk_box_append(GTK_BOX(leftBox), reloadBtn);
    
    // Home button
    GtkWidget* homeBtn = gtk_button_new_from_icon_name("go-home-symbolic");
    gtk_widget_add_css_class(homeBtn, "nav-btn");
    gtk_widget_set_tooltip_text(homeBtn, "Home");
    gtk_widget_set_can_focus(homeBtn, FALSE);
    g_signal_connect(homeBtn, "clicked", G_CALLBACK(onHomeClicked), this);
    gtk_box_append(GTK_BOX(leftBox), homeBtn);

    gtk_header_bar_pack_start(GTK_HEADER_BAR(headerbar), leftBox);

    // Center: URL bar with security indicator - EXPANDED for v1.0.9
    urlBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_add_css_class(urlBox, "url-box");
    gtk_widget_set_hexpand(urlBox, TRUE);
    gtk_widget_set_size_request(urlBox, 900, -1);

    // Security indicator (🔒 for HTTPS, 🔓 for HTTP)
    securityIcon = gtk_image_new_from_icon_name("security-medium-symbolic");
    gtk_widget_set_tooltip_text(securityIcon, "Connection security");
    gtk_widget_add_css_class(securityIcon, "security-icon");
    gtk_box_append(GTK_BOX(urlBox), securityIcon);

    // URL entry
    urlEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(urlEntry), "Search or enter address");
    gtk_widget_add_css_class(urlEntry, "url-entry");
    gtk_widget_set_hexpand(urlEntry, TRUE);
    g_signal_connect(urlEntry, "activate", G_CALLBACK(onUrlActivate), this);


    // Select all text when URL entry gets focus.
    // Defer via g_idle_add so the selection runs after the mouse-release event
    // that would otherwise immediately clear it.
    GtkEventController* focus_controller = gtk_event_controller_focus_new();
    g_signal_connect_swapped(focus_controller, "enter",
        G_CALLBACK(+[](GtkWidget* entry) {
            g_idle_add([](gpointer data) -> gboolean {
                GtkWidget* e = GTK_WIDGET(data);
                if (GTK_IS_EDITABLE(e))
                    gtk_editable_select_region(GTK_EDITABLE(e), 0, -1);
                return G_SOURCE_REMOVE;
            }, entry);
        }), urlEntry);
    gtk_widget_add_controller(urlEntry, focus_controller);

    gtk_box_append(GTK_BOX(urlBox), urlEntry);

    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(headerbar), urlBox);
    
    // Right side box for all right controls
    GtkWidget* rightBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(rightBox, "navbar-right");
    
    // Bookmark button
    GtkWidget* bookmarkBtn = gtk_button_new_from_icon_name("starred-symbolic");
    gtk_widget_set_tooltip_text(bookmarkBtn, "Bookmark This Page (Ctrl+D)");
    gtk_widget_add_css_class(bookmarkBtn, "action-btn");
    g_signal_connect(bookmarkBtn, "clicked", G_CALLBACK(onAddBookmarkClicked), this);
    gtk_box_append(GTK_BOX(rightBox), bookmarkBtn);
    
    // Reader mode button
    GtkWidget* readerBtn = gtk_button_new_from_icon_name("document-properties-symbolic");
    gtk_widget_set_tooltip_text(readerBtn, "Reader Mode (Ctrl+R+M)");
    gtk_widget_add_css_class(readerBtn, "action-btn");
    g_signal_connect(readerBtn, "clicked", G_CALLBACK(onReaderModeClicked), this);
    gtk_box_append(GTK_BOX(rightBox), readerBtn);
    
    // Screenshot button
    GtkWidget* screenshotBtn = gtk_button_new_from_icon_name("camera-photo-symbolic");
    gtk_widget_set_tooltip_text(screenshotBtn, "Take Screenshot (Ctrl+Shift+S)");
    gtk_widget_add_css_class(screenshotBtn, "action-btn");
    g_signal_connect(screenshotBtn, "clicked", G_CALLBACK(onScreenshotClicked), this);
    gtk_box_append(GTK_BOX(rightBox), screenshotBtn);
    
    // Split view button
    GtkWidget* splitViewBtn = gtk_button_new_from_icon_name("view-dual-symbolic");
    gtk_widget_set_tooltip_text(splitViewBtn, "Toggle Split View (Ctrl+Shift+D)");
    gtk_widget_add_css_class(splitViewBtn, "action-btn");
    g_signal_connect_swapped(splitViewBtn, "clicked", G_CALLBACK(+[](BrayaWindow* win) {
        win->toggleSplitView();
    }), this);
    gtk_box_append(GTK_BOX(rightBox), splitViewBtn);
    
    // Developer tools button
    GtkWidget* devToolsBtn = gtk_button_new_from_icon_name("utilities-terminal-symbolic");
    gtk_widget_set_tooltip_text(devToolsBtn, "Developer Tools (F12)");
    gtk_widget_add_css_class(devToolsBtn, "action-btn");
    g_signal_connect(devToolsBtn, "clicked", G_CALLBACK(onDevToolsClicked), this);
    gtk_box_append(GTK_BOX(rightBox), devToolsBtn);

    // Extension buttons container (will be populated after extensions load)
    extensionButtonsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_add_css_class(extensionButtonsBox, "extension-buttons");
    gtk_box_append(GTK_BOX(rightBox), extensionButtonsBox);

    // Ad-Blocker Shield button
    adBlockerShieldBtn = gtk_button_new_with_label("🛡️");
    gtk_widget_set_tooltip_text(adBlockerShieldBtn, "Ad-Blocker");
    gtk_widget_add_css_class(adBlockerShieldBtn, "action-btn");
    gtk_widget_add_css_class(adBlockerShieldBtn, "shield-btn");
    g_signal_connect(adBlockerShieldBtn, "clicked", G_CALLBACK(onAdBlockerShieldClicked), this);
    gtk_box_append(GTK_BOX(rightBox), adBlockerShieldBtn);

    // Settings button
    GtkWidget* settingsBtn = gtk_button_new_from_icon_name("open-menu-symbolic");
    gtk_widget_set_tooltip_text(settingsBtn, "Settings");
    gtk_widget_add_css_class(settingsBtn, "action-btn");
    g_signal_connect(settingsBtn, "clicked", G_CALLBACK(onSettingsClicked), this);
    gtk_box_append(GTK_BOX(rightBox), settingsBtn);

    gtk_header_bar_pack_end(GTK_HEADER_BAR(headerbar), rightBox);
    
    // Create dummy navbar variable for compatibility
    navbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
}

void BrayaWindow::createBookmarksBar() {
    std::cout << "=== Creating bookmarks bar ===" << std::endl;

    // Use the visual bookmarks bar from BrayaBookmarks
    if (bookmarksManager) {
        bookmarksBar = bookmarksManager->createBookmarksBar();

        if (!bookmarksBar) {
            std::cerr << "ERROR: bookmarksBar is NULL!" << std::endl;
            return;
        }

        // Make sure it's visible
        gtk_widget_set_visible(bookmarksBar, TRUE);
        gtk_widget_set_size_request(bookmarksBar, -1, 28);

        // Set up refresh callback so folder operations can trigger UI updates
        bookmarksManager->setRefreshCallback([this]() {
            std::cout << "🔄 Refreshing bookmarks bar from callback..." << std::endl;
            refreshBookmarksBar();
        });

        // Wire up click handlers for the bookmarks
        // gtk_scrolled_window_get_child returns the child (which might be a viewport)
        GtkWidget* child = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(bookmarksBar));
        if (child) {
            std::cout << "Got scrolled window child, type: " << G_OBJECT_TYPE_NAME(child) << std::endl;

            // In GTK4, if a viewport was automatically created, we need to get its child
            GtkWidget* boxWidget = child;
            if (GTK_IS_VIEWPORT(child)) {
                std::cout << "Child is a viewport, getting its child..." << std::endl;
                boxWidget = gtk_viewport_get_child(GTK_VIEWPORT(child));
                if (boxWidget) {
                    std::cout << "Got viewport child, type: " << G_OBJECT_TYPE_NAME(boxWidget) << std::endl;
                }
            }

            if (boxWidget && GTK_IS_BOX(boxWidget)) {
                bookmarksManager->updateBookmarksBar(
                    boxWidget,
                    this,
                    G_CALLBACK(onBookmarkBarItemClicked),
                    G_CALLBACK(onBookmarkBarAddClicked),
                    G_CALLBACK(onBookmarkBarItemRightClick)
                );
            } else {
                std::cerr << "ERROR: Could not find GtkBox widget!" << std::endl;
            }
        } else {
            std::cerr << "ERROR: Could not get scrolled window child!" << std::endl;
        }

        std::cout << "✓ Bookmarks bar created with click handlers" << std::endl;
    } else {
        std::cerr << "ERROR: bookmarksManager is NULL!" << std::endl;
        // Fallback to simple bar
        bookmarksBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_widget_add_css_class(bookmarksBar, "bookmarks-bar");
    }
}

void BrayaWindow::createStatusBar() {
    statusBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_add_css_class(statusBar, "status-bar");
    gtk_widget_set_margin_start(statusBar, 15);
    gtk_widget_set_margin_end(statusBar, 15);
    gtk_widget_set_margin_top(statusBar, 5);
    gtk_widget_set_margin_bottom(statusBar, 5);
    
    statusLabel = gtk_label_new("Ready");
    gtk_widget_set_hexpand(statusLabel, TRUE);
    gtk_label_set_xalign(GTK_LABEL(statusLabel), 0);
    gtk_box_append(GTK_BOX(statusBar), statusLabel);

    // 📊 Memory indicator label (visibility controlled by settings)
    memoryLabel = gtk_label_new("");
    gtk_widget_add_css_class(memoryLabel, "memory-indicator");
    gtk_box_append(GTK_BOX(statusBar), memoryLabel);

    GtkWidget* engineLabel = gtk_label_new("WebKit Engine");
    gtk_widget_add_css_class(engineLabel, "braya-title");
    gtk_box_append(GTK_BOX(statusBar), engineLabel);
}

// Tab management
void BrayaWindow::createTab(const char* url) {
    auto tab = std::make_unique<BrayaTab>(nextTabId++, url, passwordManager.get(), extensionManager.get());

    // Set up extension installation callback for automatic installation from web pages
    tab->setExtensionInstallCallback([this](const std::string& extensionUrl, const std::string& downloadUrl) {
        std::cout << "🔌 Installing extension automatically from: " << extensionUrl << std::endl;

        // Use the download URL if provided, otherwise use the extension page URL
        std::string installUrl = downloadUrl.empty() ? extensionUrl : downloadUrl;

        // Create installer and install
        ExtensionInstaller installer(extensionManager.get());
        installer.installFromUrl(installUrl, [this](bool success, const std::string& message) {
            if (success) {
                std::cout << "✓ Extension installed successfully: " << message << std::endl;

                // Update toolbar buttons
                updateExtensionButtons();

                // If settings is open, refresh the extensions list
                settings->refreshExtensionsList();
                settings->saveExtensionStates();
            } else {
                std::cerr << "✗ Extension installation failed: " << message << std::endl;

                // Show error notification to user
                GtkWidget* dialog = gtk_message_dialog_new(
                    GTK_WINDOW(window),
                    GTK_DIALOG_MODAL,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_OK,
                    "Extension Installation Failed"
                );
                gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                    "%s", message.c_str());
                gtk_window_present(GTK_WINDOW(dialog));
                g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), nullptr);
            }
        });
    });

    // Set up new window/popup callback (proper WebKitGTK approach)
    tab->setNewWindowCallback([this](WebKitWebView* related) -> GtkWidget* {
        std::cout << "🪟 Creating new WebView for popup/window..." << std::endl;

        // Step 1: Create a new WebView (WebKitGTK 6.0 automatically handles context sharing)
        WebKitWebView* newWebView = WEBKIT_WEB_VIEW(webkit_web_view_new());

        // Step 2: Connect to "ready-to-show" signal to know when to display
        g_signal_connect(newWebView, "ready-to-show",
            G_CALLBACK(+[](WebKitWebView* webView, gpointer data) {
                BrayaWindow* window = static_cast<BrayaWindow*>(data);

                std::cout << "✓ New window ready to show, creating tab..." << std::endl;

                // Create a scrolled window for the WebView
                GtkWidget* scrolledWindow = gtk_scrolled_window_new();
                gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_WIDGET(webView));

                // Add to tab stack
                char name[64];
                snprintf(name, sizeof(name), "tab-%d", window->nextTabId);
                gtk_stack_add_named(GTK_STACK(window->tabStack), scrolledWindow, name);

                // Create tab button
                GtkWidget* faviconBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
                gtk_widget_set_halign(faviconBox, GTK_ALIGN_CENTER);
                gtk_widget_set_valign(faviconBox, GTK_ALIGN_CENTER);
                gtk_widget_set_size_request(faviconBox, 32, 32);
                gtk_widget_add_css_class(faviconBox, "tab-favicon");

                GtkWidget* tabBtn = gtk_button_new();
                gtk_widget_add_css_class(tabBtn, "tab-button");
                gtk_widget_set_size_request(tabBtn, 48, 48);
                gtk_button_set_child(GTK_BUTTON(tabBtn), faviconBox);

                g_object_set_data(G_OBJECT(tabBtn), "tab-index", GINT_TO_POINTER(window->tabs.size()));
                g_object_set_data(G_OBJECT(tabBtn), "favicon-box", faviconBox);
                g_signal_connect(tabBtn, "clicked", G_CALLBACK(onTabClicked), window);

                gtk_box_append(GTK_BOX(window->tabsBox), tabBtn);

                // Switch to the new tab
                window->activeTabIndex = window->tabs.size() - 1;
                gtk_stack_set_visible_child_name(GTK_STACK(window->tabStack), name);
                window->updateUI();

                std::cout << "✓ New popup window opened in tab " << window->nextTabId << std::endl;
            }), this);

        // Step 3: Return the WebView (don't show it yet - wait for ready-to-show)
        return GTK_WIDGET(newWebView);
    });

    // Set up new tab callback (for opening links in new tabs)
    tab->setNewTabCallback([this](const std::string& url) {
        std::cout << "🔗 Opening link in new tab: " << url << std::endl;
        createTab(url.c_str());
    });

    // Set up favicon cache callbacks
    tab->setFaviconCacheCallback([this](const std::string& url, GdkTexture* favicon) {
        cacheFavicon(url, favicon);
    });
    tab->setFaviconGetCallback([this](const std::string& url) -> GdkTexture* {
        return getCachedFavicon(url);
    });

    // Connect download handler via decide-policy and context
    WebKitWebContext* context = webkit_web_view_get_context(tab->getWebView());
    
    // Modern WebKit2GTK 6.0 approach: handle downloads via policy decision
    // Downloads are created when webkit_policy_decision_download() is called
    g_signal_connect(tab->getWebView(), "decide-policy",
        G_CALLBACK(+[](WebKitWebView* webView, WebKitPolicyDecision* decision, 
                       WebKitPolicyDecisionType type, gpointer data) -> gboolean {
            if (type == WEBKIT_POLICY_DECISION_TYPE_RESPONSE) {
                WebKitResponsePolicyDecision* response = WEBKIT_RESPONSE_POLICY_DECISION(decision);
                
                // Check if this is a download (unsupported MIME type)
                if (!webkit_response_policy_decision_is_mime_type_supported(response)) {
                    std::cout << "🔽 Download triggered - allowing download..." << std::endl;
                    
                    // Tell WebKit to download - this will create a WebKitDownload
                    // that we'll catch via the web context's download-started signal
                    webkit_policy_decision_download(decision);
                    return TRUE;
                }
            }
            return FALSE;
        }), this);
    
    // Add to stack
    char name[32];
    snprintf(name, sizeof(name), "tab-%d", tab->getId());
    gtk_stack_add_named(GTK_STACK(tabStack), tab->getScrolledWindow(), name);
    
    // Simple clean tab - just favicon area
    GtkWidget* faviconBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(faviconBox, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(faviconBox, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(faviconBox, 32, 32);
    gtk_widget_add_css_class(faviconBox, "tab-favicon");

    // Create close button (Zen-style - appears on hover)
    GtkWidget* closeBtn = gtk_button_new_from_icon_name("window-close-symbolic");
    gtk_widget_add_css_class(closeBtn, "tab-close-button");
    
    // Apply red background directly via CSS provider (GTK4 ignores CSS background on buttons)
    GtkCssProvider* btnProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(btnProvider,
        ".tab-close-button { "
        "  background: #dc3232; "
        "  min-width: 14px; "
        "  min-height: 14px; "
        "  padding: 0px; "
        "  border: none; "
        "  border-radius: 3px; "
        "} "
        ".tab-close-button:hover { background: #ff3c3c; }"
    );
    gtk_style_context_add_provider(gtk_widget_get_style_context(closeBtn),
                                   GTK_STYLE_PROVIDER(btnProvider),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(btnProvider);
    
    gtk_widget_set_size_request(closeBtn, 14, 14);  // Very small
    gtk_widget_set_halign(closeBtn, GTK_ALIGN_START);  // Upper LEFT corner
    gtk_widget_set_valign(closeBtn, GTK_ALIGN_START);
    gtk_widget_set_can_focus(closeBtn, FALSE);  // Don't steal focus
    gtk_widget_set_opacity(closeBtn, 0.0);  // Hidden by default - shows on hover
    gtk_widget_set_margin_start(closeBtn, 3);  // Small margin from edge
    gtk_widget_set_margin_top(closeBtn, 3);

    // Make overlay pass-through except for close button (so tab is still clickable)
    gtk_widget_set_can_target(closeBtn, TRUE);

    // Create overlay to hold favicon and close button
    GtkWidget* overlay = gtk_overlay_new();
    gtk_overlay_set_child(GTK_OVERLAY(overlay), faviconBox);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), closeBtn);

    // Main tab button
    GtkWidget* tabBtn = gtk_button_new();
    gtk_widget_add_css_class(tabBtn, "tab-button");
    gtk_widget_set_size_request(tabBtn, 48, 48);
    gtk_button_set_child(GTK_BUTTON(tabBtn), overlay);

    g_object_set_data(G_OBJECT(tabBtn), "tab-index", GINT_TO_POINTER(tabs.size()));
    g_object_set_data(G_OBJECT(tabBtn), "favicon-box", faviconBox);
    g_object_set_data(G_OBJECT(tabBtn), "close-button", closeBtn);
    g_object_set_data(G_OBJECT(tabBtn), "window", this);

    // Close button click handler with event stopping
    GtkGesture* closeClickGesture = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(closeClickGesture), GDK_BUTTON_PRIMARY);

    g_object_set_data(G_OBJECT(closeClickGesture), "tab-button", tabBtn);
    g_object_set_data(G_OBJECT(closeClickGesture), "window", this);

    g_signal_connect(closeClickGesture, "pressed", G_CALLBACK(+[](GtkGestureClick* gesture, int n_press, double x, double y, gpointer data) {
        // Get window and tab button
        BrayaWindow* window = static_cast<BrayaWindow*>(g_object_get_data(G_OBJECT(gesture), "window"));
        GtkWidget* tabBtn = GTK_WIDGET(g_object_get_data(G_OBJECT(gesture), "tab-button"));
        GtkWidget* closeBtn = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));

        // Stop event propagation so tab doesn't switch
        gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);

        if (!window || !tabBtn || !closeBtn) return;

        // Check if button is already disabled (closing in progress)
        if (!gtk_widget_get_sensitive(closeBtn)) {
            std::cout << "⚠️  Close button already clicked, ignoring duplicate event" << std::endl;
            return;
        }

        // Disable button immediately to prevent double-clicks
        gtk_widget_set_sensitive(closeBtn, FALSE);

        // Find the tab index by matching the tab button
        int index = -1;
        for (size_t i = 0; i < window->tabs.size(); i++) {
            if (window->tabs[i] && window->tabs[i]->getTabButton() == tabBtn) {
                index = i;
                break;
            }
        }

        if (index >= 0 && index < (int)window->tabs.size() && window->tabs.size() > 1) {
            std::cout << "✕ Closing tab " << index << " via close button" << std::endl;
            window->closeTab(index);
        } else {
            // Re-enable button if we couldn't close (edge case)
            gtk_widget_set_sensitive(closeBtn, TRUE);
        }
    }), nullptr);

    gtk_widget_add_controller(closeBtn, GTK_EVENT_CONTROLLER(closeClickGesture));

    // Hover controller to show/hide close button
    GtkEventController* hoverController = gtk_event_controller_motion_new();

    auto showCloseBtn = +[](GtkEventControllerMotion* controller, double x, double y, gpointer data) {
        GtkWidget* tabBtn = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));

        // 🛡️ CRASH FIX: Validate widget before accessing
        if (!tabBtn || !GTK_IS_WIDGET(tabBtn)) return;

        BrayaWindow* window = static_cast<BrayaWindow*>(g_object_get_data(G_OBJECT(tabBtn), "window"));

        // 🛡️ CRASH FIX: Abort if tab is being destroyed
        if (window && window->destroyingTabs.count(tabBtn) > 0) return;

        GtkWidget* closeBtn = GTK_WIDGET(g_object_get_data(G_OBJECT(tabBtn), "close-button"));

        if (closeBtn && GTK_IS_WIDGET(closeBtn)) {
            // Smooth fade-in animation
            gtk_widget_set_opacity(closeBtn, 1.0);
        }
    };

    auto hideCloseBtn = +[](GtkEventControllerMotion* controller, gpointer data) {
        GtkWidget* tabBtn = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));

        // 🛡️ CRASH FIX: Validate widget before accessing
        if (!tabBtn || !GTK_IS_WIDGET(tabBtn)) return;

        BrayaWindow* window = static_cast<BrayaWindow*>(g_object_get_data(G_OBJECT(tabBtn), "window"));

        // 🛡️ CRASH FIX: Abort if tab is being destroyed
        if (window && window->destroyingTabs.count(tabBtn) > 0) return;

        GtkWidget* closeBtn = GTK_WIDGET(g_object_get_data(G_OBJECT(tabBtn), "close-button"));

        if (closeBtn && GTK_IS_WIDGET(closeBtn)) {
            // Smooth fade-out animation
            gtk_widget_set_opacity(closeBtn, 0.0);
        }
    };

    g_signal_connect(hoverController, "enter", G_CALLBACK(showCloseBtn), nullptr);
    g_signal_connect(hoverController, "leave", G_CALLBACK(hideCloseBtn), nullptr);
    gtk_widget_add_controller(tabBtn, hoverController);

    // Tab preview disabled - causing problems
    if (false && settings && settings->getShowTabPreviews()) {
        GtkEventController* motion_controller = gtk_event_controller_motion_new();

        auto enterCallback = +[](GtkEventControllerMotion* controller, double x, double y, gpointer data) {
            GtkWidget* tabBtn = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));

            // 🛡️ CRASH FIX: Check if widget is valid and not being destroyed
            if (!tabBtn || !GTK_IS_WIDGET(tabBtn)) return;

            BrayaWindow* window = static_cast<BrayaWindow*>(g_object_get_data(G_OBJECT(tabBtn), "window"));
            if (!window) return;

            // 🛡️ CRASH FIX: Abort if tab is being destroyed
            if (window->destroyingTabs.count(tabBtn) > 0) return;

            int tabIndex = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(tabBtn), "tab-index"));
            if (tabIndex < 0 || tabIndex >= window->tabs.size()) return;

            // Check if we already have a popover showing
            GtkWidget* existingPopover = GTK_WIDGET(g_object_get_data(G_OBJECT(tabBtn), "preview-popover"));
            if (existingPopover && GTK_IS_WIDGET(existingPopover)) {
                return; // Already showing preview
            }

            // Cancel any existing timer
            guint* existingTimer = (guint*)g_object_get_data(G_OBJECT(tabBtn), "preview-timer");
            if (existingTimer && *existingTimer > 0) {
                g_source_remove(*existingTimer);
                delete existingTimer;
                g_object_set_data(G_OBJECT(tabBtn), "preview-timer", nullptr);
            }

            // Create a timer to delay the preview (400ms)
            struct TimerData {
                GtkWidget* tabBtn;
                BrayaWindow* window;
                int tabIndex;
            };

            TimerData* timerData = new TimerData{tabBtn, window, tabIndex};

            guint* timerId = new guint;
            *timerId = g_timeout_add(400, [](gpointer user_data) -> gboolean {
                TimerData* data = static_cast<TimerData*>(user_data);
                GtkWidget* tabBtn = data->tabBtn;
                BrayaWindow* window = data->window;
                int tabIndex = data->tabIndex;

                delete data;

                // 🛡️ CRASH FIX: Validate widget before accessing
                if (!tabBtn || !GTK_IS_WIDGET(tabBtn)) {
                    return G_SOURCE_REMOVE;
                }

                // 🛡️ CRASH FIX: Check if tab is being destroyed
                if (window && window->destroyingTabs.count(tabBtn) > 0) {
                    return G_SOURCE_REMOVE;
                }

                // Clear the timer reference
                guint* storedTimer = (guint*)g_object_get_data(G_OBJECT(tabBtn), "preview-timer");
                if (storedTimer) {
                    delete storedTimer;
                    g_object_set_data(G_OBJECT(tabBtn), "preview-timer", nullptr);
                }

                // Verify tab still exists
                if (!window || tabIndex < 0 || tabIndex >= window->tabs.size()) {
                    return G_SOURCE_REMOVE;
                }

                BrayaTab* tab = window->tabs[tabIndex].get();
                WebKitWebView* webView = tab->getWebView();
                if (!webView) return G_SOURCE_REMOVE;

                // Create preview popover
                GtkWidget* popover = gtk_popover_new();
                gtk_widget_set_parent(popover, tabBtn);
                gtk_popover_set_position(GTK_POPOVER(popover), GTK_POS_RIGHT);
                gtk_popover_set_autohide(GTK_POPOVER(popover), TRUE);
                gtk_popover_set_has_arrow(GTK_POPOVER(popover), FALSE);
                gtk_widget_add_css_class(popover, "tab-preview-popover");

                // Store popover reference
                g_object_set_data(G_OBJECT(tabBtn), "preview-popover", popover);

                // Auto-cleanup on close
                g_signal_connect(popover, "closed", G_CALLBACK(+[](GtkPopover* pop, gpointer data) {
                    GtkWidget* btn = GTK_WIDGET(data);
                    g_object_set_data(G_OBJECT(btn), "preview-popover", nullptr);
                    gtk_widget_unparent(GTK_WIDGET(pop));
                }), tabBtn);

                // Get snapshot of the web view
                webkit_web_view_get_snapshot(
                    webView,
                    WEBKIT_SNAPSHOT_REGION_VISIBLE,
                    WEBKIT_SNAPSHOT_OPTIONS_NONE,
                    nullptr,
                    [](GObject* object, GAsyncResult* result, gpointer user_data) {
                        GtkWidget* popover = GTK_WIDGET(user_data);
                        if (!popover || !GTK_IS_WIDGET(popover)) return;

                        GError* error = nullptr;
                        GdkTexture* texture = webkit_web_view_get_snapshot_finish(
                            WEBKIT_WEB_VIEW(object), result, &error);

                        if (error) {
                            g_error_free(error);
                            gtk_popover_popdown(GTK_POPOVER(popover));
                            return;
                        }

                        if (texture) {
                            // Create scaled preview (300px wide)
                            int origWidth = gdk_texture_get_width(texture);
                            int origHeight = gdk_texture_get_height(texture);
                            int previewWidth = 300;
                            int previewHeight = (origHeight * previewWidth) / origWidth;

                            GtkWidget* picture = gtk_picture_new_for_paintable(GDK_PAINTABLE(texture));
                            gtk_widget_set_size_request(picture, previewWidth, previewHeight);
                            gtk_picture_set_content_fit(GTK_PICTURE(picture), GTK_CONTENT_FIT_SCALE_DOWN);

                            GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
                            gtk_box_append(GTK_BOX(box), picture);

                            gtk_popover_set_child(GTK_POPOVER(popover), box);
                            gtk_widget_show(popover);

                            g_object_unref(texture);
                        }
                    },
                    popover
                );

                return G_SOURCE_REMOVE;
            }, timerData);

            g_object_set_data(G_OBJECT(tabBtn), "preview-timer", timerId);
        };

        g_signal_connect(motion_controller, "enter", G_CALLBACK(enterCallback), nullptr);

        auto leaveCallback = +[](GtkEventControllerMotion* controller, gpointer data) {
            GtkWidget* tabBtn = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));

            // 🛡️ CRASH FIX: Validate widget before accessing
            if (!tabBtn || !GTK_IS_WIDGET(tabBtn)) return;

            BrayaWindow* window = static_cast<BrayaWindow*>(g_object_get_data(G_OBJECT(tabBtn), "window"));

            // 🛡️ CRASH FIX: Abort if tab is being destroyed
            if (window && window->destroyingTabs.count(tabBtn) > 0) return;

            // Cancel pending timer if exists
            guint* existingTimer = (guint*)g_object_get_data(G_OBJECT(tabBtn), "preview-timer");
            if (existingTimer && *existingTimer > 0) {
                g_source_remove(*existingTimer);
                delete existingTimer;
                g_object_set_data(G_OBJECT(tabBtn), "preview-timer", nullptr);
            }

            // Close popover if showing
            GtkWidget* popover = GTK_WIDGET(g_object_get_data(G_OBJECT(tabBtn), "preview-popover"));
            if (popover && GTK_IS_WIDGET(popover)) {
                gtk_popover_popdown(GTK_POPOVER(popover));
            }
        };

        g_signal_connect(motion_controller, "leave", G_CALLBACK(leaveCallback), nullptr);

        gtk_widget_add_controller(tabBtn, motion_controller);
    }

    g_signal_connect(tabBtn, "clicked", G_CALLBACK(onTabClicked), this);
    
    // Middle-click to close tab
    GtkGesture* middleClick = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(middleClick), GDK_BUTTON_MIDDLE);
    g_object_set_data(G_OBJECT(middleClick), "tab-index", GINT_TO_POINTER(tabs.size()));
    g_signal_connect(middleClick, "pressed", G_CALLBACK(onTabMiddleClick), this);
    gtk_widget_add_controller(tabBtn, GTK_EVENT_CONTROLLER(middleClick));
    
    gtk_box_append(GTK_BOX(tabsBox), tabBtn);
    
    tab->setTabButton(tabBtn);
    
    // Connect to load-changed to update navigation buttons and history
    g_signal_connect(tab->getWebView(), "load-changed", 
        G_CALLBACK(+[](WebKitWebView* webView, WebKitLoadEvent loadEvent, gpointer data) {
            if (loadEvent == WEBKIT_LOAD_FINISHED) {
                BrayaWindow* window = static_cast<BrayaWindow*>(data);
                window->updateUI();
                
                // Add to history
                const gchar* url = webkit_web_view_get_uri(webView);
                const gchar* title = webkit_web_view_get_title(webView);
                if (url && window->history) {
                    window->history->addEntry(url, title ? title : "");
                }
            }
        }), this);

    // Apply ad-blocker to this tab
    WebKitWebView* webView = tab->getWebView();
    if (webView && adBlocker) {
        WebKitUserContentManager* contentManager = webkit_web_view_get_user_content_manager(webView);
        if (contentManager) {
            adBlocker->applyToContentManager(contentManager);
        }
    }

    tabs.push_back(std::move(tab));
    
    // Show bookmarks bar on new tab
    if (bookmarksBar && GTK_IS_WIDGET(bookmarksBar)) {
        gtk_widget_set_visible(bookmarksBar, TRUE);
        showBookmarksBar = true;
    }
    
    // Switch to new tab
    switchToTab(tabs.size() - 1);
    
    // Clear URL bar for new tab so user can type immediately (but not on first tab)
    if (tabs.size() > 1) {
        gtk_editable_set_text(GTK_EDITABLE(urlEntry), "");
        gtk_widget_grab_focus(urlEntry);
    }
}

void BrayaWindow::switchToTab(int index) {
    if (index < 0 || index >= (int)tabs.size()) return;

    activeTabIndex = index;
    BrayaTab* tab = tabs[index].get();

    // 💤 Phase 2: Resume suspended tab if needed
    if (tab->isSuspended()) {
        tab->resume();
    }

    // 🔧 Validate GTK stack child exists before switching
    char name[32];
    snprintf(name, sizeof(name), "tab-%d", tab->getId());

    // Check if the child exists in the stack before trying to switch
    GtkWidget* stackChild = gtk_stack_get_child_by_name(GTK_STACK(tabStack), name);
    if (stackChild && GTK_IS_WIDGET(stackChild)) {
        gtk_stack_set_visible_child_name(GTK_STACK(tabStack), name);
    } else {
        std::cerr << "⚠️ Warning: Stack child '" << name << "' not found, skipping switch" << std::endl;
    }

    // Update URL bar
    gtk_editable_set_text(GTK_EDITABLE(urlEntry), tab->getUrl().c_str());

    // Update navigation buttons
    WebKitWebView* webView = tab->getWebView();
    if (webView && WEBKIT_IS_WEB_VIEW(webView)) {
        gtk_widget_set_sensitive(backBtn, webkit_web_view_can_go_back(webView));
        gtk_widget_set_sensitive(forwardBtn, webkit_web_view_can_go_forward(webView));
    }
    
    // Update tab button styles
    for (size_t i = 0; i < tabs.size(); i++) {
        GtkWidget* btn = tabs[i]->getTabButton();
        if ((int)i == index) {
            gtk_widget_add_css_class(btn, "active");
        } else {
            gtk_widget_remove_css_class(btn, "active");
        }
    }
    
    // Update window title (no emoji - we have logo label in headerbar)
    std::string title = "Braya Browser - " + tab->getTitle();
    gtk_window_set_title(GTK_WINDOW(window), title.c_str());
}

void BrayaWindow::closeTab(int index) {
    if (tabs.size() <= 1) return; // Don't close last tab
    if (index < 0 || index >= (int)tabs.size()) return;

    // Save tab info before closing
    ClosedTab closedTab;
    closedTab.url = tabs[index]->getUrl();
    closedTab.title = tabs[index]->getTitle();

    // Add to recently closed tabs (limit to MAX_CLOSED_TABS)
    recentlyClosedTabs.push_back(closedTab);
    if (recentlyClosedTabs.size() > MAX_CLOSED_TABS) {
        recentlyClosedTabs.erase(recentlyClosedTabs.begin());
    }

    std::cout << "💾 Saved closed tab: " << closedTab.title << " (" << closedTab.url << ")" << std::endl;

    // Properly close the WebView before destroying it
    WebKitWebView* webView = tabs[index]->getWebView();
    if (webView && WEBKIT_IS_WEB_VIEW(webView)) {
        // Use try_close() which properly terminates the web process
        webkit_web_view_try_close(webView);
        // Disconnect all WebView signals to prevent callbacks during destruction
        g_signal_handlers_disconnect_by_data(webView, this);

        // 🧹 Memory optimization: WebKit will clean up the WebView memory automatically
        // when the web process terminates via try_close()
    }

    // Clean up any preview popover and timer before removing the tab button
    GtkWidget* tabBtn = tabs[index]->getTabButton();
    if (tabBtn && GTK_IS_WIDGET(tabBtn)) {
        // 🛡️ CRASH FIX: Mark tab as destroying to prevent callbacks from executing
        destroyingTabs.insert(tabBtn);

        // 🛡️ PHASE 2 FIX: Block signals first to prevent new events from being queued
        guint blockedCount = g_signal_handlers_block_matched(tabBtn, G_SIGNAL_MATCH_DATA, 0, 0, nullptr, nullptr, this);

        // Process any pending GTK events that are already queued
        // This ensures callbacks in flight complete before we disconnect
        while (g_main_context_pending(nullptr)) {
            g_main_context_iteration(nullptr, FALSE);
        }

        // Now safe to disconnect - all queued events have been processed
        g_signal_handlers_disconnect_matched(tabBtn, G_SIGNAL_MATCH_DATA, 0, 0, nullptr, nullptr, this);

        if (blockedCount > 0) {
            std::cout << "🛡️  Blocked and disconnected " << blockedCount << " signal handlers" << std::endl;
        }

        // Cancel any pending timer
        guint* existingTimer = (guint*)g_object_get_data(G_OBJECT(tabBtn), "preview-timer");
        if (existingTimer && *existingTimer > 0) {
            g_source_remove(*existingTimer);
            delete existingTimer;
            g_object_set_data(G_OBJECT(tabBtn), "preview-timer", nullptr);
        }

        // Clean up popover
        GtkWidget* popover = GTK_WIDGET(g_object_get_data(G_OBJECT(tabBtn), "preview-popover"));
        if (popover && GTK_IS_WIDGET(popover)) {
            gtk_widget_unparent(popover);
            g_object_set_data(G_OBJECT(tabBtn), "preview-popover", nullptr);
        }

        // Disconnect close button and clear gesture data to prevent stale callbacks
        GtkWidget* closeBtn = GTK_WIDGET(g_object_get_data(G_OBJECT(tabBtn), "close-button"));
        if (closeBtn && GTK_IS_WIDGET(closeBtn)) {
            // Find and clear data on all gesture controllers
            GListModel* controllers = gtk_widget_observe_controllers(closeBtn);
            guint n_controllers = g_list_model_get_n_items(controllers);
            for (guint i = 0; i < n_controllers; i++) {
                GtkEventController* controller = GTK_EVENT_CONTROLLER(g_list_model_get_item(controllers, i));
                if (controller && GTK_IS_GESTURE_CLICK(controller)) {
                    // Clear the gesture's object data to prevent callbacks from accessing invalid pointers
                    g_object_set_data(G_OBJECT(controller), "window", nullptr);
                    g_object_set_data(G_OBJECT(controller), "tab-button", nullptr);
                }
                if (controller) {
                    g_object_unref(controller);
                }
            }

            // Disconnect all signals
            g_signal_handlers_disconnect_matched(closeBtn, G_SIGNAL_MATCH_DATA, 0, 0, nullptr, nullptr, nullptr);
        }

        // Remove from UI
        gtk_box_remove(GTK_BOX(tabsBox), tabBtn);
    }

    // Remove from tabs vector
    tabs.erase(tabs.begin() + index);
    
    // Update all tab button indices AND close button indices
    for (size_t i = 0; i < tabs.size(); i++) {
        GtkWidget* btn = tabs[i]->getTabButton();
        if (btn) {
            g_object_set_data(G_OBJECT(btn), "tab-index", GINT_TO_POINTER(i));
            
            // Update close button index
            GtkWidget* closeBtn = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "close-btn"));
            if (closeBtn) {
                g_object_set_data(G_OBJECT(closeBtn), "tab-index", GINT_TO_POINTER(i));
            }
        }
    }
    
    // Switch to another tab
    if (activeTabIndex == index) {
        switchToTab(std::min(index, (int)tabs.size() - 1));
    } else if (activeTabIndex > index) {
        activeTabIndex--;
    }
}

void BrayaWindow::reopenClosedTab() {
    if (recentlyClosedTabs.empty()) {
        std::cout << "ℹ️  No recently closed tabs to reopen" << std::endl;
        return;
    }

    // Get the most recently closed tab
    ClosedTab closedTab = recentlyClosedTabs.back();
    recentlyClosedTabs.pop_back();

    std::cout << "🔄 Reopening closed tab: " << closedTab.title << " (" << closedTab.url << ")" << std::endl;

    // Create new tab with the URL
    createTab(closedTab.url.c_str());
}

// Split View Implementation
void BrayaWindow::toggleSplitView() {
    isSplitView = !isSplitView;

    if (isSplitView) {
        // Entering split view mode
        std::cout << "🔀 Enabling split view mode" << std::endl;

        // Show the second pane
        gtk_widget_set_visible(tabStack2, TRUE);

        // Find a different tab to show in the second pane
        if (tabs.size() >= 2) {
            // Use the next tab or the previous tab
            if (activeTabIndex + 1 < tabs.size()) {
                activeTabIndexPane2 = activeTabIndex + 1;
            } else if (activeTabIndex > 0) {
                activeTabIndexPane2 = activeTabIndex - 1;
            } else {
                activeTabIndexPane2 = 1;
            }

            // Move the tab's widget to the second stack
            BrayaTab* tab2 = tabs[activeTabIndexPane2].get();
            GtkWidget* scrolledWindow = tab2->getScrolledWindow();

            // Get the current parent stack
            GtkWidget* currentParent = gtk_widget_get_parent(scrolledWindow);
            if (currentParent && GTK_IS_STACK(currentParent)) {
                // Get the stack child name before removing
                char name[32];
                snprintf(name, sizeof(name), "tab-%d", tab2->getId());

                // Remove from current stack and add to second stack
                g_object_ref(scrolledWindow);  // Add ref to prevent destruction
                gtk_stack_remove(GTK_STACK(currentParent), scrolledWindow);
                gtk_stack_add_named(GTK_STACK(tabStack2), scrolledWindow, name);
                g_object_unref(scrolledWindow);  // Release our ref

                // Show it in the second pane
                gtk_stack_set_visible_child_name(GTK_STACK(tabStack2), name);
            }
        } else {
            // Only one tab - just show message
            std::cout << "ℹ️  Need at least 2 tabs for split view" << std::endl;
            isSplitView = FALSE;
            gtk_widget_set_visible(tabStack2, FALSE);
            return;
        }

        // Set split position to 50%
        int width = gtk_widget_get_width(splitPane);
        gtk_paned_set_position(GTK_PANED(splitPane), width / 2);

    } else {
        // Exiting split view mode
        std::cout << "🔀 Disabling split view mode" << std::endl;

        // Move the tab back to the first stack if needed
        if (activeTabIndexPane2 >= 0 && activeTabIndexPane2 < tabs.size()) {
            BrayaTab* tab2 = tabs[activeTabIndexPane2].get();
            GtkWidget* scrolledWindow = tab2->getScrolledWindow();
            GtkWidget* currentParent = gtk_widget_get_parent(scrolledWindow);

            if (currentParent == tabStack2) {
                char name[32];
                snprintf(name, sizeof(name), "tab-%d", tab2->getId());

                g_object_ref(scrolledWindow);
                gtk_stack_remove(GTK_STACK(tabStack2), scrolledWindow);
                gtk_stack_add_named(GTK_STACK(tabStack), scrolledWindow, name);
                g_object_unref(scrolledWindow);
            }
        }

        // Hide the second pane
        gtk_widget_set_visible(tabStack2, FALSE);
        activeTabIndexPane2 = -1;
    }
}

void BrayaWindow::setSplitOrientation(bool horizontal) {
    splitHorizontal = horizontal;

    // Recreate the paned widget with new orientation
    GtkOrientation orientation = horizontal ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;

    // Save current children
    GtkWidget* child1 = gtk_paned_get_start_child(GTK_PANED(splitPane));
    GtkWidget* child2 = gtk_paned_get_end_child(GTK_PANED(splitPane));

    g_object_ref(child1);
    g_object_ref(child2);

    // Remove from old pane
    gtk_paned_set_start_child(GTK_PANED(splitPane), nullptr);
    gtk_paned_set_end_child(GTK_PANED(splitPane), nullptr);

    // Remove old pane from parent
    GtkWidget* parent = gtk_widget_get_parent(splitPane);
    gtk_box_remove(GTK_BOX(parent), splitPane);

    // Create new pane with correct orientation
    splitPane = gtk_paned_new(orientation);
    gtk_paned_set_wide_handle(GTK_PANED(splitPane), TRUE);
    gtk_widget_set_vexpand(splitPane, TRUE);

    // Add children back
    gtk_paned_set_start_child(GTK_PANED(splitPane), child1);
    gtk_paned_set_end_child(GTK_PANED(splitPane), child2);
    gtk_paned_set_resize_start_child(GTK_PANED(splitPane), TRUE);
    gtk_paned_set_resize_end_child(GTK_PANED(splitPane), TRUE);
    gtk_paned_set_shrink_start_child(GTK_PANED(splitPane), FALSE);
    gtk_paned_set_shrink_end_child(GTK_PANED(splitPane), FALSE);

    g_object_unref(child1);
    g_object_unref(child2);

    // Add new pane to parent (after bookmarks bar)
    gtk_box_append(GTK_BOX(parent), splitPane);

    std::cout << "🔀 Split orientation changed to " << (horizontal ? "horizontal" : "vertical") << std::endl;
}

void BrayaWindow::moveTabToSplitPane(int tabIndex) {
    if (!isSplitView || tabIndex < 0 || tabIndex >= tabs.size()) return;

    activeTabIndexPane2 = tabIndex;

    // Show the tab in second pane
    BrayaTab* tab = tabs[tabIndex].get();
    char name[32];
    snprintf(name, sizeof(name), "tab-%d", tab->getId());
    gtk_stack_set_visible_child_name(GTK_STACK(tabStack2), name);

    std::cout << "🔀 Moved tab " << tabIndex << " to split pane" << std::endl;
}

// Session Management Implementation
void BrayaWindow::saveSession() {
    std::string sessionPath = std::string(g_get_home_dir()) + "/.config/braya-browser/session.json";
    std::string configDir = std::string(g_get_home_dir()) + "/.config/braya-browser";
    g_mkdir_with_parents(configDir.c_str(), 0755);

    // Build JSON object
    JsonBuilder* builder = json_builder_new();
    json_builder_begin_object(builder);

    // Save tabs array
    json_builder_set_member_name(builder, "tabs");
    json_builder_begin_array(builder);
    for (const auto& tab : tabs) {
        json_builder_begin_object(builder);

        json_builder_set_member_name(builder, "url");
        json_builder_add_string_value(builder, tab->getUrl().c_str());

        json_builder_set_member_name(builder, "title");
        json_builder_add_string_value(builder, tab->getTitle().c_str());

        json_builder_set_member_name(builder, "pinned");
        json_builder_add_boolean_value(builder, tab->isPinned());

        json_builder_set_member_name(builder, "muted");
        json_builder_add_boolean_value(builder, tab->isMuted());

        json_builder_set_member_name(builder, "tabId");
        json_builder_add_int_value(builder, tab->getTabId());

        json_builder_end_object(builder);
    }
    json_builder_end_array(builder);

    // Save active tab index
    json_builder_set_member_name(builder, "activeTabIndex");
    json_builder_add_int_value(builder, activeTabIndex);

    // Save tab groups
    json_builder_set_member_name(builder, "groups");
    json_builder_begin_array(builder);
    for (const auto& group : tabGroups) {
        json_builder_begin_object(builder);

        json_builder_set_member_name(builder, "name");
        json_builder_add_string_value(builder, group->getName().c_str());

        json_builder_set_member_name(builder, "color");
        json_builder_add_string_value(builder, group->getColor().c_str());

        // Save tab IDs in this group
        json_builder_set_member_name(builder, "tabIds");
        json_builder_begin_array(builder);
        for (int tabId : group->getTabs()) {
            json_builder_add_int_value(builder, tabId);
        }
        json_builder_end_array(builder);

        json_builder_end_object(builder);
    }
    json_builder_end_array(builder);

    json_builder_end_object(builder);

    // Generate JSON string
    JsonNode* root = json_builder_get_root(builder);
    JsonGenerator* generator = json_generator_new();
    json_generator_set_root(generator, root);
    json_generator_set_pretty(generator, TRUE);
    gchar* jsonData = json_generator_to_data(generator, nullptr);

    // Write to file
    std::ofstream file(sessionPath);
    if (file.is_open()) {
        file << jsonData;
        file.close();
        std::cout << "💾 Saved session (" << tabs.size() << " tabs, " << tabGroups.size() << " groups)" << std::endl;
    } else {
        std::cerr << "ERROR: Could not write session to " << sessionPath << std::endl;
    }

    g_free(jsonData);
    g_object_unref(generator);
    json_node_free(root);
    g_object_unref(builder);
}

void BrayaWindow::restoreSession() {
    std::string sessionPath = std::string(g_get_home_dir()) + "/.config/braya-browser/session.json";

    // Check if session file exists
    if (!g_file_test(sessionPath.c_str(), G_FILE_TEST_EXISTS)) {
        std::cout << "ℹ️  No saved session found" << std::endl;
        return;
    }

    // Read file
    std::ifstream file(sessionPath);
    if (!file.is_open()) {
        std::cerr << "ERROR: Could not open session file" << std::endl;
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    std::string jsonContent = buffer.str();

    // Parse JSON
    GError* error = nullptr;
    JsonParser* parser = json_parser_new();

    if (!json_parser_load_from_data(parser, jsonContent.c_str(), -1, &error)) {
        std::cerr << "ERROR: Failed to parse session.json: " << error->message << std::endl;
        g_error_free(error);
        g_object_unref(parser);
        return;
    }

    JsonNode* root = json_parser_get_root(parser);
    if (!JSON_NODE_HOLDS_OBJECT(root)) {
        g_object_unref(parser);
        return;
    }

    JsonObject* rootObj = json_node_get_object(root);

    // Restore tabs
    if (json_object_has_member(rootObj, "tabs")) {
        JsonArray* tabsArray = json_object_get_array_member(rootObj, "tabs");
        int numTabs = json_array_get_length(tabsArray);

        std::cout << "🔄 Restoring " << numTabs << " tabs from session..." << std::endl;

        // 🚀 Phase 4: Smart Loading - Only load first 5 tabs immediately
        const int MAX_IMMEDIATE_LOAD = 5;

        for (int i = 0; i < numTabs; i++) {
            JsonObject* tabObj = json_array_get_object_element(tabsArray, i);

            const char* url = json_object_get_string_member(tabObj, "url");
            bool pinned = json_object_has_member(tabObj, "pinned") ?
                         json_object_get_boolean_member(tabObj, "pinned") : false;
            bool muted = json_object_has_member(tabObj, "muted") ?
                        json_object_get_boolean_member(tabObj, "muted") : false;

            // Create tab with the URL
            createTab(url);

            // Apply pinned/muted state
            if (tabs.size() > 0) {
                auto& tab = tabs.back();
                if (pinned) tab->setPinned(true);
                if (muted) tab->setMuted(true);

                // 🚀 Phase 4: Suspend tabs after the first 5 (or if not pinned and past first 5)
                // Don't suspend pinned tabs as they're likely important
                if (i >= MAX_IMMEDIATE_LOAD && !pinned) {
                    std::cout << "  💤 Auto-suspending tab " << (i + 1) << " for lazy loading" << std::endl;
                    tab->suspend();
                }
            }
        }
    }

    // Restore tab groups
    if (json_object_has_member(rootObj, "groups")) {
        JsonArray* groupsArray = json_object_get_array_member(rootObj, "groups");
        int numGroups = json_array_get_length(groupsArray);

        std::cout << "🔄 Restoring " << numGroups << " tab groups..." << std::endl;

        for (int i = 0; i < numGroups; i++) {
            JsonObject* groupObj = json_array_get_object_element(groupsArray, i);

            const char* name = json_object_get_string_member(groupObj, "name");
            const char* color = json_object_get_string_member(groupObj, "color");

            // Create the group
            createTabGroup(name, color);
            int groupId = tabGroups.size() - 1;

            // Add tabs to group
            if (json_object_has_member(groupObj, "tabIds")) {
                JsonArray* tabIdsArray = json_object_get_array_member(groupObj, "tabIds");
                int numTabIds = json_array_get_length(tabIdsArray);

                for (int j = 0; j < numTabIds; j++) {
                    int tabId = json_array_get_int_element(tabIdsArray, j);

                    // Find tab with this ID and add to group
                    for (size_t k = 0; k < tabs.size(); k++) {
                        if (tabs[k]->getTabId() == tabId) {
                            addTabToGroup(tabId, groupId);
                            break;
                        }
                    }
                }
            }
        }
    }

    // Restore active tab
    if (json_object_has_member(rootObj, "activeTabIndex")) {
        int savedActiveIndex = json_object_get_int_member(rootObj, "activeTabIndex");
        if (savedActiveIndex >= 0 && savedActiveIndex < tabs.size()) {
            switchToTab(savedActiveIndex);
        }
    }

    g_object_unref(parser);
    std::cout << "✓ Session restored successfully" << std::endl;
}

// Favicon Caching Implementation

void BrayaWindow::cacheFavicon(const std::string& url, GdkTexture* favicon) {
    if (!favicon || !GDK_IS_TEXTURE(favicon)) {
        return;
    }

    // Extract domain from URL for caching key
    std::string cacheKey = url;

    // Simple domain extraction (e.g., "https://example.com/page" -> "example.com")
    size_t protocolEnd = url.find("://");
    if (protocolEnd != std::string::npos) {
        size_t domainStart = protocolEnd + 3;
        size_t domainEnd = url.find("/", domainStart);
        if (domainEnd == std::string::npos) {
            domainEnd = url.length();
        }
        cacheKey = url.substr(domainStart, domainEnd - domainStart);
    }

    // 🛡️ MEMORY FIX: GObjectPtr handles ref counting automatically
    // Old favicon (if exists) is automatically unreffed when replaced
    // New favicon is automatically reffed by GObjectPtr constructor
    faviconCache[cacheKey] = GObjectPtr<GdkTexture>(favicon, true);

    std::cout << "💾 Cached favicon for: " << cacheKey << " (cache size: " << faviconCache.size() << ")" << std::endl;
}

GdkTexture* BrayaWindow::getCachedFavicon(const std::string& url) {
    // Extract domain from URL
    std::string cacheKey = url;

    size_t protocolEnd = url.find("://");
    if (protocolEnd != std::string::npos) {
        size_t domainStart = protocolEnd + 3;
        size_t domainEnd = url.find("/", domainStart);
        if (domainEnd == std::string::npos) {
            domainEnd = url.length();
        }
        cacheKey = url.substr(domainStart, domainEnd - domainStart);
    }

    auto it = faviconCache.find(cacheKey);
    if (it != faviconCache.end() && it->second) {
        // 🛡️ MEMORY FIX: Return raw pointer from GObjectPtr (doesn't transfer ownership)
        GdkTexture* texture = it->second.get();
        if (texture && GDK_IS_TEXTURE(texture)) {
            std::cout << "✓ Found cached favicon for: " << cacheKey << std::endl;
            return texture;
        }
    }

    return nullptr;
}

void BrayaWindow::navigateTo(const char* url) {
    if (!url || strlen(url) == 0) {
        std::cerr << "Empty URL provided" << std::endl;
        return;
    }

    if (activeTabIndex < 0 || activeTabIndex >= (int)tabs.size()) {
        std::cerr << "Invalid tab index: " << activeTabIndex << std::endl;
        return;
    }

    BrayaTab* tab = tabs[activeTabIndex].get();
    if (!tab || !tab->getWebView()) {
        std::cerr << "Invalid tab or webview" << std::endl;
        return;
    }
    
    try {
        std::string finalUrl = url;
        
        // Handle about:braya - load home page
        if (finalUrl == "about:braya") {
            finalUrl = "file://" + getResourcePath("home.html");
        }
        // Add https:// if needed
        else if (finalUrl.find("://") == std::string::npos) {
            if (finalUrl.find('.') != std::string::npos && finalUrl.find(' ') == std::string::npos) {
                finalUrl = "https://" + finalUrl;
            } else {
                // Use configured search engine
                std::string searchEngine = settings->getSearchEngine();
                std::string searchUrl;
                
                if (searchEngine == "Google") {
                    searchUrl = "https://www.google.com/search?q=";
                } else if (searchEngine == "Bing") {
                    searchUrl = "https://www.bing.com/search?q=";
                } else if (searchEngine == "Brave") {
                    searchUrl = "https://search.brave.com/search?q=";
                } else {
                    searchUrl = "https://duckduckgo.com/?q=";
                }
                
                finalUrl = searchUrl + finalUrl;
            }
        }
        
        // Hide bookmarks bar after first navigation (unless it's the home page)
        std::string homePage = settings->getHomePage();
        if (showBookmarksBar && finalUrl != homePage) {
            if (bookmarksBar && GTK_IS_WIDGET(bookmarksBar)) {
                gtk_widget_set_visible(bookmarksBar, FALSE);
                showBookmarksBar = false;
            }
        }
        
        webkit_web_view_load_uri(tab->getWebView(), finalUrl.c_str());
        
        // No status bar to update anymore
    } catch (const std::exception& e) {
        std::cerr << "Exception in navigateTo: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error in navigateTo" << std::endl;
    }
}

void BrayaWindow::updateUI() {
    if (activeTabIndex >= 0 && activeTabIndex < (int)tabs.size()) {
        BrayaTab* tab = tabs[activeTabIndex].get();
        WebKitWebView* webView = tab->getWebView();

        gtk_widget_set_sensitive(backBtn, webkit_web_view_can_go_back(webView));
        gtk_widget_set_sensitive(forwardBtn, webkit_web_view_can_go_forward(webView));

        // Update security indicator
        updateSecurityIndicator(tab->getUrl());
    }
}

void BrayaWindow::updateSecurityIndicator(const std::string& url) {
    if (!securityIcon) return;

    if (url.find("https://") == 0) {
        // Secure HTTPS connection
        gtk_image_set_from_icon_name(GTK_IMAGE(securityIcon), "channel-secure-symbolic");
        gtk_widget_set_tooltip_text(securityIcon, "Connection is secure (HTTPS)");
        gtk_widget_remove_css_class(securityIcon, "insecure");
        gtk_widget_add_css_class(securityIcon, "secure");
    } else if (url.find("http://") == 0) {
        // Insecure HTTP connection
        gtk_image_set_from_icon_name(GTK_IMAGE(securityIcon), "channel-insecure-symbolic");
        gtk_widget_set_tooltip_text(securityIcon, "Connection is not secure (HTTP)");
        gtk_widget_remove_css_class(securityIcon, "secure");
        gtk_widget_add_css_class(securityIcon, "insecure");
    } else {
        // Local or special pages (about:, file:, etc.)
        gtk_image_set_from_icon_name(GTK_IMAGE(securityIcon), "security-medium-symbolic");
        gtk_widget_set_tooltip_text(securityIcon, "Local or internal page");
        gtk_widget_remove_css_class(securityIcon, "secure");
        gtk_widget_remove_css_class(securityIcon, "insecure");
    }
}

void BrayaWindow::showHistory() {
    if (history) {
        history->showHistoryDialog(GTK_WINDOW(window));
    }
}

void BrayaWindow::showDownloads() {
    if (downloads) {
        downloads->showDownloadsDialog(GTK_WINDOW(window));
    }
}

void BrayaWindow::showFindBar() {
    gtk_widget_set_visible(findBar, TRUE);
    gtk_widget_grab_focus(findEntry);
}

void BrayaWindow::hideFindBar() {
    gtk_widget_set_visible(findBar, FALSE);
    gtk_editable_set_text(GTK_EDITABLE(findEntry), "");
    gtk_label_set_text(GTK_LABEL(findMatchLabel), "");
    
    // Clear find in current tab
    if (activeTabIndex >= 0 && activeTabIndex < (int)tabs.size()) {
        BrayaTab* tab = tabs[activeTabIndex].get();
        WebKitFindController* findController = webkit_web_view_get_find_controller(tab->getWebView());
        webkit_find_controller_search_finish(findController);
    }
}

void BrayaWindow::findNext() {
    if (activeTabIndex < 0 || activeTabIndex >= (int)tabs.size()) return;

    BrayaTab* tab = tabs[activeTabIndex].get();
    WebKitFindController* findController = webkit_web_view_get_find_controller(tab->getWebView());
    webkit_find_controller_search_next(findController);
}

void BrayaWindow::findPrevious() {
    if (activeTabIndex < 0 || activeTabIndex >= (int)tabs.size()) return;

    BrayaTab* tab = tabs[activeTabIndex].get();
    WebKitFindController* findController = webkit_web_view_get_find_controller(tab->getWebView());
    webkit_find_controller_search_previous(findController);
}

void BrayaWindow::showBookmarksManager() {
    if (bookmarksManager) {
        bookmarksManager->showBookmarksManager(GTK_WINDOW(window));
    }
}

void BrayaWindow::show() {
    gtk_window_present(GTK_WINDOW(window));
}

// Callbacks
void BrayaWindow::onNewTabClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    window->createTab();
    // Show bookmarks bar on new tab
    if (window->bookmarksBar && GTK_IS_WIDGET(window->bookmarksBar)) {
        gtk_widget_set_visible(window->bookmarksBar, TRUE);
        window->showBookmarksBar = true;
    }
}

void BrayaWindow::onTabClicked(GtkWidget* widget, gpointer data) {
    if (!data || !widget) return;
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "tab-index"));
    if (index >= 0 && index < (int)window->tabs.size()) {
        window->switchToTab(index);
    }
}

void BrayaWindow::onTabMiddleClick(GtkGestureClick* gesture, int n_press, double x, double y, gpointer data) {
    if (!data) return;
    
    GtkWidget* widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
    if (!widget) return;
    
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "tab-index"));
    
    if (index >= 0 && index < (int)window->tabs.size() && window->tabs.size() > 1) {
        window->closeTab(index);
    }
}

void BrayaWindow::onCloseTabClicked(GtkWidget* widget, gpointer data) {
    // Get window and index from button itself
    BrayaWindow* window = static_cast<BrayaWindow*>(g_object_get_data(G_OBJECT(widget), "braya-window"));
    int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "tab-index"));
    
    std::cout << "🔴 CLOSE CLICKED! Widget: " << widget << " Window: " << window << " Index: " << index << std::endl;
    
    if (!window) {
        std::cerr << "ERROR: No window!" << std::endl;
        return;
    }
    
    std::cout << "   Tabs size: " << window->tabs.size() << std::endl;
    
    if (index >= 0 && index < (int)window->tabs.size() && window->tabs.size() > 1) {
        std::cout << "✅ Closing tab " << index << std::endl;
        window->closeTab(index);
    } else {
        std::cout << "❌ Can't close - only one tab or invalid index" << std::endl;
    }
}

void BrayaWindow::onUrlActivate(GtkWidget* widget, gpointer data) {
    if (!data || !widget) return;
    try {
        BrayaWindow* window = static_cast<BrayaWindow*>(data);
        const char* url = gtk_editable_get_text(GTK_EDITABLE(widget));
        if (url && strlen(url) > 0) {
            window->navigateTo(url);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in onUrlActivate: " << e.what() << std::endl;
    }
}

void BrayaWindow::onBackClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    if (!window || window->activeTabIndex < 0 || window->activeTabIndex >= (int)window->tabs.size()) {
        return;
    }
    BrayaTab* tab = window->tabs[window->activeTabIndex].get();
    if (tab && tab->getWebView() && WEBKIT_IS_WEB_VIEW(tab->getWebView())) {
        webkit_web_view_go_back(tab->getWebView());
    }
}

void BrayaWindow::onForwardClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    if (!window || window->activeTabIndex < 0 || window->activeTabIndex >= (int)window->tabs.size()) {
        return;
    }
    BrayaTab* tab = window->tabs[window->activeTabIndex].get();
    if (tab && tab->getWebView() && WEBKIT_IS_WEB_VIEW(tab->getWebView())) {
        webkit_web_view_go_forward(tab->getWebView());
    }
}

void BrayaWindow::onReloadClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    if (!window || window->activeTabIndex < 0 || window->activeTabIndex >= (int)window->tabs.size()) {
        return;
    }
    BrayaTab* tab = window->tabs[window->activeTabIndex].get();
    webkit_web_view_reload(tab->getWebView());
}

void BrayaWindow::onHomeClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    if (!window || !window->settings) {
        return;
    }
    std::string homePage = window->settings->getHomePage();
    window->navigateTo(homePage.c_str());
}

void BrayaWindow::onBookmarkClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    const char* url = (const char*)g_object_get_data(G_OBJECT(widget), "url");
    if (url) {
        window->navigateTo(url);
    }
}

void BrayaWindow::onSettingsClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    window->settings->show(GTK_WINDOW(window->window));
}

void BrayaWindow::onAdBlockerShieldClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    
    // Create popover
    GtkWidget* popover = gtk_popover_new();
    gtk_widget_set_parent(popover, widget);
    
    // Create popover content
    GtkWidget* popoverBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(popoverBox, 12);
    gtk_widget_set_margin_end(popoverBox, 12);
    gtk_widget_set_margin_top(popoverBox, 12);
    gtk_widget_set_margin_bottom(popoverBox, 12);
    
    // Title
    GtkWidget* title = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(title), "<b>🛡️  Ad Blocker</b>");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(popoverBox), title);
    
    // Separator
    GtkWidget* separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_append(GTK_BOX(popoverBox), separator);
    
    // Toggle row
    GtkWidget* toggleRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    
    GtkWidget* toggleLabel = gtk_label_new("Enable Ad Blocking");
    gtk_widget_set_hexpand(toggleLabel, TRUE);
    gtk_widget_set_halign(toggleLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(toggleRow), toggleLabel);
    
    GtkWidget* toggleSwitch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(toggleSwitch), window->adBlocker && window->adBlocker->isEnabled());
    
    // Store references in a struct for the callback
    struct ToggleData {
        BrayaWindow* window;
        GtkWidget* popover;
    };
    ToggleData* toggleData = new ToggleData{window, popover};
    
    g_signal_connect_data(toggleSwitch, "state-set",
        G_CALLBACK(+[](GtkSwitch* switchWidget, gboolean state, gpointer userData) -> gboolean {
            ToggleData* data = static_cast<ToggleData*>(userData);
            if (data->window->adBlocker) {
                if (state) {
                    data->window->adBlocker->enable();
                } else {
                    data->window->adBlocker->disable();
                }
                // Update shield icon
                data->window->updateAdBlockerShield();
            }
            return FALSE;
        }), toggleData,
        [](gpointer data, GClosure*) {
            delete static_cast<ToggleData*>(data);
        }, GConnectFlags(0));
    
    gtk_box_append(GTK_BOX(toggleRow), toggleSwitch);
    gtk_box_append(GTK_BOX(popoverBox), toggleRow);
    
    // Info text
    GtkWidget* infoLabel = gtk_label_new("Blocks ads and trackers on websites");
    gtk_widget_add_css_class(infoLabel, "dim-label");
    gtk_label_set_wrap(GTK_LABEL(infoLabel), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(infoLabel), 30);
    gtk_widget_set_halign(infoLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(popoverBox), infoLabel);
    
    // Separator
    GtkWidget* separator2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_append(GTK_BOX(popoverBox), separator2);
    
    // Settings button
    GtkWidget* settingsBtn = gtk_button_new_with_label("Settings...");
    gtk_widget_set_halign(settingsBtn, GTK_ALIGN_CENTER);
    
    // Store window reference for settings button
    g_signal_connect_data(settingsBtn, "clicked",
        G_CALLBACK(+[](GtkButton* button, gpointer userData) {
            ToggleData* data = static_cast<ToggleData*>(userData);
            gtk_popover_popdown(GTK_POPOVER(data->popover));
            if (data->window->settings) {
                data->window->settings->showTab(GTK_WINDOW(data->window->window), "adblocker");
            }
        }), toggleData,
        nullptr, GConnectFlags(0));
    
    gtk_box_append(GTK_BOX(popoverBox), settingsBtn);
    
    gtk_popover_set_child(GTK_POPOVER(popover), popoverBox);
    
    // Show popover
    gtk_popover_popup(GTK_POPOVER(popover));
}

void BrayaWindow::onDownloadsClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    window->showDownloads();
}

void BrayaWindow::onFindNextClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    window->findNext();
}

void BrayaWindow::onFindPrevClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    window->findPrevious();
}

void BrayaWindow::onFindCloseClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    window->hideFindBar();
}

void BrayaWindow::onFindEntryChanged(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    if (window->activeTabIndex < 0 || window->activeTabIndex >= (int)window->tabs.size()) return;
    
    const char* searchText = gtk_editable_get_text(GTK_EDITABLE(widget));
    if (strlen(searchText) == 0) {
        gtk_label_set_text(GTK_LABEL(window->findMatchLabel), "");
        return;
    }
    
    BrayaTab* tab = window->tabs[window->activeTabIndex].get();
    WebKitFindController* findController = webkit_web_view_get_find_controller(tab->getWebView());
    
    // Start new search
    webkit_find_controller_search(findController, searchText, 
        WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE | WEBKIT_FIND_OPTIONS_WRAP_AROUND,
        G_MAXUINT);
    
    // Connect to counted signal to update match label
    g_signal_connect(findController, "counted-matches",
        G_CALLBACK(+[](WebKitFindController* controller, guint matchCount, gpointer data) {
            BrayaWindow* window = static_cast<BrayaWindow*>(data);
            if (matchCount > 0) {
                std::string label = std::to_string(matchCount) + " matches";
                gtk_label_set_text(GTK_LABEL(window->findMatchLabel), label.c_str());
            } else {
                gtk_label_set_text(GTK_LABEL(window->findMatchLabel), "No matches");
            }
        }), window);
}

void BrayaWindow::onDevToolsClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    if (window->activeTabIndex >= 0 && window->activeTabIndex < (int)window->tabs.size()) {
        BrayaTab* tab = window->tabs[window->activeTabIndex].get();
        WebKitWebInspector* inspector = webkit_web_view_get_inspector(tab->getWebView());
        webkit_web_inspector_show(inspector);
    }
}

void BrayaWindow::onMinimizeClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    gtk_window_minimize(GTK_WINDOW(window->window));
}

void BrayaWindow::onMaximizeClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    if (gtk_window_is_maximized(GTK_WINDOW(window->window))) {
        gtk_window_unmaximize(GTK_WINDOW(window->window));
    } else {
        gtk_window_maximize(GTK_WINDOW(window->window));
    }
}

void BrayaWindow::onWindowCloseClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    gtk_window_close(GTK_WINDOW(window->window));
}

gboolean BrayaWindow::onKeyPress(GtkEventControllerKey* controller, guint keyval, guint keycode, GdkModifierType state, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    
    if (state & GDK_CONTROL_MASK) {
        if (keyval == GDK_KEY_t) {
            window->createTab();
            return TRUE;
        } else if (keyval == GDK_KEY_T && (state & GDK_SHIFT_MASK)) {
            // Ctrl+Shift+T - Reopen closed tab
            window->reopenClosedTab();
            return TRUE;
        } else if (keyval == GDK_KEY_w) {
            if (window->tabs.size() > 1 && window->activeTabIndex >= 0 && window->activeTabIndex < (int)window->tabs.size()) {
                window->closeTab(window->activeTabIndex);
            }
            return TRUE;
        } else if (keyval == GDK_KEY_l) {
            gtk_widget_grab_focus(window->urlEntry);
            return TRUE;
        } else if (keyval == GDK_KEY_r) {
            window->onReloadClicked(NULL, window);
            return TRUE;
        } else if (keyval == GDK_KEY_h) {
            window->showHistory();
            return TRUE;
        } else if (keyval == GDK_KEY_j) {
            window->showDownloads();
            return TRUE;
        } else if (keyval == GDK_KEY_f) {
            window->showFindBar();
            return TRUE;
        } else if (keyval == GDK_KEY_b) {
            // Toggle sidebar (like Zen browser)
            gboolean visible = gtk_widget_get_visible(window->sidebar);
            gtk_widget_set_visible(window->sidebar, !visible);
            return TRUE;
        } else if (keyval == GDK_KEY_d || keyval == GDK_KEY_D) {
            window->onAddBookmarkClicked(NULL, window);
            return TRUE;
        }
    }
    
    // Ctrl+Shift shortcuts
    if ((state & GDK_CONTROL_MASK) && (state & GDK_SHIFT_MASK)) {
        if (keyval == GDK_KEY_S || keyval == GDK_KEY_s) {
            window->takeScreenshot();
            return TRUE;
        } else if (keyval == GDK_KEY_D || keyval == GDK_KEY_d) {
            window->toggleSplitView();
            return TRUE;
        } else if (keyval == GDK_KEY_P || keyval == GDK_KEY_p) {
            // Password Manager - now opens Settings→Passwords
            if (window->settings) {
                window->settings->showTab(GTK_WINDOW(window->window), "passwords");
            }
            return TRUE;
        } else if (keyval == GDK_KEY_B || keyval == GDK_KEY_b) {
            // Toggle bookmarks bar visibility
            if (window->bookmarksBar && GTK_IS_WIDGET(window->bookmarksBar)) {
                gboolean visible = gtk_widget_get_visible(window->bookmarksBar);
                gtk_widget_set_visible(window->bookmarksBar, !visible);
                window->showBookmarksBar = !visible;
                std::cout << "✓ Bookmarks bar " << (!visible ? "shown" : "hidden") << std::endl;
            }
            return TRUE;
        }
    }
    
    // Alt+Shift shortcuts  
    if ((state & GDK_ALT_MASK) && (state & GDK_SHIFT_MASK)) {
        if (keyval == GDK_KEY_R || keyval == GDK_KEY_r) {
            window->toggleReaderMode();
            return TRUE;
        }
    }
    
    if (keyval == GDK_KEY_F12) {
        if (window->activeTabIndex >= 0 && window->activeTabIndex < (int)window->tabs.size()) {
            BrayaTab* tab = window->tabs[window->activeTabIndex].get();
            WebKitWebInspector* inspector = webkit_web_view_get_inspector(tab->getWebView());
            webkit_web_inspector_show(inspector);
        }
        return TRUE;
    }
    
    return FALSE;
}

// Tab Groups Implementation
void BrayaWindow::createTabGroup(const std::string& name, const std::string& color) {
    auto group = std::make_unique<TabGroup>(name, color);
    tabGroups.push_back(std::move(group));
    std::cout << "✓ Created tab group: " << name << " (" << color << ")" << std::endl;
}

void BrayaWindow::addTabToGroup(int tabId, int groupId) {
    if (groupId < 0 || groupId >= tabGroups.size()) return;

    // Remove from old group if exists
    removeTabFromGroup(tabId);

    // Add to new group
    tabGroups[groupId]->addTab(tabId);
    tabToGroup[tabId] = groupId;

    // Update tab visual indicator
    for (auto& tab : tabs) {
        if (tab->getTabId() == tabId) {
            GtkWidget* tabButton = tab->getTabButton();
            if (tabButton) {
                // Add colored border indicator using CSS
                std::string colorClass = "tab-group-" + std::to_string(groupId);
                gtk_widget_add_css_class(tabButton, colorClass.c_str());

                // Create CSS provider for this color if it doesn't exist
                std::string cssData = "." + colorClass + " { border-left: 4px solid " + tabGroups[groupId]->getColor() + "; }";
                GtkCssProvider* provider = gtk_css_provider_new();
                gtk_css_provider_load_from_string(provider, cssData.c_str());
                gtk_style_context_add_provider_for_display(
                    gdk_display_get_default(),
                    GTK_STYLE_PROVIDER(provider),
                    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
                );
                g_object_unref(provider);

                std::cout << "✓ Added tab " << tabId << " to group '" << tabGroups[groupId]->getName()
                          << "' with color " << tabGroups[groupId]->getColor() << std::endl;
            }
        }
    }
}

void BrayaWindow::removeTabFromGroup(int tabId) {
    auto it = tabToGroup.find(tabId);
    if (it != tabToGroup.end()) {
        int groupId = it->second;
        if (groupId >= 0 && groupId < tabGroups.size()) {
            tabGroups[groupId]->removeTab(tabId);

            // Remove visual indicator
            for (auto& tab : tabs) {
                if (tab->getTabId() == tabId) {
                    GtkWidget* tabButton = tab->getTabButton();
                    if (tabButton) {
                        std::string colorClass = "tab-group-" + std::to_string(groupId);
                        gtk_widget_remove_css_class(tabButton, colorClass.c_str());
                    }
                }
            }
        }
        tabToGroup.erase(it);
    }
}

void BrayaWindow::toggleGroupCollapse(int groupId) {
    if (groupId < 0 || groupId >= tabGroups.size()) return;
    
    TabGroup* group = tabGroups[groupId].get();
    group->setCollapsed(!group->isCollapsed());
    
    // Hide/show tabs in the group
    for (int tabId : group->getTabs()) {
        for (auto& tab : tabs) {
            if (tab->getTabId() == tabId) {
                GtkWidget* tabButton = tab->getTabButton();
                if (tabButton) {
                    gtk_widget_set_visible(tabButton, !group->isCollapsed());
                }
            }
        }
    }
}

void BrayaWindow::showTabContextMenu(int tabId) {
    GtkWidget* menu = gtk_popover_menu_new_from_model(nullptr);
    GMenu* menuModel = g_menu_new();
    
    // Add menu items
    GMenuItem* newGroup = g_menu_item_new("New Group", nullptr);
    g_menu_append_item(menuModel, newGroup);
    
    // Add existing groups
    for (size_t i = 0; i < tabGroups.size(); i++) {
        std::string label = "Add to: " + tabGroups[i]->getName();
        GMenuItem* item = g_menu_item_new(label.c_str(), nullptr);
        g_menu_append_item(menuModel, item);
        g_object_unref(item);
    }
    
    g_object_unref(newGroup);
    gtk_popover_set_child(GTK_POPOVER(menu), GTK_WIDGET(menuModel));
}

void BrayaWindow::onTabRightClick(GtkGestureClick* gesture, int n_press, double x, double y, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);

    // Get the tab button that was clicked
    GtkWidget* tabButton = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
    int tabIndex = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(tabButton), "tab-index"));

    if (tabIndex < 0 || tabIndex >= window->tabs.size()) {
        return;
    }

    BrayaTab* tab = window->tabs[tabIndex].get();

    // Show context menu
    GtkWidget* menu = gtk_popover_new();
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_margin_start(box, 10);
    gtk_widget_set_margin_end(box, 10);
    gtk_widget_set_margin_top(box, 10);
    gtk_widget_set_margin_bottom(box, 10);

    // Pin/Unpin button
    std::string pinLabel = tab->isPinned() ? "📍 Unpin Tab" : "📌 Pin Tab";
    GtkWidget* pinBtn = gtk_button_new_with_label(pinLabel.c_str());
    g_object_set_data(G_OBJECT(pinBtn), "tab", tab);
    g_signal_connect(pinBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        BrayaTab* t = static_cast<BrayaTab*>(g_object_get_data(G_OBJECT(btn), "tab"));
        if (t) {
            t->setPinned(!t->isPinned());
        }
    }), nullptr);
    gtk_box_append(GTK_BOX(box), pinBtn);

    // Mute/Unmute button (only show if tab has audio or is muted)
    bool hasAudio = webkit_web_view_is_playing_audio(tab->getWebView());
    if (hasAudio || tab->isMuted()) {
        std::string muteLabel = tab->isMuted() ? "🔊 Unmute Tab" : "🔇 Mute Tab";
        GtkWidget* muteBtn = gtk_button_new_with_label(muteLabel.c_str());
        g_object_set_data(G_OBJECT(muteBtn), "tab", tab);
        g_signal_connect(muteBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
            BrayaTab* t = static_cast<BrayaTab*>(g_object_get_data(G_OBJECT(btn), "tab"));
            if (t) {
                t->setMuted(!t->isMuted());
            }
        }), nullptr);
        gtk_box_append(GTK_BOX(box), muteBtn);
    }

    // Separator
    GtkWidget* separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_append(GTK_BOX(box), separator);

    // Recently Closed Tabs submenu
    if (!window->recentlyClosedTabs.empty()) {
        GtkWidget* reopenBtn = gtk_button_new_with_label("🔄 Reopen Closed Tab");
        g_object_set_data(G_OBJECT(reopenBtn), "window", window);
        g_signal_connect(reopenBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
            BrayaWindow* w = static_cast<BrayaWindow*>(g_object_get_data(G_OBJECT(btn), "window"));
            if (w) {
                w->reopenClosedTab();
            }
        }), nullptr);
        gtk_box_append(GTK_BOX(box), reopenBtn);

        // Show list of recently closed tabs (up to 5 in context menu)
        int count = std::min((int)window->recentlyClosedTabs.size(), 5);
        for (int i = window->recentlyClosedTabs.size() - 1; i >= window->recentlyClosedTabs.size() - count; i--) {
            const auto& closed = window->recentlyClosedTabs[i];
            std::string label = "  • " + closed.title;
            if (label.length() > 40) {
                label = label.substr(0, 37) + "...";
            }
            GtkWidget* itemBtn = gtk_button_new_with_label(label.c_str());
            gtk_widget_add_css_class(itemBtn, "dim-label");
            g_object_set_data(G_OBJECT(itemBtn), "window", window);
            g_object_set_data(G_OBJECT(itemBtn), "url", g_strdup(closed.url.c_str()));
            g_signal_connect(itemBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
                BrayaWindow* w = static_cast<BrayaWindow*>(g_object_get_data(G_OBJECT(btn), "window"));
                const char* url = (const char*)g_object_get_data(G_OBJECT(btn), "url");
                if (w && url) {
                    w->createTab(url);
                }
            }), nullptr);
            gtk_box_append(GTK_BOX(box), itemBtn);
        }

        GtkWidget* separator2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_box_append(GTK_BOX(box), separator2);
    }

    // Create group button
    GtkWidget* createBtn = gtk_button_new_with_label("📁 Create New Group");
    g_object_set_data(G_OBJECT(createBtn), "window", window);
    g_object_set_data(G_OBJECT(createBtn), "tab", tab);
    g_object_set_data(G_OBJECT(createBtn), "menu", menu);

    auto onCreateGroupClick = [](GtkButton* btn, gpointer data) -> void {
        BrayaWindow* w = static_cast<BrayaWindow*>(g_object_get_data(G_OBJECT(btn), "window"));
        BrayaTab* t = static_cast<BrayaTab*>(g_object_get_data(G_OBJECT(btn), "tab"));
        GtkWidget* m = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "menu"));

        if (!w || !t) return;

        // Close the context menu first
        gtk_popover_popdown(GTK_POPOVER(m));

        // Show dialog to create new group
        GtkWidget* dialog = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(dialog), "Create Tab Group");
        gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 200);
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(w->getWindow()));

        GtkWidget* dialogBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
        gtk_widget_set_margin_start(dialogBox, 20);
        gtk_widget_set_margin_end(dialogBox, 20);
        gtk_widget_set_margin_top(dialogBox, 20);
        gtk_widget_set_margin_bottom(dialogBox, 20);
        gtk_window_set_child(GTK_WINDOW(dialog), dialogBox);

        // Group name input
        GtkWidget* nameLabel = gtk_label_new("Group Name:");
        gtk_widget_set_halign(nameLabel, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(dialogBox), nameLabel);

        GtkWidget* nameEntry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(nameEntry), "My Group");
        gtk_box_append(GTK_BOX(dialogBox), nameEntry);

        // Color picker
        GtkWidget* colorLabel = gtk_label_new("Group Color:");
        gtk_widget_set_halign(colorLabel, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(dialogBox), colorLabel);

        GtkWidget* colorBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        const char* colors[] = {"#FF6B6B", "#4ECDC4", "#45B7D1", "#FFA07A", "#98D8C8", "#F7DC6F", "#BB8FCE", "#85C1E2"};
        const char* colorNames[] = {"Red", "Teal", "Blue", "Coral", "Mint", "Yellow", "Purple", "Sky"};

        for (int i = 0; i < 8; i++) {
            GtkWidget* colorBtn = gtk_button_new_with_label(colorNames[i]);
            gtk_widget_set_size_request(colorBtn, 60, 36);
            g_object_set_data_full(G_OBJECT(colorBtn), "color", g_strdup(colors[i]), g_free);

            // Set background color using CSS
            GtkCssProvider* provider = gtk_css_provider_new();
            std::string css = "button { background: " + std::string(colors[i]) + "; color: white; font-weight: bold; }";
            gtk_css_provider_load_from_string(provider, css.c_str());
            gtk_style_context_add_provider(gtk_widget_get_style_context(colorBtn),
                                          GTK_STYLE_PROVIDER(provider),
                                          GTK_STYLE_PROVIDER_PRIORITY_USER);
            g_object_unref(provider);

            g_object_set_data(G_OBJECT(colorBtn), "nameEntry", nameEntry);
            g_object_set_data(G_OBJECT(colorBtn), "window", w);
            g_object_set_data(G_OBJECT(colorBtn), "tab", t);
            g_object_set_data(G_OBJECT(colorBtn), "dialog", dialog);

            g_signal_connect(colorBtn, "clicked", G_CALLBACK(+[](GtkButton* cb, gpointer data) {
                const char* color = (const char*)g_object_get_data(G_OBJECT(cb), "color");
                GtkWidget* entry = GTK_WIDGET(g_object_get_data(G_OBJECT(cb), "nameEntry"));
                BrayaWindow* win = static_cast<BrayaWindow*>(g_object_get_data(G_OBJECT(cb), "window"));
                BrayaTab* tab = static_cast<BrayaTab*>(g_object_get_data(G_OBJECT(cb), "tab"));
                GtkWidget* dlg = GTK_WIDGET(g_object_get_data(G_OBJECT(cb), "dialog"));

                if (!color || !win || !tab) return;

                // Get group name
                const char* name = gtk_editable_get_text(GTK_EDITABLE(entry));
                std::string groupName = (name && strlen(name) > 0) ? name : "New Group";

                // Create the group
                win->createTabGroup(groupName, color);
                int groupId = win->tabGroups.size() - 1;

                // Add the current tab to the group
                win->addTabToGroup(tab->getTabId(), groupId);

                // Close dialog
                gtk_window_close(GTK_WINDOW(dlg));
            }), nullptr);

            gtk_box_append(GTK_BOX(colorBox), colorBtn);
        }
        gtk_box_append(GTK_BOX(dialogBox), colorBox);

        gtk_window_present(GTK_WINDOW(dialog));
    };

    g_signal_connect(createBtn, "clicked", G_CALLBACK(+onCreateGroupClick), nullptr);
    gtk_box_append(GTK_BOX(box), createBtn);

    // Add to existing group buttons
    for (size_t i = 0; i < window->tabGroups.size(); i++) {
        std::string label = "➕ " + window->tabGroups[i]->getName();
        GtkWidget* btn = gtk_button_new_with_label(label.c_str());
        g_object_set_data(G_OBJECT(btn), "window", window);
        g_object_set_data(G_OBJECT(btn), "tab", tab);
        g_object_set_data(G_OBJECT(btn), "group-id", GINT_TO_POINTER(i));
        g_object_set_data(G_OBJECT(btn), "menu", menu);
        g_signal_connect(btn, "clicked", G_CALLBACK(+[](GtkButton* b, gpointer data) {
            BrayaWindow* w = static_cast<BrayaWindow*>(g_object_get_data(G_OBJECT(b), "window"));
            BrayaTab* t = static_cast<BrayaTab*>(g_object_get_data(G_OBJECT(b), "tab"));
            int groupId = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b), "group-id"));
            GtkWidget* m = GTK_WIDGET(g_object_get_data(G_OBJECT(b), "menu"));

            if (w && t) {
                w->addTabToGroup(t->getTabId(), groupId);
                gtk_popover_popdown(GTK_POPOVER(m));
            }
        }), nullptr);
        gtk_box_append(GTK_BOX(box), btn);
    }

    gtk_popover_set_child(GTK_POPOVER(menu), box);
    gtk_widget_set_parent(menu, tabButton);
    gtk_popover_popup(GTK_POPOVER(menu));
}

void BrayaWindow::showPasswordManager() {
    // Password manager is now in Settings→Passwords tab
    if (settings) {
        settings->showTab(GTK_WINDOW(window), "passwords");
    }
}

// Quick Wins Feature Implementations

void BrayaWindow::toggleReaderMode() {
    if (activeTabIndex >= 0 && activeTabIndex < tabs.size()) {
        tabs[activeTabIndex]->toggleReaderMode();
    }
}

void BrayaWindow::takeScreenshot() {
    if (activeTabIndex < 0 || activeTabIndex >= tabs.size()) return;
    
    auto* webView = tabs[activeTabIndex]->getWebView();
    if (!webView) return;
    
    std::cout << "📸 Taking screenshot..." << std::endl;
    
    webkit_web_view_get_snapshot(
        webView,
        WEBKIT_SNAPSHOT_REGION_VISIBLE,
        WEBKIT_SNAPSHOT_OPTIONS_NONE,
        nullptr,
        [](GObject* object, GAsyncResult* result, gpointer user_data) {
            GError* error = nullptr;
            GdkTexture* texture = webkit_web_view_get_snapshot_finish(
                WEBKIT_WEB_VIEW(object), result, &error);
            
            if (error) {
                std::cerr << "✗ Screenshot error: " << error->message << std::endl;
                g_error_free(error);
                return;
            }
            
            if (texture) {
                // Generate filename
                time_t now = time(nullptr);
                char filename[256];
                strftime(filename, sizeof(filename), "braya-screenshot-%Y%m%d-%H%M%S.png", localtime(&now));
                
                // Save to Pictures directory
                const char* homeDir = g_get_home_dir();
                std::string filepath = std::string(homeDir) + "/Pictures/" + filename;
                
                // Save texture to file
                gboolean saved = gdk_texture_save_to_png(texture, filepath.c_str());
                
                if (saved) {
                    std::cout << "✓ Screenshot saved: " << filepath << std::endl;
                    
                    // Show notification with simple dialog
                    GtkWidget* parentWindow = GTK_WIDGET(user_data);
                    GtkWidget* dialog = gtk_window_new();
                    gtk_window_set_title(GTK_WINDOW(dialog), "Screenshot Saved");
                    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 120);
                    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parentWindow));
                    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
                    
                    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
                    gtk_widget_set_margin_start(box, 20);
                    gtk_widget_set_margin_end(box, 20);
                    gtk_widget_set_margin_top(box, 20);
                    gtk_widget_set_margin_bottom(box, 20);
                    gtk_window_set_child(GTK_WINDOW(dialog), box);
                    
                    GtkWidget* label = gtk_label_new(("Screenshot saved to:\n" + filepath).c_str());
                    gtk_box_append(GTK_BOX(box), label);
                    
                    GtkWidget* okBtn = gtk_button_new_with_label("OK");
                    gtk_widget_add_css_class(okBtn, "suggested-action");
                    g_signal_connect_swapped(okBtn, "clicked", G_CALLBACK(gtk_window_close), dialog);
                    gtk_box_append(GTK_BOX(box), okBtn);
                    
                    gtk_window_present(GTK_WINDOW(dialog));
                } else {
                    std::cerr << "✗ Failed to save screenshot" << std::endl;
                }
                
                g_object_unref(texture);
            }
        },
        window
    );
}

void BrayaWindow::toggleTabPin(int tabId) {
    for (auto& tab : tabs) {
        if (tab->getId() == tabId) {
            tab->setPinned(!tab->isPinned());
            updateUI();
            break;
        }
    }
}

void BrayaWindow::toggleTabMute(int tabId) {
    for (auto& tab : tabs) {
        if (tab->getId() == tabId) {
            tab->setMuted(!tab->isMuted());
            break;
        }
    }
}

// Quick Wins Callbacks

void BrayaWindow::onReaderModeClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    window->toggleReaderMode();
}

void BrayaWindow::onScreenshotClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    window->takeScreenshot();
}

void BrayaWindow::onAddBookmarkClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    
    if (window->activeTabIndex >= 0 && window->activeTabIndex < window->tabs.size()) {
        BrayaTab* tab = window->tabs[window->activeTabIndex].get();
        std::string title = tab->getTitle();
        std::string url = tab->getUrl();
        
        // Get favicon
        WebKitWebView* webView = tab->getWebView();
        GdkTexture* favicon = webkit_web_view_get_favicon(webView);
        
        // Add to bookmarks
        if (window->bookmarksManager) {
            window->bookmarksManager->addCurrentPage(title, url, favicon);
            
            // Update bookmarks bar
            if (window->bookmarksBar) {
                // Get the box widget (might be wrapped in viewport)
                GtkWidget* child = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(window->bookmarksBar));
                if (child) {
                    // Handle viewport wrapping
                    GtkWidget* boxWidget = child;
                    if (GTK_IS_VIEWPORT(child)) {
                        boxWidget = gtk_viewport_get_child(GTK_VIEWPORT(child));
                    }

                    if (boxWidget && GTK_IS_BOX(boxWidget)) {
                        window->bookmarksManager->updateBookmarksBar(
                            boxWidget,
                            window,
                            G_CALLBACK(onBookmarkBarItemClicked),
                            G_CALLBACK(onBookmarkBarAddClicked),
                            G_CALLBACK(onBookmarkBarItemRightClick)
                        );
                    }
                }
            }
            
            std::cout << "✓ Bookmarked: " << title << std::endl;
        }
    }
}

// Bookmarks Bar Callbacks

void BrayaWindow::onBookmarkBarItemClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    const char* url = (const char*)g_object_get_data(G_OBJECT(widget), "url");

    if (url && window) {
        std::cout << "✓ Navigating to bookmark: " << url << std::endl;
        window->navigateTo(url);
    }
}

void BrayaWindow::onBookmarkBarAddClicked(GtkWidget* widget, gpointer data) {
    // Reuse the existing add bookmark functionality
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    onAddBookmarkClicked(widget, data);
}

void BrayaWindow::onBookmarkBarItemRightClick(GtkGestureClick* gesture, int n_press, double x, double y, gpointer data) {
    GtkWidget* btn = GTK_WIDGET(data);  // The button that was right-clicked
    const char* url = (const char*)g_object_get_data(G_OBJECT(btn), "url");
    const char* name = (const char*)g_object_get_data(G_OBJECT(btn), "bookmark-name");

    if (!url || !name) return;

    // Get the window from the button's parent hierarchy
    GtkWidget* widget = btn;
    while (widget && !GTK_IS_WINDOW(widget)) {
        widget = gtk_widget_get_parent(widget);
    }
    if (!widget) return;

    // Create popover menu
    GtkWidget* popover = gtk_popover_new();
    gtk_widget_set_parent(popover, btn);

    GtkWidget* menuBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_popover_set_child(GTK_POPOVER(popover), menuBox);

    // Store window pointer for callbacks
    g_object_set_data(G_OBJECT(popover), "window", widget);

    // Edit button
    GtkWidget* editBtn = gtk_button_new_with_label("✏️ Edit Bookmark");
    gtk_widget_add_css_class(editBtn, "flat");
    g_object_set_data_full(G_OBJECT(editBtn), "bookmark-url", g_strdup(url), g_free);
    g_object_set_data_full(G_OBJECT(editBtn), "bookmark-name", g_strdup(name), g_free);
    g_signal_connect(editBtn, "clicked", G_CALLBACK(+[](GtkWidget* widget, gpointer data) {
        GtkWidget* popover = GTK_WIDGET(data);
        gtk_popover_popdown(GTK_POPOVER(popover));

        const char* url = (const char*)g_object_get_data(G_OBJECT(widget), "bookmark-url");
        const char* name = (const char*)g_object_get_data(G_OBJECT(widget), "bookmark-name");
        GtkWidget* parentWindow = GTK_WIDGET(g_object_get_data(G_OBJECT(popover), "window"));

        // Create edit dialog
        GtkWidget* dialog = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(dialog), "Edit Bookmark");
        gtk_window_set_default_size(GTK_WINDOW(dialog), 450, 250);
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parentWindow));
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
        gtk_widget_set_margin_start(box, 20);
        gtk_widget_set_margin_end(box, 20);
        gtk_widget_set_margin_top(box, 20);
        gtk_widget_set_margin_bottom(box, 20);
        gtk_window_set_child(GTK_WINDOW(dialog), box);

        // Name field
        GtkWidget* nameLabel = gtk_label_new("Name:");
        gtk_widget_set_halign(nameLabel, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(box), nameLabel);

        GtkWidget* nameEntry = gtk_entry_new();
        gtk_editable_set_text(GTK_EDITABLE(nameEntry), name);
        gtk_box_append(GTK_BOX(box), nameEntry);

        // URL field
        GtkWidget* urlLabel = gtk_label_new("URL:");
        gtk_widget_set_halign(urlLabel, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(box), urlLabel);

        GtkWidget* urlEntry = gtk_entry_new();
        gtk_editable_set_text(GTK_EDITABLE(urlEntry), url);
        gtk_box_append(GTK_BOX(box), urlEntry);

        // Folder field
        GtkWidget* folderLabel = gtk_label_new("Folder:");
        gtk_widget_set_halign(folderLabel, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(box), folderLabel);

        // Get current folder to pre-select
        std::string currentFolder;
        BrayaWindow* tempWindow = (BrayaWindow*)g_object_get_data(G_OBJECT(parentWindow), "braya-window-instance");
        if (tempWindow && tempWindow->bookmarksManager) {
            int bmIndex = tempWindow->bookmarksManager->findBookmarkByUrl(url);
            if (bmIndex >= 0) {
                auto bookmarks = tempWindow->bookmarksManager->getBookmarks();
                if (bmIndex < (int)bookmarks.size()) {
                    currentFolder = bookmarks[bmIndex].folder;
                }
            }
        }

        GtkStringList* folderModel = gtk_string_list_new(nullptr);
        std::vector<std::string> folders;
        guint selectedIndex = 0;
        if (tempWindow && tempWindow->bookmarksManager) {
            folders = tempWindow->bookmarksManager->getUniqueFolders();
            for (size_t i = 0; i < folders.size(); i++) {
                gtk_string_list_append(folderModel, folders[i].c_str());
                if (folders[i] == currentFolder || (currentFolder.empty() && folders[i] == "Bookmarks Bar")) {
                    selectedIndex = i;
                }
            }
        } else {
            gtk_string_list_append(folderModel, "Bookmarks Bar");
        }
        gtk_string_list_append(folderModel, "Other Bookmarks");
        gtk_string_list_append(folderModel, "📁 New Folder...");

        GtkWidget* folderDropdown = gtk_drop_down_new(G_LIST_MODEL(folderModel), nullptr);
        gtk_drop_down_set_selected(GTK_DROP_DOWN(folderDropdown), selectedIndex);
        gtk_box_append(GTK_BOX(box), folderDropdown);

        // Buttons
        GtkWidget* btnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_box_append(GTK_BOX(box), btnBox);

        GtkWidget* saveBtn = gtk_button_new_with_label("Save");
        gtk_widget_set_hexpand(saveBtn, TRUE);
        gtk_widget_add_css_class(saveBtn, "suggested-action");
        gtk_box_append(GTK_BOX(btnBox), saveBtn);

        GtkWidget* cancelBtn = gtk_button_new_with_label("Cancel");
        gtk_widget_set_hexpand(cancelBtn, TRUE);
        gtk_box_append(GTK_BOX(btnBox), cancelBtn);

        // Get the BrayaWindow instance from the original window
        // We need to traverse back to find it
        BrayaWindow* brayaWindow = nullptr;
        g_object_get_data(G_OBJECT(parentWindow), "braya-window-instance");  // Will add this

        // Store data for save callback
        g_object_set_data_full(G_OBJECT(saveBtn), "old-url", g_strdup(url), g_free);
        g_object_set_data(G_OBJECT(saveBtn), "name-entry", nameEntry);
        g_object_set_data(G_OBJECT(saveBtn), "url-entry", urlEntry);
        g_object_set_data(G_OBJECT(saveBtn), "folder-dropdown", folderDropdown);
        g_object_set_data(G_OBJECT(saveBtn), "dialog", dialog);
        g_object_set_data(G_OBJECT(saveBtn), "parent-window", parentWindow);

        g_signal_connect(saveBtn, "clicked", G_CALLBACK(+[](GtkWidget* btn, gpointer data) {
            const char* oldUrl = (const char*)g_object_get_data(G_OBJECT(btn), "old-url");
            GtkWidget* nameEntry = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "name-entry"));
            GtkWidget* urlEntry = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "url-entry"));
            GtkWidget* folderDropdown = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "folder-dropdown"));
            GtkWidget* dialog = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "dialog"));
            GtkWidget* parentWindow = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "parent-window"));

            const char* newName = gtk_editable_get_text(GTK_EDITABLE(nameEntry));
            const char* newUrl = gtk_editable_get_text(GTK_EDITABLE(urlEntry));

            // Get selected folder from dropdown
            GtkDropDown* dropdown = GTK_DROP_DOWN(folderDropdown);
            guint selected = gtk_drop_down_get_selected(dropdown);
            GtkStringList* model = GTK_STRING_LIST(gtk_drop_down_get_model(dropdown));
            const char* folder = gtk_string_list_get_string(model, selected);

            std::string finalFolder = folder;

            // Handle "New Folder..." option
            if (g_str_has_prefix(folder, "📁 New Folder")) {
                GtkWindow* parent = GTK_WINDOW(dialog);
                std::string customFolder = BrayaBookmarks::showNewFolderDialog(parent);
                if (!customFolder.empty()) {
                    finalFolder = customFolder;
                } else {
                    finalFolder = "Other Bookmarks";  // Default if canceled
                }
            }

            const char* newFolder = finalFolder.c_str();

            // Get BrayaWindow instance
            BrayaWindow* window = (BrayaWindow*)g_object_get_data(G_OBJECT(parentWindow), "braya-window-instance");

            if (window && window->bookmarksManager) {
                // If a new folder was created, ensure it exists
                if (g_str_has_prefix(folder, "📁 New Folder") && !finalFolder.empty() && finalFolder != "Other Bookmarks") {
                    // Check if folder already exists
                    if (!window->bookmarksManager->findFolder(finalFolder)) {
                        // Create the folder if it doesn't exist
                        window->bookmarksManager->addFolder(finalFolder);
                        std::cout << "✓ Created new folder: " << finalFolder << std::endl;
                    }
                }

                // Edit the bookmark
                window->bookmarksManager->editBookmarkByUrl(oldUrl, newName, newUrl, newFolder);

                // Refresh the bookmarks bar
                window->refreshBookmarksBar();

                std::cout << "✓ Edited bookmark: " << oldUrl << " → " << newUrl << std::endl;
            } else {
                std::cerr << "ERROR: Could not get BrayaWindow instance to edit bookmark!" << std::endl;
            }

            gtk_window_destroy(GTK_WINDOW(dialog));
        }), nullptr);

        g_signal_connect(cancelBtn, "clicked", G_CALLBACK(+[](GtkWidget* btn, gpointer data) {
            GtkWidget* dialog = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "dialog"));
            gtk_window_destroy(GTK_WINDOW(dialog));
        }), nullptr);
        g_object_set_data(G_OBJECT(cancelBtn), "dialog", dialog);

        gtk_window_present(GTK_WINDOW(dialog));
    }), popover);
    gtk_box_append(GTK_BOX(menuBox), editBtn);

    // Delete button
    GtkWidget* deleteBtn = gtk_button_new_with_label("🗑️ Delete Bookmark");
    gtk_widget_add_css_class(deleteBtn, "flat");
    gtk_widget_add_css_class(deleteBtn, "destructive-action");
    g_object_set_data_full(G_OBJECT(deleteBtn), "bookmark-url", g_strdup(url), g_free);
    g_object_set_data_full(G_OBJECT(deleteBtn), "bookmark-name", g_strdup(name), g_free);
    g_signal_connect(deleteBtn, "clicked", G_CALLBACK(+[](GtkWidget* widget, gpointer data) {
        GtkWidget* popover = GTK_WIDGET(data);
        gtk_popover_popdown(GTK_POPOVER(popover));

        const char* url = (const char*)g_object_get_data(G_OBJECT(widget), "bookmark-url");
        const char* name = (const char*)g_object_get_data(G_OBJECT(widget), "bookmark-name");
        GtkWidget* parentWindow = GTK_WIDGET(g_object_get_data(G_OBJECT(popover), "window"));

        // Create confirmation dialog
        GtkWidget* dialog = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(dialog), "Delete Bookmark");
        gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 150);
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parentWindow));
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
        gtk_widget_set_margin_start(box, 20);
        gtk_widget_set_margin_end(box, 20);
        gtk_widget_set_margin_top(box, 20);
        gtk_widget_set_margin_bottom(box, 20);
        gtk_window_set_child(GTK_WINDOW(dialog), box);

        std::string message = std::string("Are you sure you want to delete:\n\n") + name + "\n" + url;
        GtkWidget* label = gtk_label_new(message.c_str());
        gtk_label_set_wrap(GTK_LABEL(label), TRUE);
        gtk_box_append(GTK_BOX(box), label);

        GtkWidget* btnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_box_append(GTK_BOX(box), btnBox);

        GtkWidget* deleteConfirmBtn = gtk_button_new_with_label("Delete");
        gtk_widget_set_hexpand(deleteConfirmBtn, TRUE);
        gtk_widget_add_css_class(deleteConfirmBtn, "destructive-action");
        g_object_set_data_full(G_OBJECT(deleteConfirmBtn), "bookmark-url", g_strdup(url), g_free);
        g_object_set_data(G_OBJECT(deleteConfirmBtn), "dialog", dialog);
        gtk_box_append(GTK_BOX(btnBox), deleteConfirmBtn);

        GtkWidget* cancelBtn = gtk_button_new_with_label("Cancel");
        gtk_widget_set_hexpand(cancelBtn, TRUE);
        g_object_set_data(G_OBJECT(cancelBtn), "dialog", dialog);
        gtk_box_append(GTK_BOX(btnBox), cancelBtn);

        // Store parent window reference
        g_object_set_data(G_OBJECT(deleteConfirmBtn), "parent-window", parentWindow);

        g_signal_connect(deleteConfirmBtn, "clicked", G_CALLBACK(+[](GtkWidget* btn, gpointer data) {
            const char* url = (const char*)g_object_get_data(G_OBJECT(btn), "bookmark-url");
            GtkWidget* dialog = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "dialog"));
            GtkWidget* parentWindow = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "parent-window"));

            // Get BrayaWindow instance
            BrayaWindow* window = (BrayaWindow*)g_object_get_data(G_OBJECT(parentWindow), "braya-window-instance");

            if (window && window->bookmarksManager) {
                // Delete the bookmark
                window->bookmarksManager->deleteBookmarkByUrl(url);

                // Refresh the bookmarks bar
                window->refreshBookmarksBar();

                std::cout << "✓ Deleted bookmark: " << url << std::endl;
            } else {
                std::cerr << "ERROR: Could not get BrayaWindow instance to delete bookmark!" << std::endl;
            }

            gtk_window_destroy(GTK_WINDOW(dialog));
        }), nullptr);

        g_signal_connect(cancelBtn, "clicked", G_CALLBACK(+[](GtkWidget* btn, gpointer data) {
            GtkWidget* dialog = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "dialog"));
            gtk_window_destroy(GTK_WINDOW(dialog));
        }), nullptr);

        gtk_window_present(GTK_WINDOW(dialog));
    }), popover);
    gtk_box_append(GTK_BOX(menuBox), deleteBtn);

    // Copy URL button
    GtkWidget* copyBtn = gtk_button_new_with_label("📋 Copy URL");
    gtk_widget_add_css_class(copyBtn, "flat");
    g_object_set_data_full(G_OBJECT(copyBtn), "bookmark-url", g_strdup(url), g_free);
    g_signal_connect(copyBtn, "clicked", G_CALLBACK(+[](GtkWidget* widget, gpointer data) {
        GtkWidget* popover = GTK_WIDGET(data);
        const char* url = (const char*)g_object_get_data(G_OBJECT(widget), "bookmark-url");
        GdkClipboard* clipboard = gdk_display_get_clipboard(gdk_display_get_default());
        gdk_clipboard_set_text(clipboard, url);
        std::cout << "✓ Copied URL to clipboard: " << url << std::endl;
        gtk_popover_popdown(GTK_POPOVER(popover));
    }), popover);
    gtk_box_append(GTK_BOX(menuBox), copyBtn);

    // Add separator
    GtkWidget* separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_append(GTK_BOX(menuBox), separator);

    // New Folder button
    GtkWidget* newFolderBtn = gtk_button_new_with_label("📁 New Folder");
    gtk_widget_add_css_class(newFolderBtn, "flat");
    g_signal_connect(newFolderBtn, "clicked", G_CALLBACK(+[](GtkWidget* widget, gpointer data) {
        GtkWidget* popover = GTK_WIDGET(data);
        GtkWidget* parentWindow = GTK_WIDGET(g_object_get_data(G_OBJECT(popover), "window"));
        gtk_popover_popdown(GTK_POPOVER(popover));

        // Get BrayaWindow instance
        BrayaWindow* window = (BrayaWindow*)g_object_get_data(G_OBJECT(parentWindow), "braya-window-instance");
        if (window && window->bookmarksManager) {
            // Show folder creation dialog
            std::string folderName = BrayaBookmarks::showNewFolderDialog(GTK_WINDOW(parentWindow));
            if (!folderName.empty()) {
                window->bookmarksManager->addFolder(folderName);
                window->refreshBookmarksBar();
                std::cout << "✓ Created folder: " << folderName << std::endl;
            }
        }
    }), popover);
    gtk_box_append(GTK_BOX(menuBox), newFolderBtn);

    // Manage Bookmarks button
    GtkWidget* manageBtn = gtk_button_new_with_label("📚 Manage Bookmarks");
    gtk_widget_add_css_class(manageBtn, "flat");
    g_signal_connect(manageBtn, "clicked", G_CALLBACK(+[](GtkWidget* widget, gpointer data) {
        GtkWidget* popover = GTK_WIDGET(data);
        GtkWidget* parentWindow = GTK_WIDGET(g_object_get_data(G_OBJECT(popover), "window"));
        gtk_popover_popdown(GTK_POPOVER(popover));

        // Get BrayaWindow instance and show bookmarks manager
        BrayaWindow* window = (BrayaWindow*)g_object_get_data(G_OBJECT(parentWindow), "braya-window-instance");
        if (window) {
            window->showBookmarksManager();
        }
    }), popover);
    gtk_box_append(GTK_BOX(menuBox), manageBtn);

    gtk_popover_popup(GTK_POPOVER(popover));
}

// Helper method to refresh bookmarks bar
void BrayaWindow::refreshBookmarksBar() {
    if (!bookmarksBar || !bookmarksManager) {
        std::cerr << "Cannot refresh bookmarks bar: bar or manager is NULL" << std::endl;
        return;
    }

    // Get the box widget (might be wrapped in viewport)
    GtkWidget* child = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(bookmarksBar));
    if (!child) {
        std::cerr << "Cannot get bookmarks bar child" << std::endl;
        return;
    }

    // Handle viewport wrapping
    GtkWidget* boxWidget = child;
    if (GTK_IS_VIEWPORT(child)) {
        boxWidget = gtk_viewport_get_child(GTK_VIEWPORT(child));
    }

    if (boxWidget && GTK_IS_BOX(boxWidget)) {
        bookmarksManager->updateBookmarksBar(
            boxWidget,
            this,
            G_CALLBACK(onBookmarkBarItemClicked),
            G_CALLBACK(onBookmarkBarAddClicked),
            G_CALLBACK(onBookmarkBarItemRightClick)
        );
        std::cout << "✓ Bookmarks bar refreshed" << std::endl;
    } else {
        std::cerr << "ERROR: Could not find GtkBox widget in bookmarks bar!" << std::endl;
    }
}

// Extension buttons
void BrayaWindow::createExtensionButtons() {
    updateExtensionButtons();
}

void BrayaWindow::updateExtensionButtons() {
    if (!extensionButtonsBox || !extensionManager) {
        return;
    }

    std::cout << "🔌 Updating extension buttons..." << std::endl;

    // Clear existing buttons
    GtkWidget* child = gtk_widget_get_first_child(extensionButtonsBox);
    while (child) {
        GtkWidget* next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(extensionButtonsBox), child);
        child = next;
    }

    // Get loaded extensions
    auto extensions = extensionManager->getWebExtensions();

    for (auto* extension : extensions) {
        if (!extension || !extension->isEnabled()) {
            continue;
        }

        BrowserAction browserAction = extension->getBrowserAction();

        // Skip if no browser_action defined
        if (browserAction.default_title.empty() && browserAction.default_icon.empty()) {
            continue;
        }

        std::cout << "  → Creating button for: " << extension->getName() << std::endl;

        GtkWidget* btn = gtk_button_new();

        // Load actual extension icon
        if (!browserAction.default_icon.empty()) {
            std::string iconPath = extension->getPath() + "/" + browserAction.default_icon;
            std::cout << "    Loading icon from: " << iconPath << std::endl;

            GError* error = nullptr;
            GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_scale(iconPath.c_str(), 16, 16, TRUE, &error);

            if (pixbuf) {
                GtkWidget* iconImage = gtk_image_new_from_pixbuf(pixbuf);
                gtk_button_set_child(GTK_BUTTON(btn), iconImage);
                g_object_unref(pixbuf);
                std::cout << "    ✓ Icon loaded successfully" << std::endl;
            } else {
                if (error) {
                    std::cerr << "    ✗ Failed to load icon: " << error->message << std::endl;
                    g_error_free(error);
                }
                // Fallback to emoji if icon loading fails
                gtk_button_set_label(GTK_BUTTON(btn), "🧩");
            }
        } else {
            // No icon specified, use emoji
            gtk_button_set_label(GTK_BUTTON(btn), "🧩");
        }

        // Set tooltip
        std::string tooltip = browserAction.default_title.empty() ?
                             extension->getName() : browserAction.default_title;
        gtk_widget_set_tooltip_text(btn, tooltip.c_str());
        gtk_widget_add_css_class(btn, "extension-btn");
        gtk_widget_add_css_class(btn, "action-btn");

        // Store extension ID in button
        g_object_set_data_full(G_OBJECT(btn), "extension-id",
                               g_strdup(extension->getId().c_str()), g_free);
        g_object_set_data(G_OBJECT(btn), "window", this);

        // Connect click handler
        g_signal_connect(btn, "clicked", G_CALLBACK(on_extension_button_clicked), nullptr);

        gtk_box_append(GTK_BOX(extensionButtonsBox), btn);
    }

    std::cout << "✓ Extension buttons updated (" << extensions.size() << " extensions)" << std::endl;
}

// 📊 Memory indicator implementation
long BrayaWindow::getMemoryUsage() {
    FILE* file = fopen("/proc/self/status", "r");
    if (!file) {
        return -1;
    }

    char line[128];
    long rss = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            // Extract the number from "VmRSS:  123456 kB"
            sscanf(line + 6, "%ld", &rss);
            break;
        }
    }

    fclose(file);
    return rss;  // Returns RSS in KB
}

void BrayaWindow::updateMemoryIndicator() {
    // Comprehensive safety checks
    if (!memoryLabel) return;
    if (!GTK_IS_WIDGET(memoryLabel)) return;
    if (!GTK_IS_LABEL(memoryLabel)) return;

    // Safety check for settings object
    if (!settings) {
        gtk_widget_set_visible(memoryLabel, FALSE);
        return;
    }

    // Check if memory indicator is enabled in settings
    bool enabled = false;
    try {
        enabled = settings->getMemoryIndicatorEnabled();
    } catch (...) {
        gtk_widget_set_visible(memoryLabel, FALSE);
        return;
    }

    if (!enabled) {
        gtk_widget_set_visible(memoryLabel, FALSE);
        return;
    }

    gtk_widget_set_visible(memoryLabel, TRUE);

    long memKB = getMemoryUsage();
    if (memKB < 0) {
        gtk_label_set_text(GTK_LABEL(memoryLabel), "Memory: N/A");
        return;
    }

    // Format the memory display
    char memText[64];
    if (memKB < 1024) {
        snprintf(memText, sizeof(memText), "📊 %ld KB", memKB);
    } else if (memKB < 1024 * 1024) {
        double memMB = memKB / 1024.0;
        snprintf(memText, sizeof(memText), "📊 %.1f MB", memMB);
    } else {
        double memGB = memKB / (1024.0 * 1024.0);
        snprintf(memText, sizeof(memText), "📊 %.2f GB", memGB);
    }

    gtk_label_set_text(GTK_LABEL(memoryLabel), memText);
}
