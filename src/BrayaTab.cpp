#include "BrayaTab.h"
#include "BrayaPasswordManager.h"
#include <iostream>
#include <cctype>
#include <fstream>
#include <sstream>
#include <vector>
#include <sys/stat.h>

BrayaTab::BrayaTab(int id, const char* url, BrayaPasswordManager* passwordMgr)
    : id(id), url(url ? url : "about:braya"), title("New Tab"), isLoading(false),
      favicon(nullptr), tabButton(nullptr), passwordManager(passwordMgr), userContentManager(nullptr) {

    // Create WebView first
    webView = WEBKIT_WEB_VIEW(webkit_web_view_new());

    // Get the UserContentManager from the webview
    userContentManager = webkit_web_view_get_user_content_manager(webView);
    
    // Enable developer tools
    WebKitSettings* settings = webkit_web_view_get_settings(webView);
    webkit_settings_set_enable_developer_extras(settings, TRUE);
    webkit_settings_set_enable_smooth_scrolling(settings, TRUE);
    
    // Create scrolled window
    scrolledWindow = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_WIDGET(webView));
    
    // Setup password manager
    if (passwordManager) {
        setupPasswordManager();
    }

    // Connect signals
    g_signal_connect(webView, "load-changed", G_CALLBACK(onLoadChanged), this);
    g_signal_connect(webView, "notify::title", G_CALLBACK(onTitleChanged), this);
    g_signal_connect(webView, "notify::uri", G_CALLBACK(onUriChanged), this);
    g_signal_connect(webView, "notify::favicon", G_CALLBACK(onFaviconChanged), this);
    g_signal_connect(webView, "web-process-terminated", G_CALLBACK(onWebProcessCrashed), this);
    
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
    // Disconnect all signals before destruction
    if (webView && WEBKIT_IS_WEB_VIEW(webView)) {
        g_signal_handlers_disconnect_by_data(webView, this);
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
            GdkTexture* favicon = webkit_web_view_get_favicon(webView);
            if (favicon && GDK_IS_TEXTURE(favicon)) {
                std::cout << "🎯 Forcing favicon update on load finish" << std::endl;
                if (tab->favicon) {
                    g_object_unref(tab->favicon);
                    tab->favicon = nullptr;
                }
                tab->favicon = GDK_TEXTURE(g_object_ref(favicon));
            }

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
        // Get favicon
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
        GtkWidget* iconBox = GTK_WIDGET(g_object_get_data(G_OBJECT(tabButton), "icon-box"));
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
        
        // If we have a favicon, show it as an image
        if (favicon && GDK_IS_TEXTURE(favicon)) {
            GtkWidget* image = gtk_image_new_from_paintable(GDK_PAINTABLE(favicon));
            if (image && GTK_IS_IMAGE(image)) {
                gtk_image_set_pixel_size(GTK_IMAGE(image), 20);
                gtk_widget_set_size_request(image, 20, 20);
                gtk_box_append(GTK_BOX(iconBox), image);
                std::cout << "✅ Favicon displayed for tab " << id << std::endl;
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
                gtk_box_append(GTK_BOX(iconBox), label);
            }
        }
        
        // Set tooltip with title and URL
        std::string tooltip = title.empty() ? "New Tab" : title;
        if (!url.empty() && url != title) {
            tooltip += "\n" + url;
        }
        gtk_widget_set_tooltip_text(tabButton, tooltip.c_str());
        
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
        while ((pos = escaped.find('\'', pos)) != std::string::npos) {
            escaped.replace(pos, 1, "\\'");
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
        std::cout << "🔑 Password captured for " << username << " at " << url << std::endl;

        // Create "Save password?" dialog
        GtkWidget* dialog = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(dialog), "Save Password?");
        gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 150);
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
        gtk_widget_set_margin_start(box, 20);
        gtk_widget_set_margin_end(box, 20);
        gtk_widget_set_margin_top(box, 20);
        gtk_widget_set_margin_bottom(box, 20);
        gtk_window_set_child(GTK_WINDOW(dialog), box);

        // Message
        GtkWidget* label = gtk_label_new(nullptr);
        std::string message = "Save password for <b>" + std::string(username) + "</b>?";
        gtk_label_set_markup(GTK_LABEL(label), message.c_str());
        gtk_box_append(GTK_BOX(box), label);

        GtkWidget* urlLabel = gtk_label_new(url);
        gtk_widget_add_css_class(urlLabel, "dim-label");
        gtk_box_append(GTK_BOX(box), urlLabel);

        // Buttons
        GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_halign(buttonBox, GTK_ALIGN_END);
        gtk_box_append(GTK_BOX(box), buttonBox);

        GtkWidget* cancelBtn = gtk_button_new_with_label("Not Now");
        g_signal_connect_swapped(cancelBtn, "clicked", G_CALLBACK(gtk_window_close), dialog);
        gtk_box_append(GTK_BOX(buttonBox), cancelBtn);

        GtkWidget* saveBtn = gtk_button_new_with_label("Save");
        gtk_widget_add_css_class(saveBtn, "suggested-action");

        // Store data for save callback
        g_object_set_data_full(G_OBJECT(saveBtn), "url", g_strdup(url), g_free);
        g_object_set_data_full(G_OBJECT(saveBtn), "username", g_strdup(username), g_free);
        g_object_set_data_full(G_OBJECT(saveBtn), "password", g_strdup(password), g_free);
        g_object_set_data(G_OBJECT(saveBtn), "manager", tab->passwordManager);
        g_object_set_data(G_OBJECT(saveBtn), "dialog", dialog);

        g_signal_connect(saveBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
            auto* mgr = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
            const char* url = (const char*)g_object_get_data(G_OBJECT(btn), "url");
            const char* user = (const char*)g_object_get_data(G_OBJECT(btn), "username");
            const char* pass = (const char*)g_object_get_data(G_OBJECT(btn), "password");
            GtkWidget* dlg = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "dialog"));

            if (mgr && url && user && pass) {
                mgr->savePassword(url, user, pass);
            }
            gtk_window_close(GTK_WINDOW(dlg));
        }), nullptr);

        gtk_box_append(GTK_BOX(buttonBox), saveBtn);

        gtk_window_present(GTK_WINDOW(dialog));
    }

    g_free(url);
    g_free(username);
    g_free(password);
}

