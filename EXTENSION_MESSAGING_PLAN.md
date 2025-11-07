# WebExtension Messaging System Implementation Plan

**Date**: November 6, 2025
**Priority**: HIGH - Critical for uBlock Origin functionality
**Estimated Effort**: 4-6 hours

---

## Current Issues

### 1. CORS Errors
```
CONSOLE SECURITY ERROR Origin null is not allowed by Access-Control-Allow-Origin. Status code: 0
```

**Problem**: Extension popups loaded via `file://` protocol can't fetch resources due to CORS restrictions.

**Impact**: Popup UI remains blank because it can't load data from background page.

### 2. No Real Message Passing
**Problem**: `chrome.runtime.sendMessage()` currently just logs and returns fake responses. There's no actual communication channel between:
- Popup WebView ↔ Background Page WebView
- Content Scripts ↔ Background Page
- Different extension contexts

**Current Implementation** (src/extensions/BrayaExtensionAPI.cpp:26):
```cpp
chrome.runtime.sendMessage = function(message, callback) {
    console.log('[chrome.runtime.sendMessage]', message);
    // Simulate async response - FAKE!
    if (callback) {
        setTimeout(() => callback({status: 'ok'}), 10);
    }
};
```

### 3. Missing Port Connections
**Problem**: No implementation of:
- `chrome.runtime.connect()` - Create persistent connection
- `Port` object with `postMessage()` and `onMessage`
- Long-lived connections for streaming data

**Required by**: uBlock Origin uses ports for efficient background ↔ popup communication

### 4. Storage Not Persistent
**Problem**: `chrome.storage.local` uses in-memory Map, data lost on restart.

---

## Architecture Overview

### Message Router System

We need a central message router that can route messages between different contexts:

```
┌─────────────────┐         ┌──────────────────────┐         ┌─────────────────┐
│  Popup WebView  │ ◄─────► │   Message Router     │ ◄─────► │  Background     │
│                 │         │  (C++ Class)         │         │  Page WebView   │
└─────────────────┘         └──────────────────────┘         └─────────────────┘
                                      ▲
                                      │
                                      ▼
                            ┌──────────────────┐
                            │ Content Scripts  │
                            │  (in Web Pages)  │
                            └──────────────────┘
```

### Key Components to Build

1. **ExtensionMessageRouter** (C++)
   - Central hub for all extension messaging
   - Maps extension ID → background page WebView
   - Routes messages between contexts
   - Manages port connections

2. **Enhanced JavaScript APIs**
   - Actual `sendMessage()` that sends to C++ layer
   - `connect()` for persistent connections
   - `Port` objects with event emitters

3. **CORS Bypass for Extensions**
   - Custom URI scheme handler for extensions
   - Or WebKit policy adjustment for file:// in extension context

---

## Implementation Tasks

### Task 1: Fix CORS Issues (1 hour)

**Option A: Custom URI Scheme**
- Register `chrome-extension://` URI scheme with WebKit
- Map extension IDs to local file paths
- Serves extension resources without CORS restrictions

**Option B: WebKit Policy Override**
- Use `webkit_web_context_set_cors_allowlist()` to allow extension origins
- Simpler but less standard-compliant

**Recommended**: Option A (more proper, better long-term)

**Files to Create/Modify**:
- `src/extensions/ExtensionProtocolHandler.h` (new)
- `src/extensions/ExtensionProtocolHandler.cpp` (new)
- `src/extensions/BrayaExtensionManager.cpp` (modify)

**Implementation Sketch**:
```cpp
class ExtensionProtocolHandler {
public:
    static void registerScheme(WebKitWebContext* context);
    static void handleRequest(WebKitURISchemeRequest* request);

private:
    static std::map<std::string, std::string> extensionPaths;
};

// In handleRequest:
// Parse chrome-extension://EXTENSION_ID/path
// Map to actual file: /path/to/extension/path
// Serve file with proper MIME type
```

---

