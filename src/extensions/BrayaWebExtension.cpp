#include "BrayaWebExtension.h"
#include "BackgroundPageRunner.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <json-glib/json-glib.h>
#include <sys/stat.h>

BrayaWebExtension::BrayaWebExtension(const std::string& id, const std::string& path)
    : m_id(id), m_path(path), m_manifestVersion(2), m_backgroundPage(nullptr), m_enabled(true) {
}

BrayaWebExtension::~BrayaWebExtension() {
    // Background page is managed by BrayaExtensionManager
}

std::string BrayaWebExtension::readFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "ERROR: Could not open file: " << filepath << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool BrayaWebExtension::loadManifest() {
    std::string manifestPath = m_path + "/manifest.json";

    // Check if file exists
    struct stat buffer;
    if (stat(manifestPath.c_str(), &buffer) != 0) {
        std::cerr << "ERROR: manifest.json not found at: " << manifestPath << std::endl;
        return false;
    }

    std::string jsonContent = readFile(manifestPath);
    if (jsonContent.empty()) {
        return false;
    }

    return parseManifestJson(jsonContent);
}

bool BrayaWebExtension::parseManifestJson(const std::string& jsonContent) {
    GError* error = nullptr;
    JsonParser* parser = json_parser_new();

    if (!json_parser_load_from_data(parser, jsonContent.c_str(), -1, &error)) {
        std::cerr << "ERROR: Failed to parse manifest.json: " << error->message << std::endl;
        g_error_free(error);
        g_object_unref(parser);
        return false;
    }

    JsonNode* root = json_parser_get_root(parser);
    if (!JSON_NODE_HOLDS_OBJECT(root)) {
        std::cerr << "ERROR: manifest.json root is not an object" << std::endl;
        g_object_unref(parser);
        return false;
    }

    JsonObject* rootObj = json_node_get_object(root);

    // Parse required fields
    if (json_object_has_member(rootObj, "name")) {
        m_name = json_object_get_string_member(rootObj, "name");
    }

    if (json_object_has_member(rootObj, "version")) {
        m_version = json_object_get_string_member(rootObj, "version");
    }

    if (json_object_has_member(rootObj, "manifest_version")) {
        m_manifestVersion = json_object_get_int_member(rootObj, "manifest_version");
    }

    if (json_object_has_member(rootObj, "description")) {
        m_description = json_object_get_string_member(rootObj, "description");
    }

    // Parse permissions array
    if (json_object_has_member(rootObj, "permissions")) {
        JsonArray* permsArray = json_object_get_array_member(rootObj, "permissions");
        guint numPerms = json_array_get_length(permsArray);
        for (guint i = 0; i < numPerms; i++) {
            const char* perm = json_array_get_string_element(permsArray, i);
            m_permissions.push_back(perm);
        }
    }

    // Parse background scripts
    if (json_object_has_member(rootObj, "background")) {
        JsonObject* bgObj = json_object_get_object_member(rootObj, "background");

        // ManifestV2: "page" - background HTML page
        if (json_object_has_member(bgObj, "page")) {
            const char* page = json_object_get_string_member(bgObj, "page");
            m_backgroundPageFile = page;
        }

        // ManifestV2: "scripts" array
        if (json_object_has_member(bgObj, "scripts")) {
            JsonArray* scriptsArray = json_object_get_array_member(bgObj, "scripts");
            guint numScripts = json_array_get_length(scriptsArray);
            for (guint i = 0; i < numScripts; i++) {
                const char* script = json_array_get_string_element(scriptsArray, i);
                m_backgroundScripts.push_back(script);
            }
        }

        // ManifestV3: "service_worker" string
        if (json_object_has_member(bgObj, "service_worker")) {
            const char* worker = json_object_get_string_member(bgObj, "service_worker");
            m_backgroundScripts.push_back(worker);
        }
    }

    // Parse content_scripts array
    if (json_object_has_member(rootObj, "content_scripts")) {
        JsonArray* csArray = json_object_get_array_member(rootObj, "content_scripts");
        guint numCS = json_array_get_length(csArray);

        for (guint i = 0; i < numCS; i++) {
            JsonObject* csObj = json_array_get_object_element(csArray, i);
            ContentScript cs;

            // Parse matches
            if (json_object_has_member(csObj, "matches")) {
                JsonArray* matchesArray = json_object_get_array_member(csObj, "matches");
                guint numMatches = json_array_get_length(matchesArray);
                for (guint j = 0; j < numMatches; j++) {
                    const char* match = json_array_get_string_element(matchesArray, j);
                    cs.matches.push_back(match);
                }
            }

            // Parse js files
            if (json_object_has_member(csObj, "js")) {
                JsonArray* jsArray = json_object_get_array_member(csObj, "js");
                guint numJS = json_array_get_length(jsArray);
                for (guint j = 0; j < numJS; j++) {
                    const char* jsFile = json_array_get_string_element(jsArray, j);
                    cs.js.push_back(jsFile);
                }
            }

            // Parse css files
            if (json_object_has_member(csObj, "css")) {
                JsonArray* cssArray = json_object_get_array_member(csObj, "css");
                guint numCSS = json_array_get_length(cssArray);
                for (guint j = 0; j < numCSS; j++) {
                    const char* cssFile = json_array_get_string_element(cssArray, j);
                    cs.css.push_back(cssFile);
                }
            }

            // Parse run_at
            if (json_object_has_member(csObj, "run_at")) {
                cs.run_at = json_object_get_string_member(csObj, "run_at");
            } else {
                cs.run_at = "document_idle";  // Default
            }

            // Parse all_frames
            if (json_object_has_member(csObj, "all_frames")) {
                cs.all_frames = json_object_get_boolean_member(csObj, "all_frames");
            } else {
                cs.all_frames = false;  // Default
            }

            m_contentScripts.push_back(cs);
        }
    }

    // Parse browser_action
    if (json_object_has_member(rootObj, "browser_action")) {
        JsonObject* baObj = json_object_get_object_member(rootObj, "browser_action");

        if (json_object_has_member(baObj, "default_icon")) {
            JsonNode* iconNode = json_object_get_member(baObj, "default_icon");

            if (JSON_NODE_HOLDS_VALUE(iconNode)) {
                // Simple string path
                m_browserAction.default_icon = json_object_get_string_member(baObj, "default_icon");
            } else if (JSON_NODE_HOLDS_OBJECT(iconNode)) {
                // Object with different sizes {"16": "path16.png", "32": "path32.png", ...}
                JsonObject* iconObj = json_node_get_object(iconNode);

                // Prefer 32px, then 16px, then 64px, then any available
                if (json_object_has_member(iconObj, "32")) {
                    m_browserAction.default_icon = json_object_get_string_member(iconObj, "32");
                } else if (json_object_has_member(iconObj, "16")) {
                    m_browserAction.default_icon = json_object_get_string_member(iconObj, "16");
                } else if (json_object_has_member(iconObj, "64")) {
                    m_browserAction.default_icon = json_object_get_string_member(iconObj, "64");
                } else {
                    // Get first available size
                    GList* members = json_object_get_members(iconObj);
                    if (members) {
                        const char* firstKey = (const char*)members->data;
                        m_browserAction.default_icon = json_object_get_string_member(iconObj, firstKey);
                        g_list_free(members);
                    }
                }
            }
        }

        if (json_object_has_member(baObj, "default_title")) {
            m_browserAction.default_title = json_object_get_string_member(baObj, "default_title");
        }

        if (json_object_has_member(baObj, "default_popup")) {
            m_browserAction.default_popup = json_object_get_string_member(baObj, "default_popup");
        }
    }

    g_object_unref(parser);

    std::cout << "✓ Loaded extension: " << m_name << " v" << m_version << std::endl;
    std::cout << "  Manifest version: " << m_manifestVersion << std::endl;
    std::cout << "  Permissions: " << m_permissions.size() << std::endl;
    std::cout << "  Background scripts: " << m_backgroundScripts.size() << std::endl;
    std::cout << "  Content scripts: " << m_contentScripts.size() << std::endl;

    return true;
}

