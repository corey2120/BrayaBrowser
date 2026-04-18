#include "BrayaCustomization.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <sys/stat.h>
#include <json-glib/json-glib.h>

BrayaCustomization::BrayaCustomization() : dialog(nullptr), notebook(nullptr), cssProvider(nullptr) {
    initializePresets();
    load();
    cssProvider = gtk_css_provider_new();
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
        "<span size='xx-large' weight='bold'>🎨 Advanced Customization Studio</span>");
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
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), createCustomCSSTab(),
        gtk_label_new("💻 Custom CSS"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), createUIVisibilityTab(),
        gtk_label_new("👁️ UI Elements"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), createAdvancedTab(),
        gtk_label_new("⚙️ Advanced"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), createTypographyTab(),
        gtk_label_new("✍️ Typography"));
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

    // Theme Presets Section
    GtkWidget* presetsLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(presetsLabel), "<b>🎨 Theme Presets</b>");
    gtk_widget_set_halign(presetsLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), presetsLabel, 0, row++, 2, 1);

    GtkWidget* presetsDesc = gtk_label_new("Choose a pre-made theme or customize colors below");
    gtk_widget_set_halign(presetsDesc, GTK_ALIGN_START);
    gtk_widget_add_css_class(presetsDesc, "dim-label");
    gtk_grid_attach(GTK_GRID(grid), presetsDesc, 0, row++, 2, 1);

    // Create preset buttons in a flow box
    GtkWidget* presetFlow = gtk_flow_box_new();
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(presetFlow), GTK_SELECTION_NONE);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(presetFlow), 5);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(presetFlow), 8);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(presetFlow), 8);
    gtk_widget_set_margin_bottom(presetFlow, 20);

    // Add preset buttons
    std::vector<std::string> presetNames = getAvailablePresets();
    for (const auto& presetName : presetNames) {
        GtkWidget* btn = gtk_button_new_with_label(presetName.c_str());
        gtk_widget_add_css_class(btn, "preset-button");
        g_object_set_data(G_OBJECT(btn), "preset-name", (gpointer)presetName.c_str());
        g_object_set_data(G_OBJECT(btn), "customization", this);
        g_signal_connect(btn, "clicked", G_CALLBACK(+[](GtkButton* button, gpointer) {
            const char* presetName = (const char*)g_object_get_data(G_OBJECT(button), "preset-name");
            BrayaCustomization* custom = (BrayaCustomization*)g_object_get_data(G_OBJECT(button), "customization");
            custom->loadPreset(presetName);
            custom->applyCustomization();
        }), nullptr);
        gtk_flow_box_append(GTK_FLOW_BOX(presetFlow), btn);
    }

    gtk_grid_attach(GTK_GRID(grid), presetFlow, 0, row++, 2, 1);

    // Separator
    GtkWidget* separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_margin_top(separator, 10);
    gtk_widget_set_margin_bottom(separator, 20);
    gtk_grid_attach(GTK_GRID(grid), separator, 0, row++, 2, 1);

    // Section: Main CSS Variables
    GtkWidget* mainLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(mainLabel), "<b>Custom Theme Colors (CSS Variables)</b>");
    gtk_widget_set_halign(mainLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), mainLabel, 0, row++, 2, 1);

    // Accent Color
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Accent Color", &colors.accentPrimary),
        0, row++, 2, 1);

    // Backgrounds
    GtkWidget* bgLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(bgLabel), "<b>Backgrounds</b>");
    gtk_widget_set_halign(bgLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(bgLabel, 15);
    gtk_grid_attach(GTK_GRID(grid), bgLabel, 0, row++, 2, 1);

    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Primary", &colors.bgPrimary),
        0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Secondary", &colors.bgSecondary),
        1, row++, 1, 1);

    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Tertiary", &colors.bgTertiary),
        0, row++, 1, 1);

    // Text Colors
    GtkWidget* textLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(textLabel), "<b>Text Colors</b>");
    gtk_widget_set_halign(textLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(textLabel, 15);
    gtk_grid_attach(GTK_GRID(grid), textLabel, 0, row++, 2, 1);

    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Primary", &colors.textPrimary),
        0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), createColorPicker("Secondary", &colors.textSecondary),
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
    gtk_widget_set_hexpand(lbl, TRUE);
    gtk_box_append(GTK_BOX(box), lbl);

    GtkColorDialog* colorDialog = gtk_color_dialog_new();
    GtkWidget* btn = gtk_color_dialog_button_new(colorDialog);
    g_object_unref(colorDialog);

    // Parse and set current color
    GdkRGBA color;
    if (gdk_rgba_parse(&color, colorVar->c_str())) {
        gtk_color_dialog_button_set_rgba(GTK_COLOR_DIALOG_BUTTON(btn), &color);
    }

    // Connect color change signal (notify::rgba fires when user picks a color)
    g_object_set_data(G_OBJECT(btn), "color-var", colorVar);
    g_signal_connect(btn, "notify::rgba", G_CALLBACK(+[](GObject* button, GParamSpec*, gpointer) {
        std::string* colorVar = (std::string*)g_object_get_data(button, "color-var");
        const GdkRGBA* rgba = gtk_color_dialog_button_get_rgba(GTK_COLOR_DIALOG_BUTTON(button));
        if (!rgba) return;

        char hex[8];
        snprintf(hex, sizeof(hex), "#%02x%02x%02x",
                 (int)(rgba->red * 255),
                 (int)(rgba->green * 255),
                 (int)(rgba->blue * 255));
        *colorVar = hex;
    }), nullptr);

    gtk_box_append(GTK_BOX(box), btn);

    return box;
}

