#include "BrayaWindow.h"
#include "BrayaTab.h"
#include "BrayaSettings.h"
#include "BrayaHistory.h"
#include "BrayaDownloads.h"
#include "BrayaBookmarks.h"
#include "BrayaPasswordManager.h"
#include "TabGroup.h"
#include <iostream>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

BrayaWindow::BrayaWindow(GtkApplication* app)
    : activeTabIndex(-1), nextTabId(1), showBookmarksBar(true), nextGroupId(1),
      settings(std::make_unique<BrayaSettings>()), 
      history(std::make_unique<BrayaHistory>()),
      downloads(std::make_unique<BrayaDownloads>()),
      bookmarksManager(std::make_unique<BrayaBookmarks>()),
      passwordManager(std::make_unique<BrayaPasswordManager>()),
      cssProvider(nullptr) {
    
    g_print("Creating Braya window...\n");
    
    // Create main window - let GTK handle decorations with headerbar
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Braya Browser");
    gtk_window_set_default_size(GTK_WINDOW(window), 1400, 900);
    
    // Don't remove decorations - we'll use headerbar for Firefox-style controls
    
    g_print("Window created\n");
    
    // Connect theme callback
    settings->setThemeCallback([this](int themeId) {
        this->applyTheme(themeId);
    });
    
    // Setup CSS
    setupCSS();
    
    g_print("CSS loaded\n");
    
    // Build UI
    setupUI();
    
    g_print("UI built\n");
    
    // Create first tab (this will give us access to WebContext)
    createTab();
    
    // Setup download handling for WebContext
    // In WebKit2GTK 2.50, we need to connect to the default context
    if (!tabs.empty()) {
        WebKitWebContext* context = webkit_web_context_get_default();
        g_print("Connecting to WebContext for downloads...\n");
        
        // Try using GObject signal connection with error suppression
        GError* error = nullptr;
        gulong handler = g_signal_connect_data(context, "download-started",
            G_CALLBACK(+[](WebKitWebContext* ctx, WebKitDownload* download, gpointer data) {
                std::cout << "🔽 Download started via context!" << std::endl;
                BrayaWindow* window = static_cast<BrayaWindow*>(data);
                if (window && window->downloads) {
                    window->downloads->handleDownload(download);
                }
            }), this, nullptr, (GConnectFlags)0);
        
        if (handler > 0) {
            g_print("✓ Download handler connected (id: %lu)\n", handler);
        } else {
            g_print("⚠ Could not connect download-started, using fallback\n");
        }
    }
    
    // Setup download handling for WebContext
    // Downloads created by webkit_policy_decision_download() should trigger this
    if (!tabs.empty()) {
        WebKitWebContext* context = webkit_web_context_get_default();
        
        // Suppress GLib warnings during connection attempt
        g_log_set_handler("GLib-GObject", (GLogLevelFlags)(G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING), 
            +[](const gchar*, GLogLevelFlags, const gchar*, gpointer){}, nullptr);
        
        gulong handler = g_signal_connect(context, "download-started",
            G_CALLBACK(+[](WebKitWebContext* ctx, WebKitDownload* download, gpointer data) {
                std::cout << "📥 Download caught by context!" << std::endl;
                BrayaWindow* window = static_cast<BrayaWindow*>(data);
                if (window && window->downloads) {
                    window->downloads->handleDownload(download);
                }
            }), this);
        
        // Restore normal logging
        g_log_set_handler("GLib-GObject", (GLogLevelFlags)(G_LOG_LEVEL_MASK), g_log_default_handler, nullptr);
        
        if (handler > 0) {
            g_print("✓ WebContext download handler connected\n");
        }
    }
    
    g_print("First tab created\n");
    
    // Connect keyboard shortcuts
    GtkEventController* key_controller = gtk_event_controller_key_new();
    g_signal_connect(key_controller, "key-pressed", G_CALLBACK(onKeyPress), this);
    gtk_widget_add_controller(window, key_controller);
    
    g_print("Braya window initialization complete!\n");
}

