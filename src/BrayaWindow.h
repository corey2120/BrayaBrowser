#ifndef BRAYA_WINDOW_H
#define BRAYA_WINDOW_H

#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <vector>
#include <memory>
#include <map>
#include <set>
#include <string>
#include "GObjectPtr.h"

class BrayaTab;
class BrayaSettings;
class BrayaHistory;
class BrayaDownloads;
class BrayaBookmarks;
class TabGroup;
class BrayaPasswordManager;
class BrayaExtensionManager;
class BrayaAdBlocker;
class BrayaCommandPalette;

class BrayaWindow {
public:
    BrayaWindow(GtkApplication* app, bool isPrivate = false);
    ~BrayaWindow();
    void openPrivateWindow();
    
    GtkWidget* getWindow() { return window; }
    void show();
    
    // Tab management
    void createTab(const char* url = "about:braya");
    void closeTab(int index);
    void switchToTab(int index);
    void reopenClosedTab();

    // Split view
    void toggleSplitView();
    void setSplitOrientation(bool horizontal); // true = horizontal, false = vertical
    void moveTabToSplitPane(int tabIndex);

    // Session management
    void saveSession();
    void restoreSession();
    
    // History
    void showHistory();
    
    // Downloads
    void showDownloads();
    
    // Find in page
    void showFindBar();
    void hideFindBar();
    void findNext();
    void findPrevious();
    
    // Quick wins features
    void toggleReaderMode();
    void takeScreenshot();
    void toggleTabPin(int tabId);
    void toggleTabMute(int tabId);
    
    // Bookmarks
    void showBookmarksManager();
    
    // Tab Groups
    void createTabGroup(const std::string& name, const std::string& color);
    void addTabToGroup(int tabId, int groupId);
    void removeTabFromGroup(int tabId);
    void toggleGroupCollapse(int groupId);
    void showTabContextMenu(int tabId);
    
    // Theme
    void applyTheme(int themeId);
    void loadUserChrome();  // Zen-style userChrome.css

    // Downloads UI
    void showDownloadsButton();
    void hideDownloadsButton();
    void updateDownloadsButton(int activeCount);

    // Ad-Blocker UI
    void updateAdBlockerShield();

    // Extension Manager (public for lambda access)
    std::unique_ptr<BrayaExtensionManager> extensionManager;

private:
    void setupUI();
    void setupCSS();
    void loadThemeCSS(const std::string& themePath);
    std::string getResourcePath(const std::string& filename);
    void createSidebar();
    void createContentArea();
    void createNavbar();
    void createBookmarksBar();
    void createStatusBar();
    void createExtensionButtons();
    void updateExtensionButtons();
    void updateSecurityIndicator(const std::string& url);
    long getMemoryUsage();
    void updateMemoryIndicator();
    std::string generateHomePageHtml();
    
    // UI Components
    GtkWidget* window;
    GtkWidget* mainBox;
    GtkWidget* sidebar;
    GtkWidget* tabsBox;
    GtkWidget* contentBox;
    GtkWidget* navbar;
    GtkWidget* urlEntry;
    GtkWidget* urlBox;  // Container for URL entry + security indicator
    GtkWidget* securityIcon;  // HTTPS/HTTP indicator
    GtkWidget* backBtn;
    GtkWidget* forwardBtn;
    GtkWidget* reloadBtn;
    GtkWidget* bookmarksBar;
    GtkWidget* statusBar;
    GtkWidget* findBar;
    GtkWidget* findEntry;
    GtkWidget* findMatchLabel;
    GtkWidget* statusLabel;
    GtkWidget* tabStack;
    GtkWidget* tabStack2;  // Second stack for split view
    GtkWidget* splitPane;  // GtkPaned container
    GtkWidget* extensionButtonsBox;
    GtkWidget* downloadsBtn;
    GtkWidget* adBlockerShieldBtn;
    GtkWidget* memoryLabel;
    GtkWidget* zoomLabel;

    GtkCssProvider* cssProvider;
    guint memoryTimerSourceId;

    // Private / incognito mode
    bool m_isPrivate;
    GtkApplication* m_app;
    WebKitNetworkSession* m_privateSession;

    // Zoom: persisted per domain
    std::map<std::string, double> m_zoomLevels;
    void adjustZoom(double delta);
    void resetZoom();
    void applyZoomForUrl(const std::string& url);
    void updateZoomLabel();

