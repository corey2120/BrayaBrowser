#include "BrayaSettings.h"
#include "BrayaCustomization.h"
#include <iostream>
#include <fstream>

BrayaSettings::BrayaSettings()
    : theme(DARK), fontSize(13), fontFamily("Sans"), showBookmarks(true),
      blockTrackers(true), blockAds(false), httpsOnly(false),
      enableJavaScript(true), enableWebGL(true), enablePlugins(false),
      downloadPath(""), homePage("about:braya"), searchEngine("DuckDuckGo"),
      dialog(nullptr), notebook(nullptr) {
    
    downloadPath = std::string(g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD));
    loadSettings();
}

void BrayaSettings::show(GtkWindow* parent) {
    if (dialog) {
        gtk_window_present(GTK_WINDOW(dialog));
        return;
    }
    
    createDialog(parent);
    updateUIFromSettings();
    gtk_window_present(GTK_WINDOW(dialog));
}

void BrayaSettings::createDialog(GtkWindow* parent) {
    dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "🐕 Braya Browser Settings");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 750, 600);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    
    GtkWidget* mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(dialog), mainBox);
    
    // Header
    GtkWidget* headerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(headerBox, 20);
    gtk_widget_set_margin_end(headerBox, 20);
    gtk_widget_set_margin_top(headerBox, 20);
    gtk_widget_set_margin_bottom(headerBox, 15);
    gtk_box_append(GTK_BOX(mainBox), headerBox);
    
    GtkWidget* titleLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(titleLabel), "<span size='xx-large' weight='bold'>⚙️ Settings</span>");
    gtk_box_append(GTK_BOX(headerBox), titleLabel);
    
    gtk_box_append(GTK_BOX(mainBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // Notebook
    notebook = gtk_notebook_new();
    gtk_widget_set_vexpand(notebook, TRUE);
    gtk_widget_set_margin_start(notebook, 10);
    gtk_widget_set_margin_end(notebook, 10);
    gtk_widget_set_margin_top(notebook, 10);
    gtk_widget_set_margin_bottom(notebook, 10);
    gtk_box_append(GTK_BOX(mainBox), notebook);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), createGeneralTab(), gtk_label_new("🏠 General"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), createAppearanceTab(), gtk_label_new("🎨 Appearance"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), createPrivacyTab(), gtk_label_new("🔒 Privacy"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), createSecurityTab(), gtk_label_new("🛡️ Security"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), createAdvancedTab(), gtk_label_new("⚡ Advanced"));
    
    gtk_box_append(GTK_BOX(mainBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // Buttons
    GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(buttonBox, 20);
    gtk_widget_set_margin_end(buttonBox, 20);
    gtk_widget_set_margin_top(buttonBox, 15);
    gtk_widget_set_margin_bottom(buttonBox, 15);
    gtk_widget_set_halign(buttonBox, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(mainBox), buttonBox);
    
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