### Task 2: Message Router Infrastructure (2-3 hours)

**Create**: `src/extensions/ExtensionMessageRouter.h`

```cpp
class ExtensionMessageRouter {
public:
    static ExtensionMessageRouter& getInstance();

    // Register extension contexts
    void registerBackgroundPage(const std::string& extensionId, WebKitWebView* webView);
    void registerPopup(const std::string& extensionId, WebKitWebView* webView);

    // Message passing
    void sendMessage(const std::string& from, const std::string& to,
                    const std::string& extensionId, const std::string& message,
                    std::function<void(const std::string&)> callback);

    // Port connections
    int createPort(const std::string& extensionId, const std::string& portName,
                  WebKitWebView* sender);
    void postMessageToPort(int portId, const std::string& message);
    void disconnectPort(int portId);

private:
    struct PortConnection {
        int id;
        std::string extensionId;
        std::string name;
        WebKitWebView* sender;
        WebKitWebView* receiver;
    };

    std::map<std::string, WebKitWebView*> backgroundPages;
    std::map<std::string, WebKitWebView*> popups;
    std::map<int, PortConnection> ports;
    int nextPortId = 1;
};
```

**Key Functions**:

1. **registerBackgroundPage()**: Store reference to background page WebView
2. **registerPopup()**: Store reference to popup WebView
3. **sendMessage()**: Route message from source to destination context
4. **createPort()**: Create persistent connection, return port ID
5. **postMessageToPort()**: Send message through port
6. **disconnectPort()**: Close port connection

---

### Task 3: Enhanced JavaScript Runtime API (1-2 hours)

**Modify**: `src/extensions/BrayaExtensionAPI.cpp`

**New `chrome.runtime.sendMessage()` Implementation**:
```javascript
chrome.runtime.sendMessage = function(message, callback) {
    console.log('[chrome.runtime.sendMessage]', message);

    // Generate unique callback ID
    const callbackId = Math.random().toString(36).substr(2, 9);

    // Store callback
    if (callback) {
        window.__messageCallbacks = window.__messageCallbacks || {};
        window.__messageCallbacks[callbackId] = callback;
    }

    // Send to native layer via webkit.messageHandlers
    const payload = {
        type: 'runtime.sendMessage',
        extensionId: chrome.runtime.id,
        message: message,
        callbackId: callbackId
    };

    window.webkit.messageHandlers.extensionMessage.postMessage(JSON.stringify(payload));
};

// Handler for responses
window.__handleMessageResponse = function(callbackId, response) {
    const callback = window.__messageCallbacks[callbackId];
    if (callback) {
        callback(response);
        delete window.__messageCallbacks[callbackId];
    }
};
```

**New `chrome.runtime.connect()` Implementation**:
```javascript
chrome.runtime.connect = function(connectInfo) {
    const portName = connectInfo ? connectInfo.name : '';

    // Create port object
    const port = {
        name: portName,
        onMessage: {
            addListener: function(callback) {
                port.__messageListeners = port.__messageListeners || [];
                port.__messageListeners.push(callback);
            }
        },
        onDisconnect: {
            addListener: function(callback) {
                port.__disconnectListeners = port.__disconnectListeners || [];
                port.__disconnectListeners.push(callback);
            }
        },
        postMessage: function(message) {
            const payload = {
                type: 'port.postMessage',
                portId: port.__id,
                message: message
            };
            window.webkit.messageHandlers.extensionMessage.postMessage(JSON.stringify(payload));
        },
        disconnect: function() {
            const payload = {
                type: 'port.disconnect',
                portId: port.__id
            };
            window.webkit.messageHandlers.extensionMessage.postMessage(JSON.stringify(payload));
        }
    };

    // Request port creation
    const payload = {
        type: 'port.create',
        extensionId: chrome.runtime.id,
        portName: portName
    };

    window.webkit.messageHandlers.extensionMessage.postMessage(JSON.stringify(payload));

    // Port ID will be assigned by response
    return port;
};
```

