#include <webkit/webkit-web-process-extension.h>
#include <jsc/jsc.h>
#include <glib.h>
#include <iostream>
#include <string>
#include <vector>

// Global state for the web extension
static WebKitWebProcessExtension* g_extension = nullptr;
static bool g_adBlockerEnabled = false;
static std::vector<std::string> g_blockedPatterns;

// Forward declarations
static void onPageCreated(WebKitWebProcessExtension* extension,
                         WebKitWebPage* page,
                         gpointer user_data);

static void onDocumentLoaded(WebKitWebPage* page,
                            gpointer user_data);

static gboolean onSendRequest(WebKitWebPage* page,
                              WebKitURIRequest* request,
                              WebKitURIResponse* redirected_response,
                              gpointer user_data);

static void onUserMessageReceived(WebKitWebPage* page,
                                  WebKitUserMessage* message,
                                  gpointer user_data);

// Send message to UI process
static void sendMessageToUI(WebKitWebPage* page, const char* name, const char* data) {
    WebKitUserMessage* message = webkit_user_message_new(name, g_variant_new_string(data));
    webkit_web_page_send_message_to_view(page, message, nullptr, nullptr, nullptr);
}

// Initialize basic ad blocking patterns
static void initializeAdBlocker() {
    // Simple patterns for testing
    g_blockedPatterns.push_back("doubleclick.net");
    g_blockedPatterns.push_back("googlesyndication.com");
    g_blockedPatterns.push_back("googleadservices.com");
    g_blockedPatterns.push_back("google-analytics.com");
    g_blockedPatterns.push_back("/ads/");
    g_blockedPatterns.push_back("/ad.js");
    g_blockedPatterns.push_back("/banner");

    std::cout << "✓ Ad blocker initialized with " << g_blockedPatterns.size() << " patterns" << std::endl;
}

// Check if URL should be blocked
static bool shouldBlockUrl(const char* url) {
    if (!g_adBlockerEnabled) {
        return false;
    }

    std::string urlStr(url);

    for (const auto& pattern : g_blockedPatterns) {
        if (urlStr.find(pattern) != std::string::npos) {
            return true;
        }
    }

    return false;
}

// Inject JavaScript into page
static void injectJavaScript(WebKitWebPage* page, const char* script) {
    WebKitFrame* frame = webkit_web_page_get_main_frame(page);
    if (!frame) return;

    WebKitScriptWorld* world = webkit_script_world_get_default();
    JSCContext* js_context = webkit_frame_get_js_context_for_script_world(frame, world);
    if (!js_context) return;

    jsc_context_evaluate(js_context, script, -1);
}

// Extension entry point - called by WebKit when web process starts
extern "C" {

G_MODULE_EXPORT void
webkit_web_extension_initialize(WebKitWebProcessExtension* extension) {
    g_extension = extension;

    std::cout << "🚀 Braya Web Extension initializing..." << std::endl;

    // Connect to page-created signal
    g_signal_connect(extension, "page-created",
                    G_CALLBACK(onPageCreated), nullptr);

    std::cout << "✓ Braya Web Extension initialized" << std::endl;
}

G_MODULE_EXPORT void
webkit_web_extension_initialize_with_user_data(WebKitWebProcessExtension* extension,
                                                GVariant* user_data) {
    g_extension = extension;

    std::cout << "🚀 Braya Web Extension initializing with user data..." << std::endl;

    // Parse initialization data from UI process
    if (user_data) {
        GVariantDict dict;
        g_variant_dict_init(&dict, user_data);

        // Get ad blocker state
        gboolean adBlockerEnabled = false;
        if (g_variant_dict_lookup(&dict, "ad-blocker-enabled", "b", &adBlockerEnabled)) {
            g_adBlockerEnabled = adBlockerEnabled;
            std::cout << "  Ad Blocker: " << (g_adBlockerEnabled ? "enabled" : "disabled") << std::endl;
        }

        g_variant_dict_clear(&dict);
    }

    // Initialize ad blocker patterns
    initializeAdBlocker();

    // Connect to page-created signal
    g_signal_connect(extension, "page-created",
                    G_CALLBACK(onPageCreated), nullptr);

    std::cout << "✓ Braya Web Extension initialized" << std::endl;
}

} // extern "C"

// Called when a new page is created
static void onPageCreated(WebKitWebProcessExtension* extension,
                         WebKitWebPage* page,
                         gpointer user_data) {
    guint64 page_id = webkit_web_page_get_id(page);
    std::cout << "📄 Page created: " << page_id << std::endl;

    // Connect to document-loaded signal
    g_signal_connect(page, "document-loaded",
                    G_CALLBACK(onDocumentLoaded), nullptr);

    // Connect to send-request signal for request interception
    g_signal_connect(page, "send-request",
                    G_CALLBACK(onSendRequest), nullptr);

    // Connect to user-message-received signal for IPC
    g_signal_connect(page, "user-message-received",
                    G_CALLBACK(onUserMessageReceived), nullptr);

    // Send log message to UI process
    sendMessageToUI(page, "extension-log", "Page created in web extension");
}

// Called when document is loaded
static void onDocumentLoaded(WebKitWebPage* page,
                            gpointer user_data) {
    guint64 page_id = webkit_web_page_get_id(page);
    std::cout << "📄 Document loaded: " << page_id << std::endl;

    // Get the page URI
    WebKitFrame* frame = webkit_web_page_get_main_frame(page);
    if (frame) {
        const char* uri = webkit_web_page_get_uri(page);
        std::cout << "  URI: " << (uri ? uri : "unknown") << std::endl;

        // Inject test JavaScript
        const char* testScript = R"(
            console.log('🔌 Braya Extension loaded on:', window.location.href);
            window.brayaExtension = {
                version: '1.0.0',
                name: 'Braya Web Extension'
            };
        )";

        injectJavaScript(page, testScript);
    }
}

// Called before a request is sent - can modify or block
static gboolean onSendRequest(WebKitWebPage* page,
                              WebKitURIRequest* request,
                              WebKitURIResponse* redirected_response,
                              gpointer user_data) {
    const char* uri = webkit_uri_request_get_uri(request);

    if (shouldBlockUrl(uri)) {
        std::cout << "🚫 Blocked: " << uri << std::endl;

        // Send blocked notification to UI
        sendMessageToUI(page, "request-blocked", uri);

        // Return TRUE to block the request
        return TRUE;
    }

    // Return FALSE to allow the request
    return FALSE;
}

// Handle messages from UI process
static void onUserMessageReceived(WebKitWebPage* page,
                                  WebKitUserMessage* message,
                                  gpointer user_data) {
    const char* name = webkit_user_message_get_name(message);
    std::cout << "📨 Web extension received message: " << name << std::endl;

    if (g_strcmp0(name, "reload-filters") == 0) {
        // Reload ad blocker filters
        std::cout << "  Reloading ad blocker filters..." << std::endl;
        initializeAdBlocker();
    }
    else if (g_strcmp0(name, "toggle-adblocker") == 0) {
        // Toggle ad blocker
        g_adBlockerEnabled = !g_adBlockerEnabled;
        std::cout << "  Ad blocker: " << (g_adBlockerEnabled ? "enabled" : "disabled") << std::endl;
    }
}
