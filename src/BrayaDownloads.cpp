#include "BrayaDownloads.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>

BrayaDownloads::BrayaDownloads() : downloadsDialog(nullptr), downloadsList(nullptr) {
}

BrayaDownloads::~BrayaDownloads() {
}

std::string BrayaDownloads::getDownloadsPath() {
    return std::string(g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD));
}

std::string BrayaDownloads::formatFileSize(guint64 bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit = 0;
    double size = bytes;
    
    while (size >= 1024 && unit < 3) {
        size /= 1024;
        unit++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return oss.str();
}

int BrayaDownloads::getActiveDownloadCount() const {
    int count = 0;
    for (const auto& item : downloads) {
        if (!item.completed && !item.failed) {
            count++;
        }
    }
    return count;
}

void BrayaDownloads::handleDownload(WebKitDownload* download) {
    DownloadItem item;
    item.download = download;
    item.url = webkit_uri_request_get_uri(webkit_download_get_request(download));
    item.startTime = std::time(nullptr);

    g_object_ref(download);

    // Connect signals
    g_signal_connect(download, "decide-destination", G_CALLBACK(onDownloadDecideDestination), this);
    g_signal_connect(download, "received-data", G_CALLBACK(onDownloadReceived), this);
    g_signal_connect(download, "finished", G_CALLBACK(onDownloadFinished), this);
    g_signal_connect(download, "failed", G_CALLBACK(onDownloadFailed), this);

    downloads.push_back(item);

    std::cout << "Download started: " << item.url << std::endl;

    // Notify callback of new active download
    if (downloadStatusCallback) {
        downloadStatusCallback(getActiveDownloadCount());
    }
}

gboolean BrayaDownloads::onDownloadDecideDestination(WebKitDownload* download, const gchar* suggested_filename, gpointer data) {
    BrayaDownloads* downloads = static_cast<BrayaDownloads*>(data);

    if (!suggested_filename || strlen(suggested_filename) == 0) {
        std::cerr << "✗ No filename suggested for download" << std::endl;
        return FALSE;
    }

    std::string downloadsPath = downloads->getDownloadsPath();

    if (downloadsPath.empty()) {
        downloadsPath = std::string(g_get_home_dir()) + "/Downloads";
    }

    std::string filePath = downloadsPath + "/" + std::string(suggested_filename);

    std::cout << "📥 Download destination: " << filePath << std::endl;

    webkit_download_set_destination(download, filePath.c_str());

    for (auto& item : downloads->downloads) {
        if (item.download == download) {
            item.filename = suggested_filename;
            break;
        }
    }

    return TRUE;
}

void BrayaDownloads::onDownloadReceived(WebKitDownload* download, guint64 data_length, gpointer user_data) {
    BrayaDownloads* downloads = static_cast<BrayaDownloads*>(user_data);
    downloads->updateDownloadProgress(download);
}

void BrayaDownloads::onDownloadFinished(WebKitDownload* download, gpointer user_data) {
    BrayaDownloads* downloads = static_cast<BrayaDownloads*>(user_data);

    for (auto& item : downloads->downloads) {
        if (item.download == download) {
            item.completed = true;
            item.receivedSize = item.totalSize;
            std::cout << "Download completed: " << item.filename << std::endl;
            break;
        }
    }

    downloads->updateDownloadProgress(download);

    // Notify callback of download completion
    if (downloads->downloadStatusCallback) {
        downloads->downloadStatusCallback(downloads->getActiveDownloadCount());
    }
}

void BrayaDownloads::onDownloadFailed(WebKitDownload* download, GError* error, gpointer user_data) {
    BrayaDownloads* downloads = static_cast<BrayaDownloads*>(user_data);

    for (auto& item : downloads->downloads) {
        if (item.download == download) {
            item.failed = true;
            std::cerr << "Download failed: " << item.filename << " - " << error->message << std::endl;
            break;
        }
    }

    downloads->updateDownloadProgress(download);

    // Notify callback of download failure
    if (downloads->downloadStatusCallback) {
        downloads->downloadStatusCallback(downloads->getActiveDownloadCount());
    }
}

void BrayaDownloads::updateDownloadProgress(WebKitDownload* download) {
    auto it = downloadRows.find(download);
    if (it == downloadRows.end()) {
        return; // Row not created yet
    }
    
    GtkWidget* row = it->second;
    GtkWidget* progressBar = GTK_WIDGET(g_object_get_data(G_OBJECT(row), "progress-bar"));
    GtkWidget* statusLabel = GTK_WIDGET(g_object_get_data(G_OBJECT(row), "status-label"));
    
    if (!progressBar || !statusLabel) return;
    
    guint64 received = webkit_download_get_received_data_length(download);
    gdouble estimated_progress = webkit_download_get_estimated_progress(download);
    
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar), estimated_progress);
    
    // Find the item
    for (const auto& item : downloads) {
        if (item.download == download) {
            std::string status;
            if (item.completed) {
                status = "✓ Complete - " + formatFileSize(received);
                gtk_widget_add_css_class(row, "download-complete");
            } else if (item.failed) {
                status = "✗ Failed";
                gtk_widget_add_css_class(row, "download-failed");
            } else {
                int percent = (int)(estimated_progress * 100);
                status = std::to_string(percent) + "% - " + formatFileSize(received);
            }
            gtk_label_set_text(GTK_LABEL(statusLabel), status.c_str());
            break;
        }
    }
}

