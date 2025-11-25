// Minimal CEF + GTK4 Prototype
// Tests if CEF embedding works with GTK4

#include <gtk/gtk.h>
#include <include/cef_app.h>
#include <include/cef_client.h>
#include <include/cef_browser.h>
#include <include/wrapper/cef_helpers.h>

// Simple CEF client implementation
class SimpleCEFClient : public CefClient,
                        public CefLifeSpanHandler,
                        public CefLoadHandler {
public:
    SimpleCEFClient() {}

    // CefClient methods
    virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override {
        return this;
    }

    virtual CefRefPtr<CefLoadHandler> GetLoadHandler() override {
        return this;
    }

    // CefLifeSpanHandler methods
    virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override {
        CEF_REQUIRE_UI_THREAD();
        g_print("✓ Browser created\n");
    }

    virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override {
        CEF_REQUIRE_UI_THREAD();
        g_print("✓ Browser closing\n");
    }

    // CefLoadHandler methods
    virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                          CefRefPtr<CefFrame> frame,
                          int httpStatusCode) override {
        CEF_REQUIRE_UI_THREAD();
        g_print("✓ Page loaded: %s\n", frame->GetURL().ToString().c_str());
    }

private:
    IMPLEMENT_REFCOUNTING(SimpleCEFClient);
};

// Simple CEF app
class SimpleCEFApp : public CefApp {
public:
    SimpleCEFApp() {}

private:
    IMPLEMENT_REFCOUNTING(SimpleCEFApp);
};

// GTK window data
struct WindowData {
    GtkWidget* window;
    CefRefPtr<CefBrowser> browser;
};

static WindowData* g_window_data = nullptr;

// CEF work scheduler for GTK main loop
static gboolean on_cef_work(gpointer data) {
    CefDoMessageLoopWork();
    return G_SOURCE_CONTINUE;  // Keep calling
}

// Window close handler
static void on_window_close(GtkWidget* widget, gpointer data) {
    if (g_window_data && g_window_data->browser) {
        g_window_data->browser->GetHost()->CloseBrowser(false);
    }
    gtk_window_destroy(GTK_WINDOW(widget));
}

// GTK activate callback
static void activate(GtkApplication* app, gpointer user_data) {
    g_print("🚀 Creating GTK4 window...\n");

    // Create window
    GtkWidget* window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "CEF + GTK4 Test");
    gtk_window_set_default_size(GTK_WINDOW(window), 1200, 800);

    g_signal_connect(window, "close-request", G_CALLBACK(on_window_close), nullptr);

    // Create drawing area for CEF browser
    GtkWidget* drawing_area = gtk_drawing_area_new();
    gtk_window_set_child(GTK_WINDOW(window), drawing_area);

    gtk_window_present(GTK_WINDOW(window));

    g_print("✓ GTK4 window created\n");
    g_print("🌐 Creating CEF browser...\n");

    // Get native window handle
    GdkSurface* surface = gtk_native_get_surface(GTK_NATIVE(window));
    if (!surface) {
        g_printerr("❌ Failed to get GdkSurface\n");
        return;
    }

    // CEF window info
    CefWindowInfo window_info;

#if defined(OS_LINUX)
    // Get X11 window ID (or Wayland equivalent)
    // This is platform-specific and might need adjustment
    GdkDisplay* display = gdk_display_get_default();

    #ifdef GDK_WINDOWING_X11
    if (GDK_IS_X11_DISPLAY(display)) {
        #include <gdk/x11/gdkx.h>
        Window xwindow = gdk_x11_surface_get_xid(surface);
        window_info.SetAsChild(xwindow, CefRect(0, 0, 1200, 800));
        g_print("✓ Using X11 window: %lu\n", xwindow);
    }
    #endif

    #ifdef GDK_WINDOWING_WAYLAND
    if (GDK_IS_WAYLAND_DISPLAY(display)) {
        g_print("⚠️  Wayland detected - CEF has limited support\n");
        // Wayland is more complex, might need OSR (off-screen rendering)
        window_info.SetAsWindowless(0);
    }
    #endif
#endif

    // Browser settings
    CefBrowserSettings browser_settings;
    browser_settings.windowless_frame_rate = 60;  // If using windowless mode

    // Create browser client
    CefRefPtr<SimpleCEFClient> client(new SimpleCEFClient());

    // Create browser
    CefRefPtr<CefBrowser> browser = CefBrowserHost::CreateBrowserSync(
        window_info,
        client,
        "https://www.google.com",
        browser_settings,
        nullptr,
        nullptr
    );

    if (browser) {
        g_print("✅ CEF browser created successfully!\n");
        g_print("🌐 Loading https://www.google.com\n");

        // Store browser reference
        g_window_data = new WindowData();
        g_window_data->window = window;
        g_window_data->browser = browser;

        // Schedule CEF message loop work
        g_timeout_add(10, on_cef_work, nullptr);
    } else {
        g_printerr("❌ Failed to create CEF browser\n");
    }
}

int main(int argc, char* argv[]) {
    g_print("🐕 Braya Browser - CEF Prototype\n");
    g_print("================================\n\n");

    // Initialize CEF
    g_print("🔧 Initializing CEF...\n");

    CefMainArgs main_args(argc, argv);

    // CEF app
    CefRefPtr<SimpleCEFApp> app(new SimpleCEFApp());

    // CEF settings
    CefSettings settings;
    settings.no_sandbox = true;  // Disable sandbox for simplicity
    settings.windowless_rendering_enabled = true;  // Enable if needed

    // Set resource paths
    CefString(&settings.resources_dir_path).FromASCII("/usr/lib64/cef");
    CefString(&settings.locales_dir_path).FromASCII("/usr/lib64/cef/locales");

    // Initialize CEF
    if (!CefInitialize(main_args, settings, app, nullptr)) {
        g_printerr("❌ Failed to initialize CEF\n");
        return 1;
    }

    g_print("✓ CEF initialized\n\n");

    // Initialize GTK
    g_print("🔧 Initializing GTK4...\n");
    GtkApplication* gtk_app = gtk_application_new(
        "dev.braya.cef.prototype",
        G_APPLICATION_DEFAULT_FLAGS
    );

    g_signal_connect(gtk_app, "activate", G_CALLBACK(activate), nullptr);

    g_print("✓ GTK4 initialized\n\n");
    g_print("🚀 Starting application...\n\n");

    // Run GTK app
    int status = g_application_run(G_APPLICATION(gtk_app), argc, argv);

    // Cleanup
    g_print("\n🧹 Cleaning up...\n");

    if (g_window_data) {
        delete g_window_data;
    }

    g_object_unref(gtk_app);

    // Shutdown CEF
    CefShutdown();
    g_print("✓ CEF shutdown complete\n");

    g_print("\n✅ Prototype complete!\n");
    return status;
}
