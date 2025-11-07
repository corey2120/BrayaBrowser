#include "BrayaSettings.h"
#include "BrayaCustomization.h"
#include "extensions/BrayaExtensionManager.h"
#include "extensions/BrayaWebExtension.h"
#include "extensions/ExtensionInstaller.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <json-glib/json-glib.h>

BrayaSettings::BrayaSettings()
    : theme(DARK), fontSize(13), fontFamily("Sans"), showBookmarks(true),
      blockTrackers(true), blockAds(false), httpsOnly(false),
      enableJavaScript(true), enableWebGL(true), enablePlugins(false),
      downloadPath(""), homePage("about:braya"), searchEngine("DuckDuckGo"),
      dialog(nullptr), notebook(nullptr), m_extensionManager(nullptr), extensionsList(nullptr) {
    
    downloadPath = std::string(g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD));
    loadSettings();
}

void BrayaSettings::show(GtkWindow* parent) {
    if (dialog && GTK_IS_WINDOW(dialog)) {
        gtk_window_present(GTK_WINDOW(dialog));
        return;
    }

    createDialog(parent);
    updateUIFromSettings();

    // Connect close signal to reset dialog pointer
    g_signal_connect(dialog, "close-request", G_CALLBACK(+[](GtkWindow* window, gpointer data) -> gboolean {
        BrayaSettings* settings = static_cast<BrayaSettings*>(data);
        settings->dialog = nullptr;
        return FALSE; // Allow the window to close
    }), this);

    gtk_window_present(GTK_WINDOW(dialog));
}

void BrayaSettings::createDialog(GtkWindow* parent) {
    dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Braya Settings");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 900, 650);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    
    GtkWidget* mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(dialog), mainBox);
    
    // Header with search
    GtkWidget* headerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(headerBox, 20);
    gtk_widget_set_margin_end(headerBox, 20);
    gtk_widget_set_margin_top(headerBox, 20);
    gtk_widget_set_margin_bottom(headerBox, 15);
    gtk_box_append(GTK_BOX(mainBox), headerBox);
    
    GtkWidget* titleLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(titleLabel), "<span size='x-large' weight='bold'>⚙️ Settings</span>");
    gtk_box_append(GTK_BOX(headerBox), titleLabel);
    
    // Search entry
    GtkWidget* searchEntry = gtk_search_entry_new();
    gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(searchEntry), "Search settings...");
    gtk_widget_set_size_request(searchEntry, 250, -1);
    gtk_box_append(GTK_BOX(headerBox), searchEntry);
    
    gtk_box_append(GTK_BOX(mainBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // Content area: Sidebar + Stack
    GtkWidget* contentBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_vexpand(contentBox, TRUE);
    gtk_box_append(GTK_BOX(mainBox), contentBox);
    
    // Create stack first
    notebook = gtk_stack_new();
    gtk_widget_set_hexpand(notebook, TRUE);
    gtk_widget_set_vexpand(notebook, TRUE);
    gtk_stack_set_transition_type(GTK_STACK(notebook), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(GTK_STACK(notebook), 200);
    
    // Add all pages to stack
    gtk_stack_add_titled(GTK_STACK(notebook), createGeneralTab(), "general", "🏠 General");
    gtk_stack_add_titled(GTK_STACK(notebook), createAppearanceTab(), "appearance", "🎨 Appearance");
    gtk_stack_add_titled(GTK_STACK(notebook), createPrivacyTab(), "privacy", "🔒 Privacy");
    gtk_stack_add_titled(GTK_STACK(notebook), createSecurityTab(), "security", "🛡️ Security");
    gtk_stack_add_titled(GTK_STACK(notebook), createExtensionsTab(), "extensions", "🔌 Extensions");
    gtk_stack_add_titled(GTK_STACK(notebook), createAdvancedTab(), "advanced", "⚡ Advanced");
    
    // Create sidebar that automatically manages the stack
    GtkWidget* stackSidebar = gtk_stack_sidebar_new();
    gtk_stack_sidebar_set_stack(GTK_STACK_SIDEBAR(stackSidebar), GTK_STACK(notebook));
    gtk_widget_add_css_class(stackSidebar, "settings-sidebar");
    gtk_widget_set_size_request(stackSidebar, 200, -1);
    gtk_widget_set_margin_start(stackSidebar, 10);
    gtk_widget_set_margin_end(stackSidebar, 10);
    gtk_widget_set_margin_top(stackSidebar, 10);
    gtk_widget_set_margin_bottom(stackSidebar, 10);
    
    gtk_box_append(GTK_BOX(contentBox), stackSidebar);
    gtk_box_append(GTK_BOX(contentBox), gtk_separator_new(GTK_ORIENTATION_VERTICAL));
    
    gtk_widget_set_margin_start(notebook, 10);
    gtk_widget_set_margin_end(notebook, 10);
    gtk_widget_set_margin_top(notebook, 10);
    gtk_widget_set_margin_bottom(notebook, 10);
    gtk_box_append(GTK_BOX(contentBox), notebook);
    
    gtk_box_append(GTK_BOX(mainBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // Buttons
    GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(buttonBox, 20);
    gtk_widget_set_margin_end(buttonBox, 20);
    gtk_widget_set_margin_top(buttonBox, 15);
    gtk_widget_set_margin_bottom(buttonBox, 15);
    gtk_widget_set_halign(buttonBox, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(mainBox), buttonBox);
    
    GtkWidget* resetBtn = gtk_button_new_with_label("Reset to Defaults");
    gtk_widget_add_css_class(resetBtn, "destructive-action");
    gtk_box_append(GTK_BOX(buttonBox), resetBtn);
    
    GtkWidget* spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(buttonBox), spacer);
    
    GtkWidget* applyBtn = gtk_button_new_with_label("✓ Apply");
    gtk_widget_add_css_class(applyBtn, "suggested-action");
    g_signal_connect(applyBtn, "clicked", G_CALLBACK(onApplyClicked), this);
    gtk_box_append(GTK_BOX(buttonBox), applyBtn);
    
    GtkWidget* closeBtn = gtk_button_new_with_label("Close");
    g_signal_connect(closeBtn, "clicked", G_CALLBACK(onCloseClicked), this);
    gtk_box_append(GTK_BOX(buttonBox), closeBtn);
}

GtkWidget* BrayaSettings::createGeneralTab() {
    GtkWidget* scrolled = gtk_scrolled_window_new();
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(box, 25);
    gtk_widget_set_margin_end(box, 25);
    gtk_widget_set_margin_top(box, 25);
    gtk_widget_set_margin_bottom(box, 25);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), box);
    
    // Startup
    GtkWidget* startupLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(startupLabel), "<span weight='bold' size='13000'>Startup</span>");
    gtk_widget_set_halign(startupLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), startupLabel);
    
    GtkWidget* homeGrid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(homeGrid), 15);
    gtk_grid_set_row_spacing(GTK_GRID(homeGrid), 10);
    gtk_box_append(GTK_BOX(box), homeGrid);
    
    GtkWidget* homeLabel = gtk_label_new("Home Page:");
    gtk_widget_set_halign(homeLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(homeGrid), homeLabel, 0, 0, 1, 1);
    
    homePageEntry = gtk_entry_new();
    gtk_widget_set_hexpand(homePageEntry, TRUE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(homePageEntry), "about:braya or URL");
    gtk_grid_attach(GTK_GRID(homeGrid), homePageEntry, 1, 0, 1, 1);
    
    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // Search
    GtkWidget* searchLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(searchLabel), "<span weight='bold' size='13000'>Search</span>");
    gtk_widget_set_halign(searchLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), searchLabel);
    
    GtkWidget* searchGrid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(searchGrid), 15);
    gtk_box_append(GTK_BOX(box), searchGrid);
    
    GtkWidget* searchEngineLabel = gtk_label_new("Default Search Engine:");
    gtk_widget_set_halign(searchEngineLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(searchGrid), searchEngineLabel, 0, 0, 1, 1);
    
    searchEngineCombo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(searchEngineCombo), "DuckDuckGo");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(searchEngineCombo), "Google");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(searchEngineCombo), "Bing");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(searchEngineCombo), "Brave Search");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(searchEngineCombo), "Ecosia");
    gtk_widget_set_hexpand(searchEngineCombo, TRUE);
    gtk_grid_attach(GTK_GRID(searchGrid), searchEngineCombo, 1, 0, 1, 1);
    
    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // Downloads
    GtkWidget* downloadLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(downloadLabel), "<span weight='bold' size='13000'>Downloads</span>");
    gtk_widget_set_halign(downloadLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), downloadLabel);
    
    GtkWidget* downloadGrid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(downloadGrid), 15);
    gtk_box_append(GTK_BOX(box), downloadGrid);
    
    GtkWidget* pathLabel = gtk_label_new("Download Location:");
    gtk_widget_set_halign(pathLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(downloadGrid), pathLabel, 0, 0, 1, 1);
    
    downloadPathEntry = gtk_entry_new();
    gtk_widget_set_hexpand(downloadPathEntry, TRUE);
    gtk_grid_attach(GTK_GRID(downloadGrid), downloadPathEntry, 1, 0, 1, 1);
    
    return scrolled;
}

