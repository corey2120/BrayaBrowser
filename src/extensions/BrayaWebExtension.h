#ifndef BRAYA_WEB_EXTENSION_H
#define BRAYA_WEB_EXTENSION_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <webkit/webkit.h>

class BackgroundPageRunner;

// Represents a content script configuration
struct ContentScript {
    std::vector<std::string> matches;      // URL patterns to match
    std::vector<std::string> js;           // JavaScript files to inject
    std::vector<std::string> css;          // CSS files to inject
    std::string run_at;                     // "document_start", "document_end", "document_idle"
    bool all_frames;                        // Inject into all frames or just main
};

// Represents a browser action (toolbar button)
struct BrowserAction {
    std::string default_icon;               // Icon path
    std::string default_title;              // Tooltip text
    std::string default_popup;              // Popup HTML path
};

// Represents a WebExtension (Chrome/Firefox extension)
class BrayaWebExtension {
public:
    BrayaWebExtension(const std::string& id, const std::string& path);
    ~BrayaWebExtension();

    // Load and parse manifest.json
    bool loadManifest();

    // Getters
    std::string getId() const { return m_id; }
    std::string getName() const { return m_name; }
    std::string getVersion() const { return m_version; }
    std::string getPath() const { return m_path; }
    int getManifestVersion() const { return m_manifestVersion; }
    std::string getManifestJson() const { return m_manifestJson; }

    std::vector<std::string> getPermissions() const { return m_permissions; }
    std::vector<ContentScript> getContentScripts() const { return m_contentScripts; }
    std::vector<std::string> getBackgroundScripts() const { return m_backgroundScripts; }
    std::string getBackgroundPageFile() const { return m_backgroundPageFile; }
    BrowserAction getBrowserAction() const { return m_browserAction; }

    bool hasPermission(const std::string& permission) const;
    bool matchesUrl(const std::string& url) const;

    // Background page management
    WebKitWebView* getBackgroundPage() const { return m_backgroundPage; }
    void setBackgroundPage(WebKitWebView* page) { m_backgroundPage = page; }
    bool startBackgroundPage();
    bool hasBackgroundScripts() const { return !m_backgroundScripts.empty() || !m_backgroundPageFile.empty(); }
    BackgroundPageRunner* getBackgroundRunner() const { return m_backgroundRunner.get(); }

    // Enable/disable
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

private:
    std::string m_id;
    std::string m_name;
    std::string m_version;
    std::string m_description;
    std::string m_path;                     // Extension directory path
    int m_manifestVersion;                   // 2 or 3
    std::string m_manifestJson;              // Raw manifest JSON

    std::vector<std::string> m_permissions;
    std::vector<ContentScript> m_contentScripts;
    std::vector<std::string> m_backgroundScripts;
    std::string m_backgroundPageFile;        // Background HTML page file (e.g., "background.html")
    BrowserAction m_browserAction;

    WebKitWebView* m_backgroundPage;         // Hidden WebView for background scripts
    bool m_enabled;
    std::unique_ptr<BackgroundPageRunner> m_backgroundRunner;

    // Helper methods
    bool parseManifestJson(const std::string& jsonContent);
    std::string readFile(const std::string& filepath);
    std::string resolveI18nString(const std::string& str);
};

#endif // BRAYA_WEB_EXTENSION_H
