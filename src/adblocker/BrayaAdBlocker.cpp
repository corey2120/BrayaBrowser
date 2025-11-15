#include "BrayaAdBlocker.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <json-glib/json-glib.h>
#include <sys/stat.h>
#include <ctime>

BrayaAdBlocker::BrayaAdBlocker()
    : m_enabled(false)
    , m_securityLevel(SecurityLevel::STANDARD)
    , m_contentFilter(nullptr)
{
    // Initialize with default settings
    m_features.block_ads = true;
    m_features.block_trackers = true;
    m_features.block_social = false;
    m_features.block_cryptominers = true;
    m_features.block_popups = true;
    m_features.block_autoplay = true;
    m_features.remove_cookie_warnings = false;
    m_features.block_nsfw = false;
}

BrayaAdBlocker::~BrayaAdBlocker() {
    if (m_contentFilter) {
        webkit_user_content_filter_unref(m_contentFilter);
    }
}

bool BrayaAdBlocker::initialize() {
    std::cout << "🛡️  Initializing Braya Ad-Blocker..." << std::endl;

    // Set settings path
    const char* home = getenv("HOME");
    if (home) {
        m_settingsPath = std::string(home) + "/.config/braya-browser/adblock-settings.json";
    } else {
        m_settingsPath = "./adblock-settings.json";
    }

    // Try to load existing settings
    if (!loadSettings(m_settingsPath)) {
        std::cout << "  → No existing settings, using defaults" << std::endl;
        loadDefaultFilterLists();
    }

    std::cout << "✓ Ad-Blocker initialized" << std::endl;
    std::cout << "  Security Level: " << (m_securityLevel == SecurityLevel::STANDARD ? "Standard" : "Custom") << std::endl;
    std::cout << "  Status: " << (m_enabled ? "Enabled" : "Disabled") << std::endl;

    return true;
}

void BrayaAdBlocker::enable() {
    if (m_enabled) return;

    m_enabled = true;
    std::cout << "🛡️  Ad-Blocker ENABLED" << std::endl;

    // Compile and apply rules
    compileRules();
    saveSettings(m_settingsPath);
}

void BrayaAdBlocker::disable() {
    if (!m_enabled) return;

    m_enabled = false;
    std::cout << "🛡️  Ad-Blocker DISABLED" << std::endl;
    saveSettings(m_settingsPath);
}

void BrayaAdBlocker::setSecurityLevel(SecurityLevel level) {
    m_securityLevel = level;
    applySecurityLevel();

    const char* levelNames[] = {"Off", "Minimal", "Standard", "Strict", "Custom"};
    std::cout << "🛡️  Security level set to: " << levelNames[static_cast<int>(level)] << std::endl;

    if (m_enabled) {
        compileRules();
    }
    saveSettings(m_settingsPath);
}

void BrayaAdBlocker::applySecurityLevel() {
    switch (m_securityLevel) {
        case SecurityLevel::OFF:
            m_enabled = false;
            break;

        case SecurityLevel::MINIMAL:
            m_features.block_ads = false;
            m_features.block_trackers = false;
            m_features.block_social = false;
            m_features.block_cryptominers = true;  // Security only
            m_features.block_popups = false;
            m_features.block_autoplay = false;
            m_features.remove_cookie_warnings = false;
            m_features.block_nsfw = false;
            break;

        case SecurityLevel::STANDARD:
            m_features.block_ads = true;
            m_features.block_trackers = true;
            m_features.block_social = false;
            m_features.block_cryptominers = true;
            m_features.block_popups = true;
            m_features.block_autoplay = false;
            m_features.remove_cookie_warnings = false;
            m_features.block_nsfw = false;
            break;

        case SecurityLevel::STRICT:
            m_features.block_ads = true;
            m_features.block_trackers = true;
            m_features.block_social = true;
            m_features.block_cryptominers = true;
            m_features.block_popups = true;
            m_features.block_autoplay = true;
            m_features.remove_cookie_warnings = true;
            m_features.block_nsfw = false;
            break;

        case SecurityLevel::CUSTOM:
            // Don't change features, user controls them
            break;
    }
}

void BrayaAdBlocker::setFeatures(const BlockingFeatures& features) {
    m_features = features;
    m_securityLevel = SecurityLevel::CUSTOM;

    if (m_enabled) {
        compileRules();
    }
    saveSettings(m_settingsPath);
}

