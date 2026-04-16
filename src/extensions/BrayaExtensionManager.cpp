#include "BrayaExtensionManager.h"
#include "BrayaWebExtension.h"
#include <iostream>
#include <sys/stat.h>
#include <glib.h>
#include <libsoup/soup.h>

BrayaExtensionManager::BrayaExtensionManager()
    : m_context(nullptr)
{
    // Initialize built-in extension states
    m_builtinExtensionState["ad-blocker"] = true;  // Enabled by default
    m_builtinExtensionState["dark-reader"] = false;
}

BrayaExtensionManager::~BrayaExtensionManager() {
}

void BrayaExtensionManager::initialize(WebKitWebContext* context) {
    m_context = context;

    std::cout << "🔌 Initializing Braya Extension System..." << std::endl;

    createExtensionDirectory();
    setupWebContext();

    std::cout << "✓ Extension system initialized" << std::endl;
    std::cout << "  Extension directory: " << m_extensionDirectory << std::endl;
}

void BrayaExtensionManager::createExtensionDirectory() {
    const char* configDir = g_get_user_config_dir();
    m_extensionDirectory = std::string(configDir) + "/braya-browser/extensions";

    // Create directory if it doesn't exist
    g_mkdir_with_parents(m_extensionDirectory.c_str(), 0755);
}

void BrayaExtensionManager::setupWebContext() {
    if (!m_context) {
        std::cerr << "ERROR: WebKitWebContext is null!" << std::endl;
        return;
    }

    // Enable favicons through WebsiteDataManager (WebKitGTK 6.0+ API)
    // Favicons are now stored automatically in the website data directory
    std::string cacheDir = std::string(g_get_user_cache_dir()) + "/braya-browser";
    g_mkdir_with_parents(cacheDir.c_str(), 0700);

    // Set cache model to optimize for web browsing
    webkit_web_context_set_cache_model(m_context, WEBKIT_CACHE_MODEL_WEB_BROWSER);
    std::cout << "✓ WebKit cache model set to WEB_BROWSER (enables favicons)" << std::endl;

    // Set the directory where WebKit will look for web process extensions
    // The web extension .so file should be in this directory
    webkit_web_context_set_web_process_extensions_directory(m_context, m_extensionDirectory.c_str());

    // Initialize web extensions with data
    // This data will be passed to webkit_web_extension_initialize()
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);

    // Pass enabled extensions state
    g_variant_builder_add(&builder, "{sv}", "ad-blocker-enabled",
                         g_variant_new_boolean(m_builtinExtensionState["ad-blocker"]));

    GVariant* data = g_variant_builder_end(&builder);
    webkit_web_context_set_web_process_extensions_initialization_user_data(m_context, data);

    // Register chrome-extension:// URI scheme handler
    webkit_web_context_register_uri_scheme(m_context, "chrome-extension",
                                          onChromeExtensionURISchemeRequest,
                                          this,
                                          nullptr);

    std::cout << "✓ WebKit context configured for extensions" << std::endl;
    std::cout << "✓ Registered chrome-extension:// URI scheme handler" << std::endl;
}

bool BrayaExtensionManager::loadNativeExtension(const std::string& path) {
    // Check if file exists
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0) {
        std::cerr << "ERROR: Extension file not found: " << path << std::endl;
        return false;
    }

    // Copy extension to extension directory
    std::string filename = path.substr(path.find_last_of('/') + 1);
    std::string destPath = m_extensionDirectory + "/" + filename;

    // Use system cp command (simple approach)
    std::string command = "cp " + path + " " + destPath;
    int result = system(command.c_str());

    if (result != 0) {
        std::cerr << "ERROR: Failed to copy extension to directory" << std::endl;
        return false;
    }

    m_loadedExtensions.push_back(filename);
    std::cout << "✓ Loaded extension: " << filename << std::endl;

    return true;
}

void BrayaExtensionManager::enableBuiltinExtension(const std::string& name, bool enabled) {
    if (m_builtinExtensionState.find(name) != m_builtinExtensionState.end()) {
        m_builtinExtensionState[name] = enabled;
        std::cout << (enabled ? "✓ Enabled" : "✗ Disabled") << " extension: " << name << std::endl;

        // TODO: Send message to web process to reload extension state
    } else {
        std::cerr << "ERROR: Unknown builtin extension: " << name << std::endl;
    }
}

bool BrayaExtensionManager::isBuiltinExtensionEnabled(const std::string& name) const {
    auto it = m_builtinExtensionState.find(name);
    if (it != m_builtinExtensionState.end()) {
        return it->second;
    }
    return false;
}

