#include "BrayaTab.h"
#include "BrayaPasswordManager.h"
#include "extensions/BrayaExtensionManager.h"
#include "extensions/BrayaWebExtension.h"
#include "extensions/BrayaExtensionAPI.h"
#include <iostream>
#include <cctype>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <algorithm>
#include <sys/stat.h>

BrayaTab::BrayaTab(int id, const char* url, BrayaPasswordManager* passwordMgr, BrayaExtensionManager* extMgr)
    : id(id), url(url ? url : "about:braya"), title("New Tab"), isLoading(false),
      favicon(nullptr), tabButton(nullptr), passwordManager(passwordMgr), extensionManager(extMgr),
      userContentManager(nullptr), pinned(false), muted(false), readerMode(false),
      suspended(false), cachedFavicon(nullptr), deferredSetupSource(0) {

    // Create per-tab UserContentManager to avoid handler name conflicts
    // Each tab needs its own manager to register message handlers independently
    userContentManager = webkit_user_content_manager_new();
    webView = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW,
                                            "user-content-manager", userContentManager,
                                            nullptr));
    
    // Enable developer tools
    WebKitSettings* settings = webkit_web_view_get_settings(webView);
    webkit_settings_set_enable_developer_extras(settings, TRUE);
    webkit_settings_set_enable_smooth_scrolling(settings, TRUE);
    
    // Create scrolled window
    scrolledWindow = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_WIDGET(webView));
    
    // Defer password manager and extension setup for faster tab creation
    if (passwordManager || extensionManager) {
        struct DeferredSetupData {
            BrayaTab* tab;
            BrayaPasswordManager* passwordMgr;
            BrayaExtensionManager* extMgr;
            guint* sourceId;
        };
        DeferredSetupData* data = new DeferredSetupData{
            this, passwordManager, extensionManager, &deferredSetupSource
        };

        deferredSetupSource = g_idle_add([](gpointer user_data) -> gboolean {
            auto* data = static_cast<DeferredSetupData*>(user_data);
            
            if (data->sourceId) {
                *data->sourceId = 0;
            }
            
            if (data->passwordMgr && data->tab) {
                data->tab->setupPasswordManager();
            }
            
            if (data->extMgr && data->tab) {
                data->tab->setupExtensionDetector();
            }
            
            delete data;
            return G_SOURCE_REMOVE;
        }, data);
    }

    // Connect signals
    g_signal_connect(webView, "load-changed", G_CALLBACK(onLoadChanged), this);
    g_signal_connect(webView, "notify::title", G_CALLBACK(onTitleChanged), this);
    g_signal_connect(webView, "notify::uri", G_CALLBACK(onUriChanged), this);
    g_signal_connect(webView, "notify::favicon", G_CALLBACK(onFaviconChanged), this);
    g_signal_connect(webView, "web-process-terminated", G_CALLBACK(onWebProcessCrashed), this);
    g_signal_connect(webView, "create", G_CALLBACK(onCreateNewWindow), this);
    g_signal_connect(webView, "decide-policy", G_CALLBACK(onDecidePolicy), this);
    
    // Load URL - handle about:braya
    if (url && strlen(url) > 0) {
        std::string finalUrl = url;

        // Handle about:braya - load home page
        if (finalUrl == "about:braya") {
            finalUrl = "file://" + getResourcePath("home.html");
        }

        webkit_web_view_load_uri(webView, finalUrl.c_str());
    }
}

BrayaTab::~BrayaTab() {
    // Cancel deferred setup if still pending
    if (deferredSetupSource != 0) {
        g_source_remove(deferredSetupSource);
        deferredSetupSource = 0;
    }
    
    // Disconnect all signals before destruction
    if (webView && WEBKIT_IS_WEB_VIEW(webView)) {
        g_signal_handlers_disconnect_by_data(webView, this);
    }

    if (autofillPopover) {
        gtk_widget_unparent(autofillPopover);
        autofillPopover = nullptr;
    }
    if (autofillToast) {
        gtk_widget_unparent(autofillToast);
        autofillToast = nullptr;
    }
    if (toastTimerSource != 0) {
        g_source_remove(toastTimerSource);
        toastTimerSource = 0;
    }
    
    if (favicon) {
        g_object_unref(favicon);
        favicon = nullptr;
    }
    
    tabButton = nullptr;
    // GTK will handle widget cleanup
}