std::string BrayaCustomization::hexToRGB(const std::string& hex) {
    // Convert #RRGGBB to "R, G, B"
    if (hex.length() != 7 || hex[0] != '#') {
        return "0, 217, 255"; // default cyan
    }

    unsigned int r, g, b;
    sscanf(hex.c_str(), "#%02x%02x%02x", &r, &g, &b);

    std::ostringstream oss;
    oss << r << ", " << g << ", " << b;
    return oss.str();
}

std::string BrayaCustomization::getConfigPath() {
    const char* home = g_get_home_dir();
    return std::string(home) + "/.config/braya-browser/customization.json";
}

ThemeColors BrayaCustomization::getCurrentTheme() const {
    ThemeColors theme;
    theme.name = "Custom";
    theme.accentColor = colors.accentPrimary;
    theme.bgPrimary = colors.bgPrimary;
    theme.bgSecondary = colors.bgSecondary;
    theme.bgTertiary = colors.bgTertiary;
    theme.textPrimary = colors.textPrimary;
    theme.textSecondary = colors.textSecondary;
    return theme;
}

void BrayaCustomization::initializePresets() {
    // Zen (default)
    ThemeColors zen;
    zen.name = "Zen";
    zen.accentColor = "#00d9ff";
    zen.bgPrimary = "#0a0f14";
    zen.bgSecondary = "#0f1419";
    zen.bgTertiary = "#1a1f26";
    zen.textPrimary = "#e0e6ed";
    zen.textSecondary = "#9ca3b0";
    presets["Zen"] = zen;

    // Arc (purple)
    ThemeColors arc;
    arc.name = "Arc";
    arc.accentColor = "#a855f7";
    arc.bgPrimary = "#0f0a14";
    arc.bgSecondary = "#1a0f26";
    arc.bgTertiary = "#261a33";
    arc.textPrimary = "#e0e6ed";
    arc.textSecondary = "#9ca3b0";
    presets["Arc"] = arc;

    // Nord
    ThemeColors nord;
    nord.name = "Nord";
    nord.accentColor = "#88c0d0";
    nord.bgPrimary = "#2e3440";
    nord.bgSecondary = "#3b4252";
    nord.bgTertiary = "#434c5e";
    nord.textPrimary = "#eceff4";
    nord.textSecondary = "#d8dee9";
    presets["Nord"] = nord;

    // Dracula
    ThemeColors dracula;
    dracula.name = "Dracula";
    dracula.accentColor = "#bd93f9";
    dracula.bgPrimary = "#282a36";
    dracula.bgSecondary = "#343746";
    dracula.bgTertiary = "#44475a";
    dracula.textPrimary = "#f8f8f2";
    dracula.textSecondary = "#6272a4";
    presets["Dracula"] = dracula;

    // Tokyo Night
    ThemeColors tokyo;
    tokyo.name = "Tokyo Night";
    tokyo.accentColor = "#7aa2f7";
    tokyo.bgPrimary = "#1a1b26";
    tokyo.bgSecondary = "#24283b";
    tokyo.bgTertiary = "#414868";
    tokyo.textPrimary = "#c0caf5";
    tokyo.textSecondary = "#a9b1d6";
    presets["Tokyo Night"] = tokyo;

    // Gruvbox Dark
    ThemeColors gruvbox;
    gruvbox.name = "Gruvbox";
    gruvbox.accentColor = "#d79921";  // warm yellow
    gruvbox.bgPrimary = "#282828";    // dark bg
    gruvbox.bgSecondary = "#3c3836";  // medium bg
    gruvbox.bgTertiary = "#504945";   // light bg
    gruvbox.textPrimary = "#ebdbb2";  // light fg
    gruvbox.textSecondary = "#a89984"; // medium fg
    presets["Gruvbox"] = gruvbox;

    // Catppuccin Mocha
    ThemeColors catppuccin;
    catppuccin.name = "Catppuccin";
    catppuccin.accentColor = "#89b4fa";  // blue
    catppuccin.bgPrimary = "#1e1e2e";    // base
    catppuccin.bgSecondary = "#313244";  // surface0
    catppuccin.bgTertiary = "#45475a";   // surface1
    catppuccin.textPrimary = "#cdd6f4";  // text
    catppuccin.textSecondary = "#bac2de"; // subtext1
    presets["Catppuccin"] = catppuccin;

    // One Dark (VS Code)
    ThemeColors onedark;
    onedark.name = "One Dark";
    onedark.accentColor = "#61afef";   // blue
    onedark.bgPrimary = "#282c34";     // editor bg
    onedark.bgSecondary = "#21252b";   // darker
    onedark.bgTertiary = "#2c313c";    // lighter
    onedark.textPrimary = "#abb2bf";   // fg
    onedark.textSecondary = "#5c6370"; // comment
    presets["One Dark"] = onedark;

    // Solarized Dark
    ThemeColors solarized;
    solarized.name = "Solarized";
    solarized.accentColor = "#268bd2";  // blue
    solarized.bgPrimary = "#002b36";    // base03
    solarized.bgSecondary = "#073642";  // base02
    solarized.bgTertiary = "#586e75";   // base01
    solarized.textPrimary = "#839496";  // base0
    solarized.textSecondary = "#657b83"; // base00
    presets["Solarized"] = solarized;

    // Monokai
    ThemeColors monokai;
    monokai.name = "Monokai";
    monokai.accentColor = "#66d9ef";   // cyan
    monokai.bgPrimary = "#272822";     // bg
    monokai.bgSecondary = "#3e3d32";   // highlight
    monokai.bgTertiary = "#49483e";    // line highlight
    monokai.textPrimary = "#f8f8f2";   // fg
    monokai.textSecondary = "#75715e"; // comment
    presets["Monokai"] = monokai;
}