std::vector<std::string> BrayaExtensionManager::getBuiltinExtensions() const {
    std::vector<std::string> extensions;
    for (const auto& pair : m_builtinExtensionState) {
        extensions.push_back(pair.first);
    }
    return extensions;
}

void BrayaExtensionManager::onUserMessageReceived(WebKitWebView* webview,
                                                   WebKitUserMessage* message,
                                                   gpointer user_data) {
    const char* name = webkit_user_message_get_name(message);
    GVariant* parameters = webkit_user_message_get_parameters(message);

    std::cout << "📨 Received message from web process: " << name << std::endl;

    if (g_strcmp0(name, "extension-log") == 0) {
        // Log message from extension
        const char* logMessage = g_variant_get_string(parameters, nullptr);
        std::cout << "  [Extension Log] " << logMessage << std::endl;
    }
    else if (g_strcmp0(name, "request-blocked") == 0) {
        // Request was blocked by ad blocker
        const char* url = g_variant_get_string(parameters, nullptr);
        std::cout << "  🚫 Blocked: " << url << std::endl;
    }
    else {
        std::cout << "  Unknown message type: " << name << std::endl;
    }
}

std::string BrayaExtensionManager::getExtensionDirectory() const {
    return m_extensionDirectory;
}

bool BrayaExtensionManager::loadWebExtension(const std::string& extensionDir) {
    // Check if directory exists
    struct stat buffer;
    if (stat(extensionDir.c_str(), &buffer) != 0 || !S_ISDIR(buffer.st_mode)) {
        std::cerr << "ERROR: Extension directory not found or not a directory: " << extensionDir << std::endl;
        return false;
    }

    // Convert to absolute path
    char* absPath = realpath(extensionDir.c_str(), nullptr);
    if (!absPath) {
        std::cerr << "ERROR: Failed to resolve extension path: " << extensionDir << std::endl;
        return false;
    }
    std::string absolutePath(absPath);
    free(absPath);

    // Generate ID from directory name
    std::string dirName = absolutePath.substr(absolutePath.find_last_of('/') + 1);
    std::string extensionId = dirName;

    std::cout << "📦 Loading WebExtension: " << extensionId << std::endl;
    std::cout << "  → Resolved path: " << absolutePath << std::endl;

    // Create extension object with absolute path
    auto extension = std::make_unique<BrayaWebExtension>(extensionId, absolutePath);

    // Load and parse manifest.json
    if (!extension->loadManifest()) {
        std::cerr << "ERROR: Failed to load manifest for extension: " << extensionId << std::endl;
        return false;
    }

    // Store extension
    m_webExtensions[extensionId] = std::move(extension);

    // Start background page if extension has background scripts
    if (m_webExtensions[extensionId]->hasBackgroundScripts()) {
        if (!m_webExtensions[extensionId]->startBackgroundPage()) {
            std::cerr << "⚠️  Failed to start background page for: " << extensionId << std::endl;
        }
    }

    std::cout << "✓ WebExtension loaded: " << extensionId << std::endl;
    return true;
}

std::vector<BrayaWebExtension*> BrayaExtensionManager::getWebExtensions() const {
    std::vector<BrayaWebExtension*> extensions;
    for (const auto& pair : m_webExtensions) {
        extensions.push_back(pair.second.get());
    }
    return extensions;
}

BrayaWebExtension* BrayaExtensionManager::getWebExtension(const std::string& id) const {
    auto it = m_webExtensions.find(id);
    if (it != m_webExtensions.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool BrayaExtensionManager::removeWebExtension(const std::string& id) {
    auto it = m_webExtensions.find(id);
    if (it == m_webExtensions.end()) {
        std::cerr << "ERROR: Extension not found: " << id << std::endl;
        return false;
    }

    std::cout << "🗑️  Removing extension: " << it->second->getName() << std::endl;

    // Stop background page if running
    if (it->second->getBackgroundPage()) {
        // Background page cleanup handled by destructor
        std::cout << "  Stopping background page..." << std::endl;
    }

    // Remove from map (this will destroy the extension object)
    m_webExtensions.erase(it);

    std::cout << "✓ Extension removed successfully" << std::endl;
    return true;
}

// Get MIME type based on file extension
const char* BrayaExtensionManager::getMimeType(const std::string& path) {
    // Find file extension
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "application/octet-stream";
    }

    std::string ext = path.substr(dotPos + 1);

    // Common MIME types
    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "json") return "application/json";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif") return "image/gif";
    if (ext == "svg") return "image/svg+xml";
    if (ext == "wasm") return "application/wasm";  // IMPORTANT for Bitwarden
    if (ext == "woff") return "font/woff";
    if (ext == "woff2") return "font/woff2";
    if (ext == "ttf") return "font/ttf";
    if (ext == "eot") return "application/vnd.ms-fontobject";
    if (ext == "xml") return "text/xml";
    if (ext == "txt") return "text/plain";
    if (ext == "ico") return "image/x-icon";
    if (ext == "webp") return "image/webp";
    if (ext == "webm") return "video/webm";
    if (ext == "mp4") return "video/mp4";
    if (ext == "mp3") return "audio/mpeg";
    if (ext == "ogg") return "audio/ogg";
    if (ext == "pdf") return "application/pdf";
    if (ext == "zip") return "application/zip";

    return "application/octet-stream";
}

