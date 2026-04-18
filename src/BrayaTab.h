#ifndef BRAYA_TAB_H
#define BRAYA_TAB_H

#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <string>
#include <functional>

class BrayaPasswordManager;
class BrayaExtensionManager;

class BrayaTab {
public:
    BrayaTab(int id, const char* url = "about:braya", BrayaPasswordManager* passwordMgr = nullptr, BrayaExtensionManager* extMgr = nullptr, WebKitNetworkSession* networkSession = nullptr);
    ~BrayaTab();
    
    int getId() const { return id; }
    int getTabId() const { return id; }
    WebKitWebView* getWebView() { return webView; }
    GtkWidget* getScrolledWindow() { return scrolledWindow; }
    GtkWidget* getTabButton() { return tabButton; }
    
    std::string getTitle() const { return title; }
    std::string getUrl() const { return url; }
    
    // Quick wins features
    bool isPinned() const { return pinned; }
    void setPinned(bool pin);
    bool isMuted() const { return muted; }
    void setMuted(bool mute);
    bool isInReaderMode() const { return readerMode; }
    void toggleReaderMode();

    // 💤 Tab suspension (Phase 2 memory optimization)
    void suspend();
    void resume();
    bool isSuspended() const { return suspended; }

    void setTabButton(GtkWidget* button) { tabButton = button; }
    void updateButton();

    // Extension installation callback
    void setExtensionInstallCallback(std::function<void(const std::string& url, const std::string& downloadUrl)> callback) {
        extensionInstallCallback = callback;
    }

    // New window/popup callback
    void setNewWindowCallback(std::function<GtkWidget*(WebKitWebView*)> callback) {
        newWindowCallback = callback;
    }

    // New tab callback (for opening links in new tabs)
    void setNewTabCallback(std::function<void(const std::string& url)> callback) {
        newTabCallback = callback;
    }

    // Favicon cache callbacks
    void setFaviconCacheCallback(std::function<void(const std::string& url, GdkTexture* favicon)> cacheCallback) {
        faviconCacheCallback = cacheCallback;
    }
    void setFaviconGetCallback(std::function<GdkTexture*(const std::string& url)> getCallback) {
        faviconGetCallback = getCallback;
    }

private:
    int id;
    std::string title;
    std::string url;
    bool isLoading;
    GdkTexture* favicon;
    BrayaPasswordManager* passwordManager;
    BrayaExtensionManager* extensionManager;
    std::function<void(const std::string& url, const std::string& downloadUrl)> extensionInstallCallback;
    std::function<GtkWidget*(WebKitWebView*)> newWindowCallback;
    std::function<void(const std::string& url)> newTabCallback;
    std::function<void(const std::string& url, GdkTexture* favicon)> faviconCacheCallback;
    std::function<GdkTexture*(const std::string& url)> faviconGetCallback;

    // Quick wins state
    bool pinned;
    bool muted;
    bool readerMode;
    bool suspended;
    std::string originalContent; // For reader mode toggle
    std::string suspendedUrl;
    std::string suspendedTitle;
    GdkTexture* cachedFavicon;

    WebKitWebView* webView;
    GtkWidget* scrolledWindow;
    GtkWidget* tabButton;
    WebKitUserContentManager* userContentManager;
    GtkWidget* autofillPopover = nullptr;
    GtkWidget* autofillToast = nullptr;
    guint toastTimerSource = 0;
    guint deferredSetupSource = 0;  // Source ID for deferred password/extension setup

    void fetchFaviconViaJS();
    void loadFaviconFromURL(const std::string& url);
    void setFaviconFromBytes(const guchar* data, gsize length);

    void setupPasswordManager();
    void injectPasswordScript();
    void autoFillPasswords();
    void showAutofillSuggestions(const GdkRectangle* anchorRect = nullptr);
    void showAutofillToast(const std::string& message, const std::string& url = "", const std::string& username = "");
    void injectExtensionContentScripts(const std::string& pageUrl);
    void setupExtensionDetector();
    void injectExtensionDetectorScript();
    std::string getResourcePath(const std::string& filename);

    static void onLoadChanged(WebKitWebView* webView, WebKitLoadEvent loadEvent, gpointer userData);
    static void onTitleChanged(WebKitWebView* webView, GParamSpec* pspec, gpointer userData);
    static void onUriChanged(WebKitWebView* webView, GParamSpec* pspec, gpointer userData);
    static void onFaviconChanged(WebKitWebView* webView, GParamSpec* pspec, gpointer userData);
    static void onWebProcessCrashed(WebKitWebView* webView, WebKitWebProcessTerminationReason reason, gpointer userData);
    static void onPasswordCaptured(WebKitUserContentManager* manager, JSCValue* value, gpointer userData);
    static void onAutofillRequest(WebKitUserContentManager* manager, JSCValue* value, gpointer userData);
    static void onCheckPasswords(WebKitUserContentManager* manager, JSCValue* value, gpointer userData);
    static void onExtensionInstallRequest(WebKitUserContentManager* manager, JSCValue* value, gpointer userData);
    static GtkWidget* onCreateNewWindow(WebKitWebView* webView, WebKitNavigationAction* navigation, gpointer userData);
    static gboolean onDecidePolicy(WebKitWebView* webView, WebKitPolicyDecision* decision, WebKitPolicyDecisionType type, gpointer userData);
};

#endif // BRAYA_TAB_H