void BrayaTab::onLoadChanged(WebKitWebView* webView, WebKitLoadEvent loadEvent, gpointer userData) {
    if (!userData || !webView) return;
    BrayaTab* tab = static_cast<BrayaTab*>(userData);
    if (!tab) return;
    
    try {
        if (loadEvent == WEBKIT_LOAD_STARTED) {
            tab->isLoading = true;
        } else if (loadEvent == WEBKIT_LOAD_FINISHED) {
            tab->isLoading = false;

            // Force favicon check after page loads
            // First check cache
            if (tab->faviconGetCallback && !tab->url.empty()) {
                GdkTexture* cachedFavicon = tab->faviconGetCallback(tab->url);
                if (cachedFavicon && GDK_IS_TEXTURE(cachedFavicon)) {
                    std::cout << "📦 Using cached favicon on load finish" << std::endl;
                    if (tab->favicon) {
                        g_object_unref(tab->favicon);
                        tab->favicon = nullptr;
                    }
                    tab->favicon = GDK_TEXTURE(g_object_ref(cachedFavicon));
                }
            }

            // If no cache hit, try to get from WebKit
            if (!tab->favicon) {
                GdkTexture* favicon = webkit_web_view_get_favicon(webView);
                if (favicon && GDK_IS_TEXTURE(favicon)) {
                    std::cout << "🎯 Forcing favicon update on load finish" << std::endl;
                    if (tab->favicon) {
                        g_object_unref(tab->favicon);
                        tab->favicon = nullptr;
                    }
                    tab->favicon = GDK_TEXTURE(g_object_ref(favicon));

                    // Cache it
                    if (tab->faviconCacheCallback && !tab->url.empty()) {
                        tab->faviconCacheCallback(tab->url, favicon);
                    }
                }
            }

            // Inject extension content scripts
            if (tab->extensionManager) {
                const char* currentUri = webkit_web_view_get_uri(webView);
                if (currentUri) {
                    tab->injectExtensionContentScripts(currentUri);
                }
            }

            // Inject extension detector script for automatic installation
            tab->injectExtensionDetectorScript();

            // Don't auto-fill on page load (Safari-style - wait for user interaction)
            // Passwords will be offered when user clicks on the field

            tab->updateButton();
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in onLoadChanged: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Error in onLoadChanged" << std::endl;
    }
}

void BrayaTab::onTitleChanged(WebKitWebView* webView, GParamSpec* pspec, gpointer userData) {
    if (!userData || !webView) return;
    BrayaTab* tab = static_cast<BrayaTab*>(userData);
    if (!tab) return;
    
    try {
        const gchar* title = webkit_web_view_get_title(webView);
        if (title && strlen(title) > 0) {
            tab->title = title;
            tab->updateButton();
        }
    } catch (...) {
        std::cerr << "Error in onTitleChanged" << std::endl;
    }
}

void BrayaTab::onUriChanged(WebKitWebView* webView, GParamSpec* pspec, gpointer userData) {
    if (!userData || !webView) return;
    BrayaTab* tab = static_cast<BrayaTab*>(userData);
    if (!tab) return;
    
    try {
        const gchar* uri = webkit_web_view_get_uri(webView);
        if (uri && strlen(uri) > 0) {
            tab->url = uri;
        }
    } catch (...) {
        std::cerr << "Error in onUriChanged" << std::endl;
    }
}

void BrayaTab::onFaviconChanged(WebKitWebView* webView, GParamSpec* pspec, gpointer userData) {
    if (!userData || !webView || !WEBKIT_IS_WEB_VIEW(webView)) return;

    BrayaTab* tab = static_cast<BrayaTab*>(userData);
    if (!tab) return;

    try {
        // First check if we have a cached favicon for this URL
        if (tab->faviconGetCallback && !tab->url.empty()) {
            GdkTexture* cachedFavicon = tab->faviconGetCallback(tab->url);
            if (cachedFavicon && GDK_IS_TEXTURE(cachedFavicon)) {
                std::cout << "📦 Using cached favicon for tab " << tab->id << std::endl;

                if (tab->favicon) {
                    g_object_unref(tab->favicon);
                    tab->favicon = nullptr;
                }
                tab->favicon = GDK_TEXTURE(g_object_ref(cachedFavicon));
                tab->updateButton();
                return;
            }
        }

        // Get favicon from WebKit
        GdkTexture* newFavicon = webkit_web_view_get_favicon(webView);
        std::cout << "🎨 Favicon signal for tab " << tab->id << ": " << (newFavicon ? "FOUND" : "NULL") << std::endl;

        if (newFavicon && GDK_IS_TEXTURE(newFavicon)) {
            int width = gdk_texture_get_width(newFavicon);
            int height = gdk_texture_get_height(newFavicon);
            std::cout << "✅ Valid favicon texture: " << width << "x" << height << std::endl;

            if (tab->favicon) {
                g_object_unref(tab->favicon);
                tab->favicon = nullptr;
            }
            tab->favicon = GDK_TEXTURE(g_object_ref(newFavicon));

            // Cache the favicon for future use
            if (tab->faviconCacheCallback && !tab->url.empty()) {
                tab->faviconCacheCallback(tab->url, newFavicon);
            }

            tab->updateButton();
        } else {
            std::cout << "❌ Favicon not ready, will retry on load finish" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in favicon update: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error updating favicon" << std::endl;
    }
}

void BrayaTab::updateButton() {
    if (!tabButton || !GTK_IS_BUTTON(tabButton)) {
        return;
    }

    try {
        // Get the icon box from the tab button
        GtkWidget* iconBox = GTK_WIDGET(g_object_get_data(G_OBJECT(tabButton), "favicon-box"));
        if (!iconBox || !GTK_IS_BOX(iconBox)) {
            std::cerr << "No icon box found!" << std::endl;
            return;
        }

        // Clear icon box
        GtkWidget* child = gtk_widget_get_first_child(iconBox);
        while (child) {
            GtkWidget* next = gtk_widget_get_next_sibling(child);
            gtk_box_remove(GTK_BOX(iconBox), child);
            child = next;
        }

        // Create an overlay box for stacking indicators
        GtkWidget* overlayBox = gtk_overlay_new();
        gtk_widget_set_size_request(overlayBox, 32, 32);

        // Main icon container (bottom layer)
        GtkWidget* mainIconBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_widget_set_halign(mainIconBox, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(mainIconBox, GTK_ALIGN_CENTER);

        // If we have a favicon, show it as an image
        if (favicon && GDK_IS_TEXTURE(favicon)) {
            GtkWidget* image = gtk_image_new_from_paintable(GDK_PAINTABLE(favicon));
            if (image && GTK_IS_IMAGE(image)) {
                gtk_image_set_pixel_size(GTK_IMAGE(image), 20);
                gtk_widget_set_size_request(image, 20, 20);
                gtk_box_append(GTK_BOX(mainIconBox), image);
            }
        } else {
            // Fallback: Show first letter of title
            std::string icon;
            if (title.empty() || title == "New Tab") {
                icon = "•";
            } else {
                char firstChar = title[0];
                if (isalpha(firstChar)) {
                    icon = std::string(1, static_cast<char>(toupper(firstChar)));
                } else {
                    icon = "•";
                }
            }

            GtkWidget* label = gtk_label_new(icon.c_str());
            if (label && GTK_IS_LABEL(label)) {
                gtk_widget_add_css_class(label, "tab-icon-label");
                gtk_box_append(GTK_BOX(mainIconBox), label);
            }
        }

        gtk_overlay_set_child(GTK_OVERLAY(overlayBox), mainIconBox);

        // Add pinned indicator (top-left overlay)
        if (pinned) {
            GtkWidget* pinLabel = gtk_label_new("📌");
            gtk_widget_set_halign(pinLabel, GTK_ALIGN_START);
            gtk_widget_set_valign(pinLabel, GTK_ALIGN_START);
            gtk_widget_add_css_class(pinLabel, "tab-pin-indicator");
            gtk_label_set_xalign(GTK_LABEL(pinLabel), 0);
            gtk_label_set_yalign(GTK_LABEL(pinLabel), 0);
            gtk_overlay_add_overlay(GTK_OVERLAY(overlayBox), pinLabel);
        }

        // Add audio/mute indicator (bottom-right overlay)
        // Check if tab is playing audio
        bool isPlayingAudio = webkit_web_view_is_playing_audio(webView);
        if (isPlayingAudio || muted) {
            std::string audioIcon = muted ? "🔇" : "🔊";
            GtkWidget* audioLabel = gtk_label_new(audioIcon.c_str());
            gtk_widget_set_halign(audioLabel, GTK_ALIGN_END);
            gtk_widget_set_valign(audioLabel, GTK_ALIGN_END);
            gtk_widget_add_css_class(audioLabel, "tab-audio-indicator");
            gtk_label_set_xalign(GTK_LABEL(audioLabel), 1);
            gtk_label_set_yalign(GTK_LABEL(audioLabel), 1);
            gtk_overlay_add_overlay(GTK_OVERLAY(overlayBox), audioLabel);

            // Make the audio indicator clickable to mute/unmute
            GtkGesture* clickGesture = gtk_gesture_click_new();
            g_object_set_data(G_OBJECT(clickGesture), "tab", this);
            g_signal_connect(clickGesture, "pressed", G_CALLBACK(+[](GtkGestureClick* gesture, int n_press, double x, double y, gpointer data) {
                BrayaTab* tab = static_cast<BrayaTab*>(g_object_get_data(G_OBJECT(gesture), "tab"));
                if (tab) {
                    tab->setMuted(!tab->isMuted());
                }
                // Stop event propagation so it doesn't switch tabs
                gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
            }), nullptr);
            gtk_widget_add_controller(audioLabel, GTK_EVENT_CONTROLLER(clickGesture));
        }

        gtk_box_append(GTK_BOX(iconBox), overlayBox);

        // Set tooltip with title, URL, and status
        std::string tooltip = title.empty() ? "New Tab" : title;
        if (!url.empty() && url != title) {
            tooltip += "\n" + url;
        }
        if (pinned) {
            tooltip += "\n📌 Pinned";
        }
        if (muted) {
            tooltip += "\n🔇 Muted";
        } else if (webkit_web_view_is_playing_audio(webView)) {
            tooltip += "\n🔊 Playing audio (click speaker to mute)";
        }
        gtk_widget_set_tooltip_text(tabButton, tooltip.c_str());

        // Adjust button size for pinned tabs (make them smaller)
        if (pinned) {
            gtk_widget_set_size_request(tabButton, 36, 48);
            gtk_widget_add_css_class(tabButton, "tab-pinned");
        } else {
            gtk_widget_set_size_request(tabButton, 48, 48);
            gtk_widget_remove_css_class(tabButton, "tab-pinned");
        }

    } catch (const std::exception& e) {
        std::cerr << "Exception in updateButton: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error in updateButton" << std::endl;
    }
}

void BrayaTab::onWebProcessCrashed(WebKitWebView* webView, gpointer userData) {
    std::cerr << "⚠️ WebKit web process crashed!" << std::endl;

    if (!userData) return;
    BrayaTab* tab = static_cast<BrayaTab*>(userData);

    // Reload the page to restart the web process
    if (webView && WEBKIT_IS_WEB_VIEW(webView)) {
        webkit_web_view_reload(webView);
    }
}

std::string BrayaTab::getResourcePath(const std::string& filename) {
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

// Handle new window/popup requests
GtkWidget* BrayaTab::onCreateNewWindow(WebKitWebView* webView, WebKitNavigationAction* navigation, gpointer userData) {
    if (!userData) {
        std::cerr << "❌ onCreateNewWindow: NULL userData" << std::endl;
        return nullptr;
    }
    
    BrayaTab* tab = static_cast<BrayaTab*>(userData);

    std::cout << "🪟 New window/tab requested - " << std::flush;

    // Get the navigation request URI
    WebKitURIRequest* request = webkit_navigation_action_get_request(navigation);
    const gchar* uri = webkit_uri_request_get_uri(request);

    if (uri && tab->newTabCallback) {
        std::cout << "opening in new tab: " << uri << std::endl;
        // Open in new tab using callback
        tab->newTabCallback(uri);
    } else if (uri) {
        std::cout << "opening URL in same tab (no callback): " << uri << std::endl;
        // Fallback: navigate in current tab
        webkit_web_view_load_uri(webView, uri);
    } else {
        std::cout << "no URI provided, ignoring" << std::endl;
    }

    // Return nullptr to tell WebKit we handled it
    return nullptr;
}

// Handle navigation policy decisions (for opening links in new tabs)
gboolean BrayaTab::onDecidePolicy(WebKitWebView* webView, WebKitPolicyDecision* decision,
                                   WebKitPolicyDecisionType type, gpointer userData) {
    BrayaTab* tab = static_cast<BrayaTab*>(userData);

    // Only handle navigation actions
    if (type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION) {
        webkit_policy_decision_use(decision);
        return FALSE;
    }

    WebKitNavigationPolicyDecision* navDecision = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
    WebKitNavigationAction* navAction = webkit_navigation_policy_decision_get_navigation_action(navDecision);

    // Get navigation type
    WebKitNavigationType navType = webkit_navigation_action_get_navigation_type(navAction);

    // Only intercept link clicks (not form submissions, reloads, etc.)
    if (navType != WEBKIT_NAVIGATION_TYPE_LINK_CLICKED) {
        webkit_policy_decision_use(decision);
        return FALSE;
    }

    // Get the target URI
    WebKitURIRequest* request = webkit_navigation_action_get_request(navAction);
    const gchar* uri = webkit_uri_request_get_uri(request);

    if (!uri || !tab->newTabCallback) {
        // No URI or no callback - allow default behavior
        webkit_policy_decision_use(decision);
        return FALSE;
    }

    // Get modifier keys and mouse button
    guint modifiers = webkit_navigation_action_get_modifiers(navAction);
    guint mouseButton = webkit_navigation_action_get_mouse_button(navAction);

    bool ctrlPressed = (modifiers & GDK_CONTROL_MASK);
    bool shiftPressed = (modifiers & GDK_SHIFT_MASK);
    bool middleClick = (mouseButton == 2);  // Middle mouse button

    std::cout << "🔍 Link click detected - button: " << mouseButton
              << ", ctrl: " << ctrlPressed
              << ", shift: " << shiftPressed << std::endl;

    // Open in new tab if:
    // - Middle click
    // - Ctrl+Click
    // - Regular left click (mouseButton == 1 or 0, as button might be 0 for some clicks)
    if (middleClick || ctrlPressed || (mouseButton <= 1 && !shiftPressed)) {
        std::cout << "🔗 Opening link in new tab: " << uri << std::endl;

        // Ignore the navigation in current tab
        webkit_policy_decision_ignore(decision);

        // IMPORTANT: Defer tab creation to avoid creating WebView inside policy handler
        // Creating a new WebView synchronously from within a policy decision handler
        // can cause WebKit assertion failures (Signal 6 SIGABRT)
        struct NewTabData {
            BrayaTab* tab;
            std::string url;
        };
        NewTabData* data = new NewTabData{tab, std::string(uri)};
        g_idle_add([](gpointer user_data) -> gboolean {
            auto* data = static_cast<NewTabData*>(user_data);
            if (data->tab && data->tab->newTabCallback) {
                data->tab->newTabCallback(data->url);
            }
            delete data;
            return G_SOURCE_REMOVE;
        }, data);

        return TRUE;  // We handled it
    }

    // For Shift+Click or other cases, use default behavior (same tab)
    std::cout << "↪️  Opening link in same tab: " << uri << std::endl;
    webkit_policy_decision_use(decision);
    return FALSE;
}

void BrayaTab::setupPasswordManager() {
    if (!userContentManager || !passwordManager) return;

    // Inject password detection script
    injectPasswordScript();

    // Register message handler for password capture
    g_signal_connect(userContentManager, "script-message-received::passwordCapture",
                     G_CALLBACK(onPasswordCaptured), this);
    webkit_user_content_manager_register_script_message_handler(userContentManager, "passwordCapture", nullptr);

    // Register message handler for autofill requests (triggered by field focus)
    g_signal_connect(userContentManager, "script-message-received::autofillRequest",
                     G_CALLBACK(onAutofillRequest), this);
    webkit_user_content_manager_register_script_message_handler(userContentManager, "autofillRequest", nullptr);

    // Register message handler for checking if passwords are available
    g_signal_connect(userContentManager, "script-message-received::checkPasswords",
                     G_CALLBACK(onCheckPasswords), this);
    webkit_user_content_manager_register_script_message_handler(userContentManager, "checkPasswords", nullptr);

    std::cout << "✓ Password manager set up for tab " << id << std::endl;
}

void BrayaTab::injectPasswordScript() {
    if (!userContentManager) return;

    // Get path to password-detect.js using resource path helper
    std::string scriptPath = getResourcePath("password-detect.js");

    // Read script content
    std::ifstream scriptFile(scriptPath);
    if (!scriptFile.is_open()) {
        std::cerr << "✗ Failed to load password-detect.js from: " << scriptPath << std::endl;
        return;
    }

    std::stringstream buffer;
    buffer << scriptFile.rdbuf();
    std::string scriptContent = buffer.str();
    std::string configScript = "window.BrayaPasswordConfig = { multiStep: " +
        std::string((passwordManager && passwordManager->isMultiStepCaptureEnabled()) ? "true" : "false") +
        " };";
    scriptContent = configScript + "\n" + scriptContent;
    scriptFile.close();

    // Create and add user script
    WebKitUserScript* script = webkit_user_script_new(
        scriptContent.c_str(),
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_END,
        nullptr,
        nullptr
    );

    webkit_user_content_manager_add_script(userContentManager, script);
    webkit_user_script_unref(script);

    std::cout << "✓ Password detection script injected for tab " << id << std::endl;
}

void BrayaTab::autoFillPasswords() {
    if (!passwordManager || url.empty()) return;

    // Get saved passwords for current URL
    auto passwords = passwordManager->getPasswordsForUrl(url);

    if (passwords.empty()) {
        return; // No saved passwords for this site
    }

    // Use the first matching password (user can trigger dropdown for multiple accounts)
    const auto& entry = passwords[0];

    // Escape single quotes in username and password for JavaScript
    auto escapeJs = [](const std::string& str) {
        std::string escaped = str;
        size_t pos = 0;
        while ((pos = escaped.find('"', pos)) != std::string::npos) {
            escaped.replace(pos, 1, "\"");
            pos += 2;
        }
        return escaped;
    };

    // Build JavaScript to call fillPassword function
    std::string script = "if (typeof fillPassword === 'function') { fillPassword('" +
                        escapeJs(entry.username) + "', '" + escapeJs(entry.password) + "'); }";

    // Execute JavaScript
    webkit_web_view_evaluate_javascript(
        webView,
        script.c_str(),
        -1,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    );

    std::cout << "✓ Auto-filled password for " << entry.username << " on tab " << id << std::endl;
    showAutofillToast("Auto-filled " + (entry.username.empty() ? std::string("login") : entry.username));
}

void BrayaTab::onPasswordCaptured(WebKitUserContentManager* manager, JSCValue* value, gpointer userData) {
    if (!userData || !value) return;

    BrayaTab* tab = static_cast<BrayaTab*>(userData);
    if (!tab || !tab->passwordManager) return;

    // Parse the message object
    JSCValue* urlValue = jsc_value_object_get_property(value, "url");
    JSCValue* usernameValue = jsc_value_object_get_property(value, "username");
    JSCValue* passwordValue = jsc_value_object_get_property(value, "password");

    if (!urlValue || !usernameValue || !passwordValue) return;

    char* url = jsc_value_to_string(urlValue);
    char* username = jsc_value_to_string(usernameValue);
    char* password = jsc_value_to_string(passwordValue);

    if (url && username && password) {
        if (tab->passwordManager->isSavingSuppressedForUrl(url)) {
            g_free(url);
            g_free(username);
            g_free(password);
            return;
        }

        auto existing = tab->passwordManager->getPasswordsForUrl(url);
        bool hasMatch = std::any_of(existing.begin(), existing.end(), [&](const PasswordEntry& entry) {
            return entry.username == username;
        });
        bool hasExisting = !existing.empty();

        GtkWidget* dialog = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(dialog), hasMatch ? "Update saved password?" : "Save password?");
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

        GtkWidget* outerBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
        gtk_widget_add_css_class(outerBox, "dialog-body");
        gtk_window_set_child(GTK_WINDOW(dialog), outerBox);

        GtkWidget* messageBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_widget_set_margin_bottom(messageBox, 6);
        gtk_box_append(GTK_BOX(outerBox), messageBox);

        GtkWidget* message = gtk_label_new(nullptr);
        std::string messageText = hasMatch
            ? "Update saved password for <b>" + std::string(username) + "</b>?"
            : "Save password for <b>" + std::string(username) + "</b>?";
        gtk_label_set_markup(GTK_LABEL(message), messageText.c_str());
        gtk_box_append(GTK_BOX(messageBox), message);

        GtkWidget* urlLabel = gtk_label_new(url);
        gtk_widget_add_css_class(urlLabel, "dim-label");
        gtk_label_set_ellipsize(GTK_LABEL(urlLabel), PANGO_ELLIPSIZE_MIDDLE);
        gtk_box_append(GTK_BOX(messageBox), urlLabel);

        // Show last updated timestamp and password mismatch warning if updating
        if (hasMatch) {
            auto it = std::find_if(existing.begin(), existing.end(), [&](const PasswordEntry& e) {
                return e.username == username;
            });

            if (it != existing.end()) {
                // Show last updated time
                std::string timeText = "Last updated: ";
                if (it->updatedAt > 0) {
                    timeText += tab->passwordManager->formatTimestampRelative(it->updatedAt);
                } else {
                    timeText += "Never";
                }
                GtkWidget* timeLabel = gtk_label_new(timeText.c_str());
                gtk_widget_add_css_class(timeLabel, "dim-label");
                gtk_box_append(GTK_BOX(messageBox), timeLabel);

                // Warn if passwords differ
                if (it->password != password) {
                    GtkWidget* warningLabel = gtk_label_new("⚠ This password differs from your saved password");
                    gtk_widget_add_css_class(warningLabel, "warning");
                    gtk_box_append(GTK_BOX(messageBox), warningLabel);
                }
            }
        }

        GtkWidget* credsBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_box_append(GTK_BOX(outerBox), credsBox);

        GtkWidget* userRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_box_append(GTK_BOX(credsBox), userRow);
        gtk_box_append(GTK_BOX(userRow), gtk_label_new("Username"));
        GtkWidget* userEntry = gtk_entry_new();
        gtk_editable_set_text(GTK_EDITABLE(userEntry), username);
        gtk_widget_set_hexpand(userEntry, TRUE);
        gtk_box_append(GTK_BOX(userRow), userEntry);

        GtkWidget* passRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_box_append(GTK_BOX(credsBox), passRow);
        gtk_box_append(GTK_BOX(passRow), gtk_label_new("Password"));
        GtkWidget* passEntry = gtk_entry_new();
        gtk_entry_set_visibility(GTK_ENTRY(passEntry), FALSE);
        gtk_editable_set_text(GTK_EDITABLE(passEntry), password);
        gtk_widget_set_hexpand(passEntry, TRUE);
        gtk_box_append(GTK_BOX(passRow), passEntry);

        GtkWidget* actionBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_box_append(GTK_BOX(outerBox), actionBox);

        GtkWidget* addRadio = gtk_check_button_new_with_label(hasExisting ? "Add as new login" : "Save password");
        gtk_check_button_set_group(GTK_CHECK_BUTTON(addRadio), GTK_CHECK_BUTTON(addRadio));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(addRadio), TRUE);
        gtk_box_append(GTK_BOX(actionBox), addRadio);

        GtkWidget* updateRadio = nullptr;
        GtkWidget* updateDropdown = nullptr;
        std::vector<std::string>* existingUsernames = nullptr;

        if (hasExisting) {
            updateRadio = gtk_check_button_new_with_label("Update an existing login");
            gtk_check_button_set_group(GTK_CHECK_BUTTON(updateRadio), GTK_CHECK_BUTTON(addRadio));
            gtk_box_append(GTK_BOX(actionBox), updateRadio);

            existingUsernames = new std::vector<std::string>();
            existingUsernames->reserve(existing.size());

            GPtrArray* labels = g_ptr_array_new_with_free_func(g_free);
            for (const auto& entry : existing) {
                std::string label = entry.username.empty() ? "(no username)" : entry.username;
                existingUsernames->push_back(entry.username);
                g_ptr_array_add(labels, g_strdup(label.c_str()));
            }
            g_ptr_array_add(labels, nullptr);

            updateDropdown = gtk_drop_down_new_from_strings(reinterpret_cast<const char* const*>(labels->pdata));
            g_ptr_array_free(labels, TRUE);
            gtk_widget_set_sensitive(updateDropdown, FALSE);
            gtk_box_append(GTK_BOX(actionBox), updateDropdown);

            g_signal_connect(updateRadio, "toggled", G_CALLBACK(+[](GtkToggleButton* btn, gpointer data) {
                GtkWidget* dropdown = GTK_WIDGET(data);
                gtk_widget_set_sensitive(dropdown, gtk_toggle_button_get_active(btn));
            }), updateDropdown);
        }

        GtkWidget* buttonRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_halign(buttonRow, GTK_ALIGN_END);
        gtk_box_append(GTK_BOX(outerBox), buttonRow);

        GtkWidget* neverBtn = gtk_button_new_with_label("Never for this site");
        gtk_widget_add_css_class(neverBtn, "destructive-action");
        g_object_set_data(G_OBJECT(neverBtn), "manager", tab->passwordManager);
        g_object_set_data_full(G_OBJECT(neverBtn), "url", g_strdup(url), g_free);
        g_object_set_data(G_OBJECT(neverBtn), "dialog", dialog);
        g_signal_connect(neverBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer) {
            auto* mgr = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
            const char* siteUrl = static_cast<const char*>(g_object_get_data(G_OBJECT(btn), "url"));
            GtkWidget* dlg = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "dialog"));
            if (mgr && siteUrl) {
                mgr->blockSavingForUrl(siteUrl);
            }
            gtk_window_close(GTK_WINDOW(dlg));
        }), nullptr);
        gtk_box_append(GTK_BOX(buttonRow), neverBtn);

        GtkWidget* dismissBtn = gtk_button_new_with_label("Not now");
        gtk_box_append(GTK_BOX(buttonRow), dismissBtn);

        GtkWidget* confirmBtn = gtk_button_new_with_label(hasExisting ? "Save / Update" : "Save");
        gtk_widget_add_css_class(confirmBtn, "suggested-action");
        gtk_box_append(GTK_BOX(buttonRow), confirmBtn);

        g_signal_connect_swapped(dismissBtn, "clicked", G_CALLBACK(gtk_window_close), dialog);

        g_object_set_data_full(G_OBJECT(confirmBtn), "url", g_strdup(url), g_free);
        g_object_set_data_full(G_OBJECT(confirmBtn), "username", g_strdup(username), g_free);
        g_object_set_data_full(G_OBJECT(confirmBtn), "password", g_strdup(password), g_free);
        g_object_set_data(G_OBJECT(confirmBtn), "manager", tab->passwordManager);
        g_object_set_data(G_OBJECT(confirmBtn), "dialog", dialog);
        g_object_set_data(G_OBJECT(confirmBtn), "add_radio", addRadio);
        g_object_set_data(G_OBJECT(confirmBtn), "update_radio", updateRadio);
        g_object_set_data(G_OBJECT(confirmBtn), "dropdown", updateDropdown);
        g_object_set_data(G_OBJECT(confirmBtn), "username_entry", userEntry);
        g_object_set_data(G_OBJECT(confirmBtn), "password_entry", passEntry);
        if (existingUsernames) {
            g_object_set_data_full(G_OBJECT(confirmBtn), "existing_usernames", existingUsernames,
                +[](gpointer data) {
                    delete static_cast<std::vector<std::string>*>(data);
                });
        }

        g_signal_connect(confirmBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
            auto* mgr = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
            if (!mgr) {
                return;
            }
            GtkWidget* dlg = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "dialog"));
            GtkEntry* userEntry = GTK_ENTRY(g_object_get_data(G_OBJECT(btn), "username_entry"));
            GtkEntry* passEntry = GTK_ENTRY(g_object_get_data(G_OBJECT(btn), "password_entry"));
            const char* updatedUsername = userEntry ? gtk_editable_get_text(GTK_EDITABLE(userEntry)) : nullptr;
            const char* updatedPassword = passEntry ? gtk_editable_get_text(GTK_EDITABLE(passEntry)) : nullptr;
            const char* url = static_cast<const char*>(g_object_get_data(G_OBJECT(btn), "url"));
            if (!url) {
                return;
            }

            if (!mgr->requestUnlock(GTK_WINDOW(dlg))) {
                return;
            }

            const char* defaultUsername = static_cast<const char*>(g_object_get_data(G_OBJECT(btn), "username"));
            const char* defaultPassword = static_cast<const char*>(g_object_get_data(G_OBJECT(btn), "password"));
            const char* finalUsername = (updatedUsername && strlen(updatedUsername) > 0) ? updatedUsername : defaultUsername;
            const char* finalPassword = (updatedPassword && strlen(updatedPassword) > 0) ? updatedPassword : defaultPassword;
            if (!finalUsername || !finalPassword) {
                gtk_window_close(GTK_WINDOW(dlg));
                return;
            }

            GtkToggleButton* updateToggle = GTK_TOGGLE_BUTTON(g_object_get_data(G_OBJECT(btn), "update_radio"));
            GtkDropDown* dropdown = GTK_DROP_DOWN(g_object_get_data(G_OBJECT(btn), "dropdown"));
            auto* usernames = static_cast<std::vector<std::string>*>(g_object_get_data(G_OBJECT(btn), "existing_usernames"));

            if (updateToggle && gtk_toggle_button_get_active(updateToggle) &&
                dropdown && usernames && !usernames->empty()) {
                guint selected = gtk_drop_down_get_selected(dropdown);
                if (selected >= usernames->size()) {
                    selected = 0;
                }
                const std::string& original = (*usernames)[selected];
                mgr->updatePassword(url, original, finalUsername, finalPassword);
            } else {
                mgr->savePassword(url, finalUsername, finalPassword);
            }

            gtk_window_close(GTK_WINDOW(dlg));
        }), nullptr);

        gtk_window_present(GTK_WINDOW(dialog));
    }

    g_free(url);
    g_free(username);
    g_free(password);
}