std::vector<std::string> BrayaCustomization::getAvailablePresets() {
    std::vector<std::string> names;
    for (const auto& pair : presets) {
        names.push_back(pair.first);
    }
    return names;
}

void BrayaCustomization::loadPreset(const std::string& presetName) {
    auto it = presets.find(presetName);
    if (it != presets.end()) {
        const ThemeColors& theme = it->second;
        colors.accentPrimary = theme.accentColor;
        colors.bgPrimary = theme.bgPrimary;
        colors.bgSecondary = theme.bgSecondary;
        colors.bgTertiary = theme.bgTertiary;
        colors.textPrimary = theme.textPrimary;
        colors.textSecondary = theme.textSecondary;

        std::cout << "✓ Loaded preset: " << presetName << std::endl;
    }
}

void BrayaCustomization::setAccentColor(const std::string& hexColor) {
    colors.accentPrimary = hexColor;
}

void BrayaCustomization::applyTheme(GtkWidget* window) {
    std::string css = generateCustomCSS();

    GError* error = nullptr;
    gtk_css_provider_load_from_string(cssProvider, css.c_str());

    if (error) {
        std::cerr << "ERROR: Failed to load custom CSS: " << error->message << std::endl;
        g_error_free(error);
        return;
    }

    // Apply to the display
    GdkDisplay* display = gdk_display_get_default();
    gtk_style_context_add_provider_for_display(
        display,
        GTK_STYLE_PROVIDER(cssProvider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );

    std::cout << "✓ Applied custom theme" << std::endl;
}

std::string BrayaCustomization::generateCustomCSS() {
    std::string css;

    css += "/* Custom Generated CSS - Override CSS Variables */\n";
    css += ":root {\n";
    css += "    --braya-accent: " + colors.accentPrimary + ";\n";
    css += "    --braya-accent-rgb: " + hexToRGB(colors.accentPrimary) + ";\n";
    css += "    --braya-bg-primary: " + colors.bgPrimary + ";\n";
    css += "    --braya-bg-secondary: " + colors.bgSecondary + ";\n";
    css += "    --braya-bg-tertiary: " + colors.bgTertiary + ";\n";
    css += "    --braya-text-primary: " + colors.textPrimary + ";\n";
    css += "    --braya-text-secondary: " + colors.textSecondary + ";\n";

    // Regenerate derived variables
    css += "    --braya-sidebar-bg: linear-gradient(180deg, var(--braya-bg-secondary) 0%, var(--braya-bg-primary) 100%);\n";
    css += "    --braya-headerbar-bg: linear-gradient(90deg, var(--braya-bg-secondary) 0%, var(--braya-bg-tertiary) 100%);\n";
    css += "    --braya-statusbar-bg: linear-gradient(90deg, var(--braya-bg-primary) 0%, var(--braya-bg-secondary) 100%);\n";

    css += "}\n";

    return css;
}

void BrayaCustomization::applyCustomization() {
    std::cout << "✓ Applying custom customization..." << std::endl;

    // Generate custom CSS
    std::string css = generateCustomCSS();

    // Apply CSS to display
    GError* error = nullptr;
    gtk_css_provider_load_from_string(cssProvider, css.c_str());

    if (error) {
        std::cerr << "ERROR: Failed to load custom CSS: " << error->message << std::endl;
        g_error_free(error);
        return;
    }

    // Apply to all displays
    GdkDisplay* display = gdk_display_get_default();
    gtk_style_context_add_provider_for_display(
        display,
        GTK_STYLE_PROVIDER(cssProvider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );

    // Save settings
    save();

    std::cout << "✅ Custom theme applied and saved!" << std::endl;
}

void BrayaCustomization::save() {
    std::string configPath = getConfigPath();
    std::string configDir = std::string(g_get_home_dir()) + "/.config/braya-browser";
    g_mkdir_with_parents(configDir.c_str(), 0755);

    // Build JSON object
    JsonBuilder* builder = json_builder_new();
    json_builder_begin_object(builder);

    // Colors
    json_builder_set_member_name(builder, "colors");
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "accentPrimary");
    json_builder_add_string_value(builder, colors.accentPrimary.c_str());
    json_builder_set_member_name(builder, "bgPrimary");
    json_builder_add_string_value(builder, colors.bgPrimary.c_str());
    json_builder_set_member_name(builder, "bgSecondary");
    json_builder_add_string_value(builder, colors.bgSecondary.c_str());
    json_builder_set_member_name(builder, "bgTertiary");
    json_builder_add_string_value(builder, colors.bgTertiary.c_str());
    json_builder_set_member_name(builder, "textPrimary");
    json_builder_add_string_value(builder, colors.textPrimary.c_str());
    json_builder_set_member_name(builder, "textSecondary");
    json_builder_add_string_value(builder, colors.textSecondary.c_str());
    json_builder_end_object(builder);

    // Typography
    json_builder_set_member_name(builder, "typography");
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "uiFont");
    json_builder_add_string_value(builder, typography.uiFont.c_str());
    json_builder_set_member_name(builder, "urlFont");
    json_builder_add_string_value(builder, typography.urlFont.c_str());
    json_builder_set_member_name(builder, "tabFont");
    json_builder_add_string_value(builder, typography.tabFont.c_str());
    json_builder_set_member_name(builder, "uiFontSize");
    json_builder_add_int_value(builder, typography.uiFontSize);
    json_builder_set_member_name(builder, "urlFontSize");
    json_builder_add_int_value(builder, typography.urlFontSize);
    json_builder_set_member_name(builder, "tabFontSize");
    json_builder_add_int_value(builder, typography.tabFontSize);
    json_builder_end_object(builder);

    // Layout
    json_builder_set_member_name(builder, "layout");
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "sidebarWidth");
    json_builder_add_int_value(builder, layout.sidebarWidth);
    json_builder_set_member_name(builder, "tabHeight");
    json_builder_add_int_value(builder, layout.tabHeight);
    json_builder_set_member_name(builder, "borderRadius");
    json_builder_add_int_value(builder, layout.borderRadius);
    json_builder_set_member_name(builder, "tabBorderRadius");
    json_builder_add_int_value(builder, layout.tabBorderRadius);
    json_builder_set_member_name(builder, "urlBarBorderRadius");
    json_builder_add_int_value(builder, layout.urlBarBorderRadius);
    json_builder_set_member_name(builder, "iconSize");
    json_builder_add_int_value(builder, layout.iconSize);
    json_builder_end_object(builder);

    // Effects
    json_builder_set_member_name(builder, "effects");
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "shadowIntensity");
    json_builder_add_int_value(builder, effects.shadowIntensity);
    json_builder_set_member_name(builder, "shadowOpacity");
    json_builder_add_double_value(builder, effects.shadowOpacity);
    json_builder_set_member_name(builder, "enableGlow");
    json_builder_add_boolean_value(builder, effects.enableGlow);
    json_builder_set_member_name(builder, "glowIntensity");
    json_builder_add_int_value(builder, effects.glowIntensity);
    json_builder_set_member_name(builder, "animationSpeed");
    json_builder_add_int_value(builder, effects.animationSpeed);
    json_builder_set_member_name(builder, "transparency");
    json_builder_add_double_value(builder, effects.transparency);
    json_builder_end_object(builder);

    json_builder_end_object(builder);

    // Generate JSON string
    JsonNode* root = json_builder_get_root(builder);
    JsonGenerator* generator = json_generator_new();
    json_generator_set_root(generator, root);
    json_generator_set_pretty(generator, TRUE);
    gchar* jsonData = json_generator_to_data(generator, nullptr);

    // Write to file
    std::ofstream file(configPath);
    if (file.is_open()) {
        file << jsonData;
        file.close();
        std::cout << "✓ Saved customization settings to " << configPath << std::endl;
    } else {
        std::cerr << "ERROR: Could not write to " << configPath << std::endl;
    }

    g_free(jsonData);
    g_object_unref(generator);
    json_node_free(root);
    g_object_unref(builder);
}

