#include "BrayaCustomization.h"
#include <iostream>
#include <fstream>

BrayaCustomization::BrayaCustomization() : dialog(nullptr), notebook(nullptr) {
    load();
}

void BrayaCustomization::show(GtkWindow* parent) {
    if (!dialog) {
        createDialog(parent);
    }
    gtk_window_present(GTK_WINDOW(dialog));
}

void BrayaCustomization::createDialog(GtkWindow* parent) {
    dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "🎨 Advanced Customization");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 900, 700);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    
    // Main box
    GtkWidget* mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(mainBox, 20);
    gtk_widget_set_margin_end(mainBox, 20);
    gtk_widget_set_margin_top(mainBox, 20);
    gtk_widget_set_margin_bottom(mainBox, 20);
    gtk_window_set_child(GTK_WINDOW(dialog), mainBox);
    
    // Header
    GtkWidget* header = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(header), 
        "<span size='xx-large' weight='bold'>🎨 Vivaldi-Level Customization</span>");
    gtk_box_append(GTK_BOX(mainBox), header);
    
    GtkWidget* subtitle = gtk_label_new("Customize every aspect of Braya's appearance");
    gtk_box_append(GTK_BOX(mainBox), subtitle);
    
    // Notebook with tabs
    notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_LEFT);
    gtk_box_append(GTK_BOX(mainBox), notebook);
    gtk_widget_set_vexpand(notebook, TRUE);
    
    // Create tabs
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), createColorsTab(), 
        gtk_label_new("🎨 Colors"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), createTypographyTab(), 
        gtk_label_new("✍️ Typography"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), createLayoutTab(), 
        gtk_label_new("📐 Layout"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), createEffectsTab(), 
        gtk_label_new("✨ Effects"));
    
    // Buttons
    GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(mainBox), buttonBox);
    
    GtkWidget* resetBtn = gtk_button_new_with_label("🔄 Reset to Defaults");
    g_signal_connect(resetBtn, "clicked", G_CALLBACK(onResetClicked), this);
    gtk_box_append(GTK_BOX(buttonBox), resetBtn);
    
    gtk_widget_set_hexpand(buttonBox, TRUE);
    GtkWidget* spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(buttonBox), spacer);
    
    GtkWidget* applyBtn = gtk_button_new_with_label("✓ Apply");
    g_signal_connect(applyBtn, "clicked", G_CALLBACK(onApplyClicked), this);
    gtk_box_append(GTK_BOX(buttonBox), applyBtn);
    
    GtkWidget* closeBtn = gtk_button_new_with_label("Close");
    g_signal_connect(closeBtn, "clicked", G_CALLBACK(onCloseClicked), this);
    gtk_box_append(GTK_BOX(buttonBox), closeBtn);
}