BrayaWindow::~BrayaWindow() {
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

void BrayaWindow::setupUI() {
    // Main horizontal box
    mainBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), mainBox);
    
    // Create sidebar
    createSidebar();
    gtk_box_append(GTK_BOX(mainBox), sidebar);
    
    // Content area
    contentBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(mainBox), contentBox);
    gtk_widget_set_hexpand(contentBox, TRUE);
    gtk_widget_set_vexpand(contentBox, TRUE);
    
    // Create components
    createNavbar();
    // Don't append navbar - it's in the headerbar now!
    
    createBookmarksBar();
    gtk_box_append(GTK_BOX(contentBox), bookmarksBar);
    
    // Tab stack for web views
    tabStack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(tabStack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(GTK_STACK(tabStack), 150);
    gtk_box_append(GTK_BOX(contentBox), tabStack);
    gtk_widget_set_vexpand(tabStack, TRUE);
    
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
    
    // No status bar - maximize space!
}

void BrayaWindow::createSidebar() {
    sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(sidebar, 56, -1);  // Zen-like width
    gtk_widget_add_css_class(sidebar, "sidebar");
    
    // No dog here - it's in the headerbar
    
    // Separator
    GtkWidget* sep1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_margin_top(sep1, 5);
    gtk_widget_set_margin_bottom(sep1, 5);
    gtk_box_append(GTK_BOX(sidebar), sep1);
    
    // Tabs scrolled window
    GtkWidget* tabsScroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tabsScroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    
    tabsBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(tabsScroll), tabsBox);
    gtk_box_append(GTK_BOX(sidebar), tabsScroll);
    gtk_widget_set_vexpand(tabsScroll, TRUE);
    
    // Bottom separator
    GtkWidget* sep2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_margin_top(sep2, 5);
    gtk_widget_set_margin_bottom(sep2, 5);
    gtk_box_append(GTK_BOX(sidebar), sep2);
    
    // New tab button (compact)
    GtkWidget* newTabBtn = gtk_button_new_with_label("+");
    gtk_widget_set_tooltip_text(newTabBtn, "New Tab (Ctrl+T)");
    gtk_widget_add_css_class(newTabBtn, "new-tab-btn");
    g_signal_connect(newTabBtn, "clicked", G_CALLBACK(onNewTabClicked), this);
    gtk_box_append(GTK_BOX(sidebar), newTabBtn);
    
    // Downloads button above settings
    GtkWidget* downloadsBtn = gtk_button_new_with_label("📥");
    gtk_widget_set_tooltip_text(downloadsBtn, "Downloads (Ctrl+J)");
    gtk_widget_add_css_class(downloadsBtn, "settings-btn");
    g_signal_connect(downloadsBtn, "clicked", G_CALLBACK(onDownloadsClicked), this);
    gtk_box_append(GTK_BOX(sidebar), downloadsBtn);
    
    // Password Manager button
    GtkWidget* passwordBtn = gtk_button_new_with_label("🔑");
    gtk_widget_set_tooltip_text(passwordBtn, "Password Manager (Ctrl+K)");
    gtk_widget_add_css_class(passwordBtn, "settings-btn");
    g_signal_connect(passwordBtn, "clicked", G_CALLBACK(+[](GtkWidget* w, gpointer data) {
        BrayaWindow* window = static_cast<BrayaWindow*>(data);
        window->showPasswordManager();
    }), this);
    gtk_box_append(GTK_BOX(sidebar), passwordBtn);
    
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
    gtk_window_set_titlebar(GTK_WINDOW(window), headerbar);
    
    // Left side box for all left controls
    GtkWidget* leftBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_add_css_class(leftBox, "navbar-left");
    gtk_widget_set_halign(leftBox, GTK_ALIGN_START);
    
    // Dog logo
    GtkWidget* dogLabel = gtk_label_new("🐕");
    gtk_widget_add_css_class(dogLabel, "braya-logo");
    gtk_box_append(GTK_BOX(leftBox), dogLabel);
    
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
    
    // Small spacer after home button
    GtkWidget* spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_size_request(spacer, 6, -1);
    gtk_box_append(GTK_BOX(leftBox), spacer);
    
    gtk_header_bar_pack_start(GTK_HEADER_BAR(headerbar), leftBox);
    
    // Center: URL entry COMPACT - like Firefox
    urlEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(urlEntry), "Search or enter address");
    gtk_widget_add_css_class(urlEntry, "url-entry");
    gtk_widget_set_hexpand(urlEntry, TRUE);
    g_signal_connect(urlEntry, "activate", G_CALLBACK(onUrlActivate), this);
    
    // Select all text when URL entry gets focus
    GtkEventController* focus_controller = gtk_event_controller_focus_new();
    g_signal_connect_swapped(focus_controller, "enter", 
        G_CALLBACK(+[](GtkWidget* entry) {
            gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
        }), urlEntry);
    gtk_widget_add_controller(urlEntry, focus_controller);
    
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(headerbar), urlEntry);
    
    // Right side box for all right controls
    GtkWidget* rightBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(rightBox, "navbar-right");
    
    // Bookmark button
    GtkWidget* bookmarkBtn = gtk_button_new_from_icon_name("starred-symbolic");
    gtk_widget_set_tooltip_text(bookmarkBtn, "Bookmark This Page");
    gtk_widget_add_css_class(bookmarkBtn, "action-btn");
    gtk_box_append(GTK_BOX(rightBox), bookmarkBtn);
    
    // Split view button
    GtkWidget* splitViewBtn = gtk_button_new_from_icon_name("view-dual-symbolic");
    gtk_widget_set_tooltip_text(splitViewBtn, "Split View (Coming Soon)");
    gtk_widget_add_css_class(splitViewBtn, "action-btn");
    gtk_box_append(GTK_BOX(rightBox), splitViewBtn);
    
    // Developer tools button
    GtkWidget* devToolsBtn = gtk_button_new_from_icon_name("utilities-terminal-symbolic");
    gtk_widget_set_tooltip_text(devToolsBtn, "Developer Tools (F12)");
    gtk_widget_add_css_class(devToolsBtn, "action-btn");
    g_signal_connect(devToolsBtn, "clicked", G_CALLBACK(onDevToolsClicked), this);
    gtk_box_append(GTK_BOX(rightBox), devToolsBtn);
    
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
    bookmarksBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_add_css_class(bookmarksBar, "bookmarks-bar");
    gtk_widget_set_margin_start(bookmarksBar, 15);
    gtk_widget_set_margin_end(bookmarksBar, 15);
    gtk_widget_set_margin_top(bookmarksBar, 5);
    gtk_widget_set_margin_bottom(bookmarksBar, 5);
    
    // Sample bookmarks
    const char* bookmarks[][2] = {
        {"🦆 DuckDuckGo", "https://duckduckgo.com"},
        {"📰 Hacker News", "https://news.ycombinator.com"},
        {"🐙 GitHub", "https://github.com"},
        {"📺 YouTube", "https://youtube.com"}
    };
    
    for (int i = 0; i < 4; i++) {
        GtkWidget* btn = gtk_button_new_with_label(bookmarks[i][0]);
        gtk_widget_add_css_class(btn, "bookmark-btn");
        g_object_set_data_full(G_OBJECT(btn), "url", g_strdup(bookmarks[i][1]), g_free);
        g_signal_connect(btn, "clicked", G_CALLBACK(onBookmarkClicked), this);
        gtk_box_append(GTK_BOX(bookmarksBar), btn);
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
    
    GtkWidget* engineLabel = gtk_label_new("WebKit Engine");
    gtk_widget_add_css_class(engineLabel, "braya-title");
    gtk_box_append(GTK_BOX(statusBar), engineLabel);
}

