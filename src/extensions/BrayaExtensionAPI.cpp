#include "BrayaExtensionAPI.h"

std::string BrayaExtensionAPI::getRuntimeAPI() {
    return R"(
    // chrome.runtime API
    chrome.runtime = chrome.runtime || {};
    chrome.runtime.id = 'braya-extension';

    chrome.runtime.getManifest = function() {
        return window.__extensionManifest || {};
    };

    chrome.runtime.getURL = function(path) {
        return 'chrome-extension://' + chrome.runtime.id + '/' + path;
    };

    // Message passing
    chrome.runtime.onMessage = {
        addListener: function(callback) {
            console.log('[chrome.runtime.onMessage] listener registered');
            window.__messageListeners = window.__messageListeners || [];
            window.__messageListeners.push(callback);
        }
    };

    // Message callback storage
    window.__messageCallbacks = window.__messageCallbacks || {};
    window.__messageCallbackId = window.__messageCallbackId || 0;

    // Handler for responses from background page
    window.__handleExtensionMessageResponse = function(callbackId, response) {
        const callback = window.__messageCallbacks[callbackId];
        if (callback) {
            callback(response);
            delete window.__messageCallbacks[callbackId];
        }
    };

    chrome.runtime.sendMessage = function(extensionId, message, options, callback) {
        // Handle overloaded parameters
        if (typeof extensionId === 'object') {
            callback = message;
            message = extensionId;
            extensionId = null;
        } else if (typeof options === 'function') {
            callback = options;
            options = null;
        }

        console.log('[chrome.runtime.sendMessage]', message);

        // Use WebKit message handler for real IPC if available
        if (window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.extensionMessage) {
            const callbackId = window.__messageCallbackId++;
            if (callback) {
                window.__messageCallbacks[callbackId] = callback;
            }

            window.webkit.messageHandlers.extensionMessage.postMessage({
                type: 'runtimeSendMessage',
                extensionId: extensionId || chrome.runtime.id,
                message: message,
                callbackId: callbackId
            });
        } else {
            // Fallback to mock response
            if (callback) {
                setTimeout(() => callback({status: 'ok'}), 10);
            }
        }
    };

    // Event listener arrays for runtime events
    window.__runtimeOnInstalledListeners = window.__runtimeOnInstalledListeners || [];
    window.__runtimeOnConnectListeners = window.__runtimeOnConnectListeners || [];
    window.__runtimeOnConnectExternalListeners = window.__runtimeOnConnectExternalListeners || [];

    chrome.runtime.onInstalled = {
        addListener: function(callback) {
            console.log('[chrome.runtime.onInstalled] listener registered');
            window.__runtimeOnInstalledListeners.push(callback);
            // Call immediately for testing
            setTimeout(() => callback({reason: 'install'}), 100);
        },
        removeListener: function(callback) {
            const index = window.__runtimeOnInstalledListeners.indexOf(callback);
            if (index > -1) window.__runtimeOnInstalledListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__runtimeOnInstalledListeners.includes(callback);
        }
    };

    chrome.runtime.onConnect = {
        addListener: function(callback) {
            console.log('[chrome.runtime.onConnect] listener registered');
            window.__runtimeOnConnectListeners.push(callback);
        },
        removeListener: function(callback) {
            const index = window.__runtimeOnConnectListeners.indexOf(callback);
            if (index > -1) window.__runtimeOnConnectListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__runtimeOnConnectListeners.includes(callback);
        }
    };

    chrome.runtime.onConnectExternal = {
        addListener: function(callback) {
            console.log('[chrome.runtime.onConnectExternal] listener registered');
            window.__runtimeOnConnectExternalListeners.push(callback);
        },
        removeListener: function(callback) {
            const index = window.__runtimeOnConnectExternalListeners.indexOf(callback);
            if (index > -1) window.__runtimeOnConnectExternalListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__runtimeOnConnectExternalListeners.includes(callback);
        }
    };

    chrome.runtime.connect = function(extensionId, connectInfo) {
        console.log('[chrome.runtime.connect]', extensionId, connectInfo);
        // Return a mock port object
        return {
            name: connectInfo?.name || '',
            disconnect: function() {},
            postMessage: function(message) {
                console.log('[port.postMessage]', message);
            },
            onDisconnect: { addListener: function() {}, removeListener: function() {}, hasListener: function() { return false; } },
            onMessage: { addListener: function() {}, removeListener: function() {}, hasListener: function() { return false; } }
        };
    };

    chrome.runtime.lastError = null;

    // Legacy chrome.extension API (for backwards compatibility)
    chrome.extension = chrome.extension || {};
    chrome.extension.getBackgroundPage = function() {
        console.log('[chrome.extension.getBackgroundPage] called');
        // Return window for background context, null for content scripts/popups
        if (typeof vAPI !== 'undefined' && vAPI.uBO) {
            return window; // We're in a background page
        }
        return null; // Content script or popup
    };
    chrome.extension.getURL = function(path) {
        return chrome.runtime.getURL(path);
    };
    )";
}