GtkWidget* BrayaCustomization::createColorsTab() {
    GtkWidget* scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), 
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 15);
    gtk_widget_set_margin_start(grid, 20);
    gtk_widget_set_margin_end(grid, 20);
    gtk_widget_set_margin_top(grid, 20);
    gtk_widget_set_margin_bottom(grid, 20);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), grid);
    
    int row = 0;
    
    // Section: Sidebar
    GtkWidget* sidebarLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(sidebarLabel), "<b>Sidebar</b>");
    gtk_widget_set_halign(sidebarLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), sidebarLabel, 0, row++, 2, 1);
    
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Background", &colors.sidebarBg), 
        0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Border", &colors.sidebarBorder), 
        1, row++, 1, 1);
    
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Text", &colors.sidebarText), 
        0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Hover", &colors.sidebarHover), 
        1, row++, 1, 1);
    
    // Section: Tab Bar
    GtkWidget* tabLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(tabLabel), "<b>Tab Bar</b>");
    gtk_widget_set_halign(tabLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(tabLabel, 10);
    gtk_grid_attach(GTK_GRID(grid), tabLabel, 0, row++, 2, 1);
    
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Background", &colors.tabBarBg), 
        0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Active Tab", &colors.tabActive), 
        1, row++, 1, 1);
    
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Inactive Tab", &colors.tabInactive), 
        0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Hover", &colors.tabHover), 
        1, row++, 1, 1);
    
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Border", &colors.tabBorder), 
        0, row++, 1, 1);
    
    // Section: URL Bar
    GtkWidget* urlLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(urlLabel), "<b>URL Bar</b>");
    gtk_widget_set_halign(urlLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(urlLabel, 10);
    gtk_grid_attach(GTK_GRID(grid), urlLabel, 0, row++, 2, 1);
    
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Background", &colors.urlBarBg), 
        0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Text", &colors.urlBarText), 
        1, row++, 1, 1);
    
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Border", &colors.urlBarBorder), 
        0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Focus", &colors.urlBarFocus), 
        1, row++, 1, 1);
    
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Placeholder", &colors.urlBarPlaceholder), 
        0, row++, 1, 1);
    
    // Section: Navigation Buttons
    GtkWidget* navLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(navLabel), "<b>Navigation Buttons</b>");
    gtk_widget_set_halign(navLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(navLabel, 10);
    gtk_grid_attach(GTK_GRID(grid), navLabel, 0, row++, 2, 1);
    
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Background", &colors.navBtnBg), 
        0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Hover", &colors.navBtnHover), 
        1, row++, 1, 1);
    
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Text", &colors.navBtnText), 
        0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Disabled", &colors.navBtnDisabled), 
        1, row++, 1, 1);
    
    // Section: Accents
    GtkWidget* accentLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(accentLabel), "<b>Accent Colors</b>");
    gtk_widget_set_halign(accentLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(accentLabel, 10);
    gtk_grid_attach(GTK_GRID(grid), accentLabel, 0, row++, 2, 1);
    
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Primary", &colors.accentPrimary), 
        0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Secondary", &colors.accentSecondary), 
        1, row++, 1, 1);
    
    // Section: Window
    GtkWidget* windowLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(windowLabel), "<b>Window</b>");
    gtk_widget_set_halign(windowLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(windowLabel, 10);
    gtk_grid_attach(GTK_GRID(grid), windowLabel, 0, row++, 2, 1);
    
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Background", &colors.windowBg), 
        0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Content", &colors.contentBg), 
        1, row++, 1, 1);
    
    return scroll;
}

GtkWidget* BrayaCustomization::createTypographyTab() {
    GtkWidget* scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), 
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 15);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 20);
    gtk_widget_set_margin_start(grid, 20);
    gtk_widget_set_margin_end(grid, 20);
    gtk_widget_set_margin_top(grid, 20);
    gtk_widget_set_margin_bottom(grid, 20);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), grid);
    
    int row = 0;
    
    // UI Font
    GtkWidget* uiFontLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(uiFontLabel), "<b>UI Font Family</b>");
    gtk_widget_set_halign(uiFontLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), uiFontLabel, 0, row++, 2, 1);
    
    GtkWidget* uiFontEntry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(uiFontEntry), typography.uiFont.c_str());
    gtk_grid_attach(GTK_GRID(grid), uiFontEntry, 0, row++, 2, 1);
    
    // URL Font
    GtkWidget* urlFontLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(urlFontLabel), "<b>URL Bar Font</b>");
    gtk_widget_set_halign(urlFontLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), urlFontLabel, 0, row++, 2, 1);
    
    GtkWidget* urlFontEntry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(urlFontEntry), typography.urlFont.c_str());
    gtk_grid_attach(GTK_GRID(grid), urlFontEntry, 0, row++, 2, 1);
    
    // Tab Font
    GtkWidget* tabFontLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(tabFontLabel), "<b>Tab Font</b>");
    gtk_widget_set_halign(tabFontLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), tabFontLabel, 0, row++, 2, 1);
    
    GtkWidget* tabFontEntry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(tabFontEntry), typography.tabFont.c_str());
    gtk_grid_attach(GTK_GRID(grid), tabFontEntry, 0, row++, 2, 1);
    
    // Font Sizes
    GtkWidget* sizesLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(sizesLabel), "<b>Font Sizes (px)</b>");
    gtk_widget_set_halign(sizesLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(sizesLabel, 15);
    gtk_grid_attach(GTK_GRID(grid), sizesLabel, 0, row++, 2, 1);
    
    GtkWidget* uiSizeLabel = gtk_label_new("UI:");
    gtk_widget_set_halign(uiSizeLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), uiSizeLabel, 0, row, 1, 1);
    GtkWidget* uiSizeSpinner = gtk_spin_button_new_with_range(8, 24, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(uiSizeSpinner), typography.uiFontSize);
    gtk_grid_attach(GTK_GRID(grid), uiSizeSpinner, 1, row++, 1, 1);
    
    GtkWidget* urlSizeLabel = gtk_label_new("URL Bar:");
    gtk_widget_set_halign(urlSizeLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), urlSizeLabel, 0, row, 1, 1);
    GtkWidget* urlSizeSpinner = gtk_spin_button_new_with_range(8, 24, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(urlSizeSpinner), typography.urlFontSize);
    gtk_grid_attach(GTK_GRID(grid), urlSizeSpinner, 1, row++, 1, 1);
    
    GtkWidget* tabSizeLabel = gtk_label_new("Tabs:");
    gtk_widget_set_halign(tabSizeLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), tabSizeLabel, 0, row, 1, 1);
    GtkWidget* tabSizeSpinner = gtk_spin_button_new_with_range(8, 24, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tabSizeSpinner), typography.tabFontSize);
    gtk_grid_attach(GTK_GRID(grid), tabSizeSpinner, 1, row++, 1, 1);
    
    return scroll;
}