void BrayaTab::showAutofillSuggestions(const GdkRectangle* anchorRect) {
    if (!passwordManager || url.empty()) return;

    auto passwords = passwordManager->getPasswordsForUrl(url);
    if (passwords.empty()) {
        return;
    }

    if (autofillPopover) {
        gtk_widget_unparent(autofillPopover);
        autofillPopover = nullptr;
    }
    if (autofillToast) {
        gtk_widget_unparent(autofillToast);
        autofillToast = nullptr;
    }

    GtkWidget* popover = gtk_popover_new();
    gtk_popover_set_has_arrow(GTK_POPOVER(popover), TRUE);
    gtk_popover_set_autohide(GTK_POPOVER(popover), TRUE);
    gtk_widget_set_parent(popover, GTK_WIDGET(webView));
    if (anchorRect) {
        gtk_popover_set_pointing_to(GTK_POPOVER(popover), anchorRect);
    }

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_start(box, 10);
    gtk_widget_set_margin_end(box, 10);
    gtk_widget_set_margin_top(box, 8);
    gtk_widget_set_margin_bottom(box, 8);
    gtk_popover_set_child(GTK_POPOVER(popover), box);

    GtkWidget* label = gtk_label_new("Choose an account to autofill");
    gtk_widget_add_css_class(label, "dim-label");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), label);

    // Store fill buttons for keyboard navigation
    std::vector<GtkWidget*>* fillButtons = new std::vector<GtkWidget*>();
    int* selectedIndex = new int(0);

    for (const auto& entry : passwords) {
        GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_box_append(GTK_BOX(box), row);

        GtkWidget* userLabel = gtk_label_new(entry.username.c_str());
        gtk_widget_set_hexpand(userLabel, TRUE);
        gtk_widget_set_halign(userLabel, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(row), userLabel);

        GtkWidget* fillBtn = gtk_button_new_with_label("Fill");
        gtk_widget_add_css_class(fillBtn, "suggested-action");
        g_object_set_data_full(G_OBJECT(fillBtn), "username", g_strdup(entry.username.c_str()), g_free);
        g_object_set_data_full(G_OBJECT(fillBtn), "password", g_strdup(entry.password.c_str()), g_free);
        g_object_set_data(G_OBJECT(fillBtn), "tab", this);
        g_object_set_data(G_OBJECT(fillBtn), "popover", popover);

        g_signal_connect(fillBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
            auto* tab = static_cast<BrayaTab*>(g_object_get_data(G_OBJECT(btn), "tab"));
            GtkWidget* pop = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "popover"));
            const char* username = static_cast<const char*>(g_object_get_data(G_OBJECT(btn), "username"));
            const char* password = static_cast<const char*>(g_object_get_data(G_OBJECT(btn), "password"));

            if (tab && username && password) {
                auto escapeJs = [](const std::string& str) {
                    std::string escaped = str;
                    size_t pos = 0;
                    while ((pos = escaped.find('"', pos)) != std::string::npos) {
                        escaped.replace(pos, 1, "\"");
                        pos += 2;
                    }
                    return escaped;
                };

                std::string script = "if (typeof fillPassword === 'function') { fillPassword('" +
                                    escapeJs(username) + "', '" + escapeJs(password) + "'); }";

                webkit_web_view_evaluate_javascript(
                    tab->webView,
                    script.c_str(),
                    -1,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr
                );

                std::cout << "✓ Filled password for " << username << std::endl;
            }

            if (pop && GTK_IS_POPOVER(pop)) {
                gtk_popover_popdown(GTK_POPOVER(pop));
            }
            if (tab) {
                tab->showAutofillToast(std::string("Filled ") + (username ? username : "login"),
                                      tab->url, username ? username : "");
            }
        }), nullptr);

        gtk_box_append(GTK_BOX(row), fillBtn);

        // Add to buttons vector for keyboard navigation
        fillButtons->push_back(fillBtn);
    }

    // Give focus to the first button
    if (!fillButtons->empty()) {
        gtk_widget_grab_focus((*fillButtons)[0]);
    }

    // Store buttons vector and selected index in popover data for cleanup
    g_object_set_data_full(G_OBJECT(popover), "fill-buttons", fillButtons,
        +[](gpointer data) { delete static_cast<std::vector<GtkWidget*>*>(data); });
    g_object_set_data_full(G_OBJECT(popover), "selected-index", selectedIndex,
        +[](gpointer data) { delete static_cast<int*>(data); });

    // Add keyboard navigation
    GtkEventController* keyController = gtk_event_controller_key_new();
    g_signal_connect(keyController, "key-pressed", G_CALLBACK(+[](
        GtkEventControllerKey* controller, guint keyval, guint keycode, GdkModifierType state, gpointer data) -> gboolean {

        GtkWidget* popover = GTK_WIDGET(data);
        auto* buttons = static_cast<std::vector<GtkWidget*>*>(g_object_get_data(G_OBJECT(popover), "fill-buttons"));
        int* selectedIdx = static_cast<int*>(g_object_get_data(G_OBJECT(popover), "selected-index"));

        if (!buttons || buttons->empty() || !selectedIdx) return FALSE;

        if (keyval == GDK_KEY_Down || keyval == GDK_KEY_Up) {
            // Move selection
            if (keyval == GDK_KEY_Down) {
                *selectedIdx = (*selectedIdx + 1) % buttons->size();
            } else {
                *selectedIdx = (*selectedIdx - 1 + buttons->size()) % buttons->size();
            }

            // Give focus to new selection
            gtk_widget_grab_focus((*buttons)[*selectedIdx]);
            return TRUE;
        }
        else if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
            // Activate selected button by emitting clicked signal
            g_signal_emit_by_name((*buttons)[*selectedIdx], "clicked");
            return TRUE;
        }
        else if (keyval == GDK_KEY_Escape) {
            // Close popover
            gtk_popover_popdown(GTK_POPOVER(popover));
            return TRUE;
        }

        return FALSE;
    }), popover);

    gtk_widget_add_controller(popover, keyController);

    g_signal_connect(popover, "closed", G_CALLBACK(+[](GtkPopover* pop, gpointer data) {
        auto* tab = static_cast<BrayaTab*>(data);
        if (tab && tab->autofillPopover == GTK_WIDGET(pop)) {
            tab->autofillPopover = nullptr;
        }
    }), this);

    autofillPopover = popover;
    gtk_popover_popup(GTK_POPOVER(popover));
}