std::string BrayaExtensionAPI::getTabsAPI() {
    return R"(
    // chrome.tabs API
    chrome.tabs = chrome.tabs || {};

    // Tab information storage
    window.__brayaTabs = window.__brayaTabs || [{
        id: 1,
        index: 0,
        windowId: 1,
        active: true,
        url: 'about:blank',
        title: 'New Tab',
        status: 'complete'
    }];

    chrome.tabs.query = function(queryInfo, callback) {
        console.log('[chrome.tabs.query]', queryInfo);
        let results = window.__brayaTabs;

        if (queryInfo.active !== undefined) {
            results = results.filter(t => t.active === queryInfo.active);
        }
        if (queryInfo.currentWindow !== undefined) {
            results = results.filter(t => t.windowId === 1);
        }

        if (callback) {
            setTimeout(() => callback(results), 10);
        }
    };

    chrome.tabs.get = function(tabId, callback) {
        console.log('[chrome.tabs.get]', tabId);
        const tab = window.__brayaTabs.find(t => t.id === tabId) || window.__brayaTabs[0];
        if (callback) {
            setTimeout(() => callback(tab), 10);
        }
    };

    chrome.tabs.getCurrent = function(callback) {
        console.log('[chrome.tabs.getCurrent]');
        if (callback) {
            setTimeout(() => callback(window.__brayaTabs[0]), 10);
        }
    };

    chrome.tabs.create = function(createProperties, callback) {
        console.log('[chrome.tabs.create]', createProperties);
        const newTab = {
            id: window.__brayaTabs.length + 1,
            index: window.__brayaTabs.length,
            windowId: 1,
            active: createProperties.active !== false,
            url: createProperties.url || 'about:blank',
            title: 'New Tab',
            status: 'loading'
        };
        window.__brayaTabs.push(newTab);
        if (callback) {
            setTimeout(() => callback(newTab), 10);
        }
    };

    chrome.tabs.update = function(tabId, updateProperties, callback) {
        console.log('[chrome.tabs.update]', tabId, updateProperties);
        const tab = window.__brayaTabs.find(t => t.id === tabId);
        if (tab && updateProperties.url) {
            tab.url = updateProperties.url;
            tab.status = 'loading';
        }
        if (callback) {
            setTimeout(() => callback(tab || window.__brayaTabs[0]), 10);
        }
    };

    chrome.tabs.remove = function(tabIds, callback) {
        console.log('[chrome.tabs.remove]', tabIds);
        const ids = Array.isArray(tabIds) ? tabIds : [tabIds];
        window.__brayaTabs = window.__brayaTabs.filter(t => !ids.includes(t.id));
        if (callback) {
            setTimeout(() => callback(), 10);
        }
    };

    chrome.tabs.sendMessage = function(tabId, message, callback) {
        console.log('[chrome.tabs.sendMessage]', tabId, message);
        if (callback) {
            setTimeout(() => callback({status: 'ok'}), 10);
        }
    };

    // Event listener arrays
    window.__tabsOnCreatedListeners = window.__tabsOnCreatedListeners || [];
    window.__tabsOnUpdatedListeners = window.__tabsOnUpdatedListeners || [];
    window.__tabsOnRemovedListeners = window.__tabsOnRemovedListeners || [];
    window.__tabsOnActivatedListeners = window.__tabsOnActivatedListeners || [];

    chrome.tabs.onCreated = {
        addListener: function(callback) {
            console.log('[chrome.tabs.onCreated] listener registered');
            window.__tabsOnCreatedListeners.push(callback);
        },
        removeListener: function(callback) {
            const index = window.__tabsOnCreatedListeners.indexOf(callback);
            if (index > -1) window.__tabsOnCreatedListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__tabsOnCreatedListeners.includes(callback);
        }
    };

    chrome.tabs.onUpdated = {
        addListener: function(callback) {
            console.log('[chrome.tabs.onUpdated] listener registered');
            window.__tabsOnUpdatedListeners.push(callback);
        },
        removeListener: function(callback) {
            const index = window.__tabsOnUpdatedListeners.indexOf(callback);
            if (index > -1) window.__tabsOnUpdatedListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__tabsOnUpdatedListeners.includes(callback);
        }
    };

    chrome.tabs.onRemoved = {
        addListener: function(callback) {
            console.log('[chrome.tabs.onRemoved] listener registered');
            window.__tabsOnRemovedListeners.push(callback);
        },
        removeListener: function(callback) {
            const index = window.__tabsOnRemovedListeners.indexOf(callback);
            if (index > -1) window.__tabsOnRemovedListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__tabsOnRemovedListeners.includes(callback);
        }
    };

    chrome.tabs.onActivated = {
        addListener: function(callback) {
            console.log('[chrome.tabs.onActivated] listener registered');
            window.__tabsOnActivatedListeners.push(callback);
        },
        removeListener: function(callback) {
            const index = window.__tabsOnActivatedListeners.indexOf(callback);
            if (index > -1) window.__tabsOnActivatedListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__tabsOnActivatedListeners.includes(callback);
        }
    };
    )";
}

std::string BrayaExtensionAPI::getStorageAPI() {
    return R"(
    // chrome.storage API
    chrome.storage = chrome.storage || {};

    // In-memory storage (would be persisted in real implementation)
    window.__brayaStorage = window.__brayaStorage || {};

    chrome.storage.local = {
        get: function(keys, callback) {
            console.log('[chrome.storage.local.get]', keys);
            let result = {};

            if (typeof keys === 'string') {
                keys = [keys];
            } else if (keys === null || keys === undefined) {
                result = {...window.__brayaStorage};
            } else if (typeof keys === 'object' && !Array.isArray(keys)) {
                // Object with defaults
                for (let key in keys) {
                    result[key] = window.__brayaStorage[key] !== undefined ?
                                  window.__brayaStorage[key] : keys[key];
                }
            }

            if (Array.isArray(keys)) {
                keys.forEach(key => {
                    if (window.__brayaStorage[key] !== undefined) {
                        result[key] = window.__brayaStorage[key];
                    }
                });
            }

            if (callback) {
                setTimeout(() => callback(result), 10);
            }
        },

        set: function(items, callback) {
            console.log('[chrome.storage.local.set]', items);
            for (let key in items) {
                window.__brayaStorage[key] = items[key];
            }
            if (callback) {
                setTimeout(() => callback(), 10);
            }
        },

        remove: function(keys, callback) {
            console.log('[chrome.storage.local.remove]', keys);
            if (typeof keys === 'string') {
                keys = [keys];
            }
            keys.forEach(key => {
                delete window.__brayaStorage[key];
            });
            if (callback) {
                setTimeout(() => callback(), 10);
            }
        },

        clear: function(callback) {
            console.log('[chrome.storage.local.clear]');
            window.__brayaStorage = {};
            if (callback) {
                setTimeout(() => callback(), 10);
            }
        }
    };

    chrome.storage.sync = chrome.storage.local; // Use same implementation for now

    // chrome.storage.onChanged event system
    window.__storageChangeListeners = window.__storageChangeListeners || [];

    chrome.storage.onChanged = {
        addListener: function(callback) {
            console.log('[chrome.storage.onChanged] listener registered');
            window.__storageChangeListeners.push(callback);
        },
        removeListener: function(callback) {
            const index = window.__storageChangeListeners.indexOf(callback);
            if (index > -1) {
                window.__storageChangeListeners.splice(index, 1);
            }
        },
        hasListener: function(callback) {
            return window.__storageChangeListeners.includes(callback);
        }
    };
    )";
}

std::string BrayaExtensionAPI::getBookmarksAPI() {
    return R"(
    // chrome.bookmarks API (basic stub)
    chrome.bookmarks = chrome.bookmarks || {};

    chrome.bookmarks.get = function(idOrArray, callback) {
        console.log('[chrome.bookmarks.get]', idOrArray);
        if (callback) {
            setTimeout(() => callback([]), 10);
        }
    };

    chrome.bookmarks.getTree = function(callback) {
        console.log('[chrome.bookmarks.getTree]');
        if (callback) {
            setTimeout(() => callback([{id: '1', title: 'Bookmarks Bar', children: []}]), 10);
        }
    };

    chrome.bookmarks.create = function(bookmark, callback) {
        console.log('[chrome.bookmarks.create]', bookmark);
        if (callback) {
            setTimeout(() => callback({id: String(Date.now()), ...bookmark}), 10);
        }
    };
    )";
}

std::string BrayaExtensionAPI::getDownloadsAPI() {
    return R"(
    // chrome.downloads API (basic stub)
    chrome.downloads = chrome.downloads || {};

    chrome.downloads.download = function(options, callback) {
        console.log('[chrome.downloads.download]', options);
        if (callback) {
            setTimeout(() => callback(Date.now()), 10);
        }
    };
    )";
}

std::string BrayaExtensionAPI::getI18nAPI() {
    return R"(
    // chrome.i18n API (internationalization)
    chrome.i18n = chrome.i18n || {};

    // Store for messages (would be loaded from _locales in real implementation)
    window.__i18nMessages = window.__i18nMessages || {};

    chrome.i18n.getMessage = function(messageName, substitutions) {
        console.log('[chrome.i18n.getMessage]', messageName);

        // Return stored message or fallback to message name
        let message = window.__i18nMessages[messageName] || messageName;

        // Handle substitutions if provided
        if (substitutions) {
            if (!Array.isArray(substitutions)) {
                substitutions = [substitutions];
            }
            substitutions.forEach((sub, i) => {
                message = message.replace('$' + (i + 1), sub);
                message = message.replace('$' + (i + 1).toString(), sub);
            });
        }

        return message;
    };

    chrome.i18n.getUILanguage = function() {
        return navigator.language || 'en-US';
    };

    chrome.i18n.getAcceptLanguages = function(callback) {
        if (callback) {
            setTimeout(() => callback([navigator.language || 'en-US']), 10);
        }
    };
    )";
}