bool BrayaWebExtension::hasPermission(const std::string& permission) const {
    for (const auto& perm : m_permissions) {
        if (perm == permission) {
            return true;
        }
    }
    return false;
}

bool BrayaWebExtension::matchesUrl(const std::string& url) const {
    // TODO: Implement proper URL pattern matching
    // For now, just check if any content script matches
    for (const auto& cs : m_contentScripts) {
        for (const auto& match : cs.matches) {
            if (match == "<all_urls>" || match == "http://*/*" || match == "https://*/*") {
                return true;
            }
            // Simple substring match for now
            if (url.find(match) != std::string::npos) {
                return true;
            }
        }
    }
    return false;
}

bool BrayaWebExtension::startBackgroundPage() {
    if (!hasBackgroundScripts()) {
        std::cout << "  No background scripts to run" << std::endl;
        return true;
    }

    std::cout << "🚀 Starting background page..." << std::endl;

    // Create background runner
    m_backgroundRunner = std::make_unique<BackgroundPageRunner>(this);

    // Initialize and start
    if (!m_backgroundRunner->initialize()) {
        std::cerr << "ERROR: Failed to initialize background page" << std::endl;
        m_backgroundRunner.reset();
        return false;
    }

    m_backgroundPage = m_backgroundRunner->getWebView();

    std::cout << "✓ Background page started" << std::endl;
    return true;
}
