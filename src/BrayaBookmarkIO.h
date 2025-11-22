#ifndef BRAYA_BOOKMARK_IO_H
#define BRAYA_BOOKMARK_IO_H

#include <string>
#include <vector>

struct ImportedBookmark {
    std::string title;
    std::string url;
    std::string folder;
    bool isFolder;
    std::vector<ImportedBookmark> children;
};

class BrayaBookmarkIO {
public:
    // Export
    static bool exportToHTML(const std::string& path, const std::vector<ImportedBookmark>& bookmarks);
    static bool exportToJSON(const std::string& path, const std::vector<ImportedBookmark>& bookmarks);
    static bool exportToCSV(const std::string& path, const std::vector<ImportedBookmark>& bookmarks);
    static bool exportToMarkdown(const std::string& path, const std::vector<ImportedBookmark>& bookmarks);

    // Import
    static std::vector<ImportedBookmark> importFromHTML(const std::string& path);
    static std::vector<ImportedBookmark> importFromJSON(const std::string& path);
    
private:
    static std::string escapeHTML(const std::string& text);
    static std::string unescapeHTML(const std::string& text);
    static void writeHTMLBookmarks(std::ofstream& file, const std::vector<ImportedBookmark>& bookmarks, int indent);
    static void parseHTMLBookmarks(const std::string& html, std::vector<ImportedBookmark>& bookmarks);
};

#endif // BRAYA_BOOKMARK_IO_H