std::string BrayaExtensionAPI::getWebRequestAPI() {
    return R"(
    // chrome.webRequest API (HTTP request interception)
    chrome.webRequest = chrome.webRequest || {};

    // Event listener arrays
    window.__webRequestOnBeforeRequestListeners = window.__webRequestOnBeforeRequestListeners || [];
    window.__webRequestOnBeforeSendHeadersListeners = window.__webRequestOnBeforeSendHeadersListeners || [];
    window.__webRequestOnSendHeadersListeners = window.__webRequestOnSendHeadersListeners || [];
    window.__webRequestOnHeadersReceivedListeners = window.__webRequestOnHeadersReceivedListeners || [];
    window.__webRequestOnCompletedListeners = window.__webRequestOnCompletedListeners || [];
    window.__webRequestOnErrorOccurredListeners = window.__webRequestOnErrorOccurredListeners || [];
    window.__webRequestOnAuthRequiredListeners = window.__webRequestOnAuthRequiredListeners || [];

    chrome.webRequest.onBeforeRequest = {
        addListener: function(callback, filter, extraInfoSpec) {
            console.log('[chrome.webRequest.onBeforeRequest] listener registered');
            window.__webRequestOnBeforeRequestListeners.push({callback, filter, extraInfoSpec});
        },
        removeListener: function(callback) {
            const index = window.__webRequestOnBeforeRequestListeners.findIndex(l => l.callback === callback);
            if (index > -1) window.__webRequestOnBeforeRequestListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__webRequestOnBeforeRequestListeners.some(l => l.callback === callback);
        }
    };

    chrome.webRequest.onBeforeSendHeaders = {
        addListener: function(callback, filter, extraInfoSpec) {
            console.log('[chrome.webRequest.onBeforeSendHeaders] listener registered');
            window.__webRequestOnBeforeSendHeadersListeners.push({callback, filter, extraInfoSpec});
        },
        removeListener: function(callback) {
            const index = window.__webRequestOnBeforeSendHeadersListeners.findIndex(l => l.callback === callback);
            if (index > -1) window.__webRequestOnBeforeSendHeadersListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__webRequestOnBeforeSendHeadersListeners.some(l => l.callback === callback);
        }
    };

    chrome.webRequest.onSendHeaders = {
        addListener: function(callback, filter, extraInfoSpec) {
            console.log('[chrome.webRequest.onSendHeaders] listener registered');
            window.__webRequestOnSendHeadersListeners.push({callback, filter, extraInfoSpec});
        },
        removeListener: function(callback) {
            const index = window.__webRequestOnSendHeadersListeners.findIndex(l => l.callback === callback);
            if (index > -1) window.__webRequestOnSendHeadersListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__webRequestOnSendHeadersListeners.some(l => l.callback === callback);
        }
    };

    chrome.webRequest.onHeadersReceived = {
        addListener: function(callback, filter, extraInfoSpec) {
            console.log('[chrome.webRequest.onHeadersReceived] listener registered');
            window.__webRequestOnHeadersReceivedListeners.push({callback, filter, extraInfoSpec});
        },
        removeListener: function(callback) {
            const index = window.__webRequestOnHeadersReceivedListeners.findIndex(l => l.callback === callback);
            if (index > -1) window.__webRequestOnHeadersReceivedListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__webRequestOnHeadersReceivedListeners.some(l => l.callback === callback);
        }
    };

    chrome.webRequest.onCompleted = {
        addListener: function(callback, filter, extraInfoSpec) {
            console.log('[chrome.webRequest.onCompleted] listener registered');
            window.__webRequestOnCompletedListeners.push({callback, filter, extraInfoSpec});
        },
        removeListener: function(callback) {
            const index = window.__webRequestOnCompletedListeners.findIndex(l => l.callback === callback);
            if (index > -1) window.__webRequestOnCompletedListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__webRequestOnCompletedListeners.some(l => l.callback === callback);
        }
    };

    chrome.webRequest.onErrorOccurred = {
        addListener: function(callback, filter, extraInfoSpec) {
            console.log('[chrome.webRequest.onErrorOccurred] listener registered');
            window.__webRequestOnErrorOccurredListeners.push({callback, filter, extraInfoSpec});
        },
        removeListener: function(callback) {
            const index = window.__webRequestOnErrorOccurredListeners.findIndex(l => l.callback === callback);
            if (index > -1) window.__webRequestOnErrorOccurredListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__webRequestOnErrorOccurredListeners.some(l => l.callback === callback);
        }
    };

    chrome.webRequest.onAuthRequired = {
        addListener: function(callback, filter, extraInfoSpec) {
            console.log('[chrome.webRequest.onAuthRequired] listener registered');
            window.__webRequestOnAuthRequiredListeners.push({callback, filter, extraInfoSpec});
        },
        removeListener: function(callback) {
            const index = window.__webRequestOnAuthRequiredListeners.findIndex(l => l.callback === callback);
            if (index > -1) window.__webRequestOnAuthRequiredListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__webRequestOnAuthRequiredListeners.some(l => l.callback === callback);
        }
    };
    )";
}

