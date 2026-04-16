#ifndef BACKGROUND_PAGE_RUNNER_H
#define BACKGROUND_PAGE_RUNNER_H

#include <webkit/webkit.h>
#include <string>
#include <vector>

class BrayaWebExtension;

// Manages background page execution for WebExtensions
// Background pages run in a hidden WebView and have access to browser APIs
class BackgroundPageRunner {
public:
    BackgroundPageRunner(BrayaWebExtension* extension);
    ~BackgroundPageRunner();

    // Create hidden WebView and load background scripts
    bool initialize();

    // Load and execute background scripts
    bool loadBackgroundScripts();

    // Get the hidden WebView
    WebKitWebView* getWebView() const { return m_webView; }

    // Check if background page is running
    bool isRunning() const { return m_webView != nullptr; }

    // Inject browser API (chrome.runtime, chrome.tabs, etc.)
    void injectBrowserAPIs();

private:
    BrayaWebExtension* m_extension;
    WebKitWebView* m_webView;
    bool m_initialized;

    // Helper methods
    std::string readScriptFile(const std::string& filepath);
    std::string generateBackgroundHTML();
    void onPageLoaded();

    // Callbacks
    static void onLoadChanged(WebKitWebView* webview,
                             WebKitLoadEvent load_event,
                             gpointer user_data);
};

#endif // BACKGROUND_PAGE_RUNNER_H