void BrayaAdBlocker::setFeature(const std::string& feature, bool enabled) {
    if (feature == "block_ads") m_features.block_ads = enabled;
    else if (feature == "block_trackers") m_features.block_trackers = enabled;
    else if (feature == "block_social") m_features.block_social = enabled;
    else if (feature == "block_cryptominers") m_features.block_cryptominers = enabled;
    else if (feature == "block_popups") m_features.block_popups = enabled;
    else if (feature == "block_autoplay") m_features.block_autoplay = enabled;
    else if (feature == "remove_cookie_warnings") m_features.remove_cookie_warnings = enabled;
    else if (feature == "block_nsfw") m_features.block_nsfw = enabled;

    m_securityLevel = SecurityLevel::CUSTOM;

    if (m_enabled) {
        compileRules();
    }
    saveSettings(m_settingsPath);
}

void BrayaAdBlocker::addToWhitelist(const std::string& domain) {
    m_whitelist.insert(domain);
    std::cout << "  ✓ Added to whitelist: " << domain << std::endl;
    saveSettings(m_settingsPath);
}

void BrayaAdBlocker::removeFromWhitelist(const std::string& domain) {
    m_whitelist.erase(domain);
    std::cout << "  ✓ Removed from whitelist: " << domain << std::endl;
    saveSettings(m_settingsPath);
}

bool BrayaAdBlocker::isWhitelisted(const std::string& domain) const {
    return m_whitelist.find(domain) != m_whitelist.end();
}

std::vector<std::string> BrayaAdBlocker::getWhitelist() const {
    return std::vector<std::string>(m_whitelist.begin(), m_whitelist.end());
}

void BrayaAdBlocker::clearWhitelist() {
    m_whitelist.clear();
    std::cout << "  ✓ Whitelist cleared" << std::endl;
    saveSettings(m_settingsPath);
}

void BrayaAdBlocker::addCustomRule(const std::string& rule) {
    m_customRules.push_back(rule);
    std::cout << "  ✓ Added custom rule: " << rule << std::endl;

    if (m_enabled) {
        compileRules();
    }
    saveSettings(m_settingsPath);
}

void BrayaAdBlocker::removeCustomRule(const std::string& rule) {
    auto it = std::find(m_customRules.begin(), m_customRules.end(), rule);
    if (it != m_customRules.end()) {
        m_customRules.erase(it);
        std::cout << "  ✓ Removed custom rule: " << rule << std::endl;

        if (m_enabled) {
            compileRules();
        }
        saveSettings(m_settingsPath);
    }
}

bool BrayaAdBlocker::shouldBlock(const std::string& url, const std::string& domain) const {
    if (!m_enabled) return false;
    if (isWhitelisted(domain)) return false;

    // Check against compiled rules
    // This is a simplified version - in production you'd use the WebKit content filter
    std::string lowerUrl = url;
    std::transform(lowerUrl.begin(), lowerUrl.end(), lowerUrl.begin(), ::tolower);

    // Common ad patterns
    if (m_features.block_ads) {
        if (lowerUrl.find("/ads/") != std::string::npos ||
            lowerUrl.find("/ad.") != std::string::npos ||
            lowerUrl.find("doubleclick") != std::string::npos ||
            lowerUrl.find("googleads") != std::string::npos ||
            lowerUrl.find("adservice") != std::string::npos) {
            return true;
        }
    }

    // Tracker patterns
    if (m_features.block_trackers) {
        if (lowerUrl.find("analytics") != std::string::npos ||
            lowerUrl.find("tracking") != std::string::npos ||
            lowerUrl.find("tracker") != std::string::npos) {
            return true;
        }
    }

    // Cryptominer patterns
    if (m_features.block_cryptominers) {
        if (lowerUrl.find("coinhive") != std::string::npos ||
            lowerUrl.find("crypto-loot") != std::string::npos ||
            lowerUrl.find("jsecoin") != std::string::npos) {
            return true;
        }
    }

    return false;
}

bool BrayaAdBlocker::shouldBlockResource(const std::string& url, const std::string& type) const {
    // Future: implement resource-specific blocking
    return shouldBlock(url, "");
}

void BrayaAdBlocker::incrementBlockedCount(const std::string& type) {
    m_stats.total_blocked++;
    m_stats.blocked_today++;

    if (type == "ad") m_stats.ads_blocked++;
    else if (type == "tracker") m_stats.trackers_blocked++;
    else if (type == "malware") m_stats.malware_blocked++;
}

void BrayaAdBlocker::resetStats() {
    m_stats.total_blocked = 0;
    m_stats.blocked_today = 0;
    m_stats.ads_blocked = 0;
    m_stats.trackers_blocked = 0;
    m_stats.malware_blocked = 0;
    saveSettings(m_settingsPath);
}