std::string BrayaExtensionAPI::getWebNavigationAPI() {
    return R"(
    // chrome.webNavigation API
    chrome.webNavigation = chrome.webNavigation || {};

    // Event listener arrays
    window.__webNavigationOnBeforeNavigateListeners = window.__webNavigationOnBeforeNavigateListeners || [];
    window.__webNavigationOnCommittedListeners = window.__webNavigationOnCommittedListeners || [];
    window.__webNavigationOnDOMContentLoadedListeners = window.__webNavigationOnDOMContentLoadedListeners || [];
    window.__webNavigationOnCompletedListeners = window.__webNavigationOnCompletedListeners || [];
    window.__webNavigationOnErrorOccurredListeners = window.__webNavigationOnErrorOccurredListeners || [];
    window.__webNavigationOnCreatedNavigationTargetListeners = window.__webNavigationOnCreatedNavigationTargetListeners || [];

    chrome.webNavigation.onBeforeNavigate = {
        addListener: function(callback, filter) {
            console.log('[chrome.webNavigation.onBeforeNavigate] listener registered');
            window.__webNavigationOnBeforeNavigateListeners.push({callback, filter});
        },
        removeListener: function(callback) {
            const index = window.__webNavigationOnBeforeNavigateListeners.findIndex(l => l.callback === callback);
            if (index > -1) window.__webNavigationOnBeforeNavigateListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__webNavigationOnBeforeNavigateListeners.some(l => l.callback === callback);
        }
    };

    chrome.webNavigation.onCommitted = {
        addListener: function(callback, filter) {
            console.log('[chrome.webNavigation.onCommitted] listener registered');
            window.__webNavigationOnCommittedListeners.push({callback, filter});
        },
        removeListener: function(callback) {
            const index = window.__webNavigationOnCommittedListeners.findIndex(l => l.callback === callback);
            if (index > -1) window.__webNavigationOnCommittedListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__webNavigationOnCommittedListeners.some(l => l.callback === callback);
        }
    };

    chrome.webNavigation.onDOMContentLoaded = {
        addListener: function(callback, filter) {
            console.log('[chrome.webNavigation.onDOMContentLoaded] listener registered');
            window.__webNavigationOnDOMContentLoadedListeners.push({callback, filter});
        },
        removeListener: function(callback) {
            const index = window.__webNavigationOnDOMContentLoadedListeners.findIndex(l => l.callback === callback);
            if (index > -1) window.__webNavigationOnDOMContentLoadedListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__webNavigationOnDOMContentLoadedListeners.some(l => l.callback === callback);
        }
    };

    chrome.webNavigation.onCompleted = {
        addListener: function(callback, filter) {
            console.log('[chrome.webNavigation.onCompleted] listener registered');
            window.__webNavigationOnCompletedListeners.push({callback, filter});
        },
        removeListener: function(callback) {
            const index = window.__webNavigationOnCompletedListeners.findIndex(l => l.callback === callback);
            if (index > -1) window.__webNavigationOnCompletedListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__webNavigationOnCompletedListeners.some(l => l.callback === callback);
        }
    };

    chrome.webNavigation.onErrorOccurred = {
        addListener: function(callback, filter) {
            console.log('[chrome.webNavigation.onErrorOccurred] listener registered');
            window.__webNavigationOnErrorOccurredListeners.push({callback, filter});
        },
        removeListener: function(callback) {
            const index = window.__webNavigationOnErrorOccurredListeners.findIndex(l => l.callback === callback);
            if (index > -1) window.__webNavigationOnErrorOccurredListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__webNavigationOnErrorOccurredListeners.some(l => l.callback === callback);
        }
    };

    chrome.webNavigation.onCreatedNavigationTarget = {
        addListener: function(callback, filter) {
            console.log('[chrome.webNavigation.onCreatedNavigationTarget] listener registered');
            window.__webNavigationOnCreatedNavigationTargetListeners.push({callback, filter});
        },
        removeListener: function(callback) {
            const index = window.__webNavigationOnCreatedNavigationTargetListeners.findIndex(l => l.callback === callback);
            if (index > -1) window.__webNavigationOnCreatedNavigationTargetListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__webNavigationOnCreatedNavigationTargetListeners.some(l => l.callback === callback);
        }
    };
    )";
}

std::string BrayaExtensionAPI::getContextMenusAPI() {
    return R"(
    // chrome.contextMenus API
    chrome.contextMenus = chrome.contextMenus || {};

    chrome.contextMenus.create = function(createProperties, callback) {
        console.log('[chrome.contextMenus.create]', createProperties);
        if (callback) {
            setTimeout(() => callback(), 10);
        }
        return String(Date.now());
    };

    chrome.contextMenus.remove = function(menuItemId, callback) {
        console.log('[chrome.contextMenus.remove]', menuItemId);
        if (callback) {
            setTimeout(() => callback(), 10);
        }
    };

    chrome.contextMenus.removeAll = function(callback) {
        console.log('[chrome.contextMenus.removeAll]');
        if (callback) {
            setTimeout(() => callback(), 10);
        }
    };

    chrome.contextMenus.update = function(id, updateProperties, callback) {
        console.log('[chrome.contextMenus.update]', id, updateProperties);
        if (callback) {
            setTimeout(() => callback(), 10);
        }
    };

    chrome.contextMenus.onClicked = {
        addListener: function(callback) {
            console.log('[chrome.contextMenus.onClicked] listener registered');
        }
    };
    )";
}

std::string BrayaExtensionAPI::getWindowsAPI() {
    return R"(
    // chrome.windows API
    chrome.windows = chrome.windows || {};

    chrome.windows.get = function(windowId, getInfo, callback) {
        console.log('[chrome.windows.get]', windowId);
        const win = {
            id: 1,
            focused: true,
            top: 0,
            left: 0,
            width: 1280,
            height: 720,
            incognito: false,
            type: 'normal',
            state: 'normal',
            alwaysOnTop: false
        };
        if (typeof getInfo === 'function') {
            setTimeout(() => getInfo(win), 10);
        } else if (callback) {
            setTimeout(() => callback(win), 10);
        }
    };

    chrome.windows.getCurrent = function(getInfo, callback) {
        console.log('[chrome.windows.getCurrent]');
        return chrome.windows.get(1, getInfo, callback);
    };

    chrome.windows.getAll = function(getInfo, callback) {
        console.log('[chrome.windows.getAll]');
        const win = {
            id: 1,
            focused: true,
            top: 0,
            left: 0,
            width: 1280,
            height: 720,
            incognito: false,
            type: 'normal',
            state: 'normal',
            alwaysOnTop: false
        };
        if (typeof getInfo === 'function') {
            setTimeout(() => getInfo([win]), 10);
        } else if (callback) {
            setTimeout(() => callback([win]), 10);
        }
    };

    chrome.windows.create = function(createData, callback) {
        console.log('[chrome.windows.create]', createData);
        if (callback) {
            setTimeout(() => callback({id: 2}), 10);
        }
    };

    chrome.windows.onCreated = {
        addListener: function(callback) {
            console.log('[chrome.windows.onCreated] listener registered');
        }
    };

    chrome.windows.onRemoved = {
        addListener: function(callback) {
            console.log('[chrome.windows.onRemoved] listener registered');
        }
    };

    chrome.windows.onFocusChanged = {
        addListener: function(callback) {
            console.log('[chrome.windows.onFocusChanged] listener registered');
        }
    };
    )";
}

std::string BrayaExtensionAPI::getCookiesAPI() {
    return R"(
    // chrome.cookies API
    chrome.cookies = chrome.cookies || {};

    chrome.cookies.get = function(details, callback) {
        console.log('[chrome.cookies.get]', details);
        if (callback) {
            setTimeout(() => callback(null), 10);
        }
    };

    chrome.cookies.getAll = function(details, callback) {
        console.log('[chrome.cookies.getAll]', details);
        if (callback) {
            setTimeout(() => callback([]), 10);
        }
    };

    chrome.cookies.set = function(details, callback) {
        console.log('[chrome.cookies.set]', details);
        if (callback) {
            setTimeout(() => callback(details), 10);
        }
    };

    chrome.cookies.remove = function(details, callback) {
        console.log('[chrome.cookies.remove]', details);
        if (callback) {
            setTimeout(() => callback(null), 10);
        }
    };

    chrome.cookies.onChanged = {
        addListener: function(callback) {
            console.log('[chrome.cookies.onChanged] listener registered');
        }
    };
    )";
}

std::string BrayaExtensionAPI::getNotificationsAPI() {
    return R"(
    // chrome.notifications API
    chrome.notifications = chrome.notifications || {};

    chrome.notifications.create = function(notificationId, options, callback) {
        console.log('[chrome.notifications.create]', notificationId, options);
        const id = notificationId || String(Date.now());
        if (callback) {
            setTimeout(() => callback(id), 10);
        }
    };

    chrome.notifications.clear = function(notificationId, callback) {
        console.log('[chrome.notifications.clear]', notificationId);
        if (callback) {
            setTimeout(() => callback(true), 10);
        }
    };

    chrome.notifications.onClicked = {
        addListener: function(callback) {
            console.log('[chrome.notifications.onClicked] listener registered');
        }
    };

    chrome.notifications.onClosed = {
        addListener: function(callback) {
            console.log('[chrome.notifications.onClosed] listener registered');
        }
    };
    )";
}