GtkWidget* BrayaSettings::createAppearanceTab() {
    GtkWidget* scrolled = gtk_scrolled_window_new();
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(box, 25);
    gtk_widget_set_margin_end(box, 25);
    gtk_widget_set_margin_top(box, 25);
    gtk_widget_set_margin_bottom(box, 25);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), box);
    
    // Theme
    GtkWidget* themeLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(themeLabel), "<span weight='bold' size='13000'>Theme</span>");
    gtk_widget_set_halign(themeLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), themeLabel);
    
    GtkWidget* themeGrid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(themeGrid), 15);
    gtk_grid_set_row_spacing(GTK_GRID(themeGrid), 10);
    gtk_box_append(GTK_BOX(box), themeGrid);
    
    GtkWidget* themeSelectLabel = gtk_label_new("Browser Theme:");
    gtk_widget_set_halign(themeSelectLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(themeGrid), themeSelectLabel, 0, 0, 1, 1);
    
    themeCombo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(themeCombo), "🌙 Dark");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(themeCombo), "☀️ Light");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(themeCombo), "🏭 Industrial");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(themeCombo), "🎨 Custom");
    gtk_widget_set_hexpand(themeCombo, TRUE);
    g_signal_connect(themeCombo, "changed", G_CALLBACK(onThemeChanged), this);
    gtk_grid_attach(GTK_GRID(themeGrid), themeCombo, 1, 0, 1, 1);
    
    // Advanced customization button
    GtkWidget* advancedBtn = gtk_button_new_with_label("🎨 Advanced Customization");
    g_signal_connect_swapped(advancedBtn, "clicked", G_CALLBACK(+[](BrayaSettings* settings) {
        static BrayaCustomization* customization = nullptr;
        if (!customization) {
            customization = new BrayaCustomization();
        }
        customization->show(GTK_WINDOW(settings->dialog));
    }), this);
    gtk_grid_attach(GTK_GRID(themeGrid), advancedBtn, 1, 1, 1, 1);
    
    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // Typography
    GtkWidget* typoLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(typoLabel), "<span weight='bold' size='13000'>Typography</span>");
    gtk_widget_set_halign(typoLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), typoLabel);
    
    GtkWidget* fontGrid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(fontGrid), 15);
    gtk_grid_set_row_spacing(GTK_GRID(fontGrid), 10);
    gtk_box_append(GTK_BOX(box), fontGrid);
    
    GtkWidget* fontLabel = gtk_label_new("Font Family:");
    gtk_widget_set_halign(fontLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(fontGrid), fontLabel, 0, 0, 1, 1);
    
    fontFamilyEntry = gtk_entry_new();
    gtk_widget_set_hexpand(fontFamilyEntry, TRUE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(fontFamilyEntry), "Sans, Monospace, etc.");
    gtk_grid_attach(GTK_GRID(fontGrid), fontFamilyEntry, 1, 0, 1, 1);
    
    GtkWidget* sizeLabel = gtk_label_new("Font Size:");
    gtk_widget_set_halign(sizeLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(fontGrid), sizeLabel, 0, 1, 1, 1);
    
    fontSpinner = gtk_spin_button_new_with_range(8, 32, 1);
    g_signal_connect(fontSpinner, "value-changed", G_CALLBACK(onFontSizeChanged), this);
    gtk_grid_attach(GTK_GRID(fontGrid), fontSpinner, 1, 1, 1, 1);
    
    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // UI Elements
    GtkWidget* uiLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(uiLabel), "<span weight='bold' size='13000'>UI Elements</span>");
    gtk_widget_set_halign(uiLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), uiLabel);
    
    GtkWidget* uiGrid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(uiGrid), 15);
    gtk_box_append(GTK_BOX(box), uiGrid);
    
    GtkWidget* bookmarksLabel = gtk_label_new("Show Bookmarks Bar:");
    gtk_widget_set_halign(bookmarksLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(uiGrid), bookmarksLabel, 0, 0, 1, 1);
    
    bookmarksSwitch = gtk_switch_new();
    g_signal_connect(bookmarksSwitch, "state-set", G_CALLBACK(onBookmarksToggled), this);
    gtk_grid_attach(GTK_GRID(uiGrid), bookmarksSwitch, 1, 0, 1, 1);
    
    return scrolled;
}

