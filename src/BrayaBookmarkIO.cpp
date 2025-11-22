#include "BrayaBookmarkIO.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>
#include <functional>
#include <json-glib/json-glib.h>

std::string BrayaBookmarkIO::escapeHTML(const std::string& text) {
    std::string result;
    for (char c : text) {
        switch (c) {
            case '&': result += "&amp;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default: result += c;
        }
    }
    return result;
}

std::string BrayaBookmarkIO::unescapeHTML(const std::string& text) {
    std::string result = text;
    std::regex amp("&amp;");
    std::regex lt("&lt;");
    std::regex gt("&gt;");
    std::regex quot("&quot;");
    std::regex apos("&#39;");
    
    result = std::regex_replace(result, amp, "&");
    result = std::regex_replace(result, lt, "<");
    result = std::regex_replace(result, gt, ">");
    result = std::regex_replace(result, quot, "\"");
    result = std::regex_replace(result, apos, "'");
    
    return result;
}

void BrayaBookmarkIO::writeHTMLBookmarks(std::ofstream& file, const std::vector<ImportedBookmark>& bookmarks, int indent) {
    std::string indentStr(indent * 4, ' ');
    
    for (const auto& bm : bookmarks) {
        if (bm.isFolder) {
            file << indentStr << "<DT><H3>" << escapeHTML(bm.title) << "</H3>\n";
            file << indentStr << "<DL><p>\n";
            writeHTMLBookmarks(file, bm.children, indent + 1);
            file << indentStr << "</DL><p>\n";
        } else {
            file << indentStr << "<DT><A HREF=\"" << escapeHTML(bm.url) << "\">" 
                 << escapeHTML(bm.title) << "</A>\n";
        }
    }
}

bool BrayaBookmarkIO::exportToHTML(const std::string& path, const std::vector<ImportedBookmark>& bookmarks) {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "ERROR: Could not open file for writing: " << path << std::endl;
        return false;
    }
    
    // Write HTML header (Netscape Bookmark File Format)
    file << "<!DOCTYPE NETSCAPE-Bookmark-file-1>\n";
    file << "<!-- This is an automatically generated file.\n";
    file << "     It will be read and overwritten.\n";
    file << "     DO NOT EDIT! -->\n";
    file << "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=UTF-8\">\n";
    file << "<TITLE>Bookmarks</TITLE>\n";
    file << "<H1>Bookmarks</H1>\n";
    file << "<DL><p>\n";
    
    // Write bookmarks
    writeHTMLBookmarks(file, bookmarks, 1);
    
    file << "</DL><p>\n";
    file.close();
    
    std::cout << "✓ Exported " << bookmarks.size() << " bookmarks to HTML" << std::endl;
    return true;
}

bool BrayaBookmarkIO::exportToJSON(const std::string& path, const std::vector<ImportedBookmark>& bookmarks) {
    JsonBuilder* builder = json_builder_new();
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "bookmarks");
    json_builder_begin_array(builder);
    
    std::function<void(const ImportedBookmark&)> addBookmark;
    addBookmark = [&](const ImportedBookmark& bm) {
        json_builder_begin_object(builder);
        
        json_builder_set_member_name(builder, "title");
        json_builder_add_string_value(builder, bm.title.c_str());
        
        json_builder_set_member_name(builder, "url");
        json_builder_add_string_value(builder, bm.url.c_str());
        
        json_builder_set_member_name(builder, "isFolder");
        json_builder_add_boolean_value(builder, bm.isFolder);
        
        if (bm.isFolder && !bm.children.empty()) {
            json_builder_set_member_name(builder, "children");
            json_builder_begin_array(builder);
            for (const auto& child : bm.children) {
                addBookmark(child);
            }
            json_builder_end_array(builder);
        }
        
        json_builder_end_object(builder);
    };
    
    for (const auto& bm : bookmarks) {
        addBookmark(bm);
    }
    
    json_builder_end_array(builder);
    json_builder_end_object(builder);
    
    JsonGenerator* gen = json_generator_new();
    JsonNode* root = json_builder_get_root(builder);
    json_generator_set_root(gen, root);
    json_generator_set_pretty(gen, TRUE);
    
    gchar* jsonData = json_generator_to_data(gen, nullptr);
    
    std::ofstream file(path);
    if (!file.is_open()) {
        g_free(jsonData);
        json_node_free(root);
        g_object_unref(gen);
        g_object_unref(builder);
        return false;
    }
    
    file << jsonData;
    file.close();
    
    g_free(jsonData);
    json_node_free(root);
    g_object_unref(gen);
    g_object_unref(builder);
    
    std::cout << "✓ Exported " << bookmarks.size() << " bookmarks to JSON" << std::endl;
    return true;
}