std::string BrayaExtensionAPI::getAlarmsAPI() {
    return R"(
    // chrome.alarms API
    chrome.alarms = chrome.alarms || {};

    chrome.alarms.create = function(name, alarmInfo) {
        console.log('[chrome.alarms.create]', name, alarmInfo);
    };

    chrome.alarms.get = function(name, callback) {
        console.log('[chrome.alarms.get]', name);
        if (callback) {
            setTimeout(() => callback(null), 10);
        }
    };

    chrome.alarms.getAll = function(callback) {
        console.log('[chrome.alarms.getAll]');
        if (callback) {
            setTimeout(() => callback([]), 10);
        }
    };

    chrome.alarms.clear = function(name, callback) {
        console.log('[chrome.alarms.clear]', name);
        if (callback) {
            setTimeout(() => callback(true), 10);
        }
    };

    chrome.alarms.clearAll = function(callback) {
        console.log('[chrome.alarms.clearAll]');
        if (callback) {
            setTimeout(() => callback(true), 10);
        }
    };

    chrome.alarms.onAlarm = {
        addListener: function(callback) {
            console.log('[chrome.alarms.onAlarm] listener registered');
        }
    };
    )";
}

std::string BrayaExtensionAPI::getBrowserActionAPI() {
    return R"(
    // chrome.browserAction / chrome.action API (toolbar button)
    chrome.browserAction = chrome.browserAction || {};
    chrome.action = chrome.action || {};

    // Shared implementation for both APIs
    const actionImpl = {
        setIcon: function(details, callback) {
            console.log('[chrome.action.setIcon]', details);
            if (callback) setTimeout(() => callback(), 10);
        },
        setTitle: function(details, callback) {
            console.log('[chrome.action.setTitle]', details);
            if (callback) setTimeout(() => callback(), 10);
        },
        setBadgeText: function(details, callback) {
            console.log('[chrome.action.setBadgeText]', details);
            if (callback) setTimeout(() => callback(), 10);
        },
        setBadgeBackgroundColor: function(details, callback) {
            console.log('[chrome.action.setBadgeBackgroundColor]', details);
            if (callback) setTimeout(() => callback(), 10);
        },
        setPopup: function(details, callback) {
            console.log('[chrome.action.setPopup]', details);
            if (callback) setTimeout(() => callback(), 10);
        },
        getTitle: function(details, callback) {
            console.log('[chrome.action.getTitle]', details);
            if (callback) setTimeout(() => callback('Extension'), 10);
        },
        getBadgeText: function(details, callback) {
            console.log('[chrome.action.getBadgeText]', details);
            if (callback) setTimeout(() => callback(''), 10);
        },
        onClicked: {
            addListener: function(callback) {
                console.log('[chrome.action.onClicked] listener registered');
            }
        }
    };

    // Apply to both browserAction and action
    Object.assign(chrome.browserAction, actionImpl);
    Object.assign(chrome.action, actionImpl);
    )";
}

std::string BrayaExtensionAPI::getCommandsAPI() {
    return R"(
    // chrome.commands API (keyboard shortcuts)
    chrome.commands = chrome.commands || {};

    chrome.commands.getAll = function(callback) {
        console.log('[chrome.commands.getAll]');
        if (callback) {
            setTimeout(() => callback([]), 10);
        }
    };

    chrome.commands.onCommand = {
        addListener: function(callback) {
            console.log('[chrome.commands.onCommand] listener registered');
        }
    };
    )";
}

std::string BrayaExtensionAPI::getHistoryAPI() {
    return R"(
    // chrome.history API
    chrome.history = chrome.history || {};

    chrome.history.search = function(query, callback) {
        console.log('[chrome.history.search]', query);
        if (callback) {
            setTimeout(() => callback([]), 10);
        }
    };

    chrome.history.getVisits = function(details, callback) {
        console.log('[chrome.history.getVisits]', details);
        if (callback) {
            setTimeout(() => callback([]), 10);
        }
    };

    chrome.history.addUrl = function(details, callback) {
        console.log('[chrome.history.addUrl]', details);
        if (callback) setTimeout(() => callback(), 10);
    };

    chrome.history.deleteUrl = function(details, callback) {
        console.log('[chrome.history.deleteUrl]', details);
        if (callback) setTimeout(() => callback(), 10);
    };

    chrome.history.onVisited = {
        addListener: function(callback) {
            console.log('[chrome.history.onVisited] listener registered');
        }
    };
    )";
}

std::string BrayaExtensionAPI::getPermissionsAPI() {
    return R"(
    // chrome.permissions API (dynamic permissions)
    chrome.permissions = chrome.permissions || {};

    chrome.permissions.contains = function(permissions, callback) {
        console.log('[chrome.permissions.contains]', permissions);
        if (callback) {
            setTimeout(() => callback(true), 10);
        }
    };

    chrome.permissions.request = function(permissions, callback) {
        console.log('[chrome.permissions.request]', permissions);
        if (callback) {
            setTimeout(() => callback(true), 10);
        }
    };

    chrome.permissions.remove = function(permissions, callback) {
        console.log('[chrome.permissions.remove]', permissions);
        if (callback) {
            setTimeout(() => callback(true), 10);
        }
    };

    chrome.permissions.getAll = function(callback) {
        console.log('[chrome.permissions.getAll]');
        if (callback) {
            setTimeout(() => callback({permissions: [], origins: []}), 10);
        }
    };

    window.__permissionsOnAddedListeners = window.__permissionsOnAddedListeners || [];
    window.__permissionsOnRemovedListeners = window.__permissionsOnRemovedListeners || [];

    chrome.permissions.onAdded = {
        addListener: function(callback) {
            console.log('[chrome.permissions.onAdded] listener registered');
            window.__permissionsOnAddedListeners.push(callback);
        },
        removeListener: function(callback) {
            const index = window.__permissionsOnAddedListeners.indexOf(callback);
            if (index > -1) window.__permissionsOnAddedListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__permissionsOnAddedListeners.includes(callback);
        }
    };

    chrome.permissions.onRemoved = {
        addListener: function(callback) {
            console.log('[chrome.permissions.onRemoved] listener registered');
            window.__permissionsOnRemovedListeners.push(callback);
        },
        removeListener: function(callback) {
            const index = window.__permissionsOnRemovedListeners.indexOf(callback);
            if (index > -1) window.__permissionsOnRemovedListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__permissionsOnRemovedListeners.includes(callback);
        }
    };
    )";
}

std::string BrayaExtensionAPI::getIdleAPI() {
    return R"(
    // chrome.idle API (detect user idle state)
    chrome.idle = chrome.idle || {};

    chrome.idle.queryState = function(detectionIntervalInSeconds, callback) {
        console.log('[chrome.idle.queryState]', detectionIntervalInSeconds);
        if (callback) {
            setTimeout(() => callback('active'), 10);
        }
    };

    chrome.idle.setDetectionInterval = function(intervalInSeconds) {
        console.log('[chrome.idle.setDetectionInterval]', intervalInSeconds);
    };

    window.__idleOnStateChangedListeners = window.__idleOnStateChangedListeners || [];

    chrome.idle.onStateChanged = {
        addListener: function(callback) {
            console.log('[chrome.idle.onStateChanged] listener registered');
            window.__idleOnStateChangedListeners.push(callback);
        },
        removeListener: function(callback) {
            const index = window.__idleOnStateChangedListeners.indexOf(callback);
            if (index > -1) window.__idleOnStateChangedListeners.splice(index, 1);
        },
        hasListener: function(callback) {
            return window.__idleOnStateChangedListeners.includes(callback);
        }
    };
    )";
}

