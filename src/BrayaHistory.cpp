#include "BrayaHistory.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>

BrayaHistory::BrayaHistory() : historyDialog(nullptr) {
    historyFilePath = getHistoryFilePath();
    loadFromFile();
}

BrayaHistory::~BrayaHistory() {
    saveToFile();
}

std::string BrayaHistory::getHistoryFilePath() {
    std::string configDir = std::string(g_get_user_config_dir()) + "/braya";
    g_mkdir_with_parents(configDir.c_str(), 0755);
    return configDir + "/history.json";
}

void BrayaHistory::addEntry(const std::string& url, const std::string& title) {
    // Don't save about:braya or empty URLs
    if (url.empty() || url.find("about:") == 0) {
        return;
    }
    
    // Remove duplicate if exists
    entries.erase(
        std::remove_if(entries.begin(), entries.end(),
            [&url](const HistoryEntry& e) { return e.url == url; }),
        entries.end()
    );
    
    // Add new entry at the beginning
    entries.insert(entries.begin(), HistoryEntry(url, title, std::time(nullptr)));
    
    // Keep only last 1000 entries
    if (entries.size() > 1000) {
        entries.resize(1000);
    }
    
    saveToFile();
}

void BrayaHistory::clearHistory() {
    entries.clear();
    saveToFile();
}

std::vector<HistoryEntry> BrayaHistory::getHistory(int limit) {
    if (limit < 0 || limit > (int)entries.size()) {
        return entries;
    }
    return std::vector<HistoryEntry>(entries.begin(), entries.begin() + limit);
}

void BrayaHistory::saveToFile() {
    std::ofstream file(historyFilePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open history file for writing: " << historyFilePath << std::endl;
        return;
    }
    
    file << "[\n";
    for (size_t i = 0; i < entries.size(); i++) {
        const auto& entry = entries[i];
        file << "  {\n";
        file << "    \"url\": \"" << entry.url << "\",\n";
        file << "    \"title\": \"" << entry.title << "\",\n";
        file << "    \"timestamp\": " << entry.timestamp << "\n";
        file << "  }" << (i < entries.size() - 1 ? "," : "") << "\n";
    }
    file << "]\n";
    file.close();
}

void BrayaHistory::loadFromFile() {
    std::ifstream file(historyFilePath);
    if (!file.is_open()) {
        return; // File doesn't exist yet, that's ok
    }
    
    entries.clear();
    std::string line;
    std::string url, title;
    time_t timestamp = 0;
    
    while (std::getline(file, line)) {
        // Simple JSON parsing (just enough for our format)
        if (line.find("\"url\":") != std::string::npos) {
            size_t start = line.find("\"", line.find(":")) + 1;
            size_t end = line.rfind("\"");
            url = line.substr(start, end - start);
        }
        else if (line.find("\"title\":") != std::string::npos) {
            size_t start = line.find("\"", line.find(":")) + 1;
            size_t end = line.rfind("\"");
            title = line.substr(start, end - start);
        }
        else if (line.find("\"timestamp\":") != std::string::npos) {
            size_t start = line.find(":") + 1;
            timestamp = std::stoll(line.substr(start));
            
            // We have a complete entry
            if (!url.empty()) {
                entries.push_back(HistoryEntry(url, title, timestamp));
                url.clear();
                title.clear();
                timestamp = 0;
            }
        }
    }
    file.close();
}

void BrayaHistory::showHistoryDialog(GtkWindow* parent) {
    if (historyDialog) {
        gtk_window_present(GTK_WINDOW(historyDialog));
        return;
    }
    
    createHistoryDialog(parent);
    gtk_window_present(GTK_WINDOW(historyDialog));
}