// Tab management
void BrayaWindow::createTab(const char* url) {
    auto tab = std::make_unique<BrayaTab>(nextTabId++, url);
    
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
    
    // Main tab button
    GtkWidget* tabBtn = gtk_button_new();
    gtk_widget_add_css_class(tabBtn, "tab-button");
    gtk_widget_set_size_request(tabBtn, 48, 48);
    gtk_button_set_child(GTK_BUTTON(tabBtn), faviconBox);
    
    g_object_set_data(G_OBJECT(tabBtn), "tab-index", GINT_TO_POINTER(tabs.size()));
    g_object_set_data(G_OBJECT(tabBtn), "favicon-box", faviconBox);
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
    
    // Switch stack view
    char name[32];
    snprintf(name, sizeof(name), "tab-%d", tab->getId());
    gtk_stack_set_visible_child_name(GTK_STACK(tabStack), name);
    
    // Update URL bar
    gtk_editable_set_text(GTK_EDITABLE(urlEntry), tab->getUrl().c_str());
    
    // Update navigation buttons
    WebKitWebView* webView = tab->getWebView();
    gtk_widget_set_sensitive(backBtn, webkit_web_view_can_go_back(webView));
    gtk_widget_set_sensitive(forwardBtn, webkit_web_view_can_go_forward(webView));
    
    // Update tab button styles
    for (size_t i = 0; i < tabs.size(); i++) {
        GtkWidget* btn = tabs[i]->getTabButton();
        if ((int)i == index) {
            gtk_widget_add_css_class(btn, "active");
        } else {
            gtk_widget_remove_css_class(btn, "active");
        }
    }
    
    // Update window title
    std::string title = "🐕 Braya Browser - " + tab->getTitle();
    gtk_window_set_title(GTK_WINDOW(window), title.c_str());
}