GtkWidget* BrayaSettings::createPrivacyTab() {
    GtkWidget* scrolled = gtk_scrolled_window_new();
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(box, 25);
    gtk_widget_set_margin_end(box, 25);
    gtk_widget_set_margin_top(box, 25);
    gtk_widget_set_margin_bottom(box, 25);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), box);
    
    // Tracking
    GtkWidget* trackingLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(trackingLabel), "<span weight='bold' size='13000'>Tracking Protection</span>");
    gtk_widget_set_halign(trackingLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), trackingLabel);
    
    GtkWidget* trackGrid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(trackGrid), 15);
    gtk_grid_set_row_spacing(GTK_GRID(trackGrid), 15);
    gtk_box_append(GTK_BOX(box), trackGrid);
    
    GtkWidget* trackersLabel = gtk_label_new("Block Trackers:");
    gtk_widget_set_halign(trackersLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(trackGrid), trackersLabel, 0, 0, 1, 1);
    
    blockTrackersSwitch = gtk_switch_new();
    gtk_grid_attach(GTK_GRID(trackGrid), blockTrackersSwitch, 1, 0, 1, 1);
    
    GtkWidget* trackersHint = gtk_label_new("Prevents websites from tracking your browsing");
    gtk_widget_add_css_class(trackersHint, "dim-label");
    gtk_widget_set_halign(trackersHint, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(trackGrid), trackersHint, 0, 1, 2, 1);
    
    GtkWidget* adsLabel = gtk_label_new("Block Ads:");
    gtk_widget_set_halign(adsLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(trackGrid), adsLabel, 0, 2, 1, 1);
    
    blockAdsSwitch = gtk_switch_new();
    gtk_grid_attach(GTK_GRID(trackGrid), blockAdsSwitch, 1, 2, 1, 1);
    
    GtkWidget* adsHint = gtk_label_new("Block advertisements and pop-ups");
    gtk_widget_add_css_class(adsHint, "dim-label");
    gtk_widget_set_halign(adsHint, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(trackGrid), adsHint, 0, 3, 2, 1);
    
    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // Do Not Track
    GtkWidget* dntLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(dntLabel), "<span weight='bold' size='13000'>Do Not Track</span>");
    gtk_widget_set_halign(dntLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), dntLabel);
    
    GtkWidget* dntGrid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(dntGrid), 15);
    gtk_box_append(GTK_BOX(box), dntGrid);
    
    GtkWidget* dntSwLabel = gtk_label_new("Send DNT Header:");
    gtk_widget_set_halign(dntSwLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(dntGrid), dntSwLabel, 0, 0, 1, 1);
    
    GtkWidget* dntSwitch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(dntSwitch), TRUE);
    gtk_grid_attach(GTK_GRID(dntGrid), dntSwitch, 1, 0, 1, 1);
    
    return scrolled;
}

GtkWidget* BrayaSettings::createSecurityTab() {
    GtkWidget* scrolled = gtk_scrolled_window_new();
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(box, 25);
    gtk_widget_set_margin_end(box, 25);
    gtk_widget_set_margin_top(box, 25);
    gtk_widget_set_margin_bottom(box, 25);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), box);
    
    // HTTPS
    GtkWidget* httpsLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(httpsLabel), "<span weight='bold' size='13000'>HTTPS</span>");
    gtk_widget_set_halign(httpsLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), httpsLabel);
    
    GtkWidget* httpsGrid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(httpsGrid), 15);
    gtk_grid_set_row_spacing(GTK_GRID(httpsGrid), 10);
    gtk_box_append(GTK_BOX(box), httpsGrid);
    
    GtkWidget* httpsSwLabel = gtk_label_new("HTTPS-Only Mode:");
    gtk_widget_set_halign(httpsSwLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(httpsGrid), httpsSwLabel, 0, 0, 1, 1);
    
    httpsOnlySwitch = gtk_switch_new();
    gtk_grid_attach(GTK_GRID(httpsGrid), httpsOnlySwitch, 1, 0, 1, 1);
    
    GtkWidget* httpsHint = gtk_label_new("Only load sites over encrypted connections");
    gtk_widget_add_css_class(httpsHint, "dim-label");
    gtk_widget_set_halign(httpsHint, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(httpsGrid), httpsHint, 0, 1, 2, 1);
    
    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // Safe Browsing
    GtkWidget* safeLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(safeLabel), "<span weight='bold' size='13000'>Safe Browsing</span>");
    gtk_widget_set_halign(safeLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), safeLabel);
    
    GtkWidget* safeGrid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(safeGrid), 15);
    gtk_box_append(GTK_BOX(box), safeGrid);
    
    GtkWidget* safeSwLabel = gtk_label_new("Warn about dangerous sites:");
    gtk_widget_set_halign(safeSwLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(safeGrid), safeSwLabel, 0, 0, 1, 1);
    
    GtkWidget* safeSwitch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(safeSwitch), TRUE);
    gtk_grid_attach(GTK_GRID(safeGrid), safeSwitch, 1, 0, 1, 1);
    
    return scrolled;
}