void BrayaHistory::createHistoryDialog(GtkWindow* parent) {
    historyDialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(historyDialog), "🐕 Browsing History");
    gtk_window_set_default_size(GTK_WINDOW(historyDialog), 700, 500);
    gtk_window_set_transient_for(GTK_WINDOW(historyDialog), parent);
    gtk_window_set_modal(GTK_WINDOW(historyDialog), TRUE);
    
    GtkWidget* mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(historyDialog), mainBox);
    
    // Header
    GtkWidget* headerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(headerBox, 15);
    gtk_widget_set_margin_end(headerBox, 15);
    gtk_widget_set_margin_top(headerBox, 15);
    gtk_widget_set_margin_bottom(headerBox, 15);
    gtk_box_append(GTK_BOX(mainBox), headerBox);
    
    GtkWidget* titleLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(titleLabel), "<span size='large' weight='bold'>📖 Browsing History</span>");
    gtk_widget_set_hexpand(titleLabel, TRUE);
    gtk_widget_set_halign(titleLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(headerBox), titleLabel);
    
    GtkWidget* clearBtn = gtk_button_new_with_label("Clear History");
    gtk_widget_add_css_class(clearBtn, "destructive-action");
    g_signal_connect(clearBtn, "clicked", G_CALLBACK(onClearHistoryClicked), this);
    gtk_box_append(GTK_BOX(headerBox), clearBtn);
    
    gtk_box_append(GTK_BOX(mainBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // Scrolled window with history list
    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_box_append(GTK_BOX(mainBox), scrolled);
    
    GtkWidget* listBox = gtk_list_box_new();
    gtk_widget_add_css_class(listBox, "history-list");
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), listBox);
    
    // Add history entries
    for (const auto& entry : entries) {
        GtkWidget* row = gtk_list_box_row_new();
        
        GtkWidget* entryBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_widget_set_margin_start(entryBox, 15);
        gtk_widget_set_margin_end(entryBox, 15);
        gtk_widget_set_margin_top(entryBox, 10);
        gtk_widget_set_margin_bottom(entryBox, 10);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), entryBox);
        
        // Title
        GtkWidget* titleLabel = gtk_label_new(entry.title.empty() ? "Untitled" : entry.title.c_str());
        gtk_widget_set_halign(titleLabel, GTK_ALIGN_START);
        gtk_label_set_ellipsize(GTK_LABEL(titleLabel), PANGO_ELLIPSIZE_END);
        gtk_widget_add_css_class(titleLabel, "history-title");
        gtk_box_append(GTK_BOX(entryBox), titleLabel);
        
        // URL
        GtkWidget* urlLabel = gtk_label_new(entry.url.c_str());
        gtk_widget_set_halign(urlLabel, GTK_ALIGN_START);
        gtk_label_set_ellipsize(GTK_LABEL(urlLabel), PANGO_ELLIPSIZE_END);
        gtk_widget_add_css_class(urlLabel, "history-url");
        gtk_box_append(GTK_BOX(entryBox), urlLabel);
        
        // Time
        char timeStr[100];
        struct tm* timeInfo = localtime(&entry.timestamp);
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeInfo);
        GtkWidget* timeLabel = gtk_label_new(timeStr);
        gtk_widget_set_halign(timeLabel, GTK_ALIGN_START);
        gtk_widget_add_css_class(timeLabel, "history-time");
        gtk_box_append(GTK_BOX(entryBox), timeLabel);
        
        // Store URL in row data
        g_object_set_data_full(G_OBJECT(row), "url", g_strdup(entry.url.c_str()), g_free);
        
        gtk_list_box_append(GTK_LIST_BOX(listBox), row);
    }
    
    g_signal_connect(listBox, "row-activated", G_CALLBACK(onHistoryItemClicked), this);
    
    gtk_box_append(GTK_BOX(mainBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // Bottom button
    GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(buttonBox, 15);
    gtk_widget_set_margin_end(buttonBox, 15);
    gtk_widget_set_margin_top(buttonBox, 10);
    gtk_widget_set_margin_bottom(buttonBox, 10);
    gtk_widget_set_halign(buttonBox, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(mainBox), buttonBox);
    
    GtkWidget* closeBtn = gtk_button_new_with_label("Close");
    g_signal_connect(closeBtn, "clicked", G_CALLBACK(onCloseClicked), this);
    gtk_box_append(GTK_BOX(buttonBox), closeBtn);
}

void BrayaHistory::onClearHistoryClicked(GtkButton* button, gpointer data) {
    BrayaHistory* history = static_cast<BrayaHistory*>(data);
    history->clearHistory();
    
    // Close and reopen dialog to refresh
    if (history->historyDialog) {
        gtk_window_destroy(GTK_WINDOW(history->historyDialog));
        history->historyDialog = nullptr;
    }
}

void BrayaHistory::onCloseClicked(GtkButton* button, gpointer data) {
    BrayaHistory* history = static_cast<BrayaHistory*>(data);
    if (history->historyDialog) {
        gtk_window_destroy(GTK_WINDOW(history->historyDialog));
        history->historyDialog = nullptr;
    }
}

void BrayaHistory::onHistoryItemClicked(GtkListBox* box, GtkListBoxRow* row, gpointer data) {
    const char* url = (const char*)g_object_get_data(G_OBJECT(row), "url");
    if (url) {
        // TODO: Navigate to URL (need reference to window)
        std::cout << "Would navigate to: " << url << std::endl;
    }
}
