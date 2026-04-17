#include "BrayaAdBlocker.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <ctime>
#include <json-glib/json-glib.h>

// ---------------------------------------------------------------------------
// Filter lists — same set uBlock Origin uses by default
// ---------------------------------------------------------------------------
static const struct { const char* name; const char* url; } FILTER_LISTS[] = {
    { "ublock-filters",
      "https://raw.githubusercontent.com/uBlockOrigin/uAssets/refs/heads/master/filters/filters.txt" },
    { "ublock-privacy",
      "https://raw.githubusercontent.com/uBlockOrigin/uAssets/refs/heads/master/filters/privacy.txt" },
    { "easylist",
      "https://easylist.to/easylist/easylist.txt" },
    { "easyprivacy",
      "https://easylist.to/easylist/easyprivacy.txt" },
};
static const int NUM_FILTER_LISTS = sizeof(FILTER_LISTS) / sizeof(FILTER_LISTS[0]);

// Max rules we'll pass to WebKit per compilation (keeps compile time reasonable)
static const int MAX_RULES = 100000;

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

BrayaAdBlocker::BrayaAdBlocker()
    : m_enabled(true)
    , m_securityLevel(SecurityLevel::STANDARD)
    , m_filterStore(nullptr)
    , m_compiledFilter(nullptr)
{
    m_features.block_ads           = true;
    m_features.block_trackers      = true;
    m_features.block_social        = false;
    m_features.block_cryptominers  = true;
    m_features.block_popups        = true;
    m_features.block_autoplay      = true;
    m_features.remove_cookie_warnings = false;
    m_features.block_nsfw          = false;
}

BrayaAdBlocker::~BrayaAdBlocker() {
    if (m_compiledFilter) {
        webkit_user_content_filter_unref(m_compiledFilter);
        m_compiledFilter = nullptr;
    }
    if (m_filterStore) {
        g_object_unref(m_filterStore);
        m_filterStore = nullptr;
    }
    // Release any queued managers
    for (auto* mgr : m_pendingManagers) {
        g_object_unref(mgr);
    }
    m_pendingManagers.clear();
}

// ---------------------------------------------------------------------------
// Core
// ---------------------------------------------------------------------------

bool BrayaAdBlocker::initialize() {
    std::cout << "🛡️  Initializing Braya Ad-Blocker (uBlock Origin filter lists)..." << std::endl;

    const char* configHome = g_get_user_config_dir();
    m_settingsPath = std::string(configHome) + "/braya-browser/adblock-settings.json";
    m_dataDir      = std::string(configHome) + "/braya-browser/filterlists";

    g_mkdir_with_parents(m_dataDir.c_str(), 0700);

    // Filter store for compiled WebKit content rules
    std::string storePath = std::string(configHome) + "/braya-browser/filterstore";
    g_mkdir_with_parents(storePath.c_str(), 0700);
    m_filterStore = webkit_user_content_filter_store_new(storePath.c_str());

    if (!loadSettings(m_settingsPath)) {
        std::cout << "  → No saved settings, using defaults" << std::endl;
        loadDefaultFilterLists();
        m_enabled = true;
    }

    if (m_enabled) {
        compileAndApplyFilters();
    }

    std::cout << "✓ Ad-Blocker initialized (status: " << (m_enabled ? "enabled" : "disabled") << ")" << std::endl;
    return true;
}

void BrayaAdBlocker::enable() {
    if (m_enabled) return;
    m_enabled = true;
    std::cout << "🛡️  Ad-Blocker ENABLED" << std::endl;
    compileAndApplyFilters();
    saveSettings(m_settingsPath);
}

void BrayaAdBlocker::disable() {
    if (!m_enabled) return;
    m_enabled = false;
    std::cout << "🛡️  Ad-Blocker DISABLED" << std::endl;
    // Remove filter from any future content managers (existing WebViews keep it
    // until they're recreated — acceptable behaviour)
    if (m_compiledFilter) {
        webkit_user_content_filter_unref(m_compiledFilter);
        m_compiledFilter = nullptr;
    }
    saveSettings(m_settingsPath);
}