void BrayaTab::showAutofillToast(const std::string& message, const std::string& url, const std::string& username) {
    if (toastTimerSource != 0) {
        g_source_remove(toastTimerSource);
        toastTimerSource = 0;
    }
    if (autofillToast) {
        gtk_widget_unparent(autofillToast);
        autofillToast = nullptr;
    }

    GtkWidget* parent = tabButton ? tabButton : GTK_WIDGET(webView);
    if (!parent) {
        return;
    }

    GtkWidget* popover = gtk_popover_new();
    gtk_popover_set_position(GTK_POPOVER(popover), GTK_POS_BOTTOM);
    gtk_popover_set_has_arrow(GTK_POPOVER(popover), TRUE);
    gtk_popover_set_autohide(GTK_POPOVER(popover), TRUE);
    gtk_widget_set_parent(popover, parent);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_top(box, 8);
    gtk_widget_set_margin_bottom(box, 8);
    gtk_popover_set_child(GTK_POPOVER(popover), box);

    GtkWidget* icon = gtk_label_new("🔐");
    gtk_box_append(GTK_BOX(box), icon);

    GtkWidget* label = gtk_label_new(message.c_str());
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_box_append(GTK_BOX(box), label);

    // Add "View" button if URL and username are provided
    if (!url.empty() && !username.empty() && passwordManager) {
        GtkWidget* viewBtn = gtk_button_new_with_label("View");
        gtk_widget_add_css_class(viewBtn, "flat");
        g_object_set_data(G_OBJECT(viewBtn), "password_manager", passwordManager);
        g_object_set_data_full(G_OBJECT(viewBtn), "url", g_strdup(url.c_str()), g_free);
        g_object_set_data_full(G_OBJECT(viewBtn), "username", g_strdup(username.c_str()), g_free);
        g_object_set_data(G_OBJECT(viewBtn), "popover", popover);

        g_signal_connect(viewBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer) {
            auto* mgr = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "password_manager"));
            const char* entryUrl = static_cast<const char*>(g_object_get_data(G_OBJECT(btn), "url"));
            const char* entryUser = static_cast<const char*>(g_object_get_data(G_OBJECT(btn), "username"));
            GtkWidget* pop = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "popover"));

            if (mgr && entryUrl && entryUser) {
                // Close the toast
                if (pop && GTK_IS_POPOVER(pop)) {
                    gtk_popover_popdown(GTK_POPOVER(pop));
                }

                // Open password manager
                GtkWidget* topLevel = GTK_WIDGET(gtk_widget_get_root(GTK_WIDGET(btn)));
                if (topLevel && GTK_IS_WINDOW(topLevel)) {
                    mgr->showPasswordManager(GTK_WINDOW(topLevel));
                }
            }
        }), nullptr);

        gtk_box_append(GTK_BOX(box), viewBtn);
    }

    autofillToast = popover;
    gtk_popover_popup(GTK_POPOVER(popover));

    toastTimerSource = g_timeout_add_seconds_full(G_PRIORITY_DEFAULT, 3,
        +[](gpointer data) -> gboolean {
            auto* tab = static_cast<BrayaTab*>(data);
            if (tab && tab->autofillToast) {
                gtk_popover_popdown(GTK_POPOVER(tab->autofillToast));
                gtk_widget_unparent(tab->autofillToast);
                tab->autofillToast = nullptr;
            }
            if (tab) {
                tab->toastTimerSource = 0;
            }
            return G_SOURCE_REMOVE;
        },
        this,
        nullptr);
}