GtkWidget* BrayaSettings::createAdvancedTab() {
    GtkWidget* scrolled = gtk_scrolled_window_new();
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(box, 25);
    gtk_widget_set_margin_end(box, 25);
    gtk_widget_set_margin_top(box, 25);
    gtk_widget_set_margin_bottom(box, 25);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), box);
    
    // Web Content
    GtkWidget* contentLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(contentLabel), "<span weight='bold' size='13000'>Web Content</span>");
    gtk_widget_set_halign(contentLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), contentLabel);
    
    GtkWidget* contentGrid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(contentGrid), 15);
    gtk_grid_set_row_spacing(GTK_GRID(contentGrid), 10);
    gtk_box_append(GTK_BOX(box), contentGrid);
    
    GtkWidget* jsLabel = gtk_label_new("Enable JavaScript:");
    gtk_widget_set_halign(jsLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(contentGrid), jsLabel, 0, 0, 1, 1);
    
    jsSwitch = gtk_switch_new();
    gtk_grid_attach(GTK_GRID(contentGrid), jsSwitch, 1, 0, 1, 1);
    
    GtkWidget* webglLabel = gtk_label_new("Enable WebGL:");
    gtk_widget_set_halign(webglLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(contentGrid), webglLabel, 0, 1, 1, 1);
    
    webglSwitch = gtk_switch_new();
    gtk_grid_attach(GTK_GRID(contentGrid), webglSwitch, 1, 1, 1, 1);
    
    GtkWidget* pluginsLabel = gtk_label_new("Enable Plugins:");
    gtk_widget_set_halign(pluginsLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(contentGrid), pluginsLabel, 0, 2, 1, 1);
    
    pluginsSwitch = gtk_switch_new();
    gtk_grid_attach(GTK_GRID(contentGrid), pluginsSwitch, 1, 2, 1, 1);
    
    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // About
    GtkWidget* aboutLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(aboutLabel), "<span weight='bold' size='13000'>About Braya</span>");
    gtk_widget_set_halign(aboutLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), aboutLabel);
    
    GtkWidget* versionLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(versionLabel), 
        "<span>🐕 <b>Braya Browser</b> v1.0.0\n"
        "Built with WebKit2GTK 6.0 & GTK 4\n"
        "Native C++ Performance Browser</span>");
    gtk_widget_set_halign(versionLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), versionLabel);
    
    return scrolled;
}

void BrayaSettings::updateUIFromSettings() {
    if (!dialog) return;
    
    if (homePageEntry) gtk_editable_set_text(GTK_EDITABLE(homePageEntry), homePage.c_str());
    if (downloadPathEntry) gtk_editable_set_text(GTK_EDITABLE(downloadPathEntry), downloadPath.c_str());
    if (fontFamilyEntry) gtk_editable_set_text(GTK_EDITABLE(fontFamilyEntry), fontFamily.c_str());
    if (fontSpinner) gtk_spin_button_set_value(GTK_SPIN_BUTTON(fontSpinner), fontSize);
    if (themeCombo) gtk_combo_box_set_active(GTK_COMBO_BOX(themeCombo), theme);
    if (bookmarksSwitch) gtk_switch_set_active(GTK_SWITCH(bookmarksSwitch), showBookmarks);
    if (blockTrackersSwitch) gtk_switch_set_active(GTK_SWITCH(blockTrackersSwitch), blockTrackers);
    if (blockAdsSwitch) gtk_switch_set_active(GTK_SWITCH(blockAdsSwitch), blockAds);
    if (httpsOnlySwitch) gtk_switch_set_active(GTK_SWITCH(httpsOnlySwitch), httpsOnly);
    if (jsSwitch) gtk_switch_set_active(GTK_SWITCH(jsSwitch), enableJavaScript);
    if (webglSwitch) gtk_switch_set_active(GTK_SWITCH(webglSwitch), enableWebGL);
    if (pluginsSwitch) gtk_switch_set_active(GTK_SWITCH(pluginsSwitch), enablePlugins);
    
    if (searchEngineCombo) {
        if (searchEngine == "DuckDuckGo") gtk_combo_box_set_active(GTK_COMBO_BOX(searchEngineCombo), 0);
        else if (searchEngine == "Google") gtk_combo_box_set_active(GTK_COMBO_BOX(searchEngineCombo), 1);
        else if (searchEngine == "Bing") gtk_combo_box_set_active(GTK_COMBO_BOX(searchEngineCombo), 2);
        else if (searchEngine == "Brave Search") gtk_combo_box_set_active(GTK_COMBO_BOX(searchEngineCombo), 3);
        else gtk_combo_box_set_active(GTK_COMBO_BOX(searchEngineCombo), 4);
    }
}

void BrayaSettings::applySettings() {
    if (homePageEntry) homePage = gtk_editable_get_text(GTK_EDITABLE(homePageEntry));
    if (downloadPathEntry) downloadPath = gtk_editable_get_text(GTK_EDITABLE(downloadPathEntry));
    if (fontFamilyEntry) fontFamily = gtk_editable_get_text(GTK_EDITABLE(fontFamilyEntry));
    if (fontSpinner) fontSize = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(fontSpinner));
    if (bookmarksSwitch) showBookmarks = gtk_switch_get_active(GTK_SWITCH(bookmarksSwitch));
    if (blockTrackersSwitch) blockTrackers = gtk_switch_get_active(GTK_SWITCH(blockTrackersSwitch));
    if (blockAdsSwitch) blockAds = gtk_switch_get_active(GTK_SWITCH(blockAdsSwitch));
    if (httpsOnlySwitch) httpsOnly = gtk_switch_get_active(GTK_SWITCH(httpsOnlySwitch));
    if (jsSwitch) enableJavaScript = gtk_switch_get_active(GTK_SWITCH(jsSwitch));
    if (webglSwitch) enableWebGL = gtk_switch_get_active(GTK_SWITCH(webglSwitch));
    if (pluginsSwitch) enablePlugins = gtk_switch_get_active(GTK_SWITCH(pluginsSwitch));
    
    if (searchEngineCombo) {
        int active = gtk_combo_box_get_active(GTK_COMBO_BOX(searchEngineCombo));
        const char* engines[] = {"DuckDuckGo", "Google", "Bing", "Brave Search", "Ecosia"};
        if (active >= 0 && active < 5) searchEngine = engines[active];
    }
    
    saveSettings();
    applyTheme();
    std::cout << "✓ Settings applied!" << std::endl;
}

void BrayaSettings::applyTheme() {
    std::cout << "Applying theme: " << theme << std::endl;
}