std::vector<ImportedBookmark> BrayaBookmarkIO::importFromHTML(const std::string& path) {
    std::vector<ImportedBookmark> bookmarks;
    
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "ERROR: Could not open file: " << path << std::endl;
        return bookmarks;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    std::string html = buffer.str();
    
    // Simple regex parsing (production would use proper HTML parser)
    std::regex linkRegex("<A\\s+HREF=\"([^\"]+)\"[^>]*>([^<]+)</A>");
    std::regex folderRegex("<H3[^>]*>([^<]+)</H3>");
    
    std::smatch match;
    std::string::const_iterator searchStart(html.cbegin());
    
    while (std::regex_search(searchStart, html.cend(), match, linkRegex)) {
        ImportedBookmark bm;
        bm.url = unescapeHTML(match[1]);
        bm.title = unescapeHTML(match[2]);
        bm.isFolder = false;
        bm.folder = "";
        bookmarks.push_back(bm);
        searchStart = match.suffix().first;
    }
    
    std::cout << "✓ Imported " << bookmarks.size() << " bookmarks from HTML" << std::endl;
    return bookmarks;
}

std::vector<ImportedBookmark> BrayaBookmarkIO::importFromJSON(const std::string& path) {
    std::vector<ImportedBookmark> bookmarks;
    
    GError* error = nullptr;
    JsonParser* parser = json_parser_new();
    
    if (!json_parser_load_from_file(parser, path.c_str(), &error)) {
        std::cerr << "ERROR parsing JSON: " << error->message << std::endl;
        g_error_free(error);
        g_object_unref(parser);
        return bookmarks;
    }
    
    JsonNode* root = json_parser_get_root(parser);
    if (!root || !JSON_NODE_HOLDS_OBJECT(root)) {
        g_object_unref(parser);
        return bookmarks;
    }
    
    JsonObject* rootObj = json_node_get_object(root);
    if (!json_object_has_member(rootObj, "bookmarks")) {
        g_object_unref(parser);
        return bookmarks;
    }
    
    JsonArray* bookmarksArray = json_object_get_array_member(rootObj, "bookmarks");
    int numBookmarks = json_array_get_length(bookmarksArray);
    
    for (int i = 0; i < numBookmarks; i++) {
        JsonObject* bmObj = json_array_get_object_element(bookmarksArray, i);
        
        ImportedBookmark bm;
        bm.title = json_object_get_string_member(bmObj, "title");
        bm.url = json_object_get_string_member(bmObj, "url");
        bm.isFolder = json_object_get_boolean_member(bmObj, "isFolder");
        bm.folder = "";
        
        bookmarks.push_back(bm);
    }
    
    g_object_unref(parser);

    std::cout << "✓ Imported " << bookmarks.size() << " bookmarks from JSON" << std::endl;
    return bookmarks;
}

// CSV Export (Title, URL, Folder)
bool BrayaBookmarkIO::exportToCSV(const std::string& path, const std::vector<ImportedBookmark>& bookmarks) {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for CSV export: " << path << std::endl;
        return false;
    }

    // Write CSV header
    file << "Title,URL,Folder\n";

    // Helper function to write bookmarks recursively
    std::function<void(const std::vector<ImportedBookmark>&, const std::string&)> writeCSV;
    writeCSV = [&](const std::vector<ImportedBookmark>& bms, const std::string& parentFolder) {
        for (const auto& bm : bms) {
            if (bm.isFolder) {
                // Recursively process folder
                std::string folderPath = parentFolder.empty() ? bm.title : parentFolder + "/" + bm.title;
                writeCSV(bm.children, folderPath);
            } else {
                // Write bookmark entry
                std::string title = bm.title;
                std::string url = bm.url;
                // Escape quotes in CSV
                if (title.find(',') != std::string::npos || title.find('"') != std::string::npos) {
                    std::string escaped = "\"";
                    for (char c : title) {
                        if (c == '"') escaped += "\"\"";
                        else escaped += c;
                    }
                    escaped += "\"";
                    title = escaped;
                }
                file << title << "," << url << "," << parentFolder << "\n";
            }
        }
    };

    writeCSV(bookmarks, "");

    file.close();
    std::cout << "✓ Exported bookmarks to CSV: " << path << std::endl;
    return true;
}

// Markdown Export
bool BrayaBookmarkIO::exportToMarkdown(const std::string& path, const std::vector<ImportedBookmark>& bookmarks) {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for Markdown export: " << path << std::endl;
        return false;
    }

    file << "# Bookmarks\n\n";
    file << "Exported from Braya Browser\n\n";
    file << "---\n\n";

    // Helper function to write bookmarks recursively
    std::function<void(const std::vector<ImportedBookmark>&, int)> writeMarkdown;
    writeMarkdown = [&](const std::vector<ImportedBookmark>& bms, int level) {
        for (const auto& bm : bms) {
            std::string indent(level * 2, ' ');
            if (bm.isFolder) {
                // Write folder as heading
                std::string heading(level + 2, '#');
                file << heading << " " << bm.title << "\n\n";
                // Recursively process children
                writeMarkdown(bm.children, level + 1);
            } else {
                // Write bookmark as markdown link
                file << indent << "- [" << bm.title << "](" << bm.url << ")\n";
            }
        }
        if (level > 0) file << "\n";
    };

    writeMarkdown(bookmarks, 0);

    file.close();
    std::cout << "✓ Exported bookmarks to Markdown: " << path << std::endl;
    return true;
}