---

### Task 4: Connect JavaScript to C++ (1 hour)

**In Background Page & Popup Initialization**:

Add message handler registration:

```cpp
// In BrayaExtensionManager::createPopupWebView() and startBackgroundPage()

// Create user content manager for message handler
WebKitUserContentManager* manager = webkit_web_view_get_user_content_manager(webView);

// Register message handler
webkit_user_content_manager_register_script_message_handler(
    manager,
    "extensionMessage"
);

// Connect signal
g_signal_connect(manager, "script-message-received::extensionMessage",
                G_CALLBACK(onExtensionMessage), this);
```

**Message Handler Callback**:
```cpp
static void onExtensionMessage(WebKitUserContentManager* manager,
                              WebKitJavascriptResult* result,
                              gpointer user_data)
{
    // Parse message
    JSCValue* value = webkit_javascript_result_get_js_value(result);
    char* str = jsc_value_to_string(value);
    std::string message(str);
    g_free(str);

    // Parse JSON
    Json::Value root;
    Json::Reader reader;
    reader.parse(message, root);

    std::string type = root["type"].asString();

    if (type == "runtime.sendMessage") {
        handleSendMessage(root);
    } else if (type == "port.create") {
        handlePortCreate(root);
    } else if (type == "port.postMessage") {
        handlePortPostMessage(root);
    } else if (type == "port.disconnect") {
        handlePortDisconnect(root);
    }
}
```

---

### Task 5: Implement Message Handlers (1-2 hours)

```cpp
void BrayaExtensionManager::handleSendMessage(const Json::Value& payload) {
    std::string extensionId = payload["extensionId"].asString();
    std::string message = payload["message"].toStyledString();
    std::string callbackId = payload["callbackId"].asString();

    // Get message router
    auto& router = ExtensionMessageRouter::getInstance();

    // Send message to background page
    router.sendMessage("popup", "background", extensionId, message,
        [this, callbackId](const std::string& response) {
            // Send response back to popup
            std::string script =
                "window.__handleMessageResponse('" + callbackId + "', " + response + ");";
            // Execute script in popup context
            executeScriptInPopup(script);
        });
}

void BrayaExtensionManager::handlePortCreate(const Json::Value& payload) {
    std::string extensionId = payload["extensionId"].asString();
    std::string portName = payload["portName"].asString();

    // Get sender WebView from context
    WebKitWebView* sender = getCurrentWebView();

    // Create port
    auto& router = ExtensionMessageRouter::getInstance();
    int portId = router.createPort(extensionId, portName, sender);

    // Send port ID back to JavaScript
    std::string script =
        "window.__lastCreatedPort.__id = " + std::to_string(portId) + ";";
    executeScriptInContext(sender, script);
}
```

---

### Task 6: Real Storage API (30 min)

**Modify**: `src/extensions/BrayaExtensionAPI.cpp`

```javascript
chrome.storage.local = {
    get: function(keys, callback) {
        // Send to native layer
        const payload = {
            type: 'storage.get',
            keys: Array.isArray(keys) ? keys : [keys]
        };

        window.webkit.messageHandlers.extensionStorage.postMessage(
            JSON.stringify(payload),
            function(response) {
                callback(JSON.parse(response));
            }
        );
    },

    set: function(items, callback) {
        const payload = {
            type: 'storage.set',
            items: items
        };

        window.webkit.messageHandlers.extensionStorage.postMessage(
            JSON.stringify(payload),
            callback || function() {}
        );
    }
};
```

**C++ Storage Handler**:
```cpp
// Save to file: ~/.config/braya-browser/extensions/EXTENSION_ID/storage.json
void BrayaExtensionManager::handleStorageSet(const Json::Value& payload) {
    std::string extensionId = getCurrentExtensionId();
    std::string storagePath = getExtensionStoragePath(extensionId);

    // Load existing storage
    Json::Value storage = loadStorage(storagePath);

    // Merge new items
    Json::Value items = payload["items"];
    for (auto& key : items.getMemberNames()) {
        storage[key] = items[key];
    }

    // Save to disk
    saveStorage(storagePath, storage);
}
```

