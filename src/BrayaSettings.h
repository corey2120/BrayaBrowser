#ifndef BRAYA_SETTINGS_H
#define BRAYA_SETTINGS_H

#include <gtk/gtk.h>
#include <string>
#include <map>

class BrayaSettings {
public:
    enum Theme {
        DARK,
        LIGHT,
        INDUSTRIAL,
        CUSTOM
    };
    
    struct ColorScheme {
        std::string sidebarBg = "#0f1419";
        std::string tabBarBg = "#0f1419";
        std::string activeTabColor = "#00d9ff";
        std::string accentColor = "#00d9ff";
        std::string textColor = "#e0e6ed";
        std::string backgroundColor = "#0a0f14";
    };
    
    BrayaSettings();
    void show(GtkWindow* parent);
    void applyToWindow(GtkWidget* window);  // Apply visual changes
    
    // Getters
    Theme getTheme() const { return theme; }
    ColorScheme getColors() const { return colors; }
    int getFontSize() const { return fontSize; }
    std::string getFontFamily() const { return fontFamily; }
    bool getShowBookmarks() const { return showBookmarks; }
    bool getBlockTrackers() const { return blockTrackers; }
    bool getBlockAds() const { return blockAds; }
    bool getHttpsOnly() const { return httpsOnly; }
    bool getEnableJavaScript() const { return enableJavaScript; }
    bool getEnableWebGL() const { return enableWebGL; }
    bool getEnablePlugins() const { return enablePlugins; }
    std::string getDownloadPath() const { return downloadPath; }
    std::string getHomePage() const { return homePage; }
    std::string getSearchEngine() const { return searchEngine; }
    
    // Setters
    void setTheme(Theme t);
    void setColors(const ColorScheme& c);
    void setFontSize(int size);
    void setFontFamily(const std::string& family);
    void setShowBookmarks(bool show);
    void setBlockTrackers(bool block);
    void setBlockAds(bool block);
    void setHttpsOnly(bool https);
    void setEnableJavaScript(bool enable);
    void setEnableWebGL(bool enable);
    void setEnablePlugins(bool enable);
    void setDownloadPath(const std::string& path);
    void setHomePage(const std::string& page);
    void setSearchEngine(const std::string& engine);
    
private:
    Theme theme;
    ColorScheme colors;
    int fontSize;
    std::string fontFamily;
    bool showBookmarks;
    bool blockTrackers;
    bool blockAds;
    bool httpsOnly;
    bool enableJavaScript;
    bool enableWebGL;
    bool enablePlugins;
    std::string downloadPath;
    std::string homePage;
    std::string searchEngine;
    
    GtkWidget* dialog;
    GtkWidget* notebook;
    
    // Appearance widgets
    GtkWidget* themeCombo;
    GtkWidget* fontSpinner;
    GtkWidget* fontFamilyEntry;
    GtkWidget* sidebarColorBtn;
    GtkWidget* accentColorBtn;
    
    // Privacy widgets
    GtkWidget* blockTrackersSwitch;
    GtkWidget* blockAdsSwitch;
    GtkWidget* httpsOnlySwitch;
    
    // Web features widgets
    GtkWidget* jsSwitch;
    GtkWidget* webglSwitch;
    GtkWidget* pluginsSwitch;
    
    // General widgets
    GtkWidget* downloadPathEntry;
    GtkWidget* homePageEntry;
    GtkWidget* searchEngineCombo;
    
    // Advanced widgets
    GtkWidget* bookmarksSwitch;
    
    void createDialog(GtkWindow* parent);
    GtkWidget* createAppearanceTab();
    GtkWidget* createGeneralTab();
    GtkWidget* createSecurityTab();
    GtkWidget* createPrivacyTab();
    GtkWidget* createAdvancedTab();
    void applySettings();
    void applyTheme();
    void saveSettings();
    void loadSettings();
    void updateUIFromSettings();
    
    // Callbacks
    static void onThemeChanged(GtkComboBox* combo, gpointer data);
    static void onColorButtonClicked(GtkButton* button, gpointer data);
    static void onApplyClicked(GtkButton* button, gpointer data);
    static void onCloseClicked(GtkButton* button, gpointer data);
    static void onBookmarksToggled(GtkSwitch* widget, gboolean state, gpointer data);
    static void onFontSizeChanged(GtkSpinButton* button, gpointer data);
};

#endif // BRAYA_SETTINGS_H