// ---------------------------------------------------------------------------
// Security level / features
// ---------------------------------------------------------------------------

void BrayaAdBlocker::setSecurityLevel(SecurityLevel level) {
    m_securityLevel = level;
    applySecurityLevel();
    const char* names[] = { "Off", "Minimal", "Standard", "Strict", "Custom" };
    std::cout << "🛡️  Security level: " << names[static_cast<int>(level)] << std::endl;
    if (m_enabled) compileAndApplyFilters();
    saveSettings(m_settingsPath);
}

void BrayaAdBlocker::applySecurityLevel() {
    switch (m_securityLevel) {
        case SecurityLevel::OFF:
            m_enabled = false;
            break;
        case SecurityLevel::MINIMAL:
            m_features = { false, false, false, true, false, false, false, false };
            break;
        case SecurityLevel::STANDARD:
            m_features = { true, true, false, true, true, false, false, false };
            break;
        case SecurityLevel::STRICT:
            m_features = { true, true, true, true, true, true, true, false };
            break;
        case SecurityLevel::CUSTOM:
            break;
    }
}

void BrayaAdBlocker::setFeatures(const BlockingFeatures& features) {
    m_features = features;
    m_securityLevel = SecurityLevel::CUSTOM;
    if (m_enabled) compileAndApplyFilters();
    saveSettings(m_settingsPath);
}

void BrayaAdBlocker::setFeature(const std::string& feature, bool enabled) {
    if      (feature == "block_ads")              m_features.block_ads = enabled;
    else if (feature == "block_trackers")         m_features.block_trackers = enabled;
    else if (feature == "block_social")           m_features.block_social = enabled;
    else if (feature == "block_cryptominers")     m_features.block_cryptominers = enabled;
    else if (feature == "block_popups")           m_features.block_popups = enabled;
    else if (feature == "block_autoplay")         m_features.block_autoplay = enabled;
    else if (feature == "remove_cookie_warnings") m_features.remove_cookie_warnings = enabled;
    else if (feature == "block_nsfw")             m_features.block_nsfw = enabled;
    m_securityLevel = SecurityLevel::CUSTOM;
    if (m_enabled) compileAndApplyFilters();
    saveSettings(m_settingsPath);
}

// ---------------------------------------------------------------------------
// Whitelist
// ---------------------------------------------------------------------------

void BrayaAdBlocker::addToWhitelist(const std::string& domain) {
    m_whitelist.insert(domain);
    saveSettings(m_settingsPath);
}
void BrayaAdBlocker::removeFromWhitelist(const std::string& domain) {
    m_whitelist.erase(domain);
    saveSettings(m_settingsPath);
}
bool BrayaAdBlocker::isWhitelisted(const std::string& domain) const {
    return m_whitelist.count(domain) > 0;
}
std::vector<std::string> BrayaAdBlocker::getWhitelist() const {
    return std::vector<std::string>(m_whitelist.begin(), m_whitelist.end());
}
void BrayaAdBlocker::clearWhitelist() {
    m_whitelist.clear();
    saveSettings(m_settingsPath);
}

// ---------------------------------------------------------------------------
// Filter list management
// ---------------------------------------------------------------------------

void BrayaAdBlocker::loadDefaultFilterLists() {
    m_filterLists.clear();
    for (int i = 0; i < NUM_FILTER_LISTS; i++) {
        FilterList fl;
        fl.name        = FILTER_LISTS[i].name;
        fl.url         = FILTER_LISTS[i].url;
        fl.local_path  = m_dataDir + "/" + fl.name + ".txt";
        fl.enabled     = true;
        m_filterLists.push_back(fl);
    }
}

