#include "BrayaExtensionManager.h"
#include "BrayaWebExtension.h"
#include <iostream>
#include <sys/stat.h>
#include <glib.h>

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

    std::cout << "✓ WebKit context configured for extensions" << std::endl;
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