void BrayaWindow::closeTab(int index) {
    if (tabs.size() <= 1) return; // Don't close last tab
    if (index < 0 || index >= (int)tabs.size()) return;
    
    // Remove from UI
    GtkWidget* tabBtn = tabs[index]->getTabButton();
    if (tabBtn && GTK_IS_WIDGET(tabBtn)) {
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
    if (activeTabIndex >= 0) {
        BrayaTab* tab = tabs[activeTabIndex].get();
        WebKitWebView* webView = tab->getWebView();
        
        gtk_widget_set_sensitive(backBtn, webkit_web_view_can_go_back(webView));
        gtk_widget_set_sensitive(forwardBtn, webkit_web_view_can_go_forward(webView));
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
    if (activeTabIndex >= 0) {
        BrayaTab* tab = tabs[activeTabIndex].get();
        WebKitFindController* findController = webkit_web_view_get_find_controller(tab->getWebView());
        webkit_find_controller_search_finish(findController);
    }
}

void BrayaWindow::findNext() {
    if (activeTabIndex < 0) return;
    
    BrayaTab* tab = tabs[activeTabIndex].get();
    WebKitFindController* findController = webkit_web_view_get_find_controller(tab->getWebView());
    webkit_find_controller_search_next(findController);
}

void BrayaWindow::findPrevious() {
    if (activeTabIndex < 0) return;
    
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
    if (window->activeTabIndex >= 0) {
        BrayaTab* tab = window->tabs[window->activeTabIndex].get();
        webkit_web_view_go_back(tab->getWebView());
    }
}

void BrayaWindow::onForwardClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    if (window->activeTabIndex >= 0) {
        BrayaTab* tab = window->tabs[window->activeTabIndex].get();
        webkit_web_view_go_forward(tab->getWebView());
    }
}

void BrayaWindow::onReloadClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
    if (window->activeTabIndex >= 0) {
        BrayaTab* tab = window->tabs[window->activeTabIndex].get();
        webkit_web_view_reload(tab->getWebView());
    }
}

void BrayaWindow::onHomeClicked(GtkWidget* widget, gpointer data) {
    BrayaWindow* window = static_cast<BrayaWindow*>(data);
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
    if (window->activeTabIndex < 0) return;
    
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
    if (window->activeTabIndex >= 0) {
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
        } else if (keyval == GDK_KEY_w) {
            if (window->tabs.size() > 1 && window->activeTabIndex >= 0) {
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
            window->showBookmarksManager();
            return TRUE;
        }
    }
    
    if (keyval == GDK_KEY_F12) {
        if (window->activeTabIndex >= 0) {
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
                // Add colored border indicator
                std::string css = "border-left: 3px solid " + tabGroups[groupId]->getColor() + ";";
                // Apply CSS inline
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
    
    // Show context menu with group options
    GtkWidget* menu = gtk_popover_new();
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_margin_start(box, 10);
    gtk_widget_set_margin_end(box, 10);
    gtk_widget_set_margin_top(box, 10);
    gtk_widget_set_margin_bottom(box, 10);
    
    // Create group button
    GtkWidget* createBtn = gtk_button_new_with_label("📁 Create New Group");
    gtk_box_append(GTK_BOX(box), createBtn);
    
    // Add to group buttons
    for (size_t i = 0; i < window->tabGroups.size(); i++) {
        std::string label = "➕ " + window->tabGroups[i]->getName();
        GtkWidget* btn = gtk_button_new_with_label(label.c_str());
        gtk_box_append(GTK_BOX(box), btn);
    }
    
    gtk_popover_set_child(GTK_POPOVER(menu), box);
    gtk_widget_set_parent(menu, GTK_WIDGET(gesture));
    gtk_popover_popup(GTK_POPOVER(menu));
}

void BrayaWindow::showPasswordManager() {
    if (passwordManager) {
        passwordManager->showPasswordManager(GTK_WINDOW(window));
    }
}