void BrayaAdBlocker::enableFilterList(const std::string& name, bool enabled) {
    for (auto& fl : m_filterLists) {
        if (fl.name == name) {
            fl.enabled = enabled;
            break;
        }
    }
    if (m_enabled) compileAndApplyFilters();
    saveSettings(m_settingsPath);
}

bool BrayaAdBlocker::loadFilterList(const std::string& path) {
    // Just verify the file exists; actual parsing happens in buildFilterJSON()
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

bool BrayaAdBlocker::loadFilterListFromURL(const std::string& url, const std::string& name) {
    std::string destPath = m_dataDir + "/" + name + ".txt";
    return downloadFile(url, destPath);
}

void BrayaAdBlocker::updateFilterLists() {
    ensureFilterListsDownloaded();
    if (m_enabled) compileAndApplyFilters();
}

// ---------------------------------------------------------------------------
// Custom rules
// ---------------------------------------------------------------------------

void BrayaAdBlocker::addCustomRule(const std::string& rule) {
    m_customRules.push_back(rule);
    if (m_enabled) compileAndApplyFilters();
    saveSettings(m_settingsPath);
}

void BrayaAdBlocker::removeCustomRule(const std::string& rule) {
    auto it = std::find(m_customRules.begin(), m_customRules.end(), rule);
    if (it != m_customRules.end()) {
        m_customRules.erase(it);
        if (m_enabled) compileAndApplyFilters();
        saveSettings(m_settingsPath);
    }
}

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------

void BrayaAdBlocker::incrementBlockedCount(const std::string& type) {
    m_stats.total_blocked++;
    m_stats.blocked_today++;
    if      (type == "ad")      m_stats.ads_blocked++;
    else if (type == "tracker") m_stats.trackers_blocked++;
    else if (type == "malware") m_stats.malware_blocked++;
}

void BrayaAdBlocker::resetStats() {
    m_stats = {};
    saveSettings(m_settingsPath);
}

// ---------------------------------------------------------------------------
// WebKit integration
// ---------------------------------------------------------------------------

void BrayaAdBlocker::applyToContentManager(WebKitUserContentManager* manager) {
    if (!manager) return;

    if (m_compiledFilter) {
        // Filter already compiled — apply immediately
        webkit_user_content_manager_add_filter(manager, m_compiledFilter);
        std::cout << "🛡️  Applied filter to content manager" << std::endl;
    } else if (m_enabled) {
        // Not compiled yet — queue and apply when ready
        m_pendingManagers.push_back(WEBKIT_USER_CONTENT_MANAGER(g_object_ref(manager)));
        std::cout << "🛡️  Queued content manager (filter still compiling)" << std::endl;
    }
}

// Static trampoline for the async compile callback
static void onFilterStoreSaveFinished(GObject* source, GAsyncResult* result, gpointer userData) {
    BrayaAdBlocker* self = static_cast<BrayaAdBlocker*>(userData);
    GError* error = nullptr;
    WebKitUserContentFilter* filter =
        webkit_user_content_filter_store_save_finish(
            WEBKIT_USER_CONTENT_FILTER_STORE(source), result, &error);
    if (error) {
        std::cerr << "❌ Filter store compile error: " << error->message << std::endl;
        g_error_free(error);
        return;
    }
    self->onFilterCompiled(filter);
}

void BrayaAdBlocker::onFilterCompiled(WebKitUserContentFilter* filter) {
    if (m_compiledFilter) {
        webkit_user_content_filter_unref(m_compiledFilter);
    }
    m_compiledFilter = filter; // already retained by WebKit

    std::cout << "✅ Filter compiled — applying to " << m_pendingManagers.size()
              << " queued content manager(s)" << std::endl;

    for (auto* mgr : m_pendingManagers) {
        webkit_user_content_manager_add_filter(mgr, m_compiledFilter);
        g_object_unref(mgr);
    }
    m_pendingManagers.clear();
}

// ---------------------------------------------------------------------------
// Download helpers
// ---------------------------------------------------------------------------

bool BrayaAdBlocker::needsUpdate(const std::string& path, int maxAgeDays) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return true; // file missing
    time_t now = time(nullptr);
    double ageDays = difftime(now, st.st_mtime) / 86400.0;
    return ageDays > maxAgeDays;
}

