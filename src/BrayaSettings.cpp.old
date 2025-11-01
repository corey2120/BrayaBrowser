#include "BrayaSettings.h"
#include <iostream>

BrayaSettings::BrayaSettings()
    : theme(DARK), fontSize(13), showBookmarks(true), dialog(nullptr) {
}

void BrayaSettings::show(GtkWindow* parent) {
    if (dialog) {
        gtk_window_present(GTK_WINDOW(dialog));
        return;
    }
    
    createDialog(parent);
    gtk_window_present(GTK_WINDOW(dialog));
}

void BrayaSettings::createDialog(GtkWindow* parent) {
    // Create dialog
    dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "🐕 Braya Browser Settings");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 400);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    
    // Main container
    GtkWidget* mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(mainBox, 20);
    gtk_widget_set_margin_end(mainBox, 20);
    gtk_widget_set_margin_top(mainBox, 20);
    gtk_widget_set_margin_bottom(mainBox, 20);
    gtk_window_set_child(GTK_WINDOW(dialog), mainBox);
    
    // Title
    GtkWidget* titleLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(titleLabel), "<span size='x-large' weight='bold'>Braya Browser Settings</span>");
    gtk_widget_set_halign(titleLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(mainBox), titleLabel);
    
    // Separator
    gtk_box_append(GTK_BOX(mainBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // Settings grid
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 15);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 15);
    gtk_box_append(GTK_BOX(mainBox), grid);
    
    int row = 0;
    
    // Theme selection
    GtkWidget* themeLabel = gtk_label_new("Theme:");
    gtk_widget_set_halign(themeLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), themeLabel, 0, row, 1, 1);
    
    themeCombo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(themeCombo), "Dark (Current)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(themeCombo), "Light");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(themeCombo), "Industrial");
    gtk_combo_box_set_active(GTK_COMBO_BOX(themeCombo), theme);
    g_signal_connect(themeCombo, "changed", G_CALLBACK(onThemeChanged), this);
    gtk_grid_attach(GTK_GRID(grid), themeCombo, 1, row, 1, 1);
    row++;
    
    // Font size
    GtkWidget* fontLabel = gtk_label_new("Font Size:");
    gtk_widget_set_halign(fontLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), fontLabel, 0, row, 1, 1);
    
    fontSpinner = gtk_spin_button_new_with_range(10, 24, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(fontSpinner), fontSize);
    g_signal_connect(fontSpinner, "value-changed", G_CALLBACK(onFontSizeChanged), this);
    gtk_grid_attach(GTK_GRID(grid), fontSpinner, 1, row, 1, 1);
    row++;
    
    // Show bookmarks bar
    GtkWidget* bookmarksLabel = gtk_label_new("Show Bookmarks Bar:");
    gtk_widget_set_halign(bookmarksLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), bookmarksLabel, 0, row, 1, 1);
    
    bookmarksSwitch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(bookmarksSwitch), showBookmarks);
    g_signal_connect(bookmarksSwitch, "state-set", G_CALLBACK(onBookmarksToggled), this);
    gtk_grid_attach(GTK_GRID(grid), bookmarksSwitch, 1, row, 1, 1);
    row++;
    
    // Info section
    GtkWidget* infoBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_margin_top(infoBox, 20);
    gtk_box_append(GTK_BOX(mainBox), infoBox);
    
    GtkWidget* infoLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(infoLabel), 
        "<span size='small'><b>🐕 Braya Browser</b>\n"
        "Version: 1.0.0\n"
        "Engine: WebKit (GTK 4)\n"
        "Platform: Native C++</span>");
    gtk_widget_set_halign(infoLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(infoBox), infoLabel);
    
    // Spacer
    GtkWidget* spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_vexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(mainBox), spacer);
    
    // Button box
    GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(buttonBox, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(mainBox), buttonBox);
    
    GtkWidget* applyBtn = gtk_button_new_with_label("Apply");
    g_signal_connect(applyBtn, "clicked", G_CALLBACK(onApplyClicked), this);
    gtk_box_append(GTK_BOX(buttonBox), applyBtn);
    
    GtkWidget* closeBtn = gtk_button_new_with_label("Close");
    g_signal_connect(closeBtn, "clicked", G_CALLBACK(onCloseClicked), this);
    gtk_box_append(GTK_BOX(buttonBox), closeBtn);
}

void BrayaSettings::setTheme(Theme t) {
    theme = t;
    applyTheme();
}

void BrayaSettings::setFontSize(int size) {
    fontSize = size;
    std::cout << "Font size set to: " << fontSize << std::endl;
}

void BrayaSettings::setShowBookmarks(bool show) {
    showBookmarks = show;
    std::cout << "Show bookmarks: " << (show ? "yes" : "no") << std::endl;
}

void BrayaSettings::applyTheme() {
    std::cout << "Applying theme: " << theme << std::endl;
    // TODO: Actually apply theme by reloading CSS
}

// Callbacks
void BrayaSettings::onThemeChanged(GtkComboBox* combo, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    settings->setTheme(static_cast<Theme>(gtk_combo_box_get_active(combo)));
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
    settings->applyTheme();
}

void BrayaSettings::onCloseClicked(GtkButton* button, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    gtk_window_destroy(GTK_WINDOW(settings->dialog));
    settings->dialog = nullptr;
}
