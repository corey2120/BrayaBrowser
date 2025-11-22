#ifndef BRAYA_ADBLOCKER_H
#define BRAYA_ADBLOCKER_H

#include <string>
#include <vector>
#include <set>
#include <memory>
#include <webkit/webkit.h>

// Security levels for ad-blocking
enum class SecurityLevel {
    OFF,        // No blocking
    MINIMAL,    // Only malware/phishing
    STANDARD,   // Ads + trackers (recommended)
    STRICT,     // Maximum blocking
    CUSTOM      // User-defined rules
};

// Types of content that can be blocked
struct BlockingFeatures {
    bool block_ads = true;
    bool block_trackers = true;
    bool block_social = false;
    bool block_cryptominers = true;
    bool block_popups = true;
    bool block_autoplay = true;
    bool remove_cookie_warnings = false;
    bool block_nsfw = false;
};

// Statistics about blocked content
struct BlockingStats {
    int total_blocked = 0;
    int blocked_today = 0;
    int ads_blocked = 0;
    int trackers_blocked = 0;
    int malware_blocked = 0;
};

// Filter list information
struct FilterList {
    std::string name;
    std::string url;
    std::string local_path;
    bool enabled;
    std::string last_updated;
};

// Main ad-blocker class
class BrayaAdBlocker {
public:
    BrayaAdBlocker();
    ~BrayaAdBlocker();

    // Core functionality
    bool initialize();
    void enable();
    void disable();
    bool isEnabled() const { return m_enabled; }

    // Security level management
    void setSecurityLevel(SecurityLevel level);
    SecurityLevel getSecurityLevel() const { return m_securityLevel; }

    // Feature toggles
    void setFeatures(const BlockingFeatures& features);
    BlockingFeatures getFeatures() const { return m_features; }
    void setFeature(const std::string& feature, bool enabled);

    // Whitelist management
    void addToWhitelist(const std::string& domain);
    void removeFromWhitelist(const std::string& domain);
    bool isWhitelisted(const std::string& domain) const;
    std::vector<std::string> getWhitelist() const;
    void clearWhitelist();

    // Filter list management
    bool loadFilterList(const std::string& path);
    bool loadFilterListFromURL(const std::string& url, const std::string& name);
    void updateFilterLists();
    std::vector<FilterList> getFilterLists() const { return m_filterLists; }
    void enableFilterList(const std::string& name, bool enabled);

    // Custom rules
    void addCustomRule(const std::string& rule);
    void removeCustomRule(const std::string& rule);
    std::vector<std::string> getCustomRules() const { return m_customRules; }

    // Blocking logic
    bool shouldBlock(const std::string& url, const std::string& domain) const;
    bool shouldBlockResource(const std::string& url, const std::string& type) const;

    // Statistics
    BlockingStats getStats() const { return m_stats; }
    void incrementBlockedCount(const std::string& type = "");
    void resetStats();

    // WebKit integration
    void applyToContentManager(WebKitUserContentManager* manager);
    void compileRules();

    // Settings persistence
    bool loadSettings(const std::string& path);
    bool saveSettings(const std::string& path);

private:
    bool m_enabled;
    SecurityLevel m_securityLevel;
    BlockingFeatures m_features;
    BlockingStats m_stats;

    std::set<std::string> m_whitelist;
    std::vector<FilterList> m_filterLists;
    std::vector<std::string> m_customRules;
    std::vector<std::string> m_compiledRules;  // Compiled filter rules

    WebKitUserContentFilter* m_contentFilter;
    WebKitUserContentFilterStore* m_filterStore;
    bool m_filterCompiled;
    std::string m_settingsPath;

    // Helper methods
    void loadDefaultFilterLists();
    void applySecurityLevel();
    bool parseFilterRule(const std::string& rule);
    std::string compileRuleToJSON(const std::string& rule);
    bool matchesDomain(const std::string& url, const std::string& pattern) const;
};

#endif // BRAYA_ADBLOCKER_H