void BrayaSettings::saveSettings() {
    std::string configDir = std::string(g_get_user_config_dir()) + "/braya";
    g_mkdir_with_parents(configDir.c_str(), 0755);
    std::string settingsPath = configDir + "/settings.json";
    
    std::ofstream file(settingsPath);
    if (!file.is_open()) return;
    
    file << "{\n";
    file << "  \"theme\": " << theme << ",\n";
    file << "  \"fontSize\": " << fontSize << ",\n";
    file << "  \"fontFamily\": \"" << fontFamily << "\",\n";
    file << "  \"showBookmarks\": " << (showBookmarks ? "true" : "false") << ",\n";
    file << "  \"blockTrackers\": " << (blockTrackers ? "true" : "false") << ",\n";
    file << "  \"blockAds\": " << (blockAds ? "true" : "false") << ",\n";
    file << "  \"httpsOnly\": " << (httpsOnly ? "true" : "false") << ",\n";
    file << "  \"enableJavaScript\": " << (enableJavaScript ? "true" : "false") << ",\n";
    file << "  \"enableWebGL\": " << (enableWebGL ? "true" : "false") << ",\n";
    file << "  \"enablePlugins\": " << (enablePlugins ? "true" : "false") << ",\n";
    file << "  \"downloadPath\": \"" << downloadPath << "\",\n";
    file << "  \"homePage\": \"" << homePage << "\",\n";
    file << "  \"searchEngine\": \"" << searchEngine << "\"\n";
    file << "}\n";
    file.close();
}

void BrayaSettings::loadSettings() {
    std::string configDir = std::string(g_get_user_config_dir()) + "/braya";
    std::string settingsPath = configDir + "/settings.json";
    
    std::ifstream file(settingsPath);
    if (!file.is_open()) return;
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("\"theme\":") != std::string::npos) {
            size_t pos = line.find(":") + 1;
            theme = static_cast<Theme>(std::stoi(line.substr(pos)));
        }
        else if (line.find("\"fontSize\":") != std::string::npos) {
            size_t pos = line.find(":") + 1;
            fontSize = std::stoi(line.substr(pos));
        }
        else if (line.find("\"fontFamily\":") != std::string::npos) {
            size_t start = line.find("\"", line.find(":")) + 1;
            size_t end = line.rfind("\"");
            fontFamily = line.substr(start, end - start);
        }
        else if (line.find("\"homePage\":") != std::string::npos) {
            size_t start = line.find("\"", line.find(":")) + 1;
            size_t end = line.rfind("\"");
            homePage = line.substr(start, end - start);
        }
        else if (line.find("\"searchEngine\":") != std::string::npos) {
            size_t start = line.find("\"", line.find(":")) + 1;
            size_t end = line.rfind("\"");
            searchEngine = line.substr(start, end - start);
        }
        else if (line.find("\"showBookmarks\":") != std::string::npos) {
            showBookmarks = line.find("true") != std::string::npos;
        }
        else if (line.find("\"blockTrackers\":") != std::string::npos) {
            blockTrackers = line.find("true") != std::string::npos;
        }
        else if (line.find("\"blockAds\":") != std::string::npos) {
            blockAds = line.find("true") != std::string::npos;
        }
        else if (line.find("\"httpsOnly\":") != std::string::npos) {
            httpsOnly = line.find("true") != std::string::npos;
        }
        else if (line.find("\"enableJavaScript\":") != std::string::npos) {
            enableJavaScript = line.find("true") != std::string::npos;
        }
        else if (line.find("\"enableWebGL\":") != std::string::npos) {
            enableWebGL = line.find("true") != std::string::npos;
        }
        else if (line.find("\"enablePlugins\":") != std::string::npos) {
            enablePlugins = line.find("true") != std::string::npos;
        }
    }
    file.close();
}

// Setters
void BrayaSettings::setTheme(Theme t) { theme = t; }
void BrayaSettings::setColors(const ColorScheme& c) { colors = c; }
void BrayaSettings::setFontSize(int size) { fontSize = size; }
void BrayaSettings::setFontFamily(const std::string& family) { fontFamily = family; }
void BrayaSettings::setShowBookmarks(bool show) { showBookmarks = show; }
void BrayaSettings::setBlockTrackers(bool block) { blockTrackers = block; }
void BrayaSettings::setBlockAds(bool block) { blockAds = block; }
void BrayaSettings::setHttpsOnly(bool https) { httpsOnly = https; }
void BrayaSettings::setEnableJavaScript(bool enable) { enableJavaScript = enable; }
void BrayaSettings::setEnableWebGL(bool enable) { enableWebGL = enable; }
void BrayaSettings::setEnablePlugins(bool enable) { enablePlugins = enable; }
void BrayaSettings::setDownloadPath(const std::string& path) { downloadPath = path; }
void BrayaSettings::setHomePage(const std::string& page) { homePage = page; }
void BrayaSettings::setSearchEngine(const std::string& engine) { searchEngine = engine; }

// Callbacks
void BrayaSettings::onThemeChanged(GtkComboBox* combo, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    int themeId = gtk_combo_box_get_active(combo);
    settings->setTheme(static_cast<Theme>(themeId));
    
    // Call the callback immediately to apply theme
    if (settings->themeCallback) {
        settings->themeCallback(themeId);
    }
}

void BrayaSettings::onFontSizeChanged(GtkSpinButton* button, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    settings->setFontSize(gtk_spin_button_get_value_as_int(button));
}

void BrayaSettings::onBookmarksToggled(GtkSwitch* widget, gboolean state, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    settings->setShowBookmarks(state);
}

void BrayaSettings::onApplyClicked(GtkButton* button, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    settings->applySettings();
}

void BrayaSettings::onCloseClicked(GtkButton* button, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    settings->applySettings();
    gtk_window_destroy(GTK_WINDOW(settings->dialog));
    settings->dialog = nullptr;
}

void BrayaSettings::onColorButtonClicked(GtkButton* button, gpointer data) {
    std::cout << "Color picker coming soon!" << std::endl;
}

// =============================================================================
// EXTENSIONS TAB
// =============================================================================

// Helper struct for extension row data
struct ExtensionRowData {
    BrayaSettings* settings;
    std::string extensionId;
};