void BrayaCustomization::load() {
    std::string configPath = getConfigPath();

    // Check if file exists
    if (!g_file_test(configPath.c_str(), G_FILE_TEST_EXISTS)) {
        std::cout << "ℹ️  No saved customization found, using defaults" << std::endl;
        return;
    }

    // Read file
    std::ifstream file(configPath);
    if (!file.is_open()) {
        std::cerr << "ERROR: Could not open " << configPath << std::endl;
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
        std::cerr << "ERROR: Failed to parse customization.json: " << error->message << std::endl;
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

    // Load colors
    if (json_object_has_member(rootObj, "colors")) {
        JsonObject* colorsObj = json_object_get_object_member(rootObj, "colors");
        if (json_object_has_member(colorsObj, "accentPrimary"))
            colors.accentPrimary = json_object_get_string_member(colorsObj, "accentPrimary");
        if (json_object_has_member(colorsObj, "bgPrimary"))
            colors.bgPrimary = json_object_get_string_member(colorsObj, "bgPrimary");
        if (json_object_has_member(colorsObj, "bgSecondary"))
            colors.bgSecondary = json_object_get_string_member(colorsObj, "bgSecondary");
        if (json_object_has_member(colorsObj, "bgTertiary"))
            colors.bgTertiary = json_object_get_string_member(colorsObj, "bgTertiary");
        if (json_object_has_member(colorsObj, "textPrimary"))
            colors.textPrimary = json_object_get_string_member(colorsObj, "textPrimary");
        if (json_object_has_member(colorsObj, "textSecondary"))
            colors.textSecondary = json_object_get_string_member(colorsObj, "textSecondary");
    }

    // Load typography
    if (json_object_has_member(rootObj, "typography")) {
        JsonObject* typoObj = json_object_get_object_member(rootObj, "typography");
        if (json_object_has_member(typoObj, "uiFont"))
            typography.uiFont = json_object_get_string_member(typoObj, "uiFont");
        if (json_object_has_member(typoObj, "urlFont"))
            typography.urlFont = json_object_get_string_member(typoObj, "urlFont");
        if (json_object_has_member(typoObj, "tabFont"))
            typography.tabFont = json_object_get_string_member(typoObj, "tabFont");
        if (json_object_has_member(typoObj, "uiFontSize"))
            typography.uiFontSize = json_object_get_int_member(typoObj, "uiFontSize");
        if (json_object_has_member(typoObj, "urlFontSize"))
            typography.urlFontSize = json_object_get_int_member(typoObj, "urlFontSize");
        if (json_object_has_member(typoObj, "tabFontSize"))
            typography.tabFontSize = json_object_get_int_member(typoObj, "tabFontSize");
    }

    // Load layout
    if (json_object_has_member(rootObj, "layout")) {
        JsonObject* layoutObj = json_object_get_object_member(rootObj, "layout");
        if (json_object_has_member(layoutObj, "sidebarWidth"))
            layout.sidebarWidth = json_object_get_int_member(layoutObj, "sidebarWidth");
        if (json_object_has_member(layoutObj, "tabHeight"))
            layout.tabHeight = json_object_get_int_member(layoutObj, "tabHeight");
        if (json_object_has_member(layoutObj, "borderRadius"))
            layout.borderRadius = json_object_get_int_member(layoutObj, "borderRadius");
        if (json_object_has_member(layoutObj, "tabBorderRadius"))
            layout.tabBorderRadius = json_object_get_int_member(layoutObj, "tabBorderRadius");
        if (json_object_has_member(layoutObj, "urlBarBorderRadius"))
            layout.urlBarBorderRadius = json_object_get_int_member(layoutObj, "urlBarBorderRadius");
        if (json_object_has_member(layoutObj, "iconSize"))
            layout.iconSize = json_object_get_int_member(layoutObj, "iconSize");
    }

    // Load effects
    if (json_object_has_member(rootObj, "effects")) {
        JsonObject* effectsObj = json_object_get_object_member(rootObj, "effects");
        if (json_object_has_member(effectsObj, "shadowIntensity"))
            effects.shadowIntensity = json_object_get_int_member(effectsObj, "shadowIntensity");
        if (json_object_has_member(effectsObj, "shadowOpacity"))
            effects.shadowOpacity = json_object_get_double_member(effectsObj, "shadowOpacity");
        if (json_object_has_member(effectsObj, "enableGlow"))
            effects.enableGlow = json_object_get_boolean_member(effectsObj, "enableGlow");
        if (json_object_has_member(effectsObj, "glowIntensity"))
            effects.glowIntensity = json_object_get_int_member(effectsObj, "glowIntensity");
        if (json_object_has_member(effectsObj, "animationSpeed"))
            effects.animationSpeed = json_object_get_int_member(effectsObj, "animationSpeed");
        if (json_object_has_member(effectsObj, "transparency"))
            effects.transparency = json_object_get_double_member(effectsObj, "transparency");
    }

    g_object_unref(parser);
    std::cout << "✓ Loaded customization settings from " << configPath << std::endl;
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

// Custom CSS Tab
GtkWidget* BrayaCustomization::createCustomCSSTab() {
    GtkWidget* scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), box);

    // Header
    GtkWidget* header = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(header), "<b>💻 Custom CSS - Zen Browser Style</b>");
    gtk_widget_set_halign(header, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), header);

    GtkWidget* desc = gtk_label_new("Import custom CSS files to completely transform Braya's appearance.\nJust like Zen Browser's mod system!");
    gtk_widget_set_halign(desc, GTK_ALIGN_START);
    gtk_widget_add_css_class(desc, "dim-label");
    gtk_box_append(GTK_BOX(box), desc);

    // Import button
    GtkWidget* importBtn = gtk_button_new_with_label("📂 Import CSS File");
    gtk_widget_set_size_request(importBtn, 200, -1);
    g_object_set_data(G_OBJECT(importBtn), "customization", this);
    g_signal_connect(importBtn, "clicked", G_CALLBACK(+[](GtkButton* button, gpointer) {
        BrayaCustomization* custom = (BrayaCustomization*)g_object_get_data(G_OBJECT(button), "customization");
        custom->importCustomCSS();
    }), nullptr);
    gtk_box_append(GTK_BOX(box), importBtn);

    // Info section
    GtkWidget* infoLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(infoLabel), "<b>✨ What You Can Do:</b>");
    gtk_widget_set_halign(infoLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(infoLabel, 20);
    gtk_box_append(GTK_BOX(box), infoLabel);

    const char* features[] = {
        "• Change any color, gradient, or background",
        "• Modify element sizes and spacing",
        "• Add custom animations and effects",
        "• Hide or show UI elements",
        "• Import community-made themes",
        "• Create your own unique look"
    };

    for (const char* feature : features) {
        GtkWidget* featureLabel = gtk_label_new(feature);
        gtk_widget_set_halign(featureLabel, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(box), featureLabel);
    }

    // Example CSS
    GtkWidget* exampleLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(exampleLabel), "<b>📝 Example CSS:</b>");
    gtk_widget_set_halign(exampleLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(exampleLabel, 20);
    gtk_box_append(GTK_BOX(box), exampleLabel);

    GtkWidget* exampleText = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(exampleText), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(exampleText), TRUE);
    gtk_widget_set_size_request(exampleText, -1, 200);

    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(exampleText));
    gtk_text_buffer_set_text(buffer, 
        "/* Custom Braya Browser Theme */\n"
        ":root {\n"
        "    --braya-accent: #ff6b6b;\n"
        "    --braya-bg-primary: #1a1a2e;\n"
        "    --braya-bg-secondary: #16213e;\n"
        "}\n\n"
        "/* Hide URL bar when not focused */\n"
        ".url-entry:not(:focus) {\n"
        "    opacity: 0.7;\n"
        "}\n\n"
        "/* Custom tab style */\n"
        ".tab-button {\n"
        "    border-radius: 12px;\n"
        "    margin: 4px;\n"
        "}", -1);

    gtk_box_append(GTK_BOX(box), exampleText);

    return scroll;
}