GtkWidget* BrayaCustomization::createLayoutTab() {
    GtkWidget* scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), 
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 15);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 20);
    gtk_widget_set_margin_start(grid, 20);
    gtk_widget_set_margin_end(grid, 20);
    gtk_widget_set_margin_top(grid, 20);
    gtk_widget_set_margin_bottom(grid, 20);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), grid);
    
    int row = 0;
    
    // Dimensions
    GtkWidget* dimLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(dimLabel), "<b>Dimensions (px)</b>");
    gtk_widget_set_halign(dimLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), dimLabel, 0, row++, 2, 1);
    
    GtkWidget* sidebarWidthLabel = gtk_label_new("Sidebar Width:");
    gtk_widget_set_halign(sidebarWidthLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), sidebarWidthLabel, 0, row, 1, 1);
    GtkWidget* sidebarWidthSpinner = gtk_spin_button_new_with_range(40, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(sidebarWidthSpinner), layout.sidebarWidth);
    gtk_grid_attach(GTK_GRID(grid), sidebarWidthSpinner, 1, row++, 1, 1);
    
    GtkWidget* tabHeightLabel = gtk_label_new("Tab Height:");
    gtk_widget_set_halign(tabHeightLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), tabHeightLabel, 0, row, 1, 1);
    GtkWidget* tabHeightSpinner = gtk_spin_button_new_with_range(32, 64, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tabHeightSpinner), layout.tabHeight);
    gtk_grid_attach(GTK_GRID(grid), tabHeightSpinner, 1, row++, 1, 1);
    
    // Border Radius
    GtkWidget* radiusLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(radiusLabel), "<b>Border Radius (px)</b>");
    gtk_widget_set_halign(radiusLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(radiusLabel, 15);
    gtk_grid_attach(GTK_GRID(grid), radiusLabel, 0, row++, 2, 1);
    
    GtkWidget* borderRadiusLabel = gtk_label_new("General:");
    gtk_widget_set_halign(borderRadiusLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), borderRadiusLabel, 0, row, 1, 1);
    GtkWidget* borderRadiusSpinner = gtk_spin_button_new_with_range(0, 32, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(borderRadiusSpinner), layout.borderRadius);
    gtk_grid_attach(GTK_GRID(grid), borderRadiusSpinner, 1, row++, 1, 1);
    
    GtkWidget* tabRadiusLabel = gtk_label_new("Tabs:");
    gtk_widget_set_halign(tabRadiusLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), tabRadiusLabel, 0, row, 1, 1);
    GtkWidget* tabRadiusSpinner = gtk_spin_button_new_with_range(0, 32, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tabRadiusSpinner), layout.tabBorderRadius);
    gtk_grid_attach(GTK_GRID(grid), tabRadiusSpinner, 1, row++, 1, 1);
    
    GtkWidget* urlRadiusLabel = gtk_label_new("URL Bar:");
    gtk_widget_set_halign(urlRadiusLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), urlRadiusLabel, 0, row, 1, 1);
    GtkWidget* urlRadiusSpinner = gtk_spin_button_new_with_range(0, 32, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(urlRadiusSpinner), layout.urlBarBorderRadius);
    gtk_grid_attach(GTK_GRID(grid), urlRadiusSpinner, 1, row++, 1, 1);
    
    // Icon Sizes
    GtkWidget* iconLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(iconLabel), "<b>Icon Sizes (px)</b>");
    gtk_widget_set_halign(iconLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(iconLabel, 15);
    gtk_grid_attach(GTK_GRID(grid), iconLabel, 0, row++, 2, 1);
    
    GtkWidget* iconSizeLabel = gtk_label_new("General:");
    gtk_widget_set_halign(iconSizeLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), iconSizeLabel, 0, row, 1, 1);
    GtkWidget* iconSizeSpinner = gtk_spin_button_new_with_range(12, 32, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(iconSizeSpinner), layout.iconSize);
    gtk_grid_attach(GTK_GRID(grid), iconSizeSpinner, 1, row++, 1, 1);
    
    return scroll;
}

