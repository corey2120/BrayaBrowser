#include "BrayaTab.h"
#include <iostream>
#include <cctype>

BrayaTab::BrayaTab(int id, const char* url)
    : id(id), url(url ? url : "about:braya"), title("New Tab"), isLoading(false), favicon(nullptr), tabButton(nullptr) {
    
    // Create WebView
    webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    
    // Enable developer tools
    WebKitSettings* settings = webkit_web_view_get_settings(webView);
    webkit_settings_set_enable_developer_extras(settings, TRUE);
    webkit_settings_set_enable_smooth_scrolling(settings, TRUE);
    
    // Create scrolled window
    scrolledWindow = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_WIDGET(webView));
    
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
            char exePath[1024];
            ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
            if (len != -1) {
                exePath[len] = '\0';
                std::string exeDir(exePath);
                size_t lastSlash = exeDir.find_last_of('/');
                if (lastSlash != std::string::npos) {
                    exeDir = exeDir.substr(0, lastSlash);
                    // Go up one level from build/ to project root
                    lastSlash = exeDir.find_last_of('/');
                    if (lastSlash != std::string::npos) {
                        exeDir = exeDir.substr(0, lastSlash);
                    }
                }
                finalUrl = "file://" + exeDir + "/resources/home.html";
            }
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
