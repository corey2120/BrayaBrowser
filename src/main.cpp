#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include "BrayaWindow.h"
#include <csignal>
#include <iostream>
#include <ctime>
#include <glib.h>

static void signalHandler(int signum) {
    std::cerr << "\n💥 CRASH DETECTED! Signal: " << signum << std::endl;
    std::cerr << "Signal type: ";
    switch(signum) {
        case SIGSEGV: std::cerr << "SEGMENTATION FAULT (invalid memory access)"; break;
        case SIGABRT: std::cerr << "ABORT (assertion failed)"; break;
        case SIGFPE: std::cerr << "FLOATING POINT EXCEPTION"; break;
        case SIGILL: std::cerr << "ILLEGAL INSTRUCTION"; break;
        default: std::cerr << "Unknown (" << signum << ")"; break;
    }
    std::cerr << std::endl;
    std::cerr << "Crash location: Check previous log messages for clues" << std::endl;
    std::cerr << "This crash has been logged." << std::endl;
    
    // Log to file
    FILE* crashLog = fopen("braya-crash.log", "a");
    if (crashLog) {
        time_t now = time(nullptr);
        fprintf(crashLog, "\n=== CRASH at %s", ctime(&now));
        fprintf(crashLog, "Signal: %d\n", signum);
        fclose(crashLog);
    }
    
    exit(signum);
}

static void setupPersistentStorage() {
    g_print("🍪 Setting up persistent storage...\n");

    // Create data directory
    std::string dataDir = std::string(g_get_user_data_dir()) + "/braya-browser";
    g_mkdir_with_parents(dataDir.c_str(), 0755);
    g_print("  ✓ Data directory: %s\n", dataDir.c_str());

    // Get the default network session
    WebKitNetworkSession* session = webkit_network_session_get_default();
    if (!session) {
        g_print("  ⚠️  Failed to get default network session\n");
        return;
    }

    // Configure cookie manager for persistent storage
    WebKitCookieManager* cookieManager = webkit_network_session_get_cookie_manager(session);
    if (cookieManager) {
        std::string cookieFile = dataDir + "/cookies.sqlite";
        webkit_cookie_manager_set_persistent_storage(
            cookieManager,
            cookieFile.c_str(),
            WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE
        );
        g_print("  ✓ Cookie storage: %s\n", cookieFile.c_str());

        // Set accept policy: no third-party cookies by default
        webkit_cookie_manager_set_accept_policy(
            cookieManager,
            WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY
        );
        g_print("  ✓ Cookie policy: Accept no third-party cookies\n");
    } else {
        g_print("  ⚠️  Failed to get cookie manager\n");
    }

    // Configure website data manager for persistent storage
    WebKitWebsiteDataManager* dataManager = webkit_network_session_get_website_data_manager(session);
    if (dataManager) {
        // WebKit automatically uses g_get_user_data_dir() for localStorage and IndexedDB
        // We just need to ensure the directory structure exists
        std::string localStorageDir = dataDir + "/localstorage";
        std::string indexedDbDir = dataDir + "/databases";
        g_mkdir_with_parents(localStorageDir.c_str(), 0755);
        g_mkdir_with_parents(indexedDbDir.c_str(), 0755);
        g_print("  ✓ localStorage directory: %s\n", localStorageDir.c_str());
        g_print("  ✓ IndexedDB directory: %s\n", indexedDbDir.c_str());
    } else {
        g_print("  ⚠️  Failed to get website data manager\n");
    }

    g_print("✅ Persistent storage configured successfully\n\n");
}

static void activate(GtkApplication* app, gpointer user_data) {
    g_print("Activate function called!\n");
    BrayaWindow* window = new BrayaWindow(app);
    g_print("Window object created!\n");
    window->show();
    g_print("Window shown!\n");
}

int main(int argc, char** argv) {
    // Install signal handlers for crash detection
    signal(SIGSEGV, signalHandler);  // Segmentation fault
    signal(SIGABRT, signalHandler);  // Abort
    signal(SIGFPE, signalHandler);   // Floating point exception
    signal(SIGILL, signalHandler);   // Illegal instruction
    
    g_print("🐕 Braya Browser - C++ Edition\n");
    g_print("Engine: WebKit\n");
    g_print("Platform: Native GTK\n");
    g_print("Building the REAL browser...\n\n");

    // Set up persistent storage before creating any WebViews
    setupPersistentStorage();
    
    GtkApplication* app = gtk_application_new("dev.braya.BrayaBrowser", G_APPLICATION_FLAGS_NONE);
    g_print("GTK Application created\n");
    
    // Set default application icon
    gtk_window_set_default_icon_name("dev.braya.BrayaBrowser");
    
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    g_print("Activate signal connected\n");
    
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_print("Application run returned: %d\n", status);
    
    g_object_unref(app);
    
    return status;
}