bool BrayaAdBlocker::downloadFile(const std::string& url, const std::string& destPath) {
    const char* argv[] = {
        "curl", "-s", "-L",
        "--max-time", "60",
        "--retry", "2",
        "-o", destPath.c_str(),
        url.c_str(),
        nullptr
    };
    GError* error = nullptr;
    int exit_status = 0;
    bool ok = g_spawn_sync(nullptr, (char**)argv, nullptr,
                           G_SPAWN_SEARCH_PATH,
                           nullptr, nullptr,
                           nullptr, nullptr,
                           &exit_status, &error);
    if (error) { g_error_free(error); return false; }
    if (!ok || exit_status != 0) return false;
    std::cout << "  ✓ Downloaded: " << destPath << std::endl;
    return true;
}

// Run downloads in a background thread so we don't block the UI
struct DownloadContext {
    BrayaAdBlocker* adBlocker;
};

static gpointer downloadThreadFunc(gpointer data) {
    DownloadContext* ctx = static_cast<DownloadContext*>(data);
    ctx->adBlocker->ensureFilterListsDownloaded();

    // Back on main thread: re-compile
    g_idle_add([](gpointer d) -> gboolean {
        DownloadContext* c = static_cast<DownloadContext*>(d);
        c->adBlocker->compileAndApplyFilters();
        delete c;
        return G_SOURCE_REMOVE;
    }, ctx);
    return nullptr;
}

void BrayaAdBlocker::ensureFilterListsDownloaded() {
    for (auto& fl : m_filterLists) {
        if (!fl.enabled) continue;
        if (needsUpdate(fl.local_path)) {
            std::cout << "  ⬇️  Downloading filter list: " << fl.name << std::endl;
            downloadFile(fl.url, fl.local_path);
        }
    }
}

// ---------------------------------------------------------------------------
// ABP filter parser
// ---------------------------------------------------------------------------

// Escape a string for inclusion inside a JSON string literal
static std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else                out += c;
    }
    return out;
}