std::string BrayaExtensionAPI::getPrivacyAPI() {
    return R"(
    // chrome.privacy API (privacy settings)
    chrome.privacy = chrome.privacy || {};
    chrome.privacy.network = chrome.privacy.network || {};
    chrome.privacy.services = chrome.privacy.services || {};
    chrome.privacy.websites = chrome.privacy.websites || {};

    // Network privacy settings
    chrome.privacy.network.networkPredictionEnabled = {
        get: function(details, callback) {
            console.log('[chrome.privacy.network.networkPredictionEnabled.get]');
            if (callback) setTimeout(() => callback({value: true}), 10);
        },
        set: function(details, callback) {
            console.log('[chrome.privacy.network.networkPredictionEnabled.set]', details);
            if (callback) setTimeout(() => callback(), 10);
        }
    };

    chrome.privacy.network.webRTCIPHandlingPolicy = {
        get: function(details, callback) {
            console.log('[chrome.privacy.network.webRTCIPHandlingPolicy.get]');
            if (callback) setTimeout(() => callback({value: 'default'}), 10);
        },
        set: function(details, callback) {
            console.log('[chrome.privacy.network.webRTCIPHandlingPolicy.set]', details);
            if (callback) setTimeout(() => callback(), 10);
        }
    };

    // Services privacy settings
    chrome.privacy.services.autofillEnabled = {
        get: function(details, callback) {
            console.log('[chrome.privacy.services.autofillEnabled.get]');
            if (callback) setTimeout(() => callback({value: true}), 10);
        },
        set: function(details, callback) {
            console.log('[chrome.privacy.services.autofillEnabled.set]', details);
            if (callback) setTimeout(() => callback(), 10);
        }
    };

    // Websites privacy settings
    chrome.privacy.websites.thirdPartyCookiesAllowed = {
        get: function(details, callback) {
            console.log('[chrome.privacy.websites.thirdPartyCookiesAllowed.get]');
            if (callback) setTimeout(() => callback({value: true}), 10);
        },
        set: function(details, callback) {
            console.log('[chrome.privacy.websites.thirdPartyCookiesAllowed.set]', details);
            if (callback) setTimeout(() => callback(), 10);
        }
    };
    )";
}

std::string BrayaExtensionAPI::getClipboardAPI() {
    return R"(
    // Clipboard API for extensions (uses document.execCommand internally)
    // Note: clipboardRead/clipboardWrite permissions give access to navigator.clipboard
    if (!navigator.clipboard) {
        navigator.clipboard = {
            writeText: function(text) {
                console.log('[navigator.clipboard.writeText]', text?.substring(0, 50));
                return new Promise((resolve, reject) => {
                    try {
                        const textarea = document.createElement('textarea');
                        textarea.value = text;
                        textarea.style.position = 'fixed';
                        textarea.style.opacity = '0';
                        document.body.appendChild(textarea);
                        textarea.select();
                        const success = document.execCommand('copy');
                        document.body.removeChild(textarea);
                        if (success) {
                            resolve();
                        } else {
                            reject(new Error('Copy failed'));
                        }
                    } catch (err) {
                        reject(err);
                    }
                });
            },
            readText: function() {
                console.log('[navigator.clipboard.readText]');
                return new Promise((resolve, reject) => {
                    // Reading from clipboard requires user interaction in most browsers
                    // For now, return empty string as fallback
                    resolve('');
                });
            },
            read: function() {
                console.log('[navigator.clipboard.read]');
                return Promise.resolve([]);
            },
            write: function(data) {
                console.log('[navigator.clipboard.write]', data);
                return Promise.resolve();
            }
        };
    }
    )";
}

std::string BrayaExtensionAPI::getNativeMessagingAPI() {
    return R"(
    // chrome.runtime.connectNative for native messaging
    chrome.runtime.connectNative = function(application) {
        console.log('[chrome.runtime.connectNative]', application);
        // Return a mock native messaging port
        return {
            name: application,
            disconnect: function() {
                console.log('[nativePort.disconnect]', application);
            },
            postMessage: function(message) {
                console.log('[nativePort.postMessage]', application, message);
            },
            onDisconnect: {
                addListener: function(callback) {
                    console.log('[nativePort.onDisconnect.addListener]', application);
                },
                removeListener: function(callback) {},
                hasListener: function(callback) { return false; }
            },
            onMessage: {
                addListener: function(callback) {
                    console.log('[nativePort.onMessage.addListener]', application);
                },
                removeListener: function(callback) {},
                hasListener: function(callback) { return false; }
            }
        };
    };

    chrome.runtime.sendNativeMessage = function(application, message, callback) {
        console.log('[chrome.runtime.sendNativeMessage]', application, message);
        if (callback) {
            setTimeout(() => callback({error: 'Native messaging not supported'}), 10);
        }
    };
    )";
}

std::string BrayaExtensionAPI::getOffscreenAPI() {
    return R"(
    // chrome.offscreen API (Manifest V3)
    chrome.offscreen = chrome.offscreen || {};

    // Reasons for creating offscreen documents
    chrome.offscreen.Reason = {
        AUDIO_PLAYBACK: 'AUDIO_PLAYBACK',
        BLOBS: 'BLOBS',
        CLIPBOARD: 'CLIPBOARD',
        DOM_PARSER: 'DOM_PARSER',
        DOM_SCRAPING: 'DOM_SCRAPING',
        IFRAME_SCRIPTING: 'IFRAME_SCRIPTING',
        LOCAL_STORAGE: 'LOCAL_STORAGE',
        MATCH_MEDIA: 'MATCH_MEDIA',
        OFFSCREEN_MESSAGES: 'OFFSCREEN_MESSAGES',
        TESTING: 'TESTING',
        USER_MEDIA: 'USER_MEDIA',
        WEB_RTC: 'WEB_RTC',
        WORKERS: 'WORKERS'
    };

    // Create offscreen document
    chrome.offscreen.createDocument = function(parameters, callback) {
        console.log('[chrome.offscreen.createDocument]', parameters);
        // Mock implementation - in real browser would create hidden document
        // Parameters: url, reasons, justification
        if (callback) {
            setTimeout(() => callback(), 10);
        }
        return Promise.resolve();
    };

    // Close offscreen document
    chrome.offscreen.closeDocument = function(callback) {
        console.log('[chrome.offscreen.closeDocument]');
        if (callback) {
            setTimeout(() => callback(), 10);
        }
        return Promise.resolve();
    };

    // Check if offscreen document exists
    chrome.offscreen.hasDocument = function(callback) {
        console.log('[chrome.offscreen.hasDocument]');
        const result = false; // Mock: no offscreen documents
        if (callback) {
            setTimeout(() => callback(result), 10);
        }
        return Promise.resolve(result);
    };
    )";
}