void BrayaDownloads::showDownloadsDialog(GtkWindow* parent) {
    if (downloadsDialog) {
        gtk_window_present(GTK_WINDOW(downloadsDialog));
        return;
    }
    
    createDownloadsDialog(parent);
    
    // Add existing downloads
    for (const auto& item : downloads) {
        addDownloadToUI(item);
    }
    
    gtk_window_present(GTK_WINDOW(downloadsDialog));
}

void BrayaDownloads::createDownloadsDialog(GtkWindow* parent) {
    downloadsDialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(downloadsDialog), "🐕 Downloads");
    gtk_window_set_default_size(GTK_WINDOW(downloadsDialog), 600, 400);
    gtk_window_set_transient_for(GTK_WINDOW(downloadsDialog), parent);
    
    GtkWidget* mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(downloadsDialog), mainBox);
    
    // Header
    GtkWidget* headerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(headerBox, 15);
    gtk_widget_set_margin_end(headerBox, 15);
    gtk_widget_set_margin_top(headerBox, 15);
    gtk_widget_set_margin_bottom(headerBox, 15);
    gtk_box_append(GTK_BOX(mainBox), headerBox);
    
    GtkWidget* titleLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(titleLabel), "<span size='large' weight='bold'>📥 Downloads</span>");
    gtk_widget_set_hexpand(titleLabel, TRUE);
    gtk_widget_set_halign(titleLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(headerBox), titleLabel);
    
    GtkWidget* clearBtn = gtk_button_new_with_label("Clear Completed");
    g_signal_connect(clearBtn, "clicked", G_CALLBACK(onClearCompletedClicked), this);
    gtk_box_append(GTK_BOX(headerBox), clearBtn);
    
    gtk_box_append(GTK_BOX(mainBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // Downloads list
    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_box_append(GTK_BOX(mainBox), scrolled);
    
    downloadsList = gtk_list_box_new();
    gtk_widget_add_css_class(downloadsList, "downloads-list");
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), downloadsList);
    
    gtk_box_append(GTK_BOX(mainBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // Bottom buttons
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

void BrayaDownloads::addDownloadToUI(const DownloadItem& item) {
    if (!downloadsList) return;
    
    GtkWidget* row = gtk_list_box_row_new();
    gtk_widget_add_css_class(row, "download-row");
    
    GtkWidget* rowBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(rowBox, 15);
    gtk_widget_set_margin_end(rowBox, 15);
    gtk_widget_set_margin_top(rowBox, 10);
    gtk_widget_set_margin_bottom(rowBox, 10);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), rowBox);
    
    // Filename
    GtkWidget* filenameLabel = gtk_label_new(item.filename.empty() ? "Downloading..." : item.filename.c_str());
    gtk_widget_set_halign(filenameLabel, GTK_ALIGN_START);
    gtk_widget_add_css_class(filenameLabel, "download-filename");
    gtk_box_append(GTK_BOX(rowBox), filenameLabel);
    
    // Progress bar
    GtkWidget* progressBar = gtk_progress_bar_new();
    gtk_widget_add_css_class(progressBar, "download-progress");
    gtk_box_append(GTK_BOX(rowBox), progressBar);
    
    // Status and buttons
    GtkWidget* bottomBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(rowBox), bottomBox);
    
    GtkWidget* statusLabel = gtk_label_new("Starting...");
    gtk_widget_set_halign(statusLabel, GTK_ALIGN_START);
    gtk_widget_set_hexpand(statusLabel, TRUE);
    gtk_widget_add_css_class(statusLabel, "download-status");
    gtk_box_append(GTK_BOX(bottomBox), statusLabel);
    
    // Action buttons
    if (item.completed) {
        GtkWidget* openBtn = gtk_button_new_with_label("Open");
        gtk_widget_add_css_class(openBtn, "flat");
        g_object_set_data(G_OBJECT(openBtn), "download", item.download);
        g_signal_connect(openBtn, "clicked", G_CALLBACK(onOpenFileClicked), this);
        gtk_box_append(GTK_BOX(bottomBox), openBtn);
        
        GtkWidget* folderBtn = gtk_button_new_with_label("Show in Folder");
        gtk_widget_add_css_class(folderBtn, "flat");
        g_object_set_data(G_OBJECT(folderBtn), "download", item.download);
        g_signal_connect(folderBtn, "clicked", G_CALLBACK(onOpenFolderClicked), this);
        gtk_box_append(GTK_BOX(bottomBox), folderBtn);
    } else if (!item.failed) {
        GtkWidget* cancelBtn = gtk_button_new_with_label("Cancel");
        gtk_widget_add_css_class(cancelBtn, "flat");
        g_object_set_data(G_OBJECT(cancelBtn), "download", item.download);
        g_signal_connect(cancelBtn, "clicked", G_CALLBACK(onCancelDownloadClicked), this);
        gtk_box_append(GTK_BOX(bottomBox), cancelBtn);
    }
    
    // Store widgets for updates
    g_object_set_data(G_OBJECT(row), "progress-bar", progressBar);
    g_object_set_data(G_OBJECT(row), "status-label", statusLabel);
    
    downloadRows[item.download] = row;
    
    gtk_list_box_append(GTK_LIST_BOX(downloadsList), row);
}