void BrayaAdBlocker::compileRules() {
    std::cout << "  🔨 Compiling ad-block rules..." << std::endl;

    m_compiledRules.clear();

    // Add basic blocking rules based on features
    if (m_features.block_ads) {
        m_compiledRules.push_back(R"({"trigger":{"url-filter":".*ads.*"},"action":{"type":"block"}})");
        m_compiledRules.push_back(R"({"trigger":{"url-filter":".*doubleclick.*"},"action":{"type":"block"}})");
        m_compiledRules.push_back(R"({"trigger":{"url-filter":".*googlesyndication.*"},"action":{"type":"block"}})");
        m_compiledRules.push_back(R"({"trigger":{"url-filter":".*adservice.*"},"action":{"type":"block"}})");
    }

    if (m_features.block_trackers) {
        m_compiledRules.push_back(R"({"trigger":{"url-filter":".*analytics.*"},"action":{"type":"block"}})");
        m_compiledRules.push_back(R"({"trigger":{"url-filter":".*tracking.*"},"action":{"type":"block"}})");
        m_compiledRules.push_back(R"({"trigger":{"url-filter":".*tracker.*"},"action":{"type":"block"}})");
    }

    if (m_features.block_cryptominers) {
        m_compiledRules.push_back(R"({"trigger":{"url-filter":".*coinhive.*"},"action":{"type":"block"}})");
        m_compiledRules.push_back(R"({"trigger":{"url-filter":".*crypto-loot.*"},"action":{"type":"block"}})");
    }

    // Add custom rules
    for (const auto& rule : m_customRules) {
        m_compiledRules.push_back(compileRuleToJSON(rule));
    }

    std::cout << "  ✓ Compiled " << m_compiledRules.size() << " blocking rules" << std::endl;
}

std::string BrayaAdBlocker::compileRuleToJSON(const std::string& rule) {
    // Simple rule compilation - in production this would be more sophisticated
    std::string json = "{\"trigger\":{\"url-filter\":\"" + rule + "\"},\"action\":{\"type\":\"block\"}}";
    return json;
}

void BrayaAdBlocker::loadDefaultFilterLists() {
    // Add default filter lists
    FilterList easyList;
    easyList.name = "EasyList";
    easyList.url = "https://easylist.to/easylist/easylist.txt";
    easyList.enabled = true;
    easyList.last_updated = "";
    m_filterLists.push_back(easyList);

    FilterList easyPrivacy;
    easyPrivacy.name = "EasyPrivacy";
    easyPrivacy.url = "https://easylist.to/easylist/easyprivacy.txt";
    easyPrivacy.enabled = true;
    easyPrivacy.last_updated = "";
    m_filterLists.push_back(easyPrivacy);

    FilterList malware;
    malware.name = "Malware Domains";
    malware.url = "https://malware-filter.gitlab.io/malware-filter/urlhaus-filter.txt";
    malware.enabled = true;
    malware.last_updated = "";
    m_filterLists.push_back(malware);
}

bool BrayaAdBlocker::loadSettings(const std::string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0) {
        return false;  // File doesn't exist
    }

    GError* error = nullptr;
    JsonParser* parser = json_parser_new();

    if (!json_parser_load_from_file(parser, path.c_str(), &error)) {
        if (error) {
            std::cerr << "Error loading ad-blocker settings: " << error->message << std::endl;
            g_error_free(error);
        }
        g_object_unref(parser);
        return false;
    }

    JsonNode* root = json_parser_get_root(parser);
    if (!root || !JSON_NODE_HOLDS_OBJECT(root)) {
        g_object_unref(parser);
        return false;
    }

    JsonObject* rootObj = json_node_get_object(root);

    // Load enabled state
    if (json_object_has_member(rootObj, "enabled")) {
        m_enabled = json_object_get_boolean_member(rootObj, "enabled");
    }

    // Load security level
    if (json_object_has_member(rootObj, "security_level")) {
        const char* level = json_object_get_string_member(rootObj, "security_level");
        if (strcmp(level, "off") == 0) m_securityLevel = SecurityLevel::OFF;
        else if (strcmp(level, "minimal") == 0) m_securityLevel = SecurityLevel::MINIMAL;
        else if (strcmp(level, "standard") == 0) m_securityLevel = SecurityLevel::STANDARD;
        else if (strcmp(level, "strict") == 0) m_securityLevel = SecurityLevel::STRICT;
        else if (strcmp(level, "custom") == 0) m_securityLevel = SecurityLevel::CUSTOM;
    }

    // Load whitelist
    if (json_object_has_member(rootObj, "whitelist")) {
        JsonArray* whitelist = json_object_get_array_member(rootObj, "whitelist");
        guint len = json_array_get_length(whitelist);
        for (guint i = 0; i < len; i++) {
            const char* domain = json_array_get_string_element(whitelist, i);
            m_whitelist.insert(domain);
        }
    }

    g_object_unref(parser);
    std::cout << "  ✓ Loaded ad-blocker settings from: " << path << std::endl;
    return true;
}

