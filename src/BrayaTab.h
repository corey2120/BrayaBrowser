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
    BrayaTab(int id, const char* url = "about:braya", BrayaPasswordManager* passwordMgr = nullptr, BrayaExtensionManager* extMgr = nullptr);
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

    void setTabButton(GtkWidget* button) { tabButton = button; }
    void updateButton();

    // Extension installation callback
    void setExtensionInstallCallback(std::function<void(const std::string& url, const std::string& downloadUrl)> callback) {
        extensionInstallCallback = callback;
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

    // Quick wins state
    bool pinned;
    bool muted;
    bool readerMode;
    std::string originalContent; // For reader mode toggle

    WebKitWebView* webView;
    GtkWidget* scrolledWindow;
    GtkWidget* tabButton;
    WebKitUserContentManager* userContentManager;

    void setupPasswordManager();
    void injectPasswordScript();
    void autoFillPasswords();
    void showAutofillSuggestions();
    void injectExtensionContentScripts(const std::string& pageUrl);
    void setupExtensionDetector();
    void injectExtensionDetectorScript();
    std::string getResourcePath(const std::string& filename);

    static void onLoadChanged(WebKitWebView* webView, WebKitLoadEvent loadEvent, gpointer userData);
    static void onTitleChanged(WebKitWebView* webView, GParamSpec* pspec, gpointer userData);
    static void onUriChanged(WebKitWebView* webView, GParamSpec* pspec, gpointer userData);
    static void onFaviconChanged(WebKitWebView* webView, GParamSpec* pspec, gpointer userData);
    static void onWebProcessCrashed(WebKitWebView* webView, gpointer userData);
    static void onPasswordCaptured(WebKitUserContentManager* manager, JSCValue* value, gpointer userData);
    static void onAutofillRequest(WebKitUserContentManager* manager, JSCValue* value, gpointer userData);
    static void onCheckPasswords(WebKitUserContentManager* manager, JSCValue* value, gpointer userData);
    static void onExtensionInstallRequest(WebKitUserContentManager* manager, JSCValue* value, gpointer userData);
};

#endif // BRAYA_TAB_H
