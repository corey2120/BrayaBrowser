#include <gtk/gtk.h>
#include "BrayaWindow.h"
#include <csignal>
#include <iostream>
#include <ctime>

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