GtkWidget* BrayaSettings::createExtensionsTab() {
    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    GtkWidget* mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), mainBox);

    // Header area
    GtkWidget* headerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_start(headerBox, 20);
    gtk_widget_set_margin_end(headerBox, 20);
    gtk_widget_set_margin_top(headerBox, 20);
    gtk_widget_set_margin_bottom(headerBox, 10);
    gtk_box_append(GTK_BOX(mainBox), headerBox);

    GtkWidget* headerLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(headerLabel), "<span size='large' weight='bold'>Browser Extensions</span>");
    gtk_widget_set_halign(headerLabel, GTK_ALIGN_START);
    gtk_widget_set_hexpand(headerLabel, TRUE);
    gtk_box_append(GTK_BOX(headerBox), headerLabel);

    // Install buttons box
    GtkWidget* installBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(headerBox), installBox);

    // Quick install box - URL entry right in the header
    GtkWidget* quickInstallBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_hexpand(quickInstallBox, TRUE);
    gtk_box_append(GTK_BOX(installBox), quickInstallBox);

    GtkWidget* urlEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(urlEntry), "Paste extension URL from addons.mozilla.org or chrome.google.com/webstore");
    gtk_widget_set_hexpand(urlEntry, TRUE);
    gtk_box_append(GTK_BOX(quickInstallBox), urlEntry);

    GtkWidget* installButton = gtk_button_new_with_label("Install");
    gtk_widget_add_css_class(installButton, "suggested-action");
    gtk_widget_set_tooltip_text(installButton, "Install extension from URL");
    g_object_set_data(G_OBJECT(installButton), "settings", this);
    g_object_set_data(G_OBJECT(installButton), "url-entry", urlEntry);
    g_signal_connect(installButton, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        BrayaSettings* s = static_cast<BrayaSettings*>(g_object_get_data(G_OBJECT(btn), "settings"));
        GtkEntry* entry = GTK_ENTRY(g_object_get_data(G_OBJECT(btn), "url-entry"));

        const char* url = gtk_editable_get_text(GTK_EDITABLE(entry));
        if (!url || strlen(url) == 0) {
            std::cerr << "No URL provided" << std::endl;
            return;
        }

        std::cout << "📦 Installing extension from URL: " << url << std::endl;

        // Disable button during installation
        gtk_widget_set_sensitive(GTK_WIDGET(btn), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(entry), FALSE);
        gtk_button_set_label(btn, "Installing...");

        // Create installer and install
        ExtensionInstaller installer(s->m_extensionManager);
        installer.installFromUrl(url, [s, btn, entry](bool success, const std::string& message) {
            // Re-enable controls
            gtk_button_set_label(GTK_BUTTON(btn), "Install");
            gtk_widget_set_sensitive(GTK_WIDGET(btn), TRUE);
            gtk_widget_set_sensitive(GTK_WIDGET(entry), TRUE);

            if (success) {
                std::cout << "✓ " << message << std::endl;

                // Clear the URL entry
                gtk_editable_set_text(GTK_EDITABLE(entry), "");

                // Save extension states
                s->saveExtensionStates();

                // Refresh the extensions list
                s->refreshExtensionsList();

                // Notify window to update extension buttons
                if (s->m_extensionChangeCallback) {
                    s->m_extensionChangeCallback();
                }

                // Show success notification
                std::cout << "✅ Extension installed and ready to use!" << std::endl;
            } else {
                std::cerr << "✗ " << message << std::endl;
            }
        });
    }), nullptr);
    gtk_box_append(GTK_BOX(quickInstallBox), installButton);

    // Add note about where to find extensions
    GtkWidget* noteLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(noteLabel),
        "<span size='small' style='italic'>💡 Just browse https://addons.mozilla.org in the browser and click install!</span>");
    gtk_label_set_wrap(GTK_LABEL(noteLabel), TRUE);
    gtk_widget_add_css_class(noteLabel, "dim-label");
    gtk_widget_set_halign(noteLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(installBox), noteLabel);

    // Load Unpacked button (for developers)
    GtkWidget* loadButton = gtk_button_new_with_label("Load Unpacked");
    gtk_widget_set_tooltip_text(loadButton, "Load an unpacked extension folder (for development)");
    g_signal_connect(loadButton, "clicked", G_CALLBACK(onLoadUnpackedClicked), this);
    gtk_box_append(GTK_BOX(installBox), loadButton);

    // Subtitle
    GtkWidget* subtitle = gtk_label_new("Install, enable, and manage your browser extensions");
    gtk_widget_set_halign(subtitle, GTK_ALIGN_START);
    gtk_widget_add_css_class(subtitle, "dim-label");
    gtk_widget_set_margin_start(subtitle, 20);
    gtk_widget_set_margin_end(subtitle, 20);
    gtk_widget_set_margin_bottom(subtitle, 15);
    gtk_box_append(GTK_BOX(mainBox), subtitle);

    // Extensions list
    extensionsList = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(extensionsList), GTK_SELECTION_NONE);
    gtk_widget_add_css_class(extensionsList, "boxed-list");
    gtk_widget_set_margin_start(extensionsList, 20);
    gtk_widget_set_margin_end(extensionsList, 20);
    gtk_widget_set_margin_bottom(extensionsList, 20);
    gtk_box_append(GTK_BOX(mainBox), extensionsList);

    // Populate list
    refreshExtensionsList();

    return scrolled;
}

void BrayaSettings::refreshExtensionsList() {
    if (!extensionsList || !m_extensionManager) {
        return;
    }

    // Clear existing rows
    GtkWidget* child = gtk_widget_get_first_child(extensionsList);
    while (child) {
        GtkWidget* next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(GTK_LIST_BOX(extensionsList), child);
        child = next;
    }

    // Get all extensions
    auto extensions = m_extensionManager->getWebExtensions();

    if (extensions.empty()) {
        // Show empty state
        GtkWidget* emptyBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
        gtk_widget_set_margin_top(emptyBox, 60);
        gtk_widget_set_margin_bottom(emptyBox, 60);

        GtkWidget* emptyLabel = gtk_label_new("No Extensions Installed");
        gtk_widget_add_css_class(emptyLabel, "title-2");
        gtk_box_append(GTK_BOX(emptyBox), emptyLabel);

        GtkWidget* emptyDesc = gtk_label_new("Click '🔍 Browse Extensions' to get started");
        gtk_widget_add_css_class(emptyDesc, "dim-label");
        gtk_box_append(GTK_BOX(emptyBox), emptyDesc);

        gtk_list_box_append(GTK_LIST_BOX(extensionsList), emptyBox);
    } else {
        // Add row for each extension
        for (auto* extension : extensions) {
            GtkWidget* row = createExtensionRow(extension);
            gtk_list_box_append(GTK_LIST_BOX(extensionsList), row);
        }
    }

    std::cout << "✓ Extension list refreshed (" << extensions.size() << " extensions)" << std::endl;
}

