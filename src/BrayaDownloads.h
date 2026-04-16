#ifndef BRAYA_DOWNLOADS_H
#define BRAYA_DOWNLOADS_H

#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <string>
#include <vector>
#include <map>
#include <functional>

struct DownloadItem {
    WebKitDownload* download;
    std::string filename;
    std::string url;
    guint64 totalSize;
    guint64 receivedSize;
    bool completed;
    bool failed;
    time_t startTime;
    
    DownloadItem() : download(nullptr), totalSize(0), receivedSize(0), 
                     completed(false), failed(false), startTime(0) {}
};

class BrayaDownloads {
public:
    BrayaDownloads();
    ~BrayaDownloads();

    void handleDownload(WebKitDownload* download);
    void showDownloadsDialog(GtkWindow* parent);
    void clearCompleted();
    int getActiveDownloadCount() const;

    // Callback for download status changes
    void setDownloadStatusCallback(std::function<void(int)> callback) {
        downloadStatusCallback = callback;
    }

private:
    std::vector<DownloadItem> downloads;
    GtkWidget* downloadsDialog;
    GtkWidget* downloadsList;
    std::map<WebKitDownload*, GtkWidget*> downloadRows;
    std::function<void(int)> downloadStatusCallback;

    void createDownloadsDialog(GtkWindow* parent);
    void addDownloadToUI(const DownloadItem& item);
    void updateDownloadProgress(WebKitDownload* download);
    
    static void onDownloadDecideDestination(WebKitDownload* download, const gchar* suggested_filename, gpointer data);
    static void onDownloadReceived(WebKitDownload* download, guint64 data_length, gpointer user_data);
    static void onDownloadFinished(WebKitDownload* download, gpointer user_data);
    static void onDownloadFailed(WebKitDownload* download, GError* error, gpointer user_data);
    
    static void onOpenFileClicked(GtkButton* button, gpointer data);
    static void onOpenFolderClicked(GtkButton* button, gpointer data);
    static void onCancelDownloadClicked(GtkButton* button, gpointer data);
    static void onClearCompletedClicked(GtkButton* button, gpointer data);
    static void onCloseClicked(GtkButton* button, gpointer data);
    
    std::string formatFileSize(guint64 bytes);
    std::string getDownloadsPath();
};

#endif // BRAYA_DOWNLOADS_H