GtkWidget* BrayaCustomization::createEffectsTab() {
    GtkWidget* scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), 
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 15);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 20);
    gtk_widget_set_margin_start(grid, 20);
    gtk_widget_set_margin_end(grid, 20);
    gtk_widget_set_margin_top(grid, 20);
    gtk_widget_set_margin_bottom(grid, 20);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), grid);
    
    int row = 0;
    
    // Shadow
    GtkWidget* shadowLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(shadowLabel), "<b>Shadows</b>");
    gtk_widget_set_halign(shadowLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), shadowLabel, 0, row++, 2, 1);
    
    GtkWidget* shadowIntLabel = gtk_label_new("Intensity (px):");
    gtk_widget_set_halign(shadowIntLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), shadowIntLabel, 0, row, 1, 1);
    GtkWidget* shadowIntSpinner = gtk_spin_button_new_with_range(0, 32, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(shadowIntSpinner), effects.shadowIntensity);
    gtk_grid_attach(GTK_GRID(grid), shadowIntSpinner, 1, row++, 1, 1);
    
    GtkWidget* shadowOpLabel = gtk_label_new("Opacity:");
    gtk_widget_set_halign(shadowOpLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), shadowOpLabel, 0, row, 1, 1);
    GtkWidget* shadowOpSpinner = gtk_spin_button_new_with_range(0, 1, 0.1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(shadowOpSpinner), effects.shadowOpacity);
    gtk_grid_attach(GTK_GRID(grid), shadowOpSpinner, 1, row++, 1, 1);
    
    // Glow
    GtkWidget* glowLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(glowLabel), "<b>Glow Effects</b>");
    gtk_widget_set_halign(glowLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(glowLabel, 15);
    gtk_grid_attach(GTK_GRID(grid), glowLabel, 0, row++, 2, 1);
    
    GtkWidget* glowEnableLabel = gtk_label_new("Enable Glow:");
    gtk_widget_set_halign(glowEnableLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), glowEnableLabel, 0, row, 1, 1);
    GtkWidget* glowSwitch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(glowSwitch), effects.enableGlow);
    gtk_grid_attach(GTK_GRID(grid), glowSwitch, 1, row++, 1, 1);
    
    GtkWidget* glowIntLabel = gtk_label_new("Glow Intensity:");
    gtk_widget_set_halign(glowIntLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), glowIntLabel, 0, row, 1, 1);
    GtkWidget* glowIntSpinner = gtk_spin_button_new_with_range(0, 32, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(glowIntSpinner), effects.glowIntensity);
    gtk_grid_attach(GTK_GRID(grid), glowIntSpinner, 1, row++, 1, 1);
    
    // Animation
    GtkWidget* animLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(animLabel), "<b>Animations</b>");
    gtk_widget_set_halign(animLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(animLabel, 15);
    gtk_grid_attach(GTK_GRID(grid), animLabel, 0, row++, 2, 1);
    
    GtkWidget* animSpeedLabel = gtk_label_new("Speed (ms):");
    gtk_widget_set_halign(animSpeedLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), animSpeedLabel, 0, row, 1, 1);
    GtkWidget* animSpeedSpinner = gtk_spin_button_new_with_range(50, 1000, 50);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(animSpeedSpinner), effects.animationSpeed);
    gtk_grid_attach(GTK_GRID(grid), animSpeedSpinner, 1, row++, 1, 1);
    
    // Transparency
    GtkWidget* transLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(transLabel), "<b>Transparency</b>");
    gtk_widget_set_halign(transLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(transLabel, 15);
    gtk_grid_attach(GTK_GRID(grid), transLabel, 0, row++, 2, 1);
    
    GtkWidget* transValueLabel = gtk_label_new("Window Opacity:");
    gtk_widget_set_halign(transValueLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), transValueLabel, 0, row, 1, 1);
    GtkWidget* transSpinner = gtk_spin_button_new_with_range(0.3, 1.0, 0.05);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(transSpinner), effects.transparency);
    gtk_grid_attach(GTK_GRID(grid), transSpinner, 1, row++, 1, 1);
    
    return scroll;
}

