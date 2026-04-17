#ifndef BRAYA_ADBLOCKER_H
#define BRAYA_ADBLOCKER_H

#include <string>
#include <vector>
#include <set>
#include <memory>
#include <functional>
#include <webkit/webkit.h>

enum class SecurityLevel {
    OFF,
    MINIMAL,
    STANDARD,
    STRICT,
    CUSTOM
};

struct BlockingFeatures {
    bool block_ads           = true;
    bool block_trackers      = true;
    bool block_social        = false;
    bool block_cryptominers  = true;
    bool block_popups        = true;
    bool block_autoplay      = true;
    bool remove_cookie_warnings = false;
    bool block_nsfw          = false;
};

struct BlockingStats {
    int total_blocked   = 0;
    int blocked_today   = 0;
    int ads_blocked     = 0;
    int trackers_blocked = 0;
    int malware_blocked = 0;
};

struct FilterList {
    std::string name;
    std::string url;
    std::string local_path;
    bool        enabled;
    std::string last_updated;
};

class BrayaAdBlocker {
public:
    BrayaAdBlocker();
    ~BrayaAdBlocker();

    // Core
    bool initialize();
    void enable();
    void disable();
    bool isEnabled() const { return m_enabled; }

    // Security level
    void          setSecurityLevel(SecurityLevel level);
    SecurityLevel getSecurityLevel() const { return m_securityLevel; }

    // Feature toggles
    void             setFeatures(const BlockingFeatures& features);
    BlockingFeatures getFeatures() const { return m_features; }
    void             setFeature(const std::string& feature, bool enabled);

    // Whitelist
    void                     addToWhitelist(const std::string& domain);
    void                     removeFromWhitelist(const std::string& domain);
    bool                     isWhitelisted(const std::string& domain) const;
    std::vector<std::string> getWhitelist() const;
    void                     clearWhitelist();

    // Filter lists
    bool                     loadFilterList(const std::string& path);
    bool                     loadFilterListFromURL(const std::string& url, const std::string& name);
    void                     updateFilterLists();
    std::vector<FilterList>  getFilterLists() const { return m_filterLists; }
    void                     enableFilterList(const std::string& name, bool enabled);

    // Custom rules
    void                     addCustomRule(const std::string& rule);
    void                     removeCustomRule(const std::string& rule);
    std::vector<std::string> getCustomRules() const { return m_customRules; }

    // Statistics
    BlockingStats getStats() const { return m_stats; }
    void          incrementBlockedCount(const std::string& type = "");
    void          resetStats();

    // WebKit integration — apply compiled filter to a content manager.
    // If the filter isn't compiled yet the manager is queued and applied
    // automatically once compilation finishes.
    void applyToContentManager(WebKitUserContentManager* manager);

    // Settings persistence
    bool loadSettings(const std::string& path);
    bool saveSettings(const std::string& path);

    // Called internally when the WebKit filter store finishes compiling
    void onFilterCompiled(WebKitUserContentFilter* filter);

    // These are called from static thread functions — must be public
    void        ensureFilterListsDownloaded();
    std::string buildFilterJSON();
    void        compileAndApplyFilters();

    WebKitUserContentFilterStore* m_filterStore;

private:
    bool             m_enabled;
    SecurityLevel    m_securityLevel;
    BlockingFeatures m_features;
    BlockingStats    m_stats;

    std::set<std::string>    m_whitelist;
    std::vector<FilterList>  m_filterLists;
    std::vector<std::string> m_customRules;
    std::string              m_settingsPath;
    std::string              m_dataDir;   // ~/.config/braya-browser/filterlists

    // WebKit compiled filter (set after async compilation finishes)
    WebKitUserContentFilter*      m_compiledFilter;

    // Content managers waiting for the filter to finish compiling
    std::vector<WebKitUserContentManager*> m_pendingManagers;

    void loadDefaultFilterLists();
    void applySecurityLevel();
    bool downloadFile(const std::string& url, const std::string& destPath);
    bool needsUpdate(const std::string& path, int maxAgeDays = 7);
    void parseAbpFile(const std::string& path, std::vector<std::string>& rules);
    std::string abpRuleToJSON(const std::string& line);
};

#endif // BRAYA_ADBLOCKER_H