bool BrayaAdBlocker::saveSettings(const std::string& path) {
    JsonBuilder* builder = json_builder_new();
    json_builder_begin_object(builder);

    // Save enabled state
    json_builder_set_member_name(builder, "enabled");
    json_builder_add_boolean_value(builder, m_enabled);

    // Save security level
    json_builder_set_member_name(builder, "security_level");
    const char* levelNames[] = {"off", "minimal", "standard", "strict", "custom"};
    json_builder_add_string_value(builder, levelNames[static_cast<int>(m_securityLevel)]);

    // Save features
    json_builder_set_member_name(builder, "features");
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "block_ads");
    json_builder_add_boolean_value(builder, m_features.block_ads);
    json_builder_set_member_name(builder, "block_trackers");
    json_builder_add_boolean_value(builder, m_features.block_trackers);
    json_builder_set_member_name(builder, "block_social");
    json_builder_add_boolean_value(builder, m_features.block_social);
    json_builder_set_member_name(builder, "block_cryptominers");
    json_builder_add_boolean_value(builder, m_features.block_cryptominers);
    json_builder_set_member_name(builder, "block_popups");
    json_builder_add_boolean_value(builder, m_features.block_popups);
    json_builder_set_member_name(builder, "block_autoplay");
    json_builder_add_boolean_value(builder, m_features.block_autoplay);
    json_builder_end_object(builder);

    // Save whitelist
    json_builder_set_member_name(builder, "whitelist");
    json_builder_begin_array(builder);
    for (const auto& domain : m_whitelist) {
        json_builder_add_string_value(builder, domain.c_str());
    }
    json_builder_end_array(builder);

    // Save stats
    json_builder_set_member_name(builder, "stats");
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "total_blocked");
    json_builder_add_int_value(builder, m_stats.total_blocked);
    json_builder_set_member_name(builder, "blocked_today");
    json_builder_add_int_value(builder, m_stats.blocked_today);
    json_builder_end_object(builder);

    json_builder_end_object(builder);

    // Generate JSON
    JsonGenerator* gen = json_generator_new();
    JsonNode* root = json_builder_get_root(builder);
    json_generator_set_root(gen, root);
    json_generator_set_pretty(gen, TRUE);

    gchar* json_data = json_generator_to_data(gen, nullptr);

    // Write to file
    std::ofstream file(path);
    if (file.is_open()) {
        file << json_data;
        file.close();
    }

    g_free(json_data);
    json_node_free(root);
    g_object_unref(gen);
    g_object_unref(builder);

    return true;
}

void BrayaAdBlocker::applyToContentManager(WebKitUserContentManager* manager) {
    if (!m_enabled || !manager) return;

    std::cout << "  🛡️  Applying ad-blocker to content manager..." << std::endl;

    // Compile rules if not already done
    if (m_compiledRules.empty()) {
        compileRules();
    }

    // Build JSON array of rules
    std::string rulesJSON = "[";
    for (size_t i = 0; i < m_compiledRules.size(); i++) {
        if (i > 0) rulesJSON += ",";
        rulesJSON += m_compiledRules[i];
    }
    rulesJSON += "]";

    // TODO: Create WebKitUserContentFilter from rules
    // This requires webkit2gtk-4.1 API
    // For now, we'll log that we're ready to apply
    std::cout << "  ✓ Ready to apply " << m_compiledRules.size() << " blocking rules" << std::endl;
}

bool BrayaAdBlocker::loadFilterList(const std::string& path) {
    std::cout << "  📄 Loading filter list from: " << path << std::endl;
    // TODO: Implement EasyList parser
    return true;
}

bool BrayaAdBlocker::loadFilterListFromURL(const std::string& url, const std::string& name) {
    std::cout << "  🌐 Downloading filter list: " << name << " from " << url << std::endl;
    // TODO: Implement HTTP download and parsing
    return true;
}

void BrayaAdBlocker::updateFilterLists() {
    std::cout << "  🔄 Updating filter lists..." << std::endl;
    for (auto& list : m_filterLists) {
        if (list.enabled) {
            loadFilterListFromURL(list.url, list.name);
        }
    }
}

void BrayaAdBlocker::enableFilterList(const std::string& name, bool enabled) {
    for (auto& list : m_filterLists) {
        if (list.name == name) {
            list.enabled = enabled;
            std::cout << "  " << (enabled ? "✓" : "✗") << " Filter list: " << name << std::endl;
            break;
        }
    }
    saveSettings(m_settingsPath);
}

bool BrayaAdBlocker::matchesDomain(const std::string& url, const std::string& pattern) const {
    // Simple pattern matching - in production use proper URL parsing and regex
    return url.find(pattern) != std::string::npos;
}
