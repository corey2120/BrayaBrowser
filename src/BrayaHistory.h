#ifndef BRAYA_HISTORY_H
#define BRAYA_HISTORY_H

#include <gtk/gtk.h>
#include <string>
#include <vector>
#include <ctime>

struct HistoryEntry {
    std::string url;
    std::string title;
    time_t timestamp;
    
    HistoryEntry() : url(""), title(""), timestamp(0) {}
    HistoryEntry(const std::string& u, const std::string& t, time_t ts)
        : url(u), title(t), timestamp(ts) {}
};

class BrayaHistory {
public:
    BrayaHistory();
    ~BrayaHistory();
    
    void addEntry(const std::string& url, const std::string& title);
    void clearHistory();
    std::vector<HistoryEntry> getHistory(int limit = 100);
    void showHistoryDialog(GtkWindow* parent);
    
    void saveToFile();
    void loadFromFile();
    
private:
    std::vector<HistoryEntry> entries;
    std::string historyFilePath;
    GtkWidget* historyDialog;
    
    void createHistoryDialog(GtkWindow* parent);
    static void onClearHistoryClicked(GtkButton* button, gpointer data);
    static void onCloseClicked(GtkButton* button, gpointer data);
    static void onHistoryItemClicked(GtkListBox* box, GtkListBoxRow* row, gpointer data);
    
    std::string getHistoryFilePath();
};

#endif // BRAYA_HISTORY_H
