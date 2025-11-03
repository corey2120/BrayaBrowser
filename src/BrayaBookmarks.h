#ifndef BRAYA_BOOKMARKS_H
#define BRAYA_BOOKMARKS_H

#include <gtk/gtk.h>
#include <string>
#include <vector>
#include <ctime>

struct Bookmark {
    std::string name;
    std::string url;
    std::string folder;
    std::string faviconPath;  // Path to cached favicon
    time_t timestamp;
    
    Bookmark() : name(""), url(""), folder(""), faviconPath(""), timestamp(0) {}
    Bookmark(const std::string& n, const std::string& u, const std::string& f = "", const std::string& fav = "")
        : name(n), url(u), folder(f), faviconPath(fav), timestamp(std::time(nullptr)) {}
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
    std::vector<Bookmark> searchBookmarks(const std::string& query);
    
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
    void addCurrentPage(const std::string& title, const std::string& url, GdkTexture* favicon);
    
    // Speed dial / New tab page
    GtkWidget* createSpeedDial();
    std::vector<Bookmark> getFavoriteBookmarks(int count = 12);
    
private:
    std::vector<Bookmark> bookmarks;
    std::string bookmarksFilePath;
    GtkWidget* managerDialog;
    GtkWidget* bookmarksList;
    GtkWidget* searchEntry;
    
    void createManagerDialog(GtkWindow* parent);
    void refreshBookmarksList();
    
    static void onAddBookmarkClicked(GtkButton* button, gpointer data);
    static void onEditBookmarkClicked(GtkButton* button, gpointer data);
    static void onDeleteBookmarkClicked(GtkButton* button, gpointer data);
    static void onBookmarkDoubleClicked(GtkListBox* box, GtkListBoxRow* row, gpointer data);
    static void onSearchChanged(GtkEntry* entry, gpointer data);
    static void onCloseClicked(GtkButton* button, gpointer data);
    
    std::string getBookmarksFilePath();
};

#endif // BRAYA_BOOKMARKS_H
