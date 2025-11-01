#ifndef BRAYA_WINDOW_H
#define BRAYA_WINDOW_H

#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <vector>
#include <memory>

class BrayaTab;
class BrayaSettings;
class BrayaHistory;
class BrayaDownloads;

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
    
private:
    void setupUI();
    void setupCSS();
    void createSidebar();
    void createContentArea();
    void createNavbar();
    void createBookmarksBar();
    void createStatusBar();
    
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
    GtkWidget* statusLabel;
    GtkWidget* tabStack;
    
    // Tab management
    std::vector<std::unique_ptr<BrayaTab>> tabs;
    int activeTabIndex;
    int nextTabId;
    bool showBookmarksBar;
    
    // Settings
    std::unique_ptr<BrayaSettings> settings;
    std::unique_ptr<BrayaHistory> history;
    std::unique_ptr<BrayaDownloads> downloads;
    
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
    static void onDevToolsClicked(GtkWidget* widget, gpointer data);
    static void onMinimizeClicked(GtkWidget* widget, gpointer data);
    static void onMaximizeClicked(GtkWidget* widget, gpointer data);
    static void onWindowCloseClicked(GtkWidget* widget, gpointer data);
    static gboolean onKeyPress(GtkEventControllerKey* controller, guint keyval, guint keycode, GdkModifierType state, gpointer data);
    
    void navigateTo(const char* url);
    void updateUI();
};

#endif // BRAYA_WINDOW_H