// Custom CSS Functions
std::string BrayaCustomization::getCustomCSSPath() {
    return std::string(g_get_home_dir()) + "/.config/braya-browser/custom.css";
}

void BrayaCustomization::importCustomCSS() {
    GtkFileDialog* dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Import Custom CSS");

    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "CSS Files");
    gtk_file_filter_add_pattern(filter, "*.css");

    GListStore* filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filters, filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));

    gtk_file_dialog_open(dialog, GTK_WINDOW(this->dialog), nullptr,
        +[](GObject* source, GAsyncResult* result, gpointer data) {
            BrayaCustomization* custom = static_cast<BrayaCustomization*>(data);
            GtkFileDialog* dialog = GTK_FILE_DIALOG(source);
            GError* error = nullptr;
            GFile* file = gtk_file_dialog_open_finish(dialog, result, &error);

            if (file) {
                char* path = g_file_get_path(file);
                custom->loadCustomCSS(path);
                g_free(path);
                g_object_unref(file);
            }
        }, this);
}

void BrayaCustomization::loadCustomCSS(const std::string& cssFile) {
    // Copy CSS file to config directory
    std::ifstream src(cssFile, std::ios::binary);
    std::ofstream dst(getCustomCSSPath(), std::ios::binary);

    if (src && dst) {
        dst << src.rdbuf();
        std::cout << "✅ Custom CSS imported: " << cssFile << std::endl;

        // Load and apply the CSS
        std::ifstream file(getCustomCSSPath());
        if (file) {
            std::string css((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

            GError* error = nullptr;
            gtk_css_provider_load_from_string(cssProvider, css.c_str());

            if (!error) {
                GdkDisplay* display = gdk_display_get_default();
                gtk_style_context_add_provider_for_display(
                    display,
                    GTK_STYLE_PROVIDER(cssProvider),
                    GTK_STYLE_PROVIDER_PRIORITY_USER
                );

                std::cout << "✅ Custom CSS applied!" << std::endl;
            } else {
                std::cerr << "Error loading CSS: " << error->message << std::endl;
                g_error_free(error);
            }
        }
    }
}

// UI Visibility Tab - Control what's shown/hidden
GtkWidget* BrayaCustomization::createUIVisibilityTab() {
    GtkWidget* scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), box);

    // Header
    GtkWidget* header = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(header), "<b>👁️ Show/Hide UI Elements</b>");
    gtk_widget_set_halign(header, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), header);

    GtkWidget* desc = gtk_label_new("Control which browser elements are visible");
    gtk_widget_set_halign(desc, GTK_ALIGN_START);
    gtk_widget_add_css_class(desc, "dim-label");
    gtk_box_append(GTK_BOX(box), desc);

    // Navigation Bar Elements
    GtkWidget* navLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(navLabel), "<b>Navigation Bar</b>");
    gtk_widget_set_halign(navLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(navLabel, 10);
    gtk_box_append(GTK_BOX(box), navLabel);

    auto createCheckbox = [&](const char* label, bool* value) {
        GtkWidget* check = gtk_check_button_new_with_label(label);
        gtk_check_button_set_active(GTK_CHECK_BUTTON(check), *value);
        g_object_set_data(G_OBJECT(check), "value-ptr", value);
        g_signal_connect(check, "toggled", G_CALLBACK(+[](GtkCheckButton* btn, gpointer) {
            bool* val = (bool*)g_object_get_data(G_OBJECT(btn), "value-ptr");
            *val = gtk_check_button_get_active(btn);
        }), nullptr);
        gtk_box_append(GTK_BOX(box), check);
    };

    createCheckbox("Show Back Button", &uiVisibility.showBackButton);
    createCheckbox("Show Forward Button", &uiVisibility.showForwardButton);
    createCheckbox("Show Reload Button", &uiVisibility.showReloadButton);
    createCheckbox("Show Home Button", &uiVisibility.showHomeButton);
    createCheckbox("Show Security Indicator (HTTPS lock)", &uiVisibility.showSecurityIndicator);

    // Other UI Elements
    GtkWidget* otherLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(otherLabel), "<b>Other Elements</b>");
    gtk_widget_set_halign(otherLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(otherLabel, 15);
    gtk_box_append(GTK_BOX(box), otherLabel);

    createCheckbox("Show Bookmarks Bar", &uiVisibility.showBookmarksBar);
    createCheckbox("Show Status Bar", &uiVisibility.showStatusBar);
    createCheckbox("Show Sidebar", &uiVisibility.showSidebar);
    createCheckbox("Show Downloads Button", &uiVisibility.showDownloadsButton);
    createCheckbox("Show Extensions Button", &uiVisibility.showExtensionsButton);
    createCheckbox("Show Settings Button", &uiVisibility.showSettingsButton);

    // Compact Mode
    GtkWidget* modeLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(modeLabel), "<b>Display Mode</b>");
    gtk_widget_set_halign(modeLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(modeLabel, 15);
    gtk_box_append(GTK_BOX(box), modeLabel);

    createCheckbox("Compact Mode (reduces spacing, smaller UI)", &uiVisibility.compactMode);

    return scroll;
}