// Convert a single ABP-format rule to a WebKit ContentExtensions JSON object.
// Returns "" if the rule should be skipped.
std::string BrayaAdBlocker::abpRuleToJSON(const std::string& line) {
    if (line.empty() || line[0] == '!' || line[0] == '[') return "";

    // Skip all cosmetic / element-hiding rules
    if (line.find("##")  != std::string::npos) return "";
    if (line.find("#@#") != std::string::npos) return "";
    if (line.find("#?#") != std::string::npos) return "";
    if (line.find("#$#") != std::string::npos) return "";

    // Extended syntax we don't support yet
    if (line.find("%2F") != std::string::npos) return "";  // encoded slashes in pattern
    if (line.find("$csp") != std::string::npos) return "";
    if (line.find("$redirect") != std::string::npos) return "";
    if (line.find("$rewrite") != std::string::npos) return "";

    bool isException = (line.size() >= 2 && line[0] == '@' && line[1] == '@');
    std::string rule = isException ? line.substr(2) : line;

    // --- Parse options after $ ---
    std::vector<std::string> resourceTypes;
    std::vector<std::string> loadTypes;

    size_t dollarPos = rule.rfind('$');
    if (dollarPos != std::string::npos) {
        std::string opts = rule.substr(dollarPos + 1);
        rule = rule.substr(0, dollarPos);

        // Split options by comma
        std::stringstream ss(opts);
        std::string opt;
        while (std::getline(ss, opt, ',')) {
            bool neg = !opt.empty() && opt[0] == '~';
            std::string key = neg ? opt.substr(1) : opt;

            if (key == "script")                  { if (!neg) resourceTypes.push_back("script"); }
            else if (key == "image" || key == "img") { if (!neg) resourceTypes.push_back("image"); }
            else if (key == "stylesheet" || key == "css") { if (!neg) resourceTypes.push_back("style-sheet"); }
            else if (key == "media")              { if (!neg) resourceTypes.push_back("media"); }
            else if (key == "font")               { if (!neg) resourceTypes.push_back("font"); }
            else if (key == "xmlhttprequest" || key == "xhr") { if (!neg) resourceTypes.push_back("raw"); }
            else if (key == "document" || key == "doc") { if (!neg) resourceTypes.push_back("document"); }
            else if (key == "subdocument")        { if (!neg) resourceTypes.push_back("document"); }
            else if (key == "third-party")        { loadTypes.push_back(neg ? "first-party" : "third-party"); }
            else if (key == "first-party")        { loadTypes.push_back(neg ? "third-party" : "first-party"); }
            // Skip: important, badfilter, redirect, csp, etc.
        }
    }

    if (rule.empty()) return "";

    // --- Convert ABP URL pattern to regex ---
    std::string regex;
    size_t i = 0;

    if (rule.size() >= 2 && rule[0] == '|' && rule[1] == '|') {
        // Domain anchor: match scheme + optional subdomains
        regex = "https?://([^/?#]*\\.)?";
        i = 2;
    } else if (!rule.empty() && rule[0] == '|') {
        regex = "^";
        i = 1;
    }

    for (; i < rule.size(); i++) {
        char c = rule[i];
        if (c == '|' && i == rule.size() - 1) {
            regex += '$';
        } else if (c == '^') {
            // ABP separator: any non-word char or end of URL
            regex += "([/?#]|$)";
        } else if (c == '*') {
            regex += ".*";
        } else if (c == '.' || c == '+' || c == '?' ||
                   c == '[' || c == ']' || c == '{' || c == '}' ||
                   c == '(' || c == ')' || c == '\\') {
            regex += '\\';
            regex += c;
        } else {
            regex += c;
        }
    }

    // Skip patterns that are too short or too broad
    if (regex.empty() || regex == ".*" || regex == "^" || regex == "$") return "";
    if (regex.size() < 4) return "";

    // --- Build JSON object ---
    std::string json = "{\"trigger\":{\"url-filter\":\"";
    json += jsonEscape(regex);
    json += "\"";

    if (!resourceTypes.empty()) {
        json += ",\"resource-type\":[";
        for (size_t j = 0; j < resourceTypes.size(); j++) {
            if (j) json += ",";
            json += "\"" + resourceTypes[j] + "\"";
        }
        json += "]";
    }

    if (!loadTypes.empty()) {
        json += ",\"load-type\":[";
        for (size_t j = 0; j < loadTypes.size(); j++) {
            if (j) json += ",";
            json += "\"" + loadTypes[j] + "\"";
        }
        json += "]";
    }

    json += "},\"action\":{\"type\":\"";
    json += isException ? "ignore-previous-rules" : "block";
    json += "\"}}";

    return json;
}

void BrayaAdBlocker::parseAbpFile(const std::string& path, std::vector<std::string>& rules) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "  ⚠️  Cannot open filter list: " << path << std::endl;
        return;
    }

    int parsed = 0, skipped = 0;
    std::string line;
    while (std::getline(file, line) && (int)rules.size() < MAX_RULES) {
        // Strip \r
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::string json = abpRuleToJSON(line);
        if (!json.empty()) {
            rules.push_back(std::move(json));
            parsed++;
        } else {
            skipped++;
        }
    }
    std::cout << "  📄 " << path << ": " << parsed << " rules ("
              << skipped << " skipped)" << std::endl;
}