void BrayaTab::onAutofillRequest(WebKitUserContentManager* manager, JSCValue* value, gpointer userData) {
    if (!userData) return;

    BrayaTab* tab = static_cast<BrayaTab*>(userData);
    if (!tab || !tab->passwordManager) return;

    GdkRectangle anchorRect{0, 0, 0, 0};
    bool hasAnchor = false;
    if (value && jsc_value_is_object(value)) {
        JSCValue* rectValue = jsc_value_object_get_property(value, "rect");
        if (rectValue && jsc_value_is_object(rectValue)) {
            auto getDouble = [](JSCValue* val) -> double {
                if (!val) {
                    return 0.0;
                }
                double result = jsc_value_to_double(val);
                g_object_unref(val);
                return result;
            };

            double x = getDouble(jsc_value_object_get_property(rectValue, "x"));
            double y = getDouble(jsc_value_object_get_property(rectValue, "y"));
            double width = getDouble(jsc_value_object_get_property(rectValue, "width"));
            double height = getDouble(jsc_value_object_get_property(rectValue, "height"));

            g_object_unref(rectValue);

            double scale = webkit_web_view_get_zoom_level(tab->webView);
            anchorRect.x = static_cast<int>(x * scale);
            anchorRect.y = static_cast<int>((y + height) * scale);
            anchorRect.width = static_cast<int>(std::max(width, 200.0) * scale);
            anchorRect.height = 1;
            hasAnchor = true;
        }
    }

    std::cout << "🔑 Autofill requested for tab " << tab->id << std::endl;
    tab->showAutofillSuggestions(hasAnchor ? &anchorRect : nullptr);
}