---

## Testing Plan

### Test 1: Basic Messaging
1. Create simple extension with popup and background page
2. Click button in popup → sends message to background
3. Background receives and responds
4. Popup displays response

### Test 2: Port Connections
1. Open popup for extension
2. Create port connection to background
3. Send multiple messages through port
4. Verify bidirectional communication
5. Disconnect and verify cleanup

### Test 3: uBlock Origin
1. Install uBlock Origin
2. Click extension button
3. Popup should load and display UI
4. Verify no CORS errors
5. Verify popup can fetch data from background
6. Test toggling blocking on/off

### Test 4: Storage Persistence
1. Extension saves data to storage
2. Close browser
3. Reopen browser
4. Verify data persists

---

## Priority Order

### Phase 1 (Must Have for uBlock):
1. ✅ Fix CORS (Task 1) - Without this, popup stays blank
2. ✅ Message Router (Task 2) - Core infrastructure
3. ✅ Enhanced JS API (Task 3) - sendMessage implementation
4. ✅ Connect JS to C++ (Task 4) - Wire it up

### Phase 2 (Should Have):
5. ⚠️ Port Connections - uBlock uses this heavily
6. ⚠️ Storage Persistence - Important for settings

### Phase 3 (Nice to Have):
7. Additional APIs (notifications, downloads, etc.)
8. Extension debugging tools
9. Performance optimizations

---

## Files to Create

```
src/extensions/
├── ExtensionProtocolHandler.h         (new)
├── ExtensionProtocolHandler.cpp       (new)
├── ExtensionMessageRouter.h           (new)
├── ExtensionMessageRouter.cpp         (new)
├── ExtensionStorageManager.h          (new)
└── ExtensionStorageManager.cpp        (new)
```

## Files to Modify

```
src/extensions/
├── BrayaExtensionAPI.cpp              (enhance APIs)
├── BrayaExtensionManager.h            (add message handlers)
├── BrayaExtensionManager.cpp          (wire up routing)
└── BackgroundPageRunner.cpp           (register message handlers)
```

---

## Estimated Timeline

- **Task 1** (CORS): 1 hour
- **Task 2** (Router): 2-3 hours
- **Task 3** (JS API): 1-2 hours
- **Task 4** (Connect): 1 hour
- **Task 5** (Handlers): 1-2 hours
- **Task 6** (Storage): 30 min
- **Testing**: 1 hour

**Total**: 7.5-10.5 hours (split across 2 sessions)

---

## Success Criteria

### Minimum Viable:
- ✅ No CORS errors in console
- ✅ `chrome.runtime.sendMessage()` works between popup and background
- ✅ uBlock Origin popup loads and displays UI

### Fully Working:
- ✅ Port connections work
- ✅ Storage persists across restarts
- ✅ uBlock Origin fully functional (can enable/disable, update filters, etc.)
- ✅ Multiple extensions work simultaneously

---

## Alternative: Simpler Extension First

If full implementation is too complex, consider:

1. **Test with simpler extension**:
   - Create minimal "Hello World" extension
   - Just popup + background with simple message
   - Prove the architecture works

2. **Incremental uBlock support**:
   - Get popup to load (CORS fix)
   - Get basic on/off toggle working (simple messaging)
   - Add advanced features later

---

## References

- [Chrome Extension Messaging Docs](https://developer.chrome.com/docs/extensions/mv3/messaging/)
- [WebKit WebExtension Support](https://webkit.org/blog/12445/new-webkit-features-in-safari-15-4/)
- [Mozilla WebExtensions API](https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions)

---

**This is a significant but achievable engineering task. The architecture is sound, and the implementation is straightforward once the infrastructure is in place.**

**Next session: Start with CORS fix + basic messaging, then expand from there.**
