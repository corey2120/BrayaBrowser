#ifndef BRAYA_SETTINGS_H
#define BRAYA_SETTINGS_H

#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <unordered_map>

class BrayaExtensionManager;
class ExtensionInstaller;
class BrayaAdBlocker;
class BrayaPasswordManager;

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
    void showTab(GtkWindow* parent, const std::string& tabName);

    // Set callback for when extensions are installed/removed
    void setExtensionChangeCallback(std::function<void()> callback) {
        m_extensionChangeCallback = callback;
    }
    void applyToWindow(GtkWidget* window);  // Apply visual changes
    void setThemeCallback(std::function<void(int)> callback) { themeCallback = callback; }
    void setExtensionManager(BrayaExtensionManager* manager) { m_extensionManager = manager; }
    void setAdBlocker(BrayaAdBlocker* blocker) { m_adBlocker = blocker; }
    void setPasswordManager(BrayaPasswordManager* manager) { m_passwordManager = manager; }
    void setWebContext(WebKitWebContext* context);
    
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
    bool getShowTabPreviews() const { return showTabPreviews; }
    bool getMemoryIndicatorEnabled() const { return memoryIndicatorEnabled; }
    bool getRestoreSession() const { return restoreSession; }
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
    void setShowTabPreviews(bool show);
    void setDownloadPath(const std::string& path);
    void setHomePage(const std::string& page);
    void setSearchEngine(const std::string& engine);

    // Extension management (public for BrayaWindow)
    void loadExtensionStates();
    void saveExtensionStates();
    void refreshExtensionsList();

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
    bool showTabPreviews;
    bool memoryIndicatorEnabled;
    bool restoreSession;
    std::string downloadPath;
    std::string homePage;
    std::string searchEngine;
    
    GtkWidget* dialog;
    GtkWidget* notebook;
    GtkWidget* searchEntry;
    GtkWidget* navList;
    WebKitWebContext* webContext;
    BrayaExtensionManager* m_extensionManager;
    BrayaAdBlocker* m_adBlocker;
    BrayaPasswordManager* m_passwordManager;
    std::function<void()> m_extensionChangeCallback;

    // Extensions widgets
    GtkWidget* extensionsList;
    
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
    GtkWidget* cookieStatusLabel;
    GtkWidget* siteDataStatusLabel;
    GtkWidget* multiStepSwitch;
    GtkWidget* tabPreviewsSwitch;

    // Ad-blocker widgets
    GtkWidget* adBlockerEnabledSwitch;
    GtkWidget* securityLevelCombo;
    GtkWidget* blockAdsCheck;
    GtkWidget* blockTrackersCheck;
    GtkWidget* blockSocialCheck;
    GtkWidget* blockCryptominersCheck;
    GtkWidget* blockPopupsCheck;
    GtkWidget* blockAutoplayCheck;
    GtkWidget* removeCookieWarningsCheck;
    GtkWidget* blockNSFWCheck;
    GtkWidget* statsLabel;
    GtkWidget* permissionsDialog = nullptr;
    GtkWidget* permissionsList = nullptr;

    // Theme callback
    std::function<void(int)> themeCallback;
    
    void createDialog(GtkWindow* parent);
    GtkWidget* createAppearanceTab();
    GtkWidget* createGeneralTab();
    GtkWidget* createSecurityTab();
    GtkWidget* createPrivacyTab();
    GtkWidget* createAdvancedTab();
    GtkWidget* createExtensionsTab();

    GtkWidget* createPasswordsTab();
    void updateAdBlockerUI();
    void applySettings();
    void applyTheme();
    void saveSettings();
    void loadSettings();
    void updateUIFromSettings();
    void navigateToSection(const std::string& sectionId);
    void buildSearchIndex();
    void handleSearchQuery(const std::string& query);
    void selectNavRow(const std::string& sectionId);
    GtkWidget* createSettingsCard(const std::string& title, const std::string& subtitle, GtkWidget** bodyOut = nullptr);
    GtkWidget* createSettingsRow(const std::string& title, const std::string& subtitle, GtkWidget* control);
    GtkWidget* createSettingsActionRow(const std::string& title, const std::string& subtitle, GtkWidget* actionButton);
    void refreshWebsiteDataStatus(WebKitWebsiteDataTypes types, GtkWidget* label, const std::string& prefix);
    void clearWebsiteData(WebKitWebsiteDataTypes types, GtkWidget* statusLabel, const std::string& successMessage);
    void populatePermissionsList();

    // Extension management helpers
    GtkWidget* createExtensionRow(void* extension);
    void loadUnpackedExtension();
    void removeExtension(const std::string& extensionId);
    
    // Callbacks
    static void onThemeChanged(GObject* obj, GParamSpec* pspec, gpointer data);
    static void onColorButtonClicked(GtkButton* button, gpointer data);
    static void onBookmarksToggled(GtkSwitch* widget, gboolean state, gpointer data);
    static void onFontSizeChanged(GtkSpinButton* button, gpointer data);

    // Extension callbacks
    static void onLoadUnpackedClicked(GtkButton* button, gpointer data);
    static void onToggleExtension(GtkSwitch* toggle, gboolean state, gpointer data);
    static void onRemoveExtension(GtkButton* button, gpointer data);

    // Ad-blocker callbacks
    static gboolean onMultiStepToggle(GtkSwitch* toggle, gboolean state, gpointer data);
    static void onAdBlockerToggled(GtkSwitch* toggle, gboolean state, gpointer data);
    static void onSecurityLevelChanged(GObject* obj, GParamSpec* pspec, gpointer data);
    static void onFeatureToggled(GtkCheckButton* button, gpointer data);
    static void onAddToWhitelist(GtkButton* button, gpointer data);
    static void onRemoveFromWhitelist(GtkButton* button, gpointer data);
    static void onWebsiteDataFetchFinished(GObject* source, GAsyncResult* result, gpointer user_data);
    static void onWebsiteDataClearFinished(GObject* source, GAsyncResult* result, gpointer user_data);
    static void onFilterListToggled(GtkSwitch* toggle, gboolean state, gpointer data);
    static void onAddCustomRule(GtkButton* button, gpointer data);
    static void onRemoveCustomRule(GtkButton* button, gpointer data);
    static void onResetStats(GtkButton* button, gpointer data);
    static void onExportSettings(GtkButton* button, gpointer data);
    static void onImportSettings(GtkButton* button, gpointer data);

    struct SectionSearchInfo {
        std::string id;
        std::string title;
        std::vector<std::string> keywords;
    };
    std::vector<SectionSearchInfo> sectionSearchIndex;
    std::unordered_map<std::string, GtkListBoxRow*> navRowMap;
};

#endif // BRAYA_SETTINGS_H