std::string BrayaAdBlocker::buildFilterJSON() {
    std::vector<std::string> rules;
    rules.reserve(50000);

    // Parse enabled filter lists
    for (const auto& fl : m_filterLists) {
        if (!fl.enabled) continue;
        if ((int)rules.size() >= MAX_RULES) break;
        parseAbpFile(fl.local_path, rules);
    }

    // Append custom rules
    for (const auto& r : m_customRules) {
        std::string json = abpRuleToJSON(r);
        if (!json.empty()) rules.push_back(std::move(json));
    }

    std::cout << "  📊 Total rules to compile: " << rules.size() << std::endl;

    // Build JSON array
    std::string json;
    json.reserve(rules.size() * 80);
    json += "[";
    for (size_t i = 0; i < rules.size(); i++) {
        if (i) json += ",";
        json += rules[i];
    }
    json += "]";
    return json;
}

// ---------------------------------------------------------------------------
// Compilation via WebKitUserContentFilterStore
// ---------------------------------------------------------------------------

struct CompileContext {
    BrayaAdBlocker* adBlocker;
    std::string*    json;  // heap-allocated so the thread can own it
};

static gpointer buildJSONThread(gpointer data) {
    CompileContext* ctx = static_cast<CompileContext*>(data);

    // Check if any filter list file exists; if not, kick off a download first
    bool anyExists = false;
    // We'll trigger compile from the main thread idle callback below regardless
    // (ensureFilterListsDownloaded will have been called separately)

    ctx->adBlocker->ensureFilterListsDownloaded();
    *ctx->json = ctx->adBlocker->buildFilterJSON();

    // Hand back to the main thread to do the WebKit API calls
    g_idle_add([](gpointer d) -> gboolean {
        CompileContext* c = static_cast<CompileContext*>(d);

        if (c->json->empty() || *c->json == "[]") {
            std::cout << "  ⚠️  No filter rules available" << std::endl;
            delete c->json;
            delete c;
            return G_SOURCE_REMOVE;
        }

        GBytes* bytes = g_bytes_new(c->json->data(), c->json->size());
        webkit_user_content_filter_store_save(
            c->adBlocker->m_filterStore,
            "braya-adblock",
            bytes,
            nullptr,
            onFilterStoreSaveFinished,
            c->adBlocker
        );
        g_bytes_unref(bytes);

        delete c->json;
        delete c;
        return G_SOURCE_REMOVE;
    }, ctx);

    return nullptr;
}

void BrayaAdBlocker::compileAndApplyFilters() {
    if (!m_filterStore) return;

    std::cout << "🛡️  Starting filter compilation (background)..." << std::endl;

    auto* ctx = new CompileContext;
    ctx->adBlocker = this;
    ctx->json      = new std::string();

    g_thread_new("braya-adblock-compile", buildJSONThread, ctx);
}

// ---------------------------------------------------------------------------
// Settings persistence
// ---------------------------------------------------------------------------