GtkWidget* BrayaSettings::createExtensionRow(void* ext) {
    auto* extension = static_cast<BrayaWebExtension*>(ext);

    // Main row box
    GtkWidget* rowBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_start(rowBox, 12);
    gtk_widget_set_margin_end(rowBox, 12);
    gtk_widget_set_margin_top(rowBox, 12);
    gtk_widget_set_margin_bottom(rowBox, 12);

    // Left side: Extension info
    GtkWidget* infoBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_hexpand(infoBox, TRUE);
    gtk_box_append(GTK_BOX(rowBox), infoBox);

    // Extension name
    GtkWidget* nameLabel = gtk_label_new(extension->getName().c_str());
    gtk_widget_set_halign(nameLabel, GTK_ALIGN_START);
    gtk_widget_add_css_class(nameLabel, "title-4");
    gtk_box_append(GTK_BOX(infoBox), nameLabel);

    // Extension version and ID
    std::string details = "Version " + extension->getVersion() + " • ID: " + extension->getId();
    GtkWidget* detailsLabel = gtk_label_new(details.c_str());
    gtk_widget_set_halign(detailsLabel, GTK_ALIGN_START);
    gtk_widget_add_css_class(detailsLabel, "dim-label");
    gtk_label_set_selectable(GTK_LABEL(detailsLabel), TRUE);
    gtk_box_append(GTK_BOX(infoBox), detailsLabel);

    // Extension path
    GtkWidget* pathLabel = gtk_label_new(extension->getPath().c_str());
    gtk_widget_set_halign(pathLabel, GTK_ALIGN_START);
    gtk_widget_add_css_class(pathLabel, "caption");
    gtk_widget_add_css_class(pathLabel, "dim-label");
    gtk_label_set_selectable(GTK_LABEL(pathLabel), TRUE);
    gtk_label_set_ellipsize(GTK_LABEL(pathLabel), PANGO_ELLIPSIZE_MIDDLE);
    gtk_label_set_max_width_chars(GTK_LABEL(pathLabel), 50);
    gtk_box_append(GTK_BOX(infoBox), pathLabel);

    // Permissions info
    auto permissions = extension->getPermissions();
    if (!permissions.empty()) {
        std::string permText = "Permissions: ";
        for (size_t i = 0; i < permissions.size() && i < 3; i++) {
            if (i > 0) permText += ", ";
            permText += permissions[i];
        }
        if (permissions.size() > 3) {
            permText += " (+" + std::to_string(permissions.size() - 3) + " more)";
        }

        GtkWidget* permLabel = gtk_label_new(permText.c_str());
        gtk_widget_set_halign(permLabel, GTK_ALIGN_START);
        gtk_widget_add_css_class(permLabel, "caption");
        gtk_box_append(GTK_BOX(infoBox), permLabel);
    }

    // Right side: Controls
    GtkWidget* controlsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_valign(controlsBox, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(rowBox), controlsBox);

    // Enable/Disable toggle
    GtkWidget* toggle = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(toggle), extension->isEnabled());
    gtk_widget_set_valign(toggle, GTK_ALIGN_CENTER);

    auto* toggleData = new ExtensionRowData{this, extension->getId()};
    g_signal_connect_data(toggle, "state-set", G_CALLBACK(onToggleExtension),
                         toggleData, [](gpointer data, GClosure*) { delete (ExtensionRowData*)data; }, (GConnectFlags)0);

    gtk_box_append(GTK_BOX(controlsBox), toggle);

    // Remove button
    GtkWidget* removeButton = gtk_button_new_with_label("Remove");
    gtk_widget_add_css_class(removeButton, "destructive-action");
    gtk_widget_set_valign(removeButton, GTK_ALIGN_CENTER);

    auto* removeData = new ExtensionRowData{this, extension->getId()};
    g_signal_connect_data(removeButton, "clicked", G_CALLBACK(onRemoveExtension),
                         removeData, [](gpointer data, GClosure*) { delete (ExtensionRowData*)data; }, (GConnectFlags)0);

    gtk_box_append(GTK_BOX(controlsBox), removeButton);

    return rowBox;
}

void BrayaSettings::onLoadUnpackedClicked(GtkButton* button, gpointer user_data) {
    auto* settings = static_cast<BrayaSettings*>(user_data);
    settings->loadUnpackedExtension();
}

void BrayaSettings::loadUnpackedExtension() {
    // Create file chooser dialog for selecting directory
    GtkWidget* fileDialog = gtk_file_chooser_dialog_new(
        "Select Extension Directory",
        GTK_WINDOW(dialog),
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Load", GTK_RESPONSE_ACCEPT,
        nullptr
    );

    // Set default folder to home directory
    const char* homeDir = g_get_home_dir();
    GFile* homeFile = g_file_new_for_path(homeDir);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fileDialog), homeFile, nullptr);
    g_object_unref(homeFile);

    // Show dialog
    g_signal_connect(fileDialog, "response", G_CALLBACK(+[](GtkDialog* dlg, int response, gpointer user_data) {
        auto* settings = static_cast<BrayaSettings*>(user_data);

        if (response == GTK_RESPONSE_ACCEPT) {
            GFile* folder = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dlg));
            char* path = g_file_get_path(folder);

            std::cout << "📦 Loading extension from: " << path << std::endl;

            // Load the extension
            if (settings->m_extensionManager->loadWebExtension(path)) {
                std::cout << "✓ Extension loaded successfully!" << std::endl;
                settings->saveExtensionStates();
                settings->refreshExtensionsList();
            } else {
                std::cerr << "❌ Failed to load extension" << std::endl;

                // Show error dialog
                GtkWidget* errorDialog = gtk_message_dialog_new(
                    GTK_WINDOW(settings->dialog),
                    GTK_DIALOG_MODAL,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_OK,
                    "Failed to Load Extension"
                );
                gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(errorDialog),
                    "Could not load extension from:\n%s\n\nMake sure the directory contains a valid manifest.json file.", path);
                gtk_window_present(GTK_WINDOW(errorDialog));
                g_signal_connect(errorDialog, "response", G_CALLBACK(gtk_window_destroy), nullptr);
            }

            g_free(path);
            g_object_unref(folder);
        }

        gtk_window_destroy(GTK_WINDOW(dlg));
    }), this);

    gtk_window_present(GTK_WINDOW(fileDialog));
}