std::string BrayaExtensionAPI::getScriptingAPI() {
    return R"(
    // chrome.scripting API (Manifest V3 - replaces tabs.executeScript)
    chrome.scripting = chrome.scripting || {};

    // Execute script in target tabs
    chrome.scripting.executeScript = function(injection, callback) {
        console.log('[chrome.scripting.executeScript]', injection);
        // injection: { target: { tabId, allFrames? }, files?, func?, args?, world? }

        // Mock implementation
        const results = [{
            result: null,
            frameId: 0
        }];

        if (callback) {
            setTimeout(() => callback(results), 10);
        }
        return Promise.resolve(results);
    };

    // Insert CSS in target tabs
    chrome.scripting.insertCSS = function(injection, callback) {
        console.log('[chrome.scripting.insertCSS]', injection);
        // injection: { target: { tabId, allFrames? }, files?, css? }

        if (callback) {
            setTimeout(() => callback(), 10);
        }
        return Promise.resolve();
    };

    // Remove CSS from target tabs
    chrome.scripting.removeCSS = function(injection, callback) {
        console.log('[chrome.scripting.removeCSS]', injection);

        if (callback) {
            setTimeout(() => callback(), 10);
        }
        return Promise.resolve();
    };

    // Register content scripts dynamically
    chrome.scripting.registerContentScripts = function(scripts, callback) {
        console.log('[chrome.scripting.registerContentScripts]', scripts);
        // scripts: array of { id, matches, js?, css?, runAt?, allFrames?, world? }

        if (callback) {
            setTimeout(() => callback(), 10);
        }
        return Promise.resolve();
    };

    // Unregister content scripts
    chrome.scripting.unregisterContentScripts = function(filter, callback) {
        console.log('[chrome.scripting.unregisterContentScripts]', filter);
        // filter: { ids?: string[] }

        if (callback) {
            setTimeout(() => callback(), 10);
        }
        return Promise.resolve();
    };

    // Get registered content scripts
    chrome.scripting.getRegisteredContentScripts = function(filter, callback) {
        console.log('[chrome.scripting.getRegisteredContentScripts]', filter);
        const scripts = []; // Mock: no registered scripts

        if (callback) {
            setTimeout(() => callback(scripts), 10);
        }
        return Promise.resolve(scripts);
    };

    // Update content scripts
    chrome.scripting.updateContentScripts = function(scripts, callback) {
        console.log('[chrome.scripting.updateContentScripts]', scripts);

        if (callback) {
            setTimeout(() => callback(), 10);
        }
        return Promise.resolve();
    };

    // Execution world constants
    chrome.scripting.ExecutionWorld = {
        ISOLATED: 'ISOLATED',
        MAIN: 'MAIN'
    };
    )";
}

std::string BrayaExtensionAPI::getBackgroundPageAPI() {
    return R"(
    // Complete chrome.* API for background pages
    if (!window.chrome) window.chrome = {};

    // Initialize vAPI object for uBlock Origin and similar extensions
    // CRITICAL: Must set on BOTH self and window for uBlock compatibility
    if (!self.vAPI) {
        const vAPIStub = {
            uBO: true,  // IMPORTANT: Prevents uBlock's vapi.js from replacing this object
            // Messaging infrastructure
            messaging: {
                send: function(channel, message) {
                    console.log('[vAPI.messaging.send] Channel:', channel, 'What:', message?.what, 'Full message:', JSON.stringify(message));

                    // Return appropriate mock data based on what uBlock is requesting
                    let response = {};

                    if (message && message.what) {
                        // localStorage requests (vapi-common.js calls this)
                        if (message.what === 'localStorage' && message.fn === 'getItemAsync') {
                            // Return null for missing items (uBlock checks for null)
                            response = null;
                            console.log('[vAPI.messaging.send] localStorage.getItemAsync for key:', message.args?.[0], '-> returning null');
                        }
                        // Popup data request
                        else if (message.what === 'popupPanelData') {
                            response = {
                                pageURL: 'https://example.com/',
                                pageHostname: 'example.com',
                                pageDomain: 'example.com',
                                pageTitle: '',
                                pageBlockedRequestCount: 0,
                                globalBlockedRequestCount: 0,
                                globalAllowedRequestCount: 0,
                                netFilteringSwitch: true,
                                canElementPicker: true,
                                noPopups: false,
                                noLargeMedia: false,
                                noCosmeticFiltering: false,
                                noRemoteFonts: false,
                                noScripting: false,
                                rawURL: 'https://example.com/',
                                hostnameDict: { 'example.com': true },
                                firewallRules: {},
                                canFirewall: true,
                                matrixIsDirty: false,
                                hasUnprocessedRequest: false,
                                tabId: 1,
                                tooltips: {}
                            };
                        }
                        // Tab context request
                        else if (message.what === 'getTabContext') {
                            response = {
                                rawURL: 'https://example.com/',
                                normalURL: 'https://example.com/',
                                origin: 'https://example.com',
                                hostname: 'example.com',
                                domain: 'example.com',
                                pageDomain: 'example.com'
                            };
                        }
                        // Filter lists request
                        else if (message.what === 'getAutoCompleteDetails') {
                            response = {
                                redirectResources: [],
                                preparseDirectiveTokens: [],
                                preparseDirectiveHints: []
                            };
                        }
                        // Get script tag filters
                        else if (message.what === 'getScriptTagFilters') {
                            response = {
                                filters: '',
                                hash: ''
                            };
                        }
                        // Default responses for common patterns
                        else if (message.what && message.what.startsWith('get')) {
                            // Provide safe defaults for getter methods
                            response = {
                                result: '',
                                data: [],
                                enabled: false,
                                version: '1.0',
                                hostname: 'example.com',
                                domain: 'example.com',
                                url: 'https://example.com/'
                            };
                        }
                    }

                    if (message && message.what && response !== null && typeof response === 'object' && Object.keys(response).length === 0) {
                        console.warn('[vAPI.messaging.send] UNHANDLED REQUEST TYPE:', message.what, '- returning safe defaults');
                        // Return safe default object instead of empty
                        response = {
                            result: '',
                            success: true,
                            data: []
                        };
                    }

                    console.log('[vAPI.messaging.send] RESPONSE:', JSON.stringify(response));
                    return Promise.resolve(response);
                }
            },
            // localStorage wrapper
            localStorage: {
                clear: function() {
                    console.log('[vAPI.localStorage.clear]');
                    return Promise.resolve();
                },
                getItemAsync: function(key) {
                    console.log('[vAPI.localStorage.getItemAsync]', key);
                    // IMPORTANT: Return null (not undefined!) - uBlock checks for null
                    return Promise.resolve(null);
                },
                removeItem: function(key) {
                    console.log('[vAPI.localStorage.removeItem]', key);
                    return Promise.resolve();
                },
                setItem: function(key, value) {
                    console.log('[vAPI.localStorage.setItem]', key, value);
                    return Promise.resolve();
                }
            }
        };
        // Assign to BOTH self and window to ensure availability
        self.vAPI = vAPIStub;
        window.vAPI = vAPIStub;

        // CRITICAL: Make vAPI non-configurable so uBlock's vapi.js can't replace it
        Object.defineProperty(self, 'vAPI', {
            value: vAPIStub,
            writable: false,
            configurable: false
        });
        Object.defineProperty(window, 'vAPI', {
            value: vAPIStub,
            writable: false,
            configurable: false
        });

        console.log('[DEBUG] vAPI initialized with uBO:', vAPIStub.uBO, 'localStorage:', typeof vAPIStub.localStorage);
    }

    // Polyfill for requestIdleCallback
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
        self.cancelIdleCallback = function(id) {
            clearTimeout(id);
        };
    }

    )" + getRuntimeAPI() + R"(

    )" + getTabsAPI() + R"(

    )" + getStorageAPI() + R"(

    )" + getBookmarksAPI() + R"(

    )" + getDownloadsAPI() + R"(

    )" + getI18nAPI() + R"(

    )" + getWebRequestAPI() + R"(

    )" + getWebNavigationAPI() + R"(

    )" + getContextMenusAPI() + R"(

    )" + getWindowsAPI() + R"(

    )" + getCookiesAPI() + R"(

    )" + getNotificationsAPI() + R"(

    )" + getAlarmsAPI() + R"(

    )" + getBrowserActionAPI() + R"(

    )" + getCommandsAPI() + R"(

    )" + getHistoryAPI() + R"(

    )" + getPermissionsAPI() + R"(

    )" + getIdleAPI() + R"(

    )" + getPrivacyAPI() + R"(

    )" + getClipboardAPI() + R"(

    )" + getNativeMessagingAPI() + R"(

    )" + getOffscreenAPI() + R"(

    )" + getScriptingAPI() + R"(

    // Firefox compatibility: browser.* = chrome.*
    if (!window.browser) window.browser = window.chrome;

    // DEBUG: Wrap chrome object in Proxy to catch undefined API access
    // This must be AFTER all APIs are defined
    (function() {
        const originalChrome = window.chrome;
        const accessedAPIs = new Set();

        const handler = {
            get: function(target, prop) {
                if (!(prop in target) && typeof prop === 'string' && !prop.startsWith('_')) {
                    if (!accessedAPIs.has(prop)) {
                        console.error('[MISSING API] chrome.' + prop + ' is undefined!');
                        accessedAPIs.add(prop);
                    }
                    // Return empty object with addListener trap
                    return new Proxy({}, {
                        get: function(subTarget, subProp) {
                            const fullPath = 'chrome.' + prop + '.' + subProp;
                            if (subProp === 'addListener' || subProp === 'removeListener' || subProp === 'hasListener') {
                                console.error('[MISSING API] ' + fullPath + ' called');
                                return function() {
                                    console.error('[MISSING API] ' + fullPath + ' - returning no-op function');
                                };
                            }
                            console.warn('[MISSING API] Accessing ' + fullPath);
                            return undefined;
                        }
                    });
                }
                return target[prop];
            }
        };

        // Only enable proxy in debug mode (check for a flag)
        if (typeof window.__DEBUG_CHROME_APIS !== 'undefined' && window.__DEBUG_CHROME_APIS) {
            window.chrome = new Proxy(originalChrome, handler);
            window.browser = window.chrome;
            console.log('[DEBUG] Chrome API Proxy installed - will report missing APIs');
        }
    })();

    // Register message handler for vAPI popup->background communication
    // This must be registered AFTER all Chrome APIs are loaded
    if (typeof vAPI !== 'undefined' && vAPI.uBO) {
        chrome.runtime.onMessage.addListener(function(message, sender, sendResponse) {
            console.log('[Background] Received message:', message);

            // Handle vAPI.messaging.send from popup
            if (message.type === 'vAPI.messaging.send') {
                console.log('[Background] Routing vAPI.messaging.send:', message.channel, message.message?.what);

                // Call the background page's vAPI.messaging.send
                vAPI.messaging.send(message.channel, message.message).then(response => {
                    console.log('[Background] Sending response back to popup:', response);
                    sendResponse(response);
                }).catch(error => {
                    console.error('[Background] vAPI message error:', error);
                    sendResponse({error: error.message});
                });

                // Return true to indicate async response
                return true;
            }

            // Handle vAPI.localStorage methods
            if (message.type === 'vAPI.localStorage.clear') {
                vAPI.localStorage.clear().then(sendResponse);
                return true;
            }
            if (message.type === 'vAPI.localStorage.getItemAsync') {
                vAPI.localStorage.getItemAsync(message.key).then(sendResponse);
                return true;
            }
            if (message.type === 'vAPI.localStorage.removeItem') {
                vAPI.localStorage.removeItem(message.key).then(sendResponse);
                return true;
            }
            if (message.type === 'vAPI.localStorage.setItem') {
                vAPI.localStorage.setItem(message.key, message.value).then(sendResponse);
                return true;
            }
        });
        console.log('[Background] vAPI message handler registered');
    }

    console.log('✓ Chrome/Firefox Extension API loaded (background page)');
    )";
}