void BrayaTab::onCheckPasswords(WebKitUserContentManager* manager, JSCValue* value, gpointer userData) {
    if (!userData) return;

    BrayaTab* tab = static_cast<BrayaTab*>(userData);
    if (!tab || !tab->passwordManager) return;

    // Check if passwords exist for this URL
    auto passwords = tab->passwordManager->getPasswordsForUrl(tab->url);
    
    if (!passwords.empty()) {
        // Tell JavaScript to add visual indicators
        std::string script = "if (typeof addPasswordIndicators === 'function') { addPasswordIndicators(); }";
        
        webkit_web_view_evaluate_javascript(
            tab->webView,
            script.c_str(),
            -1,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr
        );
        
        std::cout << "✓ Added password indicators for " << passwords.size() << " account(s)" << std::endl;
    }
}

// Extension Detector for Automatic Installation

void BrayaTab::setupExtensionDetector() {
    if (!userContentManager) return;

    // Register message handler for extension installation requests
    g_signal_connect(userContentManager, "script-message-received::installExtension",
                     G_CALLBACK(onExtensionInstallRequest), this);
    webkit_user_content_manager_register_script_message_handler(userContentManager, "installExtension", nullptr);

    std::cout << "✓ Extension detector set up for tab " << id << std::endl;
}

