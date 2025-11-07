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

    chrome.runtime.sendMessage = function(message, callback) {
        console.log('[chrome.runtime.sendMessage]', message);
        // Simulate async response
        if (callback) {
            setTimeout(() => callback({status: 'ok'}), 10);
        }
    };

    chrome.runtime.onInstalled = {
        addListener: function(callback) {
            console.log('[chrome.runtime.onInstalled] listener registered');
            // Call immediately for testing
            setTimeout(() => callback({reason: 'install'}), 100);
        }
    };

    chrome.runtime.lastError = null;
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

    chrome.tabs.onCreated = {
        addListener: function(callback) {
            console.log('[chrome.tabs.onCreated] listener registered');
        }
    };

    chrome.tabs.onUpdated = {
        addListener: function(callback) {
            console.log('[chrome.tabs.onUpdated] listener registered');
        }
    };

    chrome.tabs.onRemoved = {
        addListener: function(callback) {
            console.log('[chrome.tabs.onRemoved] listener registered');
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

    chrome.storage.onChanged = {
        addListener: function(callback) {
            console.log('[chrome.storage.onChanged] listener registered');
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
                                canFirewall: true
                            };
                        }
                        // Tab context request
                        else if (message.what === 'getTabContext') {
                            response = {
                                rawURL: 'https://example.com/',
                                normalURL: 'https://example.com/',
                                origin: 'https://example.com',
                                hostname: 'example.com',
                                domain: 'example.com'
                            };
                        }
                    }

                    if (message && message.what && response !== null && typeof response === 'object' && Object.keys(response).length === 0) {
                        console.warn('[vAPI.messaging.send] UNHANDLED REQUEST TYPE:', message.what);
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

    // Firefox compatibility: browser.* = chrome.*
    if (!window.browser) window.browser = window.chrome;

    console.log('✓ Chrome/Firefox Extension API loaded (background page)');
    )";
}

std::string BrayaExtensionAPI::getContentScriptAPI() {
    return R"(
    // Limited chrome.* API for content scripts
    if (!window.chrome) window.chrome = {};

    )" + getRuntimeAPI() + R"(

    )" + getStorageAPI() + R"(

    console.log('✓ Chrome Extension API loaded (content script)');
    )";
}