void BrayaTab::showAutofillSuggestions() {
    if (!passwordManager || url.empty()) return;

    // Get saved passwords for current URL
    auto passwords = passwordManager->getPasswordsForUrl(url);

    if (passwords.empty()) {
        return; // No saved passwords for this site
    }

    // If only one password, auto-fill immediately
    if (passwords.size() == 1) {
        autoFillPasswords();
        return;
    }

    // Multiple passwords - show selection dialog (Safari-like)
    GtkWidget* dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Choose Account");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 300);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_window_set_child(GTK_WINDOW(dialog), box);

    GtkWidget* label = gtk_label_new("Select an account to autofill:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), label);

    // Scrolled window for account list
    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_box_append(GTK_BOX(box), scrolled);

    GtkWidget* listBox = gtk_list_box_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), listBox);

    // Add account entries
    for (const auto& entry : passwords) {
        GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_margin_start(row, 10);
        gtk_widget_set_margin_end(row, 10);
        gtk_widget_set_margin_top(row, 10);
        gtk_widget_set_margin_bottom(row, 10);

        GtkWidget* accountLabel = gtk_label_new(entry.username.c_str());
        gtk_widget_set_halign(accountLabel, GTK_ALIGN_START);
        gtk_widget_set_hexpand(accountLabel, TRUE);
        gtk_box_append(GTK_BOX(row), accountLabel);

        GtkWidget* fillBtn = gtk_button_new_with_label("Fill");
        gtk_widget_add_css_class(fillBtn, "suggested-action");

        // Store data for callback
        g_object_set_data_full(G_OBJECT(fillBtn), "username", g_strdup(entry.username.c_str()), g_free);
        g_object_set_data_full(G_OBJECT(fillBtn), "password", g_strdup(entry.password.c_str()), g_free);
        g_object_set_data(G_OBJECT(fillBtn), "tab", this);
        g_object_set_data(G_OBJECT(fillBtn), "dialog", dialog);

        g_signal_connect(fillBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
            auto* tab = static_cast<BrayaTab*>(g_object_get_data(G_OBJECT(btn), "tab"));
            const char* username = (const char*)g_object_get_data(G_OBJECT(btn), "username");
            const char* password = (const char*)g_object_get_data(G_OBJECT(btn), "password");
            GtkWidget* dlg = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "dialog"));

            if (tab && username && password) {
                // Escape quotes for JavaScript
                auto escapeJs = [](const std::string& str) {
                    std::string escaped = str;
                    size_t pos = 0;
                    while ((pos = escaped.find('\'', pos)) != std::string::npos) {
                        escaped.replace(pos, 1, "\\'");
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

            gtk_window_close(GTK_WINDOW(dlg));
        }), nullptr);

        gtk_box_append(GTK_BOX(row), fillBtn);
        gtk_list_box_append(GTK_LIST_BOX(listBox), row);
    }

    // Cancel button
    GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(buttonBox, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(box), buttonBox);

    GtkWidget* cancelBtn = gtk_button_new_with_label("Cancel");
    g_signal_connect_swapped(cancelBtn, "clicked", G_CALLBACK(gtk_window_close), dialog);
    gtk_box_append(GTK_BOX(buttonBox), cancelBtn);

    gtk_window_present(GTK_WINDOW(dialog));
}

void BrayaTab::onAutofillRequest(WebKitUserContentManager* manager, JSCValue* value, gpointer userData) {
    if (!userData) return;

    BrayaTab* tab = static_cast<BrayaTab*>(userData);
    if (!tab || !tab->passwordManager) return;

    std::cout << "🔑 Autofill requested for tab " << tab->id << std::endl;
    tab->showAutofillSuggestions();
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