void BrayaTab::injectExtensionDetectorScript() {
    // Get path to extension-detector.js
    std::string scriptPath = getResourcePath("extension-detector.js");

    // Read script content
    std::ifstream scriptFile(scriptPath);
    if (!scriptFile.is_open()) {
        std::cerr << "✗ Failed to load extension-detector.js from: " << scriptPath << std::endl;
        return;
    }

    std::stringstream buffer;
    buffer << scriptFile.rdbuf();
    std::string scriptContent = buffer.str();
    scriptFile.close();

    // Inject the script using evaluate_javascript with proper callback
    webkit_web_view_evaluate_javascript(
        webView,
        scriptContent.c_str(),
        -1,
        nullptr,
        nullptr,
        nullptr,
        +[](GObject* object, GAsyncResult* result, gpointer user_data) {
            GError* error = nullptr;
            JSCValue* value = webkit_web_view_evaluate_javascript_finish(
                WEBKIT_WEB_VIEW(object), result, &error);

            if (error) {
                std::cerr << "✗ Extension detector script error: " << error->message << std::endl;
                g_error_free(error);
            } else {
                std::cout << "✓ Extension detector script executed successfully" << std::endl;
            }

            if (value) {
                g_object_unref(value);
            }
        },
        nullptr
    );

    std::cout << "✓ Extension detector script injected for tab " << id << std::endl;
}

void BrayaTab::onExtensionInstallRequest(WebKitUserContentManager* manager, JSCValue* value, gpointer userData) {
    if (!userData || !value) return;

    BrayaTab* tab = static_cast<BrayaTab*>(userData);
    if (!tab) return;

    std::cout << "🔌 Extension install request received from webpage" << std::endl;

    // Parse the JSON message from JavaScript
    char* jsonStr = jsc_value_to_json(value, 0);
    if (!jsonStr) {
        std::cerr << "✗ Failed to parse extension install message" << std::endl;
        return;
    }

    std::string jsonData(jsonStr);
    g_free(jsonStr);

    // Simple JSON parsing to extract url and downloadUrl
    std::string extensionUrl;
    std::string downloadUrl;

    // Find "url" field
    size_t urlPos = jsonData.find("\"url\"");
    if (urlPos != std::string::npos) {
        size_t colonPos = jsonData.find(":", urlPos);
        size_t quoteStart = jsonData.find("\"", colonPos);
        size_t quoteEnd = jsonData.find("\"", quoteStart + 1);
        if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
            extensionUrl = jsonData.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        }
    }

    // Find "downloadUrl" field (optional, might not be present for Chrome Web Store)
    size_t downloadPos = jsonData.find("\"downloadUrl\"");
    if (downloadPos != std::string::npos) {
        size_t colonPos = jsonData.find(":", downloadPos);
        size_t quoteStart = jsonData.find("\"", colonPos);
        size_t quoteEnd = jsonData.find("\"", quoteStart + 1);
        if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
            downloadUrl = jsonData.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        }
    }

    std::cout << "📦 Extension URL: " << extensionUrl << std::endl;
    if (!downloadUrl.empty()) {
        std::cout << "📦 Download URL: " << downloadUrl << std::endl;
    }

    // Call the callback if set
    if (tab->extensionInstallCallback) {
        tab->extensionInstallCallback(extensionUrl, downloadUrl);
    } else {
        std::cerr << "⚠️  No extension install callback set!" << std::endl;
    }
}

// Quick Wins Feature Implementations

void BrayaTab::setPinned(bool pin) {
    pinned = pin;
    updateButton();
    std::cout << (pin ? "📌" : "📍") << " Tab " << id << " " << (pin ? "pinned" : "unpinned") << std::endl;
}

void BrayaTab::setMuted(bool mute) {
    muted = mute;
    webkit_web_view_set_is_muted(webView, mute);
    updateButton();
    std::cout << (mute ? "🔇" : "🔊") << " Tab " << id << " " << (mute ? "muted" : "unmuted") << std::endl;
}

void BrayaTab::toggleReaderMode() {
    if (!webView) return;
    
    if (readerMode) {
        // Exit reader mode - reload original page
        readerMode = false;
        webkit_web_view_load_uri(webView, url.c_str());
        std::cout << "📖 → 🌐 Exited reader mode" << std::endl;
    } else {
        // Enter reader mode - extract main content
        readerMode = true;
        
        // JavaScript to extract readable content
        const char* script = R"(
            (function() {
                // Find main content
                const article = document.querySelector('article') || 
                               document.querySelector('[role="main"]') ||
                               document.querySelector('main') ||
                               document.body;
                
                // Get title
                const title = document.title;
                
                // Get readable text
                const clonedArticle = article.cloneNode(true);
                
                // Remove scripts, ads, nav, etc
                const removeSelectors = ['script', 'style', 'nav', 'header', 'footer', 
                                        'aside', '.ad', '.ads', '.advertisement',
                                        '.social-share', '.comments'];
                removeSelectors.forEach(sel => {
                    clonedArticle.querySelectorAll(sel).forEach(el => el.remove());
                });
                
                // Create clean HTML
                const readerHTML = `
                    <!DOCTYPE html>
                    <html>
                    <head>
                        <meta charset="UTF-8">
                        <title>${title}</title>
                        <style>
                            body {
                                max-width: 700px;
                                margin: 40px auto;
                                padding: 20px;
                                font-family: Georgia, serif;
                                font-size: 18px;
                                line-height: 1.6;
                                color: #333;
                                background: #fafafa;
                            }
                            h1, h2, h3 { font-family: system-ui, sans-serif; }
                            h1 { font-size: 32px; margin-bottom: 10px; }
                            img { max-width: 100%; height: auto; }
                            pre { background: #f0f0f0; padding: 15px; border-radius: 5px; overflow-x: auto; }
                            code { background: #f0f0f0; padding: 2px 6px; border-radius: 3px; }
                            blockquote { border-left: 4px solid #ddd; margin: 0; padding-left: 20px; color: #666; }
                            a { color: #0066cc; text-decoration: none; }
                            a:hover { text-decoration: underline; }
                        </style>
                    </head>
                    <body>
                        <h1>${title}</h1>
                        ${clonedArticle.innerHTML}
                    </body>
                    </html>
                `;
                
                return readerHTML;
            })();
        )";
        
        // Execute JavaScript and load result
        webkit_web_view_evaluate_javascript(
            webView,
            script,
            -1,
            nullptr,
            nullptr,
            nullptr,
            [](GObject* object, GAsyncResult* result, gpointer user_data) {
                BrayaTab* tab = static_cast<BrayaTab*>(user_data);
                GError* error = nullptr;
                
                JSCValue* value = webkit_web_view_evaluate_javascript_finish(
                    WEBKIT_WEB_VIEW(object), result, &error);
                
                if (error) {
                    std::cerr << "✗ Reader mode error: " << error->message << std::endl;
                    g_error_free(error);
                    tab->readerMode = false;
                    return;
                }
                
                if (value && jsc_value_is_string(value)) {
                    char* html = jsc_value_to_string(value);
                    webkit_web_view_load_html(WEBKIT_WEB_VIEW(object), html, tab->url.c_str());
                    g_free(html);
                    std::cout << "📖 Entered reader mode" << std::endl;
                }
                
                if (value) g_object_unref(value);
            },
            this
        );
    }
    
    updateButton();
}