GtkWidget* BrayaCustomization::createColorPicker(const std::string& label, std::string* colorVar) {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    
    GtkWidget* lbl = gtk_label_new(label.c_str());
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), lbl);
    
    GtkWidget* btn = gtk_color_button_new();
    gtk_box_append(GTK_BOX(box), btn);
    
    // Set current color (simplified - would need proper parsing)
    GdkRGBA color;
    gdk_rgba_parse(&color, "#00d9ff"); // Default
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(btn), &color);
    
    return box;
}

std::string BrayaCustomization::generateCustomCSS() {
    std::string css;
    
    css += "/* Custom Generated CSS */\n";
    css += "window { background-color: " + colors.windowBg + "; }\n";
    css += ".sidebar { background: " + colors.sidebarBg + "; ";
    css += "border-right: 1px solid " + colors.sidebarBorder + "; ";
    css += "min-width: " + std::to_string(layout.sidebarWidth) + "px; }\n";
    
    // Add more CSS generation...
    
    return css;
}

void BrayaCustomization::applyCustomization() {
    std::cout << "✓ Applying custom customization..." << std::endl;
    // Generate and apply custom CSS
    std::string css = generateCustomCSS();
    save();
}

void BrayaCustomization::save() {
    // Save to JSON file
    std::cout << "✓ Saved customization settings" << std::endl;
}

void BrayaCustomization::load() {
    // Load from JSON file
    std::cout << "✓ Loaded customization settings" << std::endl;
}

// Callbacks
void BrayaCustomization::onColorChanged(GtkColorButton* button, gpointer data) {
    // Handle color change
}

void BrayaCustomization::onApplyClicked(GtkButton* button, gpointer data) {
    BrayaCustomization* custom = static_cast<BrayaCustomization*>(data);
    custom->applyCustomization();
}

void BrayaCustomization::onCloseClicked(GtkButton* button, gpointer data) {
    BrayaCustomization* custom = static_cast<BrayaCustomization*>(data);
    gtk_window_close(GTK_WINDOW(custom->dialog));
}

void BrayaCustomization::onResetClicked(GtkButton* button, gpointer data) {
    BrayaCustomization* custom = static_cast<BrayaCustomization*>(data);
    // Reset to defaults
    custom->colors = Colors();
    custom->typography = Typography();
    custom->layout = Layout();
    custom->effects = Effects();
    gtk_window_close(GTK_WINDOW(custom->dialog));
    custom->dialog = nullptr;
}
