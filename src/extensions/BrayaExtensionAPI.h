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

    // Get chrome.webRequest API implementation
    static std::string getWebRequestAPI();

    // Get chrome.webNavigation API implementation
    static std::string getWebNavigationAPI();

    // Get chrome.contextMenus API implementation
    static std::string getContextMenusAPI();

    // Get chrome.windows API implementation
    static std::string getWindowsAPI();

    // Get chrome.cookies API implementation
    static std::string getCookiesAPI();

    // Get chrome.notifications API implementation
    static std::string getNotificationsAPI();

    // Get chrome.alarms API implementation
    static std::string getAlarmsAPI();

    // Get chrome.browserAction / chrome.action API implementation
    static std::string getBrowserActionAPI();

    // Get chrome.commands API implementation
    static std::string getCommandsAPI();

    // Get chrome.history API implementation
    static std::string getHistoryAPI();

    // Get chrome.permissions API implementation
    static std::string getPermissionsAPI();

    // Get chrome.idle API implementation
    static std::string getIdleAPI();

    // Get chrome.privacy API implementation
    static std::string getPrivacyAPI();

    // Get clipboard API implementation (navigator.clipboard)
    static std::string getClipboardAPI();

    // Get chrome.runtime.connectNative / sendNativeMessage for native messaging
    static std::string getNativeMessagingAPI();

    // Get chrome.offscreen API implementation (Manifest V3)
    static std::string getOffscreenAPI();

    // Get chrome.scripting API implementation (Manifest V3)
    static std::string getScriptingAPI();
};

#endif // BRAYA_EXTENSION_API_H
