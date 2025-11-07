#ifndef BRAYA_WINDOW_H
#define BRAYA_WINDOW_H

#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <vector>
#include <memory>
#include <map>

class BrayaTab;
class BrayaSettings;
class BrayaHistory;
class BrayaDownloads;
class BrayaBookmarks;
class TabGroup;
class BrayaPasswordManager;
class BrayaExtensionManager;

class BrayaWindow {
public:
    BrayaWindow(GtkApplication* app);
    ~BrayaWindow();
    
    GtkWidget* getWindow() { return window; }
    void show();
    
    // Tab management
    void createTab(const char* url = "about:braya");
    void closeTab(int index);
    void switchToTab(int index);
    
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
    
    // UI Components
    GtkWidget* window;
    GtkWidget* mainBox;
    GtkWidget* sidebar;
    GtkWidget* tabsBox;
    GtkWidget* contentBox;
    GtkWidget* navbar;
    GtkWidget* urlEntry;
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
    GtkWidget* extensionButtonsBox;
    
    GtkCssProvider* cssProvider;
    
    // Tab management
    std::vector<std::unique_ptr<BrayaTab>> tabs;
    int activeTabIndex;
    int nextTabId;
    bool showBookmarksBar;
    
    // Tab groups
    std::vector<std::unique_ptr<TabGroup>> tabGroups;
    std::map<int, int> tabToGroup; // tabId -> groupId
    int nextGroupId;
    
    // Settings
    std::unique_ptr<BrayaSettings> settings;
    std::unique_ptr<BrayaHistory> history;
    std::unique_ptr<BrayaDownloads> downloads;
    std::unique_ptr<BrayaBookmarks> bookmarksManager;
    std::unique_ptr<BrayaPasswordManager> passwordManager;
    std::unique_ptr<BrayaExtensionManager> extensionManager;

    // Password Manager
    void showPasswordManager();
    
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