// Advanced Tab - Granular controls
GtkWidget* BrayaCustomization::createAdvancedTab() {
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

    // Header
    GtkWidget* header = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(header), "<b>⚙️ Advanced Layout Controls</b>");
    gtk_widget_set_halign(header, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), header, 0, row++, 2, 1);

    auto createSpinner = [&](const char* label, int* value, int min, int max) {
        GtkWidget* lbl = gtk_label_new(label);
        gtk_widget_set_halign(lbl, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid), lbl, 0, row, 1, 1);

        GtkWidget* spin = gtk_spin_button_new_with_range(min, max, 1);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), *value);
        g_object_set_data(G_OBJECT(spin), "value-ptr", value);
        g_signal_connect(spin, "value-changed", G_CALLBACK(+[](GtkSpinButton* btn, gpointer) {
            int* val = (int*)g_object_get_data(G_OBJECT(btn), "value-ptr");
            *val = gtk_spin_button_get_value_as_int(btn);
        }), nullptr);
        gtk_grid_attach(GTK_GRID(grid), spin, 1, row++, 1, 1);
    };

    auto createCheckbox = [&](const char* label, bool* value) {
        GtkWidget* check = gtk_check_button_new_with_label(label);
        gtk_check_button_set_active(GTK_CHECK_BUTTON(check), *value);
        g_object_set_data(G_OBJECT(check), "value-ptr", value);
        g_signal_connect(check, "toggled", G_CALLBACK(+[](GtkCheckButton* btn, gpointer) {
            bool* val = (bool*)g_object_get_data(G_OBJECT(btn), "value-ptr");
            *val = gtk_check_button_get_active(btn);
        }), nullptr);
        gtk_grid_attach(GTK_GRID(grid), check, 0, row++, 2, 1);
    };

    // Tab Settings
    GtkWidget* tabLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(tabLabel), "<b>Tab Appearance</b>");
    gtk_widget_set_halign(tabLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(tabLabel, 15);
    gtk_grid_attach(GTK_GRID(grid), tabLabel, 0, row++, 2, 1);

    createSpinner("Tab Minimum Width (px):", &advancedLayout.tabMinWidth, 100, 400);
    createSpinner("Tab Maximum Width (px):", &advancedLayout.tabMaxWidth, 150, 500);
    createSpinner("Tab Height (px):", &advancedLayout.tabHeight, 30, 80);
    createSpinner("Tab Spacing (px):", &advancedLayout.tabSpacing, 0, 20);
    createCheckbox("Show Tab Close Buttons", &advancedLayout.showTabCloseButton);
    createCheckbox("Show Tab Icons/Favicons", &advancedLayout.showTabIcons);
    createCheckbox("Animate Tab Transitions", &advancedLayout.animateTabs);

    // URL Bar Settings
    GtkWidget* urlLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(urlLabel), "<b>URL Bar</b>");
    gtk_widget_set_halign(urlLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(urlLabel, 15);
    gtk_grid_attach(GTK_GRID(grid), urlLabel, 0, row++, 2, 1);

    createSpinner("URL Bar Height (px):", &advancedLayout.urlBarHeight, 30, 60);
    createSpinner("URL Bar Border Radius (px):", &advancedLayout.urlBarBorderRadius, 0, 30);
    createCheckbox("Show Protocol (https://)", &advancedLayout.urlBarShowProtocol);
    createCheckbox("Center URL Text", &advancedLayout.urlBarCenterText);

    // Sidebar Settings
    GtkWidget* sidebarLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(sidebarLabel), "<b>Sidebar</b>");
    gtk_widget_set_halign(sidebarLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(sidebarLabel, 15);
    gtk_grid_attach(GTK_GRID(grid), sidebarLabel, 0, row++, 2, 1);

    createSpinner("Sidebar Width (px):", &advancedLayout.sidebarWidth, 40, 100);
    createCheckbox("Auto-hide Sidebar", &advancedLayout.sidebarAutoHide);

    // Global Spacing
    GtkWidget* spacingLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(spacingLabel), "<b>Spacing</b>");
    gtk_widget_set_halign(spacingLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(spacingLabel, 15);
    gtk_grid_attach(GTK_GRID(grid), spacingLabel, 0, row++, 2, 1);

    createSpinner("Global Spacing (px):", &advancedLayout.globalSpacing, 0, 20);
    createSpinner("Button Spacing (px):", &advancedLayout.buttonSpacing, 0, 20);

    return scroll;
}