void BrayaSettings::onToggleExtension(GtkSwitch* toggle, gboolean state, gpointer user_data) {
    auto* data = static_cast<ExtensionRowData*>(user_data);
    auto* extension = data->settings->m_extensionManager->getWebExtension(data->extensionId);

    if (extension) {
        extension->setEnabled(state);
        std::cout << (state ? "✓ Enabled" : "✗ Disabled") << " extension: " << extension->getName() << std::endl;
        data->settings->saveExtensionStates();

        // Update the extension buttons in the toolbar via callback
        if (data->settings->m_extensionChangeCallback) {
            data->settings->m_extensionChangeCallback();
        }
    }
}

void BrayaSettings::onRemoveExtension(GtkButton* button, gpointer user_data) {
    auto* data = static_cast<ExtensionRowData*>(user_data);
    data->settings->removeExtension(data->extensionId);
}

void BrayaSettings::removeExtension(const std::string& extensionId) {
    auto* extension = m_extensionManager->getWebExtension(extensionId);
    if (!extension) {
        return;
    }

    std::string extensionName = extension->getName();

    // Show confirmation dialog
    GtkWidget* confirmDialog = gtk_message_dialog_new(
        GTK_WINDOW(dialog),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_NONE,
        "Remove Extension?"
    );
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(confirmDialog),
        "Are you sure you want to remove '%s'?\n\nThe extension files will not be deleted from disk.",
        extensionName.c_str());

    gtk_dialog_add_buttons(GTK_DIALOG(confirmDialog),
        "Cancel", GTK_RESPONSE_CANCEL,
        "Remove", GTK_RESPONSE_ACCEPT,
        nullptr);

    // Make Remove button destructive
    GtkWidget* removeBtn = gtk_dialog_get_widget_for_response(GTK_DIALOG(confirmDialog), GTK_RESPONSE_ACCEPT);
    gtk_widget_add_css_class(removeBtn, "destructive-action");

    auto* data = new std::pair<BrayaSettings*, std::string>(this, extensionId);
    auto callback = +[](GtkDialog* dlg, int response, gpointer user_data) {
        auto* data = static_cast<std::pair<BrayaSettings*, std::string>*>(user_data);

        if (response == GTK_RESPONSE_ACCEPT) {
            std::cout << "🗑️  Removing extension: " << data->second << std::endl;
            data->first->m_extensionManager->removeWebExtension(data->second);
            data->first->saveExtensionStates();
            data->first->refreshExtensionsList();

            // Update the extension buttons in the toolbar via callback
            if (data->first->m_extensionChangeCallback) {
                data->first->m_extensionChangeCallback();
            }
        }

        delete data;
        gtk_window_destroy(GTK_WINDOW(dlg));
    };
    g_signal_connect_data(confirmDialog, "response", G_CALLBACK(callback), data, nullptr, (GConnectFlags)0);

    gtk_window_present(GTK_WINDOW(confirmDialog));
}

void BrayaSettings::saveExtensionStates() {
    if (!m_extensionManager) return;

    const char* configDir = g_get_user_config_dir();
    std::string configPath = std::string(configDir) + "/braya-browser";
    std::string extensionsFile = configPath + "/extensions.json";

    // Create JSON array of installed extensions
    JsonBuilder* builder = json_builder_new();
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "extensions");
    json_builder_begin_array(builder);

    auto extensions = m_extensionManager->getWebExtensions();
    for (auto* extension : extensions) {
        json_builder_begin_object(builder);
        json_builder_set_member_name(builder, "id");
        json_builder_add_string_value(builder, extension->getId().c_str());
        json_builder_set_member_name(builder, "path");
        json_builder_add_string_value(builder, extension->getPath().c_str());
        json_builder_set_member_name(builder, "enabled");
        json_builder_add_boolean_value(builder, extension->isEnabled());
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
    std::ofstream file(extensionsFile);
    if (file.is_open()) {
        file << jsonData;
        file.close();
        std::cout << "✓ Saved extension states to: " << extensionsFile << std::endl;
    } else {
        std::cerr << "ERROR: Could not write to: " << extensionsFile << std::endl;
    }

    g_free(jsonData);
    g_object_unref(generator);
    json_node_free(root);
    g_object_unref(builder);
}

void BrayaSettings::loadExtensionStates() {
    if (!m_extensionManager) return;

    const char* configDir = g_get_user_config_dir();
    std::string extensionsFile = std::string(configDir) + "/braya-browser/extensions.json";

    // Check if file exists
    if (!g_file_test(extensionsFile.c_str(), G_FILE_TEST_EXISTS)) {
        std::cout << "ℹ️  No saved extensions file found" << std::endl;
        return;
    }

    // Read file
    std::ifstream file(extensionsFile);
    if (!file.is_open()) {
        std::cerr << "ERROR: Could not open: " << extensionsFile << std::endl;
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
        std::cerr << "ERROR: Failed to parse extensions.json: " << error->message << std::endl;
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
    if (!json_object_has_member(rootObj, "extensions")) {
        g_object_unref(parser);
        return;
    }

    JsonArray* extensionsArray = json_object_get_array_member(rootObj, "extensions");
    guint numExtensions = json_array_get_length(extensionsArray);

    std::cout << "📦 Loading " << numExtensions << " saved extensions..." << std::endl;

    for (guint i = 0; i < numExtensions; i++) {
        JsonObject* extObj = json_array_get_object_element(extensionsArray, i);

        if (json_object_has_member(extObj, "path")) {
            const char* path = json_object_get_string_member(extObj, "path");
            bool enabled = json_object_has_member(extObj, "enabled") ?
                          json_object_get_boolean_member(extObj, "enabled") : true;

            if (m_extensionManager->loadWebExtension(path)) {
                // Set enabled state
                const char* id = json_object_get_string_member(extObj, "id");
                auto* extension = m_extensionManager->getWebExtension(id);
                if (extension) {
                    extension->setEnabled(enabled);
                }
            }
        }
    }

    g_object_unref(parser);
    std::cout << "✓ Extensions loaded from config" << std::endl;
}