bool BrayaAdBlocker::loadSettings(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;

    GError* error = nullptr;
    JsonParser* parser = json_parser_new();
    if (!json_parser_load_from_file(parser, path.c_str(), &error)) {
        if (error) { std::cerr << "adblock settings: " << error->message << std::endl; g_error_free(error); }
        g_object_unref(parser);
        return false;
    }

    JsonNode* root = json_parser_get_root(parser);
    if (!root || !JSON_NODE_HOLDS_OBJECT(root)) { g_object_unref(parser); return false; }
    JsonObject* obj = json_node_get_object(root);

    if (json_object_has_member(obj, "enabled"))
        m_enabled = json_object_get_boolean_member(obj, "enabled");

    if (json_object_has_member(obj, "security_level")) {
        const char* lvl = json_object_get_string_member(obj, "security_level");
        if      (!strcmp(lvl, "off"))     m_securityLevel = SecurityLevel::OFF;
        else if (!strcmp(lvl, "minimal")) m_securityLevel = SecurityLevel::MINIMAL;
        else if (!strcmp(lvl, "standard"))m_securityLevel = SecurityLevel::STANDARD;
        else if (!strcmp(lvl, "strict"))  m_securityLevel = SecurityLevel::STRICT;
        else if (!strcmp(lvl, "custom"))  m_securityLevel = SecurityLevel::CUSTOM;
    }

    if (json_object_has_member(obj, "whitelist")) {
        JsonArray* wl = json_object_get_array_member(obj, "whitelist");
        for (guint i = 0; i < json_array_get_length(wl); i++)
            m_whitelist.insert(json_array_get_string_element(wl, i));
    }

    if (json_object_has_member(obj, "stats")) {
        JsonObject* stats = json_object_get_object_member(obj, "stats");
        if (json_object_has_member(stats, "total_blocked"))
            m_stats.total_blocked = (int)json_object_get_int_member(stats, "total_blocked");
        if (json_object_has_member(stats, "blocked_today"))
            m_stats.blocked_today = (int)json_object_get_int_member(stats, "blocked_today");
    }

    // Rebuild filter list paths from defaults (URLs may change between versions)
    loadDefaultFilterLists();

    g_object_unref(parser);
    std::cout << "  ✓ Loaded ad-blocker settings" << std::endl;
    return true;
}

bool BrayaAdBlocker::saveSettings(const std::string& path) {
    // Ensure parent directory exists
    std::string dir = path.substr(0, path.rfind('/'));
    g_mkdir_with_parents(dir.c_str(), 0700);

    JsonBuilder* b = json_builder_new();
    json_builder_begin_object(b);

    json_builder_set_member_name(b, "enabled");
    json_builder_add_boolean_value(b, m_enabled);

    const char* lvlNames[] = { "off", "minimal", "standard", "strict", "custom" };
    json_builder_set_member_name(b, "security_level");
    json_builder_add_string_value(b, lvlNames[static_cast<int>(m_securityLevel)]);

    json_builder_set_member_name(b, "features");
    json_builder_begin_object(b);
    #define BOOL_MEMBER(n, v) json_builder_set_member_name(b, n); json_builder_add_boolean_value(b, v);
    BOOL_MEMBER("block_ads",               m_features.block_ads)
    BOOL_MEMBER("block_trackers",          m_features.block_trackers)
    BOOL_MEMBER("block_social",            m_features.block_social)
    BOOL_MEMBER("block_cryptominers",      m_features.block_cryptominers)
    BOOL_MEMBER("block_popups",            m_features.block_popups)
    BOOL_MEMBER("block_autoplay",          m_features.block_autoplay)
    BOOL_MEMBER("remove_cookie_warnings",  m_features.remove_cookie_warnings)
    #undef BOOL_MEMBER
    json_builder_end_object(b);

    json_builder_set_member_name(b, "whitelist");
    json_builder_begin_array(b);
    for (const auto& d : m_whitelist) json_builder_add_string_value(b, d.c_str());
    json_builder_end_array(b);

    json_builder_set_member_name(b, "stats");
    json_builder_begin_object(b);
    json_builder_set_member_name(b, "total_blocked"); json_builder_add_int_value(b, m_stats.total_blocked);
    json_builder_set_member_name(b, "blocked_today"); json_builder_add_int_value(b, m_stats.blocked_today);
    json_builder_end_object(b);

    json_builder_end_object(b);

    JsonGenerator* gen = json_generator_new();
    JsonNode* root = json_builder_get_root(b);
    json_generator_set_root(gen, root);
    json_generator_set_pretty(gen, TRUE);
    gchar* data = json_generator_to_data(gen, nullptr);

    std::ofstream f(path);
    if (f.is_open()) { f << data; f.close(); }

    g_free(data);
    json_node_free(root);
    g_object_unref(gen);
    g_object_unref(b);
    return true;
}