// Extension content script injection
void BrayaTab::injectExtensionContentScripts(const std::string& pageUrl) {
    if (!extensionManager || !webView) {
        return;
    }

    std::cout << "🔌 Checking for content scripts to inject for: " << pageUrl << std::endl;

    // IMPORTANT: Don't inject content scripts into extension pages (popups, options, etc.)
    // Content scripts are meant for web pages, not extension internal pages
    if (pageUrl.find("file://") == 0) {
        // Check if this is an extension URL (contains the extensions directory)
        std::string extensionsDir = g_get_home_dir() + std::string("/.config/braya-browser/extensions/");
        if (pageUrl.find(extensionsDir) != std::string::npos) {
            std::cout << "  ⏭️  Skipping content script injection for extension page" << std::endl;
            return;
        }
    }

    auto extensions = extensionManager->getWebExtensions();

    for (auto* extension : extensions) {
        if (!extension || !extension->isEnabled()) {
            continue;
        }

        auto contentScripts = extension->getContentScripts();

        for (const auto& cs : contentScripts) {
            // Check if this content script matches the current URL
            bool matches = false;
            for (const auto& pattern : cs.matches) {
                if (pattern == "<all_urls>" ||
                    pattern == "http://*/*" ||
                    pattern == "https://*/*" ||
                    pageUrl.find(pattern) != std::string::npos) {
                    matches = true;
                    break;
                }

                // Basic wildcard matching for patterns like "https://example.com/*"
                if (pattern.back() == '*' && pageUrl.find(pattern.substr(0, pattern.length() - 1)) == 0) {
                    matches = true;
                    break;
                }
            }

            if (!matches) {
                continue;
            }

            std::cout << "  ✓ Extension '" << extension->getName() << "' matches this page" << std::endl;

            // Inject CSS files first
            for (const auto& cssFile : cs.css) {
                std::string cssPath = extension->getPath() + "/" + cssFile;
                std::ifstream file(cssPath);
                if (file.is_open()) {
                    std::stringstream buffer;
                    buffer << file.rdbuf();
                    std::string cssContent = buffer.str();
                    file.close();

                    // Escape CSS content for JavaScript string
                    std::string escapedCss;
                    for (char c : cssContent) {
                        if (c == '\\') escapedCss += "\\\\";
                        else if (c == '\'') escapedCss += "\'";
                        else if (c == '"') escapedCss += "\"";
                        else if (c == '\n') escapedCss += "\n";
                        else if (c == '\r') escapedCss += "\r";
                        else if (c == '\t') escapedCss += "\t";
                        else escapedCss += c;
                    }

                    // Inject CSS into the page
                    std::string script = "(function() { "
                                       "var style = document.createElement('style'); "
                                       "style.textContent = '" + escapedCss + "'; "
                                       "document.head.appendChild(style); "
                                       "})();";

                    webkit_web_view_evaluate_javascript(
                        webView,
                        script.c_str(),
                        -1,
                        nullptr,
                        nullptr,
                        nullptr,
                        nullptr,
                        nullptr
                    );

                    std::cout << "    → Injected CSS: " << cssFile << std::endl;
                }
            }

            // Inject JavaScript files
            for (const auto& jsFile : cs.js) {
                std::string jsPath = extension->getPath() + "/" + jsFile;
                std::ifstream file(jsPath);
                if (file.is_open()) {
                    std::stringstream buffer;
                    buffer << file.rdbuf();
                    std::string jsContent = buffer.str();
                    file.close();

                    // Wrap the script to provide comprehensive API for content scripts
                    std::string wrappedScript =
                        "(function() { \n" +
                        BrayaExtensionAPI::getContentScriptAPI() + "\n"
                        "  // Firefox compatibility: browser.* = chrome.*\n"
                        "  if (!window.browser) window.browser = window.chrome;\n"
                        "  \n"
                        "  console.log('🔌 [" + extension->getName() + "] Content script loading...');\n"
                        "  \n"
                        "  // Execute the actual content script\n"
                        "  try {\n" +
                        jsContent + "\n"
                        "    console.log('✓ [" + extension->getName() + "] Content script loaded successfully');\n"
                        "  } catch(e) {\n"
                        "    console.error('❌ [" + extension->getName() + "] Content script error:', e);\n"
                        "  }\n"
                        "})();";

                    webkit_web_view_evaluate_javascript(
                        webView,
                        wrappedScript.c_str(),
                        -1,
                        nullptr,
                        nullptr,
                        nullptr,
                        nullptr,
                        nullptr
                    );

                    std::cout << "    → Injected JS: " << jsFile << std::endl;
                }
            }
        }
    }
}

// 💤 Tab Suspension Implementation (Phase 2 Memory Optimization)

void BrayaTab::suspend() {
    if (suspended || !webView) return;

    // 🔧 Validate WebView is still valid before suspending
    if (!WEBKIT_IS_WEB_VIEW(webView)) {
        std::cerr << "⚠️ Warning: WebView for tab " << id << " is not valid, marking as suspended" << std::endl;
        suspended = true;
        return;
    }

    std::cout << "💤 Suspending tab " << id << ": " << title << std::endl;

    // Save current state
    suspendedUrl = url;
    suspendedTitle = title;

    // Cache favicon if available
    if (favicon && GDK_IS_TEXTURE(favicon)) {
        cachedFavicon = (GdkTexture*)g_object_ref(favicon);
    }

    // Disconnect signals before destroying WebView
    if (WEBKIT_IS_WEB_VIEW(webView)) {
        g_signal_handlers_disconnect_by_data(webView, this);
    }

    // Destroy WebView to free memory
    if (scrolledWindow && GTK_IS_WIDGET(scrolledWindow)) {
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolledWindow), nullptr);
    }

    if (webView && GTK_IS_WIDGET(webView)) {
        webkit_web_view_try_close(webView);
        // WebView will be destroyed automatically when its last reference is dropped
    }

    webView = nullptr;
    suspended = true;

    // Update tab button to show suspended state
    if (tabButton && GTK_IS_WIDGET(tabButton)) {
        gtk_widget_add_css_class(tabButton, "suspended");
    }

    std::cout << "  ✓ Tab suspended, WebView destroyed" << std::endl;
}

void BrayaTab::resume() {
    if (!suspended) return;

    std::cout << "⏰ Resuming tab " << id << ": " << suspendedTitle << std::endl;

    // Recreate WebView
    webView = WEBKIT_WEB_VIEW(webkit_web_view_new());

    // 🔧 Validate WebView was created successfully
    if (!webView || !WEBKIT_IS_WEB_VIEW(webView)) {
        std::cerr << "❌ Error: Failed to create WebView for tab " << id << std::endl;
        return;
    }

    // Get the UserContentManager from the webview
    userContentManager = webkit_web_view_get_user_content_manager(webView);

    // 🔧 Validate UserContentManager
    if (!userContentManager) {
        std::cerr << "❌ Error: Failed to get UserContentManager for tab " << id << std::endl;
        return;
    }

    // Enable developer tools
    WebKitSettings* settings = webkit_web_view_get_settings(webView);
    webkit_settings_set_enable_developer_extras(settings, TRUE);
    webkit_settings_set_enable_smooth_scrolling(settings, TRUE);

    // Reattach to scrolled window
    if (scrolledWindow && GTK_IS_WIDGET(scrolledWindow)) {
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_WIDGET(webView));
    }

    // Setup password manager again
    if (passwordManager) {
        setupPasswordManager();
    }

    // Setup extension detector again
    setupExtensionDetector();

    // Reconnect signals
    g_signal_connect(webView, "load-changed", G_CALLBACK(onLoadChanged), this);
    g_signal_connect(webView, "notify::title", G_CALLBACK(onTitleChanged), this);
    g_signal_connect(webView, "notify::uri", G_CALLBACK(onUriChanged), this);
    g_signal_connect(webView, "notify::favicon", G_CALLBACK(onFaviconChanged), this);
    g_signal_connect(webView, "web-process-terminated", G_CALLBACK(onWebProcessCrashed), this);
    g_signal_connect(webView, "create", G_CALLBACK(onCreateNewWindow), this);
    g_signal_connect(webView, "decide-policy", G_CALLBACK(onDecidePolicy), this);

    // Reload the URL
    if (!suspendedUrl.empty()) {
        std::string finalUrl = suspendedUrl;

        // Handle about:braya
        if (finalUrl == "about:braya") {
            finalUrl = "file://" + getResourcePath("home.html");
        }

        webkit_web_view_load_uri(webView, finalUrl.c_str());
    }

    suspended = false;

    // Remove suspended CSS class
    if (tabButton && GTK_IS_WIDGET(tabButton)) {
        gtk_widget_remove_css_class(tabButton, "suspended");
    }

    // Clean up cached favicon
    if (cachedFavicon) {
        g_object_unref(cachedFavicon);
        cachedFavicon = nullptr;
    }

    std::cout << "  ✓ Tab resumed, WebView recreated" << std::endl;
}