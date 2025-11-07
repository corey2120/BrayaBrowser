#ifndef BRAYA_EXTENSION_API_H
#define BRAYA_EXTENSION_API_H

#include <string>

// Provides JavaScript implementations of Chrome Extension APIs
// These are injected into background pages and content scripts
class BrayaExtensionAPI {
public:
    // Get the complete chrome.* API stub for background pages
    static std::string getBackgroundPageAPI();

    // Get the chrome.* API stub for content scripts (limited API)
    static std::string getContentScriptAPI();

    // Get chrome.tabs API implementation
    static std::string getTabsAPI();

    // Get chrome.storage API implementation
    static std::string getStorageAPI();

    // Get chrome.runtime API implementation
    static std::string getRuntimeAPI();

    // Get chrome.bookmarks API implementation
    static std::string getBookmarksAPI();

    // Get chrome.downloads API implementation
    static std::string getDownloadsAPI();

    // Get chrome.i18n API implementation
    static std::string getI18nAPI();
};

#endif // BRAYA_EXTENSION_API_H