void BrayaDownloads::clearCompleted() {
    downloads.erase(
        std::remove_if(downloads.begin(), downloads.end(),
            [](const DownloadItem& item) { return item.completed; }),
        downloads.end()
    );
    
    // Rebuild UI
    if (downloadsDialog) {
        gtk_window_destroy(GTK_WINDOW(downloadsDialog));
        downloadsDialog = nullptr;
        downloadsList = nullptr;
        downloadRows.clear();
    }
}

void BrayaDownloads::onClearCompletedClicked(GtkButton* button, gpointer data) {
    BrayaDownloads* downloads = static_cast<BrayaDownloads*>(data);
    downloads->clearCompleted();
}

void BrayaDownloads::onCloseClicked(GtkButton* button, gpointer data) {
    BrayaDownloads* downloads = static_cast<BrayaDownloads*>(data);
    if (downloads->downloadsDialog) {
        gtk_window_destroy(GTK_WINDOW(downloads->downloadsDialog));
        downloads->downloadsDialog = nullptr;
        downloads->downloadsList = nullptr;
        downloads->downloadRows.clear();
    }
}

void BrayaDownloads::onOpenFileClicked(GtkButton* button, gpointer data) {
    WebKitDownload* download = (WebKitDownload*)g_object_get_data(G_OBJECT(button), "download");
    if (download) {
        const gchar* destination = webkit_download_get_destination(download);
        if (destination) {
            // Open file with default application
            GtkFileLauncher* launcher = gtk_file_launcher_new(g_file_new_for_uri(destination));
            gtk_file_launcher_launch(launcher, nullptr, nullptr, nullptr, nullptr);
            g_object_unref(launcher);
        }
    }
}

void BrayaDownloads::onOpenFolderClicked(GtkButton* button, gpointer data) {
    BrayaDownloads* downloads = static_cast<BrayaDownloads*>(data);
    std::string downloadsPath = "file://" + downloads->getDownloadsPath();
    
    GtkFileLauncher* launcher = gtk_file_launcher_new(g_file_new_for_uri(downloadsPath.c_str()));
    gtk_file_launcher_launch(launcher, nullptr, nullptr, nullptr, nullptr);
    g_object_unref(launcher);
}

void BrayaDownloads::onCancelDownloadClicked(GtkButton* button, gpointer data) {
    WebKitDownload* download = (WebKitDownload*)g_object_get_data(G_OBJECT(button), "download");
    if (download) {
        webkit_download_cancel(download);
    }
}