    // Split view state
    bool isSplitView;
    bool splitHorizontal;  // true = side-by-side, false = top-bottom
    int activeTabIndexPane2;  // Active tab in second pane
    
    // Tab management
    std::vector<std::unique_ptr<BrayaTab>> tabs;
    int activeTabIndex;
    int nextTabId;
    bool showBookmarksBar;

    // Recently closed tabs
    struct ClosedTab {
        std::string url;
        std::string title;
    };
    std::vector<ClosedTab> recentlyClosedTabs;
    static constexpr int MAX_CLOSED_TABS = 10;
    
    // Tab groups
    std::vector<std::unique_ptr<TabGroup>> tabGroups;
    std::map<int, int> tabToGroup; // tabId -> groupId
    int nextGroupId;

    // Favicon cache - using RAII wrapper for automatic memory management
    std::map<std::string, GObjectPtr<GdkTexture>> faviconCache; // url -> favicon texture
    void cacheFavicon(const std::string& url, GdkTexture* favicon);
    GdkTexture* getCachedFavicon(const std::string& url);

    // Crash prevention: Track tabs being destroyed to prevent use-after-free
    std::set<GtkWidget*> destroyingTabs;
    
    // Settings
    std::unique_ptr<BrayaSettings> settings;
    std::unique_ptr<BrayaHistory> history;
    std::unique_ptr<BrayaDownloads> downloads;
    std::unique_ptr<BrayaBookmarks> bookmarksManager;
    std::unique_ptr<BrayaPasswordManager> passwordManager;
    std::unique_ptr<BrayaAdBlocker> adBlocker;
    std::unique_ptr<BrayaCommandPalette> commandPalette;

    // Password Manager
    void showPasswordManager();
    void showSiteSettings(BrayaTab* tab);
    
    // Callbacks
    static void onNewTabClicked(GtkWidget* widget, gpointer data);
    static void onTabClicked(GtkWidget* widget, gpointer data);
    static void onTabMiddleClick(GtkGestureClick* gesture, int n_press, double x, double y, gpointer data);
    static void onCloseTabClicked(GtkWidget* widget, gpointer data);
    static void onUrlActivate(GtkWidget* widget, gpointer data);
    static void onBackClicked(GtkWidget* widget, gpointer data);
    static void onForwardClicked(GtkWidget* widget, gpointer data);
    static void onReloadClicked(GtkWidget* widget, gpointer data);
    static void onHomeClicked(GtkWidget* widget, gpointer data);
    static void onBookmarkClicked(GtkWidget* widget, gpointer data);
    static void onSettingsClicked(GtkWidget* widget, gpointer data);
    static void onDownloadsClicked(GtkWidget* widget, gpointer data);
    static void onDevToolsClicked(GtkWidget* widget, gpointer data);
    static void onAdBlockerShieldClicked(GtkWidget* widget, gpointer data);
    static void onFindNextClicked(GtkWidget* widget, gpointer data);
    static void onFindPrevClicked(GtkWidget* widget, gpointer data);
    static void onFindCloseClicked(GtkWidget* widget, gpointer data);
    static void onFindEntryChanged(GtkWidget* widget, gpointer data);
    static void onMinimizeClicked(GtkWidget* widget, gpointer data);
    static void onMaximizeClicked(GtkWidget* widget, gpointer data);
    static void onWindowCloseClicked(GtkWidget* widget, gpointer data);
    static gboolean onKeyPress(GtkEventControllerKey* controller, guint keyval, guint keycode, GdkModifierType state, gpointer data);
    static void onTabRightClick(GtkGestureClick* gesture, int n_press, double x, double y, gpointer data);
    
    // Quick wins callbacks
    static void onReaderModeClicked(GtkWidget* widget, gpointer data);
    static void onScreenshotClicked(GtkWidget* widget, gpointer data);
    static void onAddBookmarkClicked(GtkWidget* widget, gpointer data);

    // Bookmarks bar callbacks
    static void onBookmarkBarItemClicked(GtkWidget* widget, gpointer data);
    static void onBookmarkBarAddClicked(GtkWidget* widget, gpointer data);
    static void onBookmarkBarItemRightClick(GtkGestureClick* gesture, int n_press, double x, double y, gpointer data);

    void navigateTo(const char* url);
    void updateUI();
    void refreshBookmarksBar();
};

#endif // BRAYA_WINDOW_H
