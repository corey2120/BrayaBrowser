#ifndef BRAYA_BOOKMARKS_H
#define BRAYA_BOOKMARKS_H

#include <gtk/gtk.h>
#include <string>
#include <vector>
#include <ctime>
#include <functional>
#include <iostream>

struct Bookmark {
    std::string name;
    std::string url;
    std::string folder;          // Parent folder path (for non-folder bookmarks)
    std::string faviconPath;     // Path to cached favicon
    time_t timestamp;
    bool isFolder;               // True if this is a folder, not a bookmark
    std::vector<Bookmark> children;  // Child bookmarks/folders (for folder items)

    Bookmark() : name(""), url(""), folder(""), faviconPath(""), timestamp(0), isFolder(false) {}
    Bookmark(const std::string& n, const std::string& u, const std::string& f = "", const std::string& fav = "")
        : name(n), url(u), folder(f), faviconPath(fav), timestamp(std::time(nullptr)), isFolder(false) {}

    // Constructor for folder
    static Bookmark createFolder(const std::string& folderName) {
        Bookmark folder;
        folder.name = folderName;
        folder.isFolder = true;
        folder.timestamp = std::time(nullptr);
        return folder;
    }
};

class BrayaBookmarks {
public:
    BrayaBookmarks();
    ~BrayaBookmarks();
    
    void addBookmark(const std::string& name, const std::string& url, const std::string& folder = "");
    void editBookmark(int index, const std::string& name, const std::string& url, const std::string& folder);
    void editBookmarkByUrl(const std::string& oldUrl, const std::string& name, const std::string& url, const std::string& folder);
    void deleteBookmark(int index);
    void deleteBookmarkByUrl(const std::string& url);
    int findBookmarkByUrl(const std::string& url);
    std::vector<Bookmark> getBookmarks();
    std::vector<Bookmark>& getBookmarksRef();  // For drag & drop reordering
    std::vector<Bookmark> searchBookmarks(const std::string& query);
    std::vector<std::string> getUniqueFolders();

    void showBookmarksManager(GtkWindow* parent);
    void saveToFile();
    void loadFromFile();
    
    // Visual bookmarks bar
    GtkWidget* createBookmarksBar();
    void updateBookmarksBar(GtkWidget* bookmarksBar,
                           gpointer windowPtr = nullptr,
                           GCallback clickCallback = nullptr,
                           GCallback addCallback = nullptr,
                           GCallback rightClickCallback = nullptr);
    void filterBookmarksBar(GtkWidget* bookmarksBar, const std::string& query);
    void addCurrentPage(const std::string& title, const std::string& url, GdkTexture* favicon);

    // Set refresh callback for UI updates
    void setRefreshCallback(std::function<void()> callback) {
        std::cout << "📌 setRefreshCallback called - callback is " << (callback ? "SET" : "NULL") << std::endl;
        refreshCallback = callback;
    }
    void triggerRefresh() {
        std::cout << "🔄 triggerRefresh called - refreshCallback is " << (refreshCallback ? "SET" : "NULL") << std::endl;
        if (refreshCallback) {
            std::cout << "🔄 Calling refreshCallback..." << std::endl;
            refreshCallback();
            std::cout << "🔄 refreshCallback returned" << std::endl;
        } else {
            std::cout << "⚠️  WARNING: refreshCallback is NULL!" << std::endl;
        }
    }

    // Speed dial / New tab page
    GtkWidget* createSpeedDial();
    std::vector<Bookmark> getFavoriteBookmarks(int count = 12);

    // Folder management
    static std::string showNewFolderDialog(GtkWindow* parent);
    void addFolder(const std::string& folderName, const std::string& parentFolder = "");
    void addBookmarkToFolder(const std::string& name, const std::string& url, const std::string& folderName);
    Bookmark* findFolder(const std::string& folderName);
    std::vector<Bookmark> getBookmarksInFolder(const std::string& folderName);
    void deleteFolder(const std::string& folderName);

    // Import/Export
    bool importFromChrome(const std::string& filepath);
    bool importFromFirefox(const std::string& filepath);
    bool importFromHTML(const std::string& filepath);
    bool exportToHTML(const std::string& filepath);
    void showImportExportDialog(GtkWindow* parent);

private:
    std::vector<Bookmark> bookmarks;
    std::string bookmarksFilePath;
    GtkWidget* managerDialog;
    GtkWidget* bookmarksList;
    GtkWidget* searchEntry;
    std::function<void()> refreshCallback;
    
    void createManagerDialog(GtkWindow* parent);
    void refreshBookmarksList();

    static void onAddBookmarkClicked(GtkButton* button, gpointer data);
    static void onAddFolderClicked(GtkButton* button, gpointer data);
    static void onEditBookmarkClicked(GtkButton* button, gpointer data);
    static void onDeleteBookmarkClicked(GtkButton* button, gpointer data);
    static void onBookmarkDoubleClicked(GtkListBox* box, GtkListBoxRow* row, gpointer data);
    static void onSearchChanged(GtkEntry* entry, gpointer data);
    static void onCloseClicked(GtkButton* button, gpointer data);
    
    std::string getBookmarksFilePath();
};

#endif // BRAYA_BOOKMARKS_H