std::string BrayaExtensionAPI::getContentScriptAPI() {
    return R"(
    // Limited chrome.* API for content scripts and popups
    if (!window.chrome) window.chrome = {};

    // Polyfill for requestIdleCallback (needed by uBlock Origin and others)
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
        self.cancelIdleCallback = function(id) {
            clearTimeout(id);
        };
    }

    // vAPI for uBlock Origin popups - routes to background page
    if (typeof vAPI === 'undefined') {
        const vAPIStub = {
            uBO: true,
            messaging: {
                send: function(channel, message) {
                    console.log('[vAPI.messaging.send] Popup->Background:', channel, message?.what);

                    // Route message to background page via chrome.runtime.sendMessage
                    return new Promise((resolve) => {
                        chrome.runtime.sendMessage({
                            type: 'vAPI.messaging.send',
                            channel: channel,
                            message: message
                        }, (response) => {
                            console.log('[vAPI.messaging.send] Background->Popup response:', response);
                            resolve(response);
                        });
                    });
                }
            },
            localStorage: {
                clear: function() {
                    return new Promise((resolve) => {
                        chrome.runtime.sendMessage({
                            type: 'vAPI.localStorage.clear'
                        }, resolve);
                    });
                },
                getItemAsync: function(key) {
                    return new Promise((resolve) => {
                        chrome.runtime.sendMessage({
                            type: 'vAPI.localStorage.getItemAsync',
                            key: key
                        }, resolve);
                    });
                },
                removeItem: function(key) {
                    return new Promise((resolve) => {
                        chrome.runtime.sendMessage({
                            type: 'vAPI.localStorage.removeItem',
                            key: key
                        }, resolve);
                    });
                },
                setItem: function(key, value) {
                    return new Promise((resolve) => {
                        chrome.runtime.sendMessage({
                            type: 'vAPI.localStorage.setItem',
                            key: key,
                            value: value
                        }, resolve);
                    });
                }
            }
        };
        Object.defineProperty(window, 'vAPI', {
            value: vAPIStub,
            writable: false,
            configurable: false
        });
        if (typeof self !== 'undefined') {
            Object.defineProperty(self, 'vAPI', {
                value: vAPIStub,
                writable: false,
                configurable: false
            });
        }
        console.log('[DEBUG] vAPI initialized for popup with background page routing');
    }

    )" + getRuntimeAPI() + R"(

    )" + getTabsAPI() + R"(

    )" + getStorageAPI() + R"(

    )" + getI18nAPI() + R"(

    )" + getBrowserActionAPI() + R"(

    )" + getWindowsAPI() + R"(

    )" + getCookiesAPI() + R"(

    )" + getPermissionsAPI() + R"(

    )" + getIdleAPI() + R"(

    )" + getPrivacyAPI() + R"(

    // Firefox compatibility: browser.* = chrome.*
    if (!window.browser) window.browser = window.chrome;

    console.log('✓ Chrome Extension API loaded (content script/popup)');
    )";
}
