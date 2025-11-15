#ifndef BRAYA_EXTENSION_MANAGER_H
#define BRAYA_EXTENSION_MANAGER_H

#include <webkit/webkit.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

class BrayaWebExtension;

class BrayaExtensionManager {
public:
    BrayaExtensionManager();
    ~BrayaExtensionManager();

    // Initialize extension system with WebKit context
    void initialize(WebKitWebContext* context);

    // Load native web process extension from .so file path
    bool loadNativeExtension(const std::string& path);

    // Load WebExtension (Chrome/Firefox extension) from directory
    bool loadWebExtension(const std::string& extensionDir);

    // Remove/uninstall a WebExtension by ID
    bool removeWebExtension(const std::string& id);

    // Get list of loaded WebExtensions
    std::vector<BrayaWebExtension*> getWebExtensions() const;
    BrayaWebExtension* getWebExtension(const std::string& id) const;

    // Enable/disable built-in extensions
    void enableBuiltinExtension(const std::string& name, bool enabled);
    bool isBuiltinExtensionEnabled(const std::string& name) const;

    // Get list of available built-in extensions
    std::vector<std::string> getBuiltinExtensions() const;

    // Handle messages from web process
    static void onUserMessageReceived(WebKitWebView* webview,
                                      WebKitUserMessage* message,
                                      gpointer user_data);

    // Get the WebKit context
    WebKitWebContext* getWebContext() const { return m_context; }

private:
    WebKitWebContext* m_context;
    std::string m_extensionDirectory;
    std::vector<std::string> m_loadedExtensions;
    std::map<std::string, bool> m_builtinExtensionState;
    std::map<std::string, std::unique_ptr<BrayaWebExtension>> m_webExtensions;

    void setupWebContext();
    void createExtensionDirectory();
    std::string getExtensionDirectory() const;

    // URI scheme handler for chrome-extension:// protocol
    static void onChromeExtensionURISchemeRequest(WebKitURISchemeRequest* request, gpointer user_data);
    static const char* getMimeType(const std::string& path);
};

#endif // BRAYA_EXTENSION_MANAGER_H
