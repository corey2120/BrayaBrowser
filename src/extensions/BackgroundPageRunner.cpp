#include "BackgroundPageRunner.h"
#include "BrayaWebExtension.h"
#include "BrayaExtensionAPI.h"
#include <iostream>
#include <fstream>
#include <sstream>

BackgroundPageRunner::BackgroundPageRunner(BrayaWebExtension* extension)
    : m_extension(extension), m_webView(nullptr), m_initialized(false) {
}

BackgroundPageRunner::~BackgroundPageRunner() {
    if (m_webView) {
        // WebView will be destroyed by GTK
        m_webView = nullptr;
    }
}

bool BackgroundPageRunner::initialize() {
    if (m_initialized) {
        return true;
    }

    std::cout << "🎭 Initializing background page for: " << m_extension->getName() << std::endl;

    // Create hidden WebView
    m_webView = WEBKIT_WEB_VIEW(webkit_web_view_new());

    if (!m_webView) {
        std::cerr << "ERROR: Failed to create background page WebView" << std::endl;
        return false;
    }

    // The WebView is hidden - never shown to user
    // It exists only to execute JavaScript in an isolated context

    // Connect to load-changed signal to know when page is loaded
    g_signal_connect(m_webView, "load-changed",
                    G_CALLBACK(onLoadChanged), this);

    // Connect console message handler to see background script logs
    WebKitWebView* wv = m_webView;
    WebKitSettings* settings = webkit_web_view_get_settings(wv);
    webkit_settings_set_enable_developer_extras(settings, TRUE);
    webkit_settings_set_enable_write_console_messages_to_stdout(settings, TRUE);

    // Check if extension has a background page HTML file (e.g., background.html)
    std::string backgroundPageFile = m_extension->getBackgroundPageFile();
    if (!backgroundPageFile.empty()) {
        // Inject browser APIs into the background page before it loads
        std::string apiScript = BrayaExtensionAPI::getBackgroundPageAPI();

        // Get the UserContentManager and add our API script
        WebKitUserContentManager* contentManager = webkit_web_view_get_user_content_manager(m_webView);
        WebKitUserScript* userScript = webkit_user_script_new(
            apiScript.c_str(),
            WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
            WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
            nullptr,  // Allow list (nullptr = all)
            nullptr   // Block list
        );
        webkit_user_content_manager_add_script(contentManager, userScript);
        webkit_user_script_unref(userScript);

        // Inject the extension manifest
        std::string manifestScript = "window.__extensionManifest = " + m_extension->getManifestJson() + ";";
        WebKitUserScript* manifestUserScript = webkit_user_script_new(
            manifestScript.c_str(),
            WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
            WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
            nullptr,
            nullptr
        );
        webkit_user_content_manager_add_script(contentManager, manifestUserScript);
        webkit_user_script_unref(manifestUserScript);

        std::cout << "  ✓ Browser APIs will inject into background page" << std::endl;

        // CRITICAL: Inject requestIdleCallback polyfill synchronously BEFORE loading page
        // This ensures it's available before any external scripts load
        std::string polyfill = R"(
            if (!self.requestIdleCallback) {
                self.requestIdleCallback = function(callback, options) {
                    const start = Date.now();
                    return setTimeout(function() {
                        callback({
                            didTimeout: false,
                            timeRemaining: function() {
                                return Math.max(0, 50 - (Date.now() - start));
                            }
                        });
                    }, 1);
                };
            }
            if (!self.cancelIdleCallback) {
                self.cancelIdleCallback = function(id) { clearTimeout(id); };
            }
            if (!window.requestIdleCallback) {
                window.requestIdleCallback = self.requestIdleCallback;
                window.cancelIdleCallback = self.cancelIdleCallback;
            }
        )";
        webkit_web_view_evaluate_javascript(m_webView, polyfill.c_str(), -1, nullptr, nullptr, nullptr, nullptr, nullptr);

        // Load the background HTML page using chrome-extension:// protocol
        // Extract numeric ID from extension ID (e.g., "extension_1763035889" -> "1763035889")
        std::string extensionId = m_extension->getId();
        size_t underscorePos = extensionId.find('_');
        std::string numericId = (underscorePos != std::string::npos) ?
                                extensionId.substr(underscorePos + 1) : extensionId;

        std::string pageUrl = "chrome-extension://" + numericId + "/" + backgroundPageFile;
        std::cout << "  📄 Loading background page: " << pageUrl << std::endl;
        webkit_web_view_load_uri(m_webView, pageUrl.c_str());
    } else {
        // Generate HTML page with background scripts
        std::string html = generateBackgroundHTML();

        // Load the HTML
        webkit_web_view_load_html(m_webView, html.c_str(), nullptr);
    }

    m_initialized = true;
    std::cout << "✓ Background page WebView created" << std::endl;

    return true;
}

std::string BackgroundPageRunner::generateBackgroundHTML() {
    std::stringstream html;

    html << "<!DOCTYPE html>\n";
    html << "<html>\n";
    html << "<head>\n";
    html << "  <meta charset=\"UTF-8\">\n";
    html << "  <title>Background Page - " << m_extension->getName() << "</title>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "  <h1 style=\"display:none;\">Background Page</h1>\n";

    // Inject comprehensive browser API
    html << "  <script>\n";
    html << BrayaExtensionAPI::getBackgroundPageAPI();
    html << "  </script>\n";

    // Load each background script
    auto backgroundScripts = m_extension->getBackgroundScripts();
    for (const auto& scriptPath : backgroundScripts) {
        std::string fullPath = m_extension->getPath() + "/" + scriptPath;
        std::string scriptContent = readScriptFile(fullPath);

        if (!scriptContent.empty()) {
            html << "  <script>\n";
            html << "    console.log('📜 Loading background script: " << scriptPath << "');\n";
            html << "    try {\n";
            html << scriptContent << "\n";
            html << "      console.log('✓ Background script loaded: " << scriptPath << "');\n";
            html << "    } catch(e) {\n";
            html << "      console.error('❌ Error in background script:', e);\n";
            html << "    }\n";
            html << "  </script>\n";
        } else {
            std::cerr << "⚠️  Could not read background script: " << fullPath << std::endl;
        }
    }

    html << "</body>\n";
    html << "</html>\n";

    return html.str();
}

std::string BackgroundPageRunner::readScriptFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "ERROR: Could not open script file: " << filepath << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool BackgroundPageRunner::loadBackgroundScripts() {
    // Scripts are loaded as part of the HTML generation
    // This method is called after the page is loaded
    std::cout << "✓ Background scripts loaded and executing" << std::endl;
    return true;
}

void BackgroundPageRunner::injectBrowserAPIs() {
    if (!m_webView) {
        return;
    }

    // TODO: Inject more sophisticated browser APIs using JSC
    // For now, basic API stubs are in the HTML
    std::cout << "✓ Browser APIs injected into background page" << std::endl;
}

void BackgroundPageRunner::onPageLoaded() {
    std::cout << "✓ Background page loaded for: " << m_extension->getName() << std::endl;
    loadBackgroundScripts();
    injectBrowserAPIs();
}

void BackgroundPageRunner::onLoadChanged(WebKitWebView* webview,
                                         WebKitLoadEvent load_event,
                                         gpointer user_data) {
    BackgroundPageRunner* runner = static_cast<BackgroundPageRunner*>(user_data);

    if (load_event == WEBKIT_LOAD_FINISHED) {
        runner->onPageLoaded();
    }
}