// URI scheme handler for chrome-extension:// protocol
void BrayaExtensionManager::onChromeExtensionURISchemeRequest(WebKitURISchemeRequest* request, gpointer user_data) {
    BrayaExtensionManager* manager = static_cast<BrayaExtensionManager*>(user_data);
    const char* uri = webkit_uri_scheme_request_get_uri(request);

    std::cout << "[chrome-extension://] Request: " << uri << std::endl;

    // Parse URI: chrome-extension://EXTENSION_ID/path/to/file.ext
    std::string uriStr(uri);
    std::string prefix = "chrome-extension://";

    if (uriStr.find(prefix) != 0) {
        std::cerr << "[chrome-extension://] ERROR: Invalid URI format" << std::endl;
        GError* error = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT, "Invalid URI format");
        webkit_uri_scheme_request_finish_error(request, error);
        g_error_free(error);
        return;
    }

    // Extract extension ID and file path
    std::string remainder = uriStr.substr(prefix.length());
    size_t slashPos = remainder.find('/');

    if (slashPos == std::string::npos) {
        std::cerr << "[chrome-extension://] ERROR: No file path in URI" << std::endl;
        GError* error = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT, "No file path");
        webkit_uri_scheme_request_finish_error(request, error);
        g_error_free(error);
        return;
    }

    std::string extensionId = remainder.substr(0, slashPos);
    std::string filePath = remainder.substr(slashPos + 1);

    // Build full file path
    std::string fullPath = manager->m_extensionDirectory + "/extension_" + extensionId + "/" + filePath;

    std::cout << "[chrome-extension://] Loading: " << fullPath << std::endl;

    // Check if file exists
    GFile* file = g_file_new_for_path(fullPath.c_str());
    GError* error = nullptr;

    GFileInputStream* stream = g_file_read(file, nullptr, &error);

    if (!stream) {
        std::cerr << "[chrome-extension://] ERROR: Failed to read file: " << fullPath << std::endl;
        if (error) {
            std::cerr << "  Error: " << error->message << std::endl;
            webkit_uri_scheme_request_finish_error(request, error);
            g_error_free(error);
        } else {
            GError* notFoundError = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "File not found");
            webkit_uri_scheme_request_finish_error(request, notFoundError);
            g_error_free(notFoundError);
        }
        g_object_unref(file);
        return;
    }

    // Get file size
    GFileInfo* fileInfo = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                            G_FILE_QUERY_INFO_NONE, nullptr, nullptr);
    goffset contentLength = -1;
    if (fileInfo) {
        contentLength = g_file_info_get_size(fileInfo);
        g_object_unref(fileInfo);
    }

    // Determine MIME type
    const char* mimeType = getMimeType(filePath);
    std::cout << "[chrome-extension://] MIME type: " << mimeType << ", Size: " << contentLength << " bytes" << std::endl;

    // Create response
    WebKitURISchemeResponse* response = webkit_uri_scheme_response_new(G_INPUT_STREAM(stream), contentLength);
    webkit_uri_scheme_response_set_content_type(response, mimeType);

    // Set HTTP status code to 200 (required for WebAssembly.instantiateStreaming)
    webkit_uri_scheme_response_set_status(response, 200, "OK");

    // Set additional headers for WASM files
    if (std::string(mimeType) == "application/wasm") {
        SoupMessageHeaders* headers = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);
        soup_message_headers_append(headers, "Content-Type", "application/wasm");
        soup_message_headers_append(headers, "X-Content-Type-Options", "nosniff");
        webkit_uri_scheme_response_set_http_headers(response, headers);
        soup_message_headers_unref(headers);
        std::cout << "[chrome-extension://] ✓ Added WASM headers" << std::endl;
    }

    // Finish request
    webkit_uri_scheme_request_finish_with_response(request, response);

    // Cleanup
    g_object_unref(response);
    g_object_unref(stream);
    g_object_unref(file);

    std::cout << "[chrome-extension://] ✓ File served successfully" << std::endl;
}
