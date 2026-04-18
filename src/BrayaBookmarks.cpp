#include "BrayaBookmarks.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <json-glib/json-glib.h>

BrayaBookmarks::BrayaBookmarks() : managerDialog(nullptr), bookmarksList(nullptr), searchEntry(nullptr) {
    bookmarksFilePath = getBookmarksFilePath();
    loadFromFile();
}

BrayaBookmarks::~BrayaBookmarks() {
    saveToFile();
}

std::string BrayaBookmarks::getBookmarksFilePath() {
    std::string configDir = std::string(g_get_user_config_dir()) + "/braya";
    g_mkdir_with_parents(configDir.c_str(), 0755);
    return configDir + "/bookmarks.json";
}

void BrayaBookmarks::addBookmark(const std::string& name, const std::string& url, const std::string& folder) {
    // If a folder is specified and it exists, add to folder's children
    if (!folder.empty() && folder != "Bookmarks Bar") {
        Bookmark* folderPtr = findFolder(folder);
        if (folderPtr) {
            Bookmark bm(name, url);
            folderPtr->children.push_back(bm);
            saveToFile();
            return;
        }
    }

    // Otherwise add as a top-level bookmark
    bookmarks.push_back(Bookmark(name, url, folder));
    saveToFile();
}

void BrayaBookmarks::editBookmark(int index, const std::string& name, const std::string& url, const std::string& folderName) {
    if (index >= 0 && index < (int)bookmarks.size()) {
        // Check if we're moving the bookmark to an actual folder
        Bookmark* targetFolder = findFolder(folderName);

        if (targetFolder) {
            // Moving to a folder - remove from root and add to folder's children
            std::cout << "📁 Moving bookmark '" << name << "' to folder '" << folderName << "'" << std::endl;

            // Save bookmark data
            Bookmark bookmark = bookmarks[index];
            bookmark.name = name;
            bookmark.url = url;
            bookmark.folder = folderName;

            // Remove from root
            bookmarks.erase(bookmarks.begin() + index);

            // Add to folder's children
            targetFolder->children.push_back(bookmark);

            saveToFile();
        } else {
            // Not moving to a folder, or folder is "Bookmarks Bar" / "Other Bookmarks"
            // Just update in place
            bookmarks[index].name = name;
            bookmarks[index].url = url;
            bookmarks[index].folder = folderName;
            saveToFile();
        }
    }
}

void BrayaBookmarks::deleteBookmark(int index) {
    if (index >= 0 && index < (int)bookmarks.size()) {
        bookmarks.erase(bookmarks.begin() + index);
        saveToFile();
    }
}

void BrayaBookmarks::editBookmarkByUrl(const std::string& oldUrl, const std::string& name, const std::string& url, const std::string& folder) {
    int index = findBookmarkByUrl(oldUrl);
    if (index >= 0) {
        editBookmark(index, name, url, folder);
    }
}

void BrayaBookmarks::deleteBookmarkByUrl(const std::string& url) {
    // Check if this is a folder identifier (format: "folder:FolderName")
    if (url.find("folder:") == 0) {
        std::string folderName = url.substr(7);  // Skip "folder:" prefix
        for (size_t i = 0; i < bookmarks.size(); i++) {
            if (bookmarks[i].isFolder && bookmarks[i].name == folderName) {
                std::cout << "🗑️ Deleting folder: " << folderName << " (contains " << bookmarks[i].children.size() << " items)" << std::endl;
                bookmarks.erase(bookmarks.begin() + i);
                saveToFile();
                return;
            }
        }
        std::cout << "❌ Folder not found: " << folderName << std::endl;
    } else {
        // Regular bookmark
        int index = findBookmarkByUrl(url);
        if (index >= 0) {
            deleteBookmark(index);
        }
    }
}

int BrayaBookmarks::findBookmarkByUrl(const std::string& url) {
    for (size_t i = 0; i < bookmarks.size(); i++) {
        if (bookmarks[i].url == url) {
            return i;
        }
    }
    return -1;
}

std::vector<Bookmark> BrayaBookmarks::getBookmarks() {
    return bookmarks;
}

std::vector<Bookmark>& BrayaBookmarks::getBookmarksRef() {
    return bookmarks;
}

std::vector<Bookmark> BrayaBookmarks::searchBookmarks(const std::string& query) {
    std::vector<Bookmark> results;
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

    for (const auto& bookmark : bookmarks) {
        std::string lowerName = bookmark.name;
        std::string lowerUrl = bookmark.url;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        std::transform(lowerUrl.begin(), lowerUrl.end(), lowerUrl.begin(), ::tolower);

        if (lowerName.find(lowerQuery) != std::string::npos ||
            lowerUrl.find(lowerQuery) != std::string::npos) {
            results.push_back(bookmark);
        }
    }
    return results;
}

std::vector<std::string> BrayaBookmarks::getUniqueFolders() {
    std::vector<std::string> folders;

    // Add default folders first
    folders.push_back("Bookmarks Bar");

    // Collect unique folders from existing bookmarks
    for (const auto& bookmark : bookmarks) {
        if (!bookmark.folder.empty() && bookmark.folder != "Bookmarks Bar") {
            // Check if folder already in list
            bool found = false;
            for (const auto& existing : folders) {
                if (existing == bookmark.folder) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                folders.push_back(bookmark.folder);
            }
        }
    }

    return folders;
}

// Helper function to recursively write bookmark/folder to JSON
// Helper function to parse a bookmark from JSON recursively
static Bookmark parseBookmarkFromJson(JsonObject* obj) {
    Bookmark bm;

    if (json_object_has_member(obj, "name")) {
        bm.name = json_object_get_string_member(obj, "name");
    }
    if (json_object_has_member(obj, "url")) {
        bm.url = json_object_get_string_member(obj, "url");
    }
    if (json_object_has_member(obj, "folder")) {
        bm.folder = json_object_get_string_member(obj, "folder");
    }
    if (json_object_has_member(obj, "isFolder")) {
        bm.isFolder = json_object_get_boolean_member(obj, "isFolder");
    }
    if (json_object_has_member(obj, "timestamp")) {
        bm.timestamp = json_object_get_int_member(obj, "timestamp");
    }

    // Parse children recursively if this is a folder
    if (bm.isFolder && json_object_has_member(obj, "children")) {
        JsonArray* children = json_object_get_array_member(obj, "children");
        guint len = json_array_get_length(children);
        for (guint i = 0; i < len; i++) {
            JsonObject* childObj = json_array_get_object_element(children, i);
            bm.children.push_back(parseBookmarkFromJson(childObj));
        }
    }

    return bm;
}

static void writeBookmarkToJson(std::ofstream& file, const Bookmark& bm, int indent = 2) {
    std::string indentStr(indent, ' ');
    std::string indentStr2(indent + 2, ' ');

    file << indentStr << "{\n";
    file << indentStr2 << "\"name\": \"" << bm.name << "\",\n";
    file << indentStr2 << "\"url\": \"" << bm.url << "\",\n";
    file << indentStr2 << "\"folder\": \"" << bm.folder << "\",\n";
    file << indentStr2 << "\"isFolder\": " << (bm.isFolder ? "true" : "false") << ",\n";
    file << indentStr2 << "\"timestamp\": " << bm.timestamp;

    if (bm.isFolder && !bm.children.empty()) {
        file << ",\n" << indentStr2 << "\"children\": [\n";
        for (size_t i = 0; i < bm.children.size(); i++) {
            writeBookmarkToJson(file, bm.children[i], indent + 4);
            if (i < bm.children.size() - 1) {
                file << ",";
            }
            file << "\n";
        }
        file << indentStr2 << "]\n";
    } else {
        file << "\n";
    }

    file << indentStr << "}";
}

void BrayaBookmarks::saveToFile() {
    std::ofstream file(bookmarksFilePath);
    if (!file.is_open()) {
        std::cerr << "Failed to save bookmarks" << std::endl;
        return;
    }

    file << "[\n";
    for (size_t i = 0; i < bookmarks.size(); i++) {
        writeBookmarkToJson(file, bookmarks[i]);
        if (i < bookmarks.size() - 1) {
            file << ",";
        }
        file << "\n";
    }
    file << "]\n";
    file.close();
}

void BrayaBookmarks::loadFromFile() {
    bookmarks.clear();

    // Check if file exists
    if (g_file_test(bookmarksFilePath.c_str(), G_FILE_TEST_EXISTS) == FALSE) {
        return;
    }

    GError* error = nullptr;
    JsonParser* parser = json_parser_new();

    if (!json_parser_load_from_file(parser, bookmarksFilePath.c_str(), &error)) {
        if (error) {
            std::cerr << "Failed to parse bookmarks: " << error->message << std::endl;
            g_error_free(error);
        }
        g_object_unref(parser);
        return;
    }

    JsonNode* root = json_parser_get_root(parser);
    if (!root || !JSON_NODE_HOLDS_ARRAY(root)) {
        g_object_unref(parser);
        return;
    }

    JsonArray* array = json_node_get_array(root);
    guint len = json_array_get_length(array);

    for (guint i = 0; i < len; i++) {
        JsonObject* obj = json_array_get_object_element(array, i);
        bookmarks.push_back(parseBookmarkFromJson(obj));
    }

    g_object_unref(parser);
}

// Folder management functions
void BrayaBookmarks::addFolder(const std::string& folderName, const std::string& parentFolder) {
    Bookmark folder = Bookmark::createFolder(folderName);
    folder.folder = parentFolder;  // Set parent folder
    bookmarks.push_back(folder);
    saveToFile();
}

void BrayaBookmarks::addBookmarkToFolder(const std::string& name, const std::string& url, const std::string& folderName) {
    Bookmark* folder = findFolder(folderName);
    if (folder) {
        Bookmark bookmark(name, url);
        folder->children.push_back(bookmark);
        saveToFile();
    } else {
        // Folder doesn't exist, add to root with folder name
        addBookmark(name, url, folderName);
    }
}

Bookmark* BrayaBookmarks::findFolder(const std::string& folderName) {
    for (auto& bookmark : bookmarks) {
        if (bookmark.isFolder && bookmark.name == folderName) {
            return &bookmark;
        }
        // Recursively search in children
        if (bookmark.isFolder) {
            for (auto& child : bookmark.children) {
                if (child.isFolder && child.name == folderName) {
                    return &child;
                }
            }
        }
    }
    return nullptr;
}

std::vector<Bookmark> BrayaBookmarks::getBookmarksInFolder(const std::string& folderName) {
    Bookmark* folder = findFolder(folderName);
    if (folder) {
        return folder->children;
    }
    return std::vector<Bookmark>();
}

void BrayaBookmarks::deleteFolder(const std::string& folderName) {
    for (auto it = bookmarks.begin(); it != bookmarks.end(); ++it) {
        if (it->isFolder && it->name == folderName) {
            bookmarks.erase(it);
            saveToFile();
            return;
        }
    }
}

void BrayaBookmarks::showBookmarksManager(GtkWindow* parent) {
    std::cout << "📚 showBookmarksManager called" << std::endl;
    if (managerDialog) {
        std::cout << "📚 Dialog already exists, presenting it" << std::endl;
        gtk_window_present(GTK_WINDOW(managerDialog));
        return;
    }

    std::cout << "📚 Creating new manager dialog..." << std::endl;
    createManagerDialog(parent);
    refreshBookmarksList();
    gtk_window_present(GTK_WINDOW(managerDialog));
    std::cout << "📚 Manager dialog shown" << std::endl;
}

void BrayaBookmarks::createManagerDialog(GtkWindow* parent) {
    managerDialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(managerDialog), "📚 Bookmarks Manager");
    gtk_window_set_default_size(GTK_WINDOW(managerDialog), 700, 500);
    gtk_window_set_transient_for(GTK_WINDOW(managerDialog), parent);
    gtk_window_set_modal(GTK_WINDOW(managerDialog), TRUE);
    
    GtkWidget* mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(managerDialog), mainBox);
    
    // Header with search
    GtkWidget* headerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(headerBox, 15);
    gtk_widget_set_margin_end(headerBox, 15);
    gtk_widget_set_margin_top(headerBox, 15);
    gtk_widget_set_margin_bottom(headerBox, 10);
    gtk_box_append(GTK_BOX(mainBox), headerBox);
    
    GtkWidget* titleLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(titleLabel), "<span size='large' weight='bold'>📚 Bookmarks</span>");
    gtk_box_append(GTK_BOX(headerBox), titleLabel);
    
    // Search entry
    searchEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(searchEntry), "🔍 Search bookmarks...");
    gtk_widget_set_hexpand(searchEntry, TRUE);
    g_signal_connect(searchEntry, "changed", G_CALLBACK(onSearchChanged), this);
    gtk_box_append(GTK_BOX(headerBox), searchEntry);
    
    gtk_box_append(GTK_BOX(mainBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // Bookmarks list
    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_box_append(GTK_BOX(mainBox), scrolled);
    
    bookmarksList = gtk_list_box_new();
    gtk_widget_add_css_class(bookmarksList, "bookmarks-list");
    g_signal_connect(bookmarksList, "row-activated", G_CALLBACK(onBookmarkDoubleClicked), this);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), bookmarksList);
    
    gtk_box_append(GTK_BOX(mainBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // Bottom buttons
    GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(buttonBox, 15);
    gtk_widget_set_margin_end(buttonBox, 15);
    gtk_widget_set_margin_top(buttonBox, 10);
    gtk_widget_set_margin_bottom(buttonBox, 10);
    gtk_box_append(GTK_BOX(mainBox), buttonBox);
    
    GtkWidget* addBtn = gtk_button_new_with_label("➕ Add Bookmark");
    g_signal_connect(addBtn, "clicked", G_CALLBACK(onAddBookmarkClicked), this);
    gtk_box_append(GTK_BOX(buttonBox), addBtn);

    GtkWidget* newFolderBtn = gtk_button_new_with_label("📁 New Folder");
    std::cout << "🔧 Creating 'New Folder' button: " << newFolderBtn << std::endl;

    gulong handler_id = g_signal_connect(newFolderBtn, "clicked", G_CALLBACK(onAddFolderClicked), this);
    std::cout << "🔧 Connected signal handler ID: " << handler_id << " to onAddFolderClicked" << std::endl;

    gtk_box_append(GTK_BOX(buttonBox), newFolderBtn);
    std::cout << "🔧 'New Folder' button added to buttonBox" << std::endl;

    GtkWidget* editBtn = gtk_button_new_with_label("✏️ Edit");
    g_signal_connect(editBtn, "clicked", G_CALLBACK(onEditBookmarkClicked), this);
    gtk_box_append(GTK_BOX(buttonBox), editBtn);
    
    GtkWidget* deleteBtn = gtk_button_new_with_label("🗑️ Delete");
    gtk_widget_add_css_class(deleteBtn, "destructive-action");
    g_signal_connect(deleteBtn, "clicked", G_CALLBACK(onDeleteBookmarkClicked), this);
    gtk_box_append(GTK_BOX(buttonBox), deleteBtn);
    
    // Spacer
    GtkWidget* spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(buttonBox), spacer);
    
    GtkWidget* closeBtn = gtk_button_new_with_label("Close");
    g_signal_connect(closeBtn, "clicked", G_CALLBACK(onCloseClicked), this);
    gtk_box_append(GTK_BOX(buttonBox), closeBtn);
}

void BrayaBookmarks::refreshBookmarksList() {
    if (!bookmarksList) return;

    // Clear existing items
    GtkWidget* child;
    while ((child = gtk_widget_get_first_child(bookmarksList)) != nullptr) {
        gtk_list_box_remove(GTK_LIST_BOX(bookmarksList), child);
    }

    // Get search query if search entry exists
    std::string searchQuery;
    if (searchEntry) {
        const char* text = gtk_editable_get_text(GTK_EDITABLE(searchEntry));
        if (text) {
            searchQuery = text;
        }
    }

    // Convert search query to lowercase for case-insensitive search
    std::string lowerQuery = searchQuery;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

    // Helper lambda to check if a bookmark matches the search query
    auto matchesSearch = [&lowerQuery](const Bookmark& bm) -> bool {
        if (lowerQuery.empty()) return true;

        std::string lowerName = bm.name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

        if (lowerName.find(lowerQuery) != std::string::npos) {
            return true;
        }

        if (!bm.isFolder) {
            std::string lowerUrl = bm.url;
            std::transform(lowerUrl.begin(), lowerUrl.end(), lowerUrl.begin(), ::tolower);
            if (lowerUrl.find(lowerQuery) != std::string::npos) {
                return true;
            }
        }

        return false;
    };

    // Helper lambda to check if a folder or any of its children match the search
    auto folderMatchesSearch = [&lowerQuery, &matchesSearch](const Bookmark& folder) -> bool {
        if (lowerQuery.empty()) return true;

        // Check if folder name matches
        if (matchesSearch(folder)) return true;

        // Check if any children match
        for (const auto& child : folder.children) {
            if (matchesSearch(child)) return true;
        }

        return false;
    };

    // Helper lambda to add a bookmark row
    auto addBookmarkRow = [this](const Bookmark& bm, int parentIdx, int childIdx, int indentLevel) {
        GtkWidget* row = gtk_list_box_row_new();

        // Store parent and child indices
        // parent = -1 means top-level bookmark (index in main bookmarks array)
        // parent >= 0 means child of folder (parent = folder index, childIdx = index in children array)
        g_object_set_data(G_OBJECT(row), "parent-index", GINT_TO_POINTER(parentIdx));
        g_object_set_data(G_OBJECT(row), "child-index", GINT_TO_POINTER(childIdx));

        GtkWidget* mainBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_widget_set_hexpand(mainBox, TRUE);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), mainBox);

        GtkWidget* rowBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_widget_set_margin_start(rowBox, 15 + (indentLevel * 20));  // Indent nested items
        gtk_widget_set_margin_end(rowBox, 15);
        gtk_widget_set_margin_top(rowBox, 10);
        gtk_widget_set_margin_bottom(rowBox, 10);
        gtk_widget_set_hexpand(rowBox, TRUE);
        gtk_box_append(GTK_BOX(mainBox), rowBox);

        // Name (with folder icon if it's a folder)
        std::string displayName = bm.isFolder ? ("📁 " + bm.name) : bm.name;
        GtkWidget* nameLabel = gtk_label_new(displayName.c_str());
        gtk_widget_set_halign(nameLabel, GTK_ALIGN_START);
        gtk_label_set_ellipsize(GTK_LABEL(nameLabel), PANGO_ELLIPSIZE_END);
        gtk_widget_add_css_class(nameLabel, "bookmark-name");
        gtk_box_append(GTK_BOX(rowBox), nameLabel);

        // URL (only for non-folder bookmarks)
        if (!bm.isFolder) {
            GtkWidget* urlLabel = gtk_label_new(bm.url.c_str());
            gtk_widget_set_halign(urlLabel, GTK_ALIGN_START);
            gtk_label_set_ellipsize(GTK_LABEL(urlLabel), PANGO_ELLIPSIZE_END);
            gtk_widget_add_css_class(urlLabel, "bookmark-url");
            gtk_box_append(GTK_BOX(rowBox), urlLabel);
        } else {
            // Show number of items in folder
            std::string itemsText = "Contains " + std::to_string(bm.children.size()) + " items";
            GtkWidget* itemsLabel = gtk_label_new(itemsText.c_str());
            gtk_widget_set_halign(itemsLabel, GTK_ALIGN_START);
            gtk_widget_add_css_class(itemsLabel, "bookmark-url");
            gtk_box_append(GTK_BOX(rowBox), itemsLabel);
        }

        // Add "Move Out" button for nested bookmarks
        if (parentIdx >= 0 && !bm.isFolder) {
            GtkWidget* moveOutBtn = gtk_button_new_with_label("↗️ Move Out");
            gtk_widget_set_valign(moveOutBtn, GTK_ALIGN_CENTER);
            gtk_widget_set_margin_end(moveOutBtn, 10);
            gtk_widget_add_css_class(moveOutBtn, "flat");
            g_object_set_data(G_OBJECT(moveOutBtn), "parent-index", GINT_TO_POINTER(parentIdx));
            g_object_set_data(G_OBJECT(moveOutBtn), "child-index", GINT_TO_POINTER(childIdx));
            g_object_set_data(G_OBJECT(moveOutBtn), "bookmarks", this);

            g_signal_connect(moveOutBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
                BrayaBookmarks* bookmarks = (BrayaBookmarks*)g_object_get_data(G_OBJECT(btn), "bookmarks");
                int parentIdx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "parent-index"));
                int childIdx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "child-index"));

                if (parentIdx >= 0 && parentIdx < (int)bookmarks->bookmarks.size() &&
                    bookmarks->bookmarks[parentIdx].isFolder &&
                    childIdx >= 0 && childIdx < (int)bookmarks->bookmarks[parentIdx].children.size()) {

                    // Move the bookmark out of the folder to the top level
                    Bookmark movedBm = bookmarks->bookmarks[parentIdx].children[childIdx];
                    bookmarks->bookmarks[parentIdx].children.erase(
                        bookmarks->bookmarks[parentIdx].children.begin() + childIdx
                    );
                    bookmarks->bookmarks.push_back(movedBm);
                    bookmarks->saveToFile();
                    bookmarks->refreshBookmarksList();

                    std::cout << "↗️ Moved '" << movedBm.name << "' out of folder" << std::endl;
                }
            }), nullptr);

            gtk_box_append(GTK_BOX(mainBox), moveOutBtn);
        }

        gtk_list_box_append(GTK_LIST_BOX(bookmarksList), row);
    };

    // Add all top-level bookmarks and their children
    for (size_t i = 0; i < bookmarks.size(); i++) {
        const auto& bm = bookmarks[i];

        // If searching, check if this item matches
        if (!lowerQuery.empty()) {
            if (bm.isFolder) {
                // For folders, show if folder name matches OR any child matches
                if (!folderMatchesSearch(bm)) {
                    continue; // Skip this folder
                }

                // Show the folder
                addBookmarkRow(bm, -1, i, 0);

                // Show only matching children
                for (size_t j = 0; j < bm.children.size(); j++) {
                    if (matchesSearch(bm.children[j])) {
                        addBookmarkRow(bm.children[j], i, j, 1);
                    }
                }
            } else {
                // For regular bookmarks, show only if it matches
                if (matchesSearch(bm)) {
                    addBookmarkRow(bm, -1, i, 0);
                }
            }
        } else {
            // No search query, show everything
            addBookmarkRow(bm, -1, i, 0);

            // If it's a folder, add its children
            if (bm.isFolder) {
                for (size_t j = 0; j < bm.children.size(); j++) {
                    addBookmarkRow(bm.children[j], i, j, 1);
                }
            }
        }
    }
}

void BrayaBookmarks::onAddBookmarkClicked(GtkButton* button, gpointer data) {
    BrayaBookmarks* bookmarks = static_cast<BrayaBookmarks*>(data);
    
    GtkWidget* dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Add Bookmark");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 200);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(bookmarks->managerDialog));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_window_set_child(GTK_WINDOW(dialog), box);
    
    GtkWidget* nameEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(nameEntry), "Bookmark name");
    gtk_box_append(GTK_BOX(box), nameEntry);
    
    GtkWidget* urlEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(urlEntry), "URL");
    gtk_box_append(GTK_BOX(box), urlEntry);

    // Folder dropdown
    GtkWidget* folderLabel = gtk_label_new("Folder:");
    gtk_widget_set_halign(folderLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), folderLabel);

    GtkStringList* folderModel = gtk_string_list_new(nullptr);
    auto folders = bookmarks->getUniqueFolders();
    for (const auto& folder : folders) {
        gtk_string_list_append(folderModel, folder.c_str());
    }
    gtk_string_list_append(folderModel, "Other Bookmarks");
    gtk_string_list_append(folderModel, "📁 New Folder...");

    GtkWidget* folderDropdown = gtk_drop_down_new(G_LIST_MODEL(folderModel), nullptr);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(folderDropdown), 0);  // Default to "Bookmarks Bar"
    gtk_box_append(GTK_BOX(box), folderDropdown);
    
    GtkWidget* btnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(box), btnBox);
    
    GtkWidget* saveBtn = gtk_button_new_with_label("Save");
    gtk_widget_set_hexpand(saveBtn, TRUE);
    gtk_box_append(GTK_BOX(btnBox), saveBtn);
    
    GtkWidget* cancelBtn = gtk_button_new_with_label("Cancel");
    gtk_widget_set_hexpand(cancelBtn, TRUE);
    gtk_box_append(GTK_BOX(btnBox), cancelBtn);

    GtkWidget** widgets = (GtkWidget**)g_malloc(sizeof(GtkWidget*) * 4);
    widgets[0] = nameEntry;
    widgets[1] = urlEntry;
    widgets[2] = folderDropdown;
    widgets[3] = dialog;
    g_object_set_data(G_OBJECT(saveBtn), "bookmarks", bookmarks);
    g_signal_connect(saveBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        GtkWidget** widgets = (GtkWidget**)data;
        BrayaBookmarks* bookmarks = (BrayaBookmarks*)g_object_get_data(G_OBJECT(btn), "bookmarks");

        const char* name = gtk_editable_get_text(GTK_EDITABLE(widgets[0]));
        const char* url = gtk_editable_get_text(GTK_EDITABLE(widgets[1]));

        // Get selected folder from dropdown
        GtkDropDown* dropdown = GTK_DROP_DOWN(widgets[2]);
        guint selected = gtk_drop_down_get_selected(dropdown);
        GtkStringList* model = GTK_STRING_LIST(gtk_drop_down_get_model(dropdown));
        const char* folder = gtk_string_list_get_string(model, selected);

        std::string finalFolder = folder;

        // Handle "New Folder..." option
        if (g_str_has_prefix(folder, "📁 New Folder")) {
            GtkWindow* parent = GTK_WINDOW(widgets[3]);
            std::string customFolder = BrayaBookmarks::showNewFolderDialog(parent);
            if (!customFolder.empty()) {
                finalFolder = customFolder;
            } else {
                finalFolder = "Other Bookmarks";  // Default if canceled
            }
        }

        if (strlen(name) > 0 && strlen(url) > 0) {
            bookmarks->addBookmark(name, url, finalFolder);
            bookmarks->refreshBookmarksList();
            bookmarks->triggerRefresh();  // Update the bookmarks bar
        }

        gtk_window_destroy(GTK_WINDOW(widgets[3]));
        g_free(widgets);
    }), widgets);
    
    g_signal_connect(cancelBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        gtk_window_destroy(GTK_WINDOW(data));
    }), dialog);
    
    gtk_window_present(GTK_WINDOW(dialog));
}

void BrayaBookmarks::onAddFolderClicked(GtkButton* button, gpointer data) {
    BrayaBookmarks* bookmarks = static_cast<BrayaBookmarks*>(data);

    std::cout << "📁 onAddFolderClicked called" << std::endl;

    std::string folderName = showNewFolderDialog(GTK_WINDOW(bookmarks->managerDialog));

    std::cout << "📁 Folder name entered: '" << folderName << "'" << std::endl;

    if (!folderName.empty()) {
        std::cout << "📁 Adding folder: " << folderName << std::endl;
        bookmarks->addFolder(folderName);
        std::cout << "📁 Refreshing bookmarks list..." << std::endl;
        bookmarks->refreshBookmarksList();
        std::cout << "📁 Triggering refresh callback..." << std::endl;
        bookmarks->triggerRefresh();  // Update the bookmarks bar
        std::cout << "📁 Folder creation complete!" << std::endl;
    } else {
        std::cout << "📁 Folder creation cancelled (empty name)" << std::endl;
    }
}

void BrayaBookmarks::onEditBookmarkClicked(GtkButton* button, gpointer data) {
    BrayaBookmarks* bookmarks = static_cast<BrayaBookmarks*>(data);

    // Get selected row
    GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(bookmarks->bookmarksList));
    if (!row) return;

    int parentIdx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "parent-index"));
    int childIdx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "child-index"));

    // Get the bookmark reference
    const Bookmark* bm = nullptr;
    if (parentIdx == -1) {
        // Top-level bookmark/folder
        if (childIdx >= 0 && childIdx < (int)bookmarks->bookmarks.size()) {
            bm = &bookmarks->bookmarks[childIdx];
        }
    } else {
        // Nested bookmark
        if (parentIdx >= 0 && parentIdx < (int)bookmarks->bookmarks.size() &&
            bookmarks->bookmarks[parentIdx].isFolder &&
            childIdx >= 0 && childIdx < (int)bookmarks->bookmarks[parentIdx].children.size()) {
            bm = &bookmarks->bookmarks[parentIdx].children[childIdx];
        }
    }

    if (!bm) return;

    // Create edit dialog
    GtkWidget* dialog = gtk_window_new();
    const char* dialogTitle = bm->isFolder ? "Edit Folder" : "Edit Bookmark";
    gtk_window_set_title(GTK_WINDOW(dialog), dialogTitle);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, bm->isFolder ? 150 : 200);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(bookmarks->managerDialog));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_window_set_child(GTK_WINDOW(dialog), box);

    // Name field (for both bookmarks and folders)
    GtkWidget* nameLabel = gtk_label_new(bm->isFolder ? "Folder Name:" : "Bookmark Name:");
    gtk_widget_set_halign(nameLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), nameLabel);

    GtkWidget* nameEntry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(nameEntry), bm->name.c_str());
    gtk_box_append(GTK_BOX(box), nameEntry);

    // URL field (only for bookmarks, not folders)
    GtkWidget* urlEntry = nullptr;
    GtkWidget* folderDropdown = nullptr;

    if (!bm->isFolder) {
        GtkWidget* urlLabel = gtk_label_new("URL:");
        gtk_widget_set_halign(urlLabel, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(box), urlLabel);

        urlEntry = gtk_entry_new();
        gtk_editable_set_text(GTK_EDITABLE(urlEntry), bm->url.c_str());
        gtk_box_append(GTK_BOX(box), urlEntry);

        // Folder dropdown (only for bookmarks, not folders)
        GtkWidget* folderLabel = gtk_label_new("Folder:");
        gtk_widget_set_halign(folderLabel, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(box), folderLabel);

        GtkStringList* folderModel = gtk_string_list_new(nullptr);
        auto folders = bookmarks->getUniqueFolders();
        guint selectedIndex = 0;
        for (size_t i = 0; i < folders.size(); i++) {
            gtk_string_list_append(folderModel, folders[i].c_str());
            if (folders[i] == bm->folder) {
                selectedIndex = i;
            }
        }
        gtk_string_list_append(folderModel, "Other Bookmarks");
        gtk_string_list_append(folderModel, "📁 New Folder...");

        folderDropdown = gtk_drop_down_new(G_LIST_MODEL(folderModel), nullptr);
        gtk_drop_down_set_selected(GTK_DROP_DOWN(folderDropdown), selectedIndex);
        gtk_box_append(GTK_BOX(box), folderDropdown);
    }

    GtkWidget* btnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(box), btnBox);

    GtkWidget* saveBtn = gtk_button_new_with_label("Save");
    gtk_widget_set_hexpand(saveBtn, TRUE);
    gtk_box_append(GTK_BOX(btnBox), saveBtn);

    GtkWidget* cancelBtn = gtk_button_new_with_label("Cancel");
    gtk_widget_set_hexpand(cancelBtn, TRUE);
    gtk_box_append(GTK_BOX(btnBox), cancelBtn);

    GtkWidget** widgets = (GtkWidget**)g_malloc(sizeof(GtkWidget*) * 4);
    widgets[0] = nameEntry;
    widgets[1] = urlEntry;
    widgets[2] = folderDropdown;
    widgets[3] = dialog;
    g_object_set_data(G_OBJECT(saveBtn), "bookmarks", bookmarks);
    g_object_set_data(G_OBJECT(saveBtn), "parent-index", GINT_TO_POINTER(parentIdx));
    g_object_set_data(G_OBJECT(saveBtn), "child-index", GINT_TO_POINTER(childIdx));
    g_object_set_data(G_OBJECT(saveBtn), "is-folder", GINT_TO_POINTER(bm->isFolder ? 1 : 0));

    g_signal_connect(saveBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        GtkWidget** widgets = (GtkWidget**)data;
        BrayaBookmarks* bookmarks = (BrayaBookmarks*)g_object_get_data(G_OBJECT(btn), "bookmarks");
        int parentIdx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "parent-index"));
        int childIdx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "child-index"));
        bool isFolder = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "is-folder")) == 1;

        const char* name = gtk_editable_get_text(GTK_EDITABLE(widgets[0]));

        if (strlen(name) == 0) {
            gtk_window_destroy(GTK_WINDOW(widgets[3]));
            g_free(widgets);
            return;
        }

        if (isFolder) {
            // For folders, just update the name
            if (parentIdx == -1 && childIdx >= 0 && childIdx < (int)bookmarks->bookmarks.size()) {
                bookmarks->bookmarks[childIdx].name = name;
                bookmarks->saveToFile();
            }
            bookmarks->refreshBookmarksList();
            bookmarks->triggerRefresh();  // Update the bookmarks bar
        } else {
            // For bookmarks, update name, URL, and folder
            const char* url = gtk_editable_get_text(GTK_EDITABLE(widgets[1]));

            if (strlen(url) == 0) {
                gtk_window_destroy(GTK_WINDOW(widgets[3]));
                g_free(widgets);
                return;
            }

            // Get selected folder from dropdown
            GtkDropDown* dropdown = GTK_DROP_DOWN(widgets[2]);
            guint selected = gtk_drop_down_get_selected(dropdown);
            GtkStringList* model = GTK_STRING_LIST(gtk_drop_down_get_model(dropdown));
            const char* folder = gtk_string_list_get_string(model, selected);

            std::string finalFolder = folder;

            // Handle "New Folder..." option
            if (g_str_has_prefix(folder, "📁 New Folder")) {
                GtkWindow* parent = GTK_WINDOW(widgets[3]);
                std::string customFolder = BrayaBookmarks::showNewFolderDialog(parent);
                if (!customFolder.empty()) {
                    finalFolder = customFolder;
                } else {
                    finalFolder = "Other Bookmarks";  // Default if canceled
                }
            }

            // Update the bookmark
            if (parentIdx == -1 && childIdx >= 0 && childIdx < (int)bookmarks->bookmarks.size()) {
                // Top-level bookmark
                bookmarks->bookmarks[childIdx].name = name;
                bookmarks->bookmarks[childIdx].url = url;
                bookmarks->bookmarks[childIdx].folder = finalFolder;
                bookmarks->saveToFile();
            } else if (parentIdx >= 0 && parentIdx < (int)bookmarks->bookmarks.size() &&
                       bookmarks->bookmarks[parentIdx].isFolder &&
                       childIdx >= 0 && childIdx < (int)bookmarks->bookmarks[parentIdx].children.size()) {
                // Nested bookmark
                bookmarks->bookmarks[parentIdx].children[childIdx].name = name;
                bookmarks->bookmarks[parentIdx].children[childIdx].url = url;
                bookmarks->saveToFile();
            }

            bookmarks->refreshBookmarksList();
            bookmarks->triggerRefresh();  // Update the bookmarks bar
        }

        gtk_window_destroy(GTK_WINDOW(widgets[3]));
        g_free(widgets);
    }), widgets);
    
    g_signal_connect(cancelBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        gtk_window_destroy(GTK_WINDOW(data));
    }), dialog);
    
    gtk_window_present(GTK_WINDOW(dialog));
}

void BrayaBookmarks::onDeleteBookmarkClicked(GtkButton* button, gpointer data) {
    BrayaBookmarks* bookmarks = static_cast<BrayaBookmarks*>(data);

    GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(bookmarks->bookmarksList));
    if (!row) {
        std::cout << "❌ No row selected for deletion" << std::endl;
        return;
    }

    int parentIdx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "parent-index"));
    int childIdx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "child-index"));

    if (parentIdx == -1) {
        // Top-level bookmark/folder
        std::cout << "🗑️ Deleting top-level item at index: " << childIdx << std::endl;
        if (childIdx >= 0 && childIdx < (int)bookmarks->bookmarks.size()) {
            std::cout << "   Deleting: " << bookmarks->bookmarks[childIdx].name << std::endl;
            bookmarks->bookmarks.erase(bookmarks->bookmarks.begin() + childIdx);
            bookmarks->saveToFile();
        }
    } else {
        // Nested bookmark inside a folder
        std::cout << "🗑️ Deleting nested item from folder at parent index: " << parentIdx << ", child index: " << childIdx << std::endl;
        if (parentIdx >= 0 && parentIdx < (int)bookmarks->bookmarks.size() &&
            bookmarks->bookmarks[parentIdx].isFolder &&
            childIdx >= 0 && childIdx < (int)bookmarks->bookmarks[parentIdx].children.size()) {
            std::cout << "   Deleting: " << bookmarks->bookmarks[parentIdx].children[childIdx].name
                      << " from folder: " << bookmarks->bookmarks[parentIdx].name << std::endl;
            bookmarks->bookmarks[parentIdx].children.erase(
                bookmarks->bookmarks[parentIdx].children.begin() + childIdx
            );
            bookmarks->saveToFile();
        }
    }

    bookmarks->refreshBookmarksList();
    bookmarks->triggerRefresh();  // Update the bookmarks bar
}

void BrayaBookmarks::onBookmarkDoubleClicked(GtkListBox* box, GtkListBoxRow* row, gpointer data) {
    // TODO: Open bookmark in browser
    int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "bookmark-index"));
    BrayaBookmarks* bookmarks = static_cast<BrayaBookmarks*>(data);
    
    if (index >= 0 && index < (int)bookmarks->bookmarks.size()) {
        std::cout << "Opening bookmark: " << bookmarks->bookmarks[index].url << std::endl;
        // Need to pass URL to browser window
    }
}

void BrayaBookmarks::onSearchChanged(GtkEntry* entry, gpointer data) {
    BrayaBookmarks* bookmarks = static_cast<BrayaBookmarks*>(data);
    bookmarks->refreshBookmarksList();
}

void BrayaBookmarks::onCloseClicked(GtkButton* button, gpointer data) {
    BrayaBookmarks* bookmarks = static_cast<BrayaBookmarks*>(data);
    if (bookmarks->managerDialog) {
        gtk_window_destroy(GTK_WINDOW(bookmarks->managerDialog));
        bookmarks->managerDialog = nullptr;
        bookmarks->bookmarksList = nullptr;
        bookmarks->searchEntry = nullptr;
    }
}

std::string BrayaBookmarks::showNewFolderDialog(GtkWindow* parent) {
    std::string result;

    GtkWidget* dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "📁 New Folder");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 350, 150);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_window_set_child(GTK_WINDOW(dialog), box);

    GtkWidget* label = gtk_label_new("Enter folder name:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), label);

    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "e.g., Work, Personal, Reading");
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
    gtk_box_append(GTK_BOX(box), entry);

    GtkWidget* btnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(box), btnBox);

    GtkWidget* cancelBtn = gtk_button_new_with_label("Cancel");
    gtk_widget_set_hexpand(cancelBtn, TRUE);
    gtk_box_append(GTK_BOX(btnBox), cancelBtn);

    GtkWidget* createBtn = gtk_button_new_with_label("Create");
    gtk_widget_set_hexpand(createBtn, TRUE);
    gtk_widget_add_css_class(createBtn, "suggested-action");
    gtk_window_set_default_widget(GTK_WINDOW(dialog), createBtn);
    gtk_box_append(GTK_BOX(btnBox), createBtn);

    // Use a struct to pass data between callbacks and retrieve result
    struct DialogData {
        std::string* result;
        GtkWidget* entry;
        GtkWidget* dialog;
        bool completed;
    };

    DialogData data;
    data.result = &result;
    data.entry = entry;
    data.dialog = dialog;
    data.completed = false;

    g_signal_connect(createBtn, "clicked", G_CALLBACK(+[](GtkWidget* btn, gpointer user_data) {
        DialogData* data = (DialogData*)user_data;
        const char* text = gtk_editable_get_text(GTK_EDITABLE(data->entry));
        if (strlen(text) > 0) {
            *data->result = text;
        }
        data->completed = true;
        gtk_window_destroy(GTK_WINDOW(data->dialog));
    }), &data);

    g_signal_connect(cancelBtn, "clicked", G_CALLBACK(+[](GtkWidget* btn, gpointer user_data) {
        DialogData* data = (DialogData*)user_data;
        data->completed = true;
        gtk_window_destroy(GTK_WINDOW(data->dialog));
    }), &data);

    // Show dialog and wait for it to close
    gtk_window_present(GTK_WINDOW(dialog));

    // Run a local event loop until dialog is closed
    GMainContext* context = g_main_context_default();
    while (!data.completed) {
        g_main_context_iteration(context, TRUE);
    }

    return result;
}

// Visual Bookmarks Bar Implementation

GtkWidget* BrayaBookmarks::createBookmarksBar() {
    std::cout << "BrayaBookmarks::createBookmarksBar() called" << std::endl;

    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    gtk_widget_set_vexpand(scrolled, FALSE);
    gtk_widget_set_size_request(scrolled, -1, 32);
    gtk_widget_add_css_class(scrolled, "bookmarks-bar-scroll");

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_add_css_class(box, "visual-bookmarks-bar");
    gtk_widget_set_margin_start(box, 0);
    gtk_widget_set_margin_end(box, 0);
    gtk_widget_set_margin_top(box, 0);
    gtk_widget_set_margin_bottom(box, 0);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), box);

    // Don't populate yet - will be done by BrayaWindow with callbacks
    std::cout << "✓ Bookmarks bar widget created (empty)" << std::endl;

    return scrolled;
}

// Forward declarations
static void renderFolderContents(GtkWidget* parentBox,
                                 const std::vector<Bookmark>& items,
                                 gpointer windowPtr,
                                 GCallback clickCallback,
                                 BrayaBookmarks* bookmarksManager);

// Drag and drop helper structure
struct DragData {
    std::string bookmarkUrl;
    std::string bookmarkName;
    bool isFolder;
    GtkWidget* bookmarksBar;
    BrayaBookmarks* bookmarksManager;
};

// Drag source callbacks
static GdkContentProvider* onDragPrepare(GtkDragSource* source, double x, double y, gpointer data) {
    GtkWidget* widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(source));
    const char* url = (const char*)g_object_get_data(G_OBJECT(widget), "url");
    const char* name = (const char*)g_object_get_data(G_OBJECT(widget), "bookmark-name");

    if (url && name) {
        // Store drag data
        g_object_set_data_full(G_OBJECT(source), "drag-url", g_strdup(url), g_free);
        g_object_set_data_full(G_OBJECT(source), "drag-name", g_strdup(name), g_free);

        GValue value = G_VALUE_INIT;
        g_value_init(&value, G_TYPE_STRING);
        g_value_set_string(&value, url);
        return gdk_content_provider_new_for_value(&value);
    }
    return nullptr;
}

static void onDragBegin(GtkDragSource* source, GdkDrag* drag, gpointer data) {
    GtkWidget* widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(source));
    gtk_widget_add_css_class(widget, "dragging");

    // If this widget is inside a popover, disable autohide to allow dragging out
    // Try to get popover from both the source and the widget
    GtkWidget* popover = GTK_WIDGET(g_object_get_data(G_OBJECT(source), "parent-popover"));
    if (!popover) {
        popover = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "parent-popover"));
    }
    if (popover && GTK_IS_POPOVER(popover)) {
        gtk_popover_set_autohide(GTK_POPOVER(popover), FALSE);
        gtk_popover_set_cascade_popdown(GTK_POPOVER(popover), FALSE);
        std::cout << "🎯 Disabled popover autohide for drag operation" << std::endl;
    }
}

static void onDragEnd(GtkDragSource* source, GdkDrag* drag, gboolean delete_data, gpointer data) {
    GtkWidget* widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(source));
    gtk_widget_remove_css_class(widget, "dragging");

    // Re-enable autohide and close the popover after drag
    GtkWidget* popover = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "parent-popover"));
    if (popover && GTK_IS_POPOVER(popover)) {
        gtk_popover_set_autohide(GTK_POPOVER(popover), TRUE);
        gtk_popover_popdown(GTK_POPOVER(popover));
        std::cout << "🎯 Re-enabled popover autohide and closed popover after drag" << std::endl;
    }
}

// Drop target callback
static gboolean onDropAccept(GtkDropTarget* target, GdkDrop* drop, gpointer data) {
    GtkWidget* widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(target));
    gtk_widget_add_css_class(widget, "drop-target");
    return TRUE;
}

static void onDropLeave(GtkDropTarget* target, gpointer data) {
    GtkWidget* widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(target));
    gtk_widget_remove_css_class(widget, "drop-target");
}

static gboolean onDrop(GtkDropTarget* target, const GValue* value, double x, double y, gpointer data) {
    GtkWidget* dropWidget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(target));
    gtk_widget_remove_css_class(dropWidget, "drop-target");

    BrayaBookmarks* bookmarksManager = (BrayaBookmarks*)data;
    const char* draggedId = g_value_get_string(value);
    const char* targetId = (const char*)g_object_get_data(G_OBJECT(dropWidget), "url");

    if (draggedId && targetId && strcmp(draggedId, targetId) != 0) {
        // Find indices of dragged and target items (bookmarks or folders)
        std::vector<Bookmark>& bookmarks = bookmarksManager->getBookmarksRef();
        int draggedIdx = -1, targetIdx = -1, draggedParentIdx = -1, draggedChildIdx = -1;
        bool targetIsFolder = false;
        Bookmark draggedBookmark;
        bool foundDragged = false;

        // First, search in top-level bookmarks bar items
        for (size_t i = 0; i < bookmarks.size(); i++) {
            // Only check items in bookmarks bar
            if (!bookmarks[i].folder.empty() && bookmarks[i].folder != "Bookmarks Bar") continue;

            std::string itemId;
            if (bookmarks[i].isFolder) {
                itemId = "folder:" + bookmarks[i].name;
            } else {
                itemId = bookmarks[i].url;
            }

            if (itemId == draggedId) {
                draggedIdx = i;
                draggedBookmark = bookmarks[i];
                foundDragged = true;
            }
            if (itemId == targetId) {
                targetIdx = i;
                targetIsFolder = bookmarks[i].isFolder;
            }
        }

        // If not found in top-level, search inside folders (for dragging OUT of folders)
        if (!foundDragged) {
            for (size_t i = 0; i < bookmarks.size(); i++) {
                if (bookmarks[i].isFolder) {
                    for (size_t j = 0; j < bookmarks[i].children.size(); j++) {
                        std::string itemId = bookmarks[i].children[j].url;
                        if (itemId == draggedId) {
                            draggedBookmark = bookmarks[i].children[j];
                            draggedParentIdx = i;
                            draggedChildIdx = j;
                            foundDragged = true;
                            std::cout << "📤 Found bookmark '" << draggedBookmark.name
                                      << "' inside folder '" << bookmarks[i].name << "'" << std::endl;
                            break;
                        }
                    }
                    if (foundDragged) break;
                }
            }
        }

        if (foundDragged && targetIdx >= 0) {
            // Dragging FROM inside a folder OUT to bookmarks bar
            if (draggedParentIdx >= 0 && draggedChildIdx >= 0) {
                std::cout << "📤 Moving bookmark OUT of folder '"
                          << bookmarks[draggedParentIdx].name << "' to bookmarks bar" << std::endl;

                // Remove from folder's children
                bookmarks[draggedParentIdx].children.erase(
                    bookmarks[draggedParentIdx].children.begin() + draggedChildIdx
                );

                // Check if target is a folder
                if (targetIsFolder) {
                    // Moving from one folder to another folder
                    bookmarks[targetIdx].children.push_back(draggedBookmark);
                    std::cout << "📁 Moved into folder '" << bookmarks[targetIdx].name << "'" << std::endl;
                } else {
                    // Insert at target position in bookmarks bar
                    draggedBookmark.folder = "Bookmarks Bar";
                    bookmarks.insert(bookmarks.begin() + targetIdx, draggedBookmark);
                    std::cout << "✓ Moved to bookmarks bar at position " << targetIdx << std::endl;
                }

                bookmarksManager->saveToFile();
            }
            // Normal drag and drop within bookmarks bar
            else if (draggedIdx >= 0 && targetIdx >= 0) {
                // Check if target is a folder - if so, move bookmark INTO folder
                if (targetIsFolder) {
                    // Don't allow dragging folders into folders (for now)
                    if (draggedBookmark.isFolder) {
                        return TRUE;
                    }

                    // Remove bookmark from its current location
                    bookmarks.erase(bookmarks.begin() + draggedIdx);

                    // Adjust target index if necessary
                    if (draggedIdx < targetIdx) targetIdx--;

                    // Add to folder's children
                    bookmarks[targetIdx].children.push_back(draggedBookmark);
                    bookmarksManager->saveToFile();

                    std::cout << "📁 Moved '" << draggedBookmark.name << "' into folder '"
                              << bookmarks[targetIdx].name << "'" << std::endl;
                } else {
                    // Normal reordering behavior
                    bookmarks.erase(bookmarks.begin() + draggedIdx);

                    // Adjust target index if necessary
                    if (draggedIdx < targetIdx) targetIdx--;

                    bookmarks.insert(bookmarks.begin() + targetIdx, draggedBookmark);
                    bookmarksManager->saveToFile();
                }
            }

            // Refresh the bookmarks bar
            GtkWidget* bookmarksBar = gtk_widget_get_parent(dropWidget);
            if (bookmarksBar) {
                // Find the window ptr and callbacks from the bar
                gpointer windowPtr = g_object_get_data(G_OBJECT(bookmarksBar), "window-ptr");
                GCallback clickCallback = (GCallback)g_object_get_data(G_OBJECT(bookmarksBar), "click-callback");
                GCallback rightClickCallback = (GCallback)g_object_get_data(G_OBJECT(bookmarksBar), "right-click-callback");

                bookmarksManager->updateBookmarksBar(bookmarksBar, windowPtr, clickCallback, nullptr, rightClickCallback);
            }
        }
    }

    return TRUE;
}

static void renderFolderContents(GtkWidget* parentBox,
                                 const std::vector<Bookmark>& items,
                                 gpointer windowPtr,
                                 GCallback clickCallback,
                                 BrayaBookmarks* bookmarksManager = nullptr) {
    for (const auto& item : items) {
        if (item.isFolder) {
            // Create a menu button for the subfolder
            GtkWidget* subfolderBtn = gtk_menu_button_new();
            gtk_widget_add_css_class(subfolderBtn, "folder-menu-item");
            gtk_widget_add_css_class(subfolderBtn, "bookmark-subfolder");

            GtkWidget* btnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

            // Folder icon
            GtkWidget* icon = gtk_image_new_from_icon_name("folder-symbolic");
            gtk_image_set_pixel_size(GTK_IMAGE(icon), 16);
            gtk_box_append(GTK_BOX(btnBox), icon);

            GtkWidget* label = gtk_label_new(item.name.c_str());
            gtk_label_set_xalign(GTK_LABEL(label), 0.0);
            gtk_widget_set_hexpand(label, TRUE);
            gtk_box_append(GTK_BOX(btnBox), label);

            // Arrow for submenu
            GtkWidget* arrow = gtk_image_new_from_icon_name("pan-end-symbolic");
            gtk_image_set_pixel_size(GTK_IMAGE(arrow), 12);
            gtk_box_append(GTK_BOX(btnBox), arrow);

            gtk_menu_button_set_child(GTK_MENU_BUTTON(subfolderBtn), btnBox);

            // Create cascading popover for subfolder
            GtkWidget* subPopover = gtk_popover_new();
            gtk_popover_set_position(GTK_POPOVER(subPopover), GTK_POS_RIGHT);

            // Configure popover to allow drag operations
            gtk_popover_set_cascade_popdown(GTK_POPOVER(subPopover), FALSE);

            GtkWidget* subMenuBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            gtk_widget_add_css_class(subMenuBox, "folder-menu");

            // Recursively render subfolder contents
            if (!item.children.empty()) {
                renderFolderContents(subMenuBox, item.children, windowPtr, clickCallback, bookmarksManager);
            } else {
                // Empty folder
                GtkWidget* emptyLabel = gtk_label_new("(Empty folder)");
                gtk_widget_add_css_class(emptyLabel, "dim-label");
                gtk_widget_set_margin_start(emptyLabel, 10);
                gtk_widget_set_margin_end(emptyLabel, 10);
                gtk_widget_set_margin_top(emptyLabel, 5);
                gtk_widget_set_margin_bottom(emptyLabel, 5);
                gtk_box_append(GTK_BOX(subMenuBox), emptyLabel);
            }

            gtk_popover_set_child(GTK_POPOVER(subPopover), subMenuBox);
            gtk_menu_button_set_popover(GTK_MENU_BUTTON(subfolderBtn), subPopover);

            gtk_box_append(GTK_BOX(parentBox), subfolderBtn);

        } else {
            // Regular bookmark item
            GtkWidget* itemBtn = gtk_button_new();
            gtk_widget_add_css_class(itemBtn, "folder-menu-item");

            GtkWidget* itemBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

            // Favicon or icon
            if (!item.faviconPath.empty()) {
                GtkWidget* favicon = gtk_image_new_from_file(item.faviconPath.c_str());
                gtk_image_set_pixel_size(GTK_IMAGE(favicon), 16);
                gtk_box_append(GTK_BOX(itemBox), favicon);
            } else {
                GtkWidget* itemIcon = gtk_image_new_from_icon_name("text-html-symbolic");
                gtk_image_set_pixel_size(GTK_IMAGE(itemIcon), 16);
                gtk_box_append(GTK_BOX(itemBox), itemIcon);
            }

            GtkWidget* itemLabel = gtk_label_new(item.name.c_str());
            gtk_label_set_xalign(GTK_LABEL(itemLabel), 0.0);
            gtk_box_append(GTK_BOX(itemBox), itemLabel);

            gtk_button_set_child(GTK_BUTTON(itemBtn), itemBox);
            gtk_widget_set_tooltip_text(itemBtn, item.url.c_str());

            // Store URL in button data
            g_object_set_data_full(G_OBJECT(itemBtn), "url", g_strdup(item.url.c_str()), g_free);
            g_object_set_data_full(G_OBJECT(itemBtn), "bookmark-name", g_strdup(item.name.c_str()), g_free);

            // Connect click handler
            if (windowPtr && clickCallback) {
                g_signal_connect(itemBtn, "clicked", clickCallback, windowPtr);
                // Get the popover ancestor to close it on click
                GtkWidget* ancestor = gtk_widget_get_ancestor(parentBox, GTK_TYPE_POPOVER);
                if (ancestor) {
                    g_signal_connect_swapped(itemBtn, "clicked", G_CALLBACK(gtk_popover_popdown), ancestor);
                }
            }

            // Add right-click context menu for editing/moving bookmarks in folders
            if (bookmarksManager) {
                GtkGesture* gesture = gtk_gesture_click_new();
                gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), GDK_BUTTON_SECONDARY);

                // Store bookmarks manager and item URL for the callback
                g_object_set_data(G_OBJECT(gesture), "bookmarks-manager", bookmarksManager);
                g_object_set_data_full(G_OBJECT(gesture), "bookmark-url", g_strdup(item.url.c_str()), g_free);
                g_object_set_data_full(G_OBJECT(gesture), "bookmark-name", g_strdup(item.name.c_str()), g_free);

                g_signal_connect(gesture, "pressed", G_CALLBACK(+[](GtkGestureClick* gesture, int n_press, double x, double y, gpointer data) {
                    GtkWidget* btn = GTK_WIDGET(gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture)));
                    BrayaBookmarks* bookmarksManager = (BrayaBookmarks*)g_object_get_data(G_OBJECT(gesture), "bookmarks-manager");
                    const char* url = (const char*)g_object_get_data(G_OBJECT(gesture), "bookmark-url");
                    const char* name = (const char*)g_object_get_data(G_OBJECT(gesture), "bookmark-name");

                    // Create context menu
                    GtkWidget* popover = gtk_popover_new();
                    gtk_widget_set_parent(popover, btn);

                    GtkWidget* menuBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
                    gtk_popover_set_child(GTK_POPOVER(popover), menuBox);

                    // "Move to Bookmarks Bar" option
                    GtkWidget* moveBtn = gtk_button_new_with_label("📤 Move to Bookmarks Bar");
                    gtk_widget_add_css_class(moveBtn, "flat");
                    g_object_set_data(G_OBJECT(moveBtn), "bookmarks-manager", bookmarksManager);
                    g_object_set_data_full(G_OBJECT(moveBtn), "bookmark-url", g_strdup(url), g_free);
                    g_object_set_data(G_OBJECT(moveBtn), "popover", popover);
                    g_signal_connect(moveBtn, "clicked", G_CALLBACK(+[](GtkWidget* widget, gpointer data) {
                        BrayaBookmarks* mgr = (BrayaBookmarks*)g_object_get_data(G_OBJECT(widget), "bookmarks-manager");
                        const char* url = (const char*)g_object_get_data(G_OBJECT(widget), "bookmark-url");
                        GtkWidget* popover = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "popover"));

                        std::cout << "📤 Moving bookmark to bookmarks bar: " << url << std::endl;

                        // Find the bookmark in folders and move it
                        std::vector<Bookmark>& bookmarks = mgr->getBookmarksRef();
                        for (size_t i = 0; i < bookmarks.size(); i++) {
                            if (bookmarks[i].isFolder) {
                                for (size_t j = 0; j < bookmarks[i].children.size(); j++) {
                                    if (bookmarks[i].children[j].url == url) {
                                        Bookmark bm = bookmarks[i].children[j];
                                        bm.folder = "Bookmarks Bar";
                                        bookmarks[i].children.erase(bookmarks[i].children.begin() + j);
                                        bookmarks.push_back(bm);
                                        mgr->saveToFile();

                                        std::cout << "✓ Moved bookmark to bookmarks bar" << std::endl;

                                        // Close all popovers and refresh
                                        gtk_popover_popdown(GTK_POPOVER(popover));

                                        // Trigger UI refresh
                                        mgr->triggerRefresh();

                                        return;
                                    }
                                }
                            }
                        }
                    }), nullptr);
                    gtk_box_append(GTK_BOX(menuBox), moveBtn);

                    // "Delete" option
                    GtkWidget* deleteBtn = gtk_button_new_with_label("🗑️ Delete Bookmark");
                    gtk_widget_add_css_class(deleteBtn, "flat");
                    gtk_widget_add_css_class(deleteBtn, "destructive-action");
                    g_object_set_data(G_OBJECT(deleteBtn), "bookmarks-manager", bookmarksManager);
                    g_object_set_data_full(G_OBJECT(deleteBtn), "bookmark-url", g_strdup(url), g_free);
                    g_object_set_data(G_OBJECT(deleteBtn), "popover", popover);
                    g_signal_connect(deleteBtn, "clicked", G_CALLBACK(+[](GtkWidget* widget, gpointer data) {
                        BrayaBookmarks* mgr = (BrayaBookmarks*)g_object_get_data(G_OBJECT(widget), "bookmarks-manager");
                        const char* url = (const char*)g_object_get_data(G_OBJECT(widget), "bookmark-url");
                        GtkWidget* popover = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "popover"));

                        std::cout << "🗑️ Deleting bookmark from folder: " << url << std::endl;

                        // Find and delete the bookmark from folder
                        std::vector<Bookmark>& bookmarks = mgr->getBookmarksRef();
                        for (size_t i = 0; i < bookmarks.size(); i++) {
                            if (bookmarks[i].isFolder) {
                                for (size_t j = 0; j < bookmarks[i].children.size(); j++) {
                                    if (bookmarks[i].children[j].url == url) {
                                        bookmarks[i].children.erase(bookmarks[i].children.begin() + j);
                                        mgr->saveToFile();
                                        std::cout << "✓ Deleted bookmark from folder" << std::endl;
                                        gtk_popover_popdown(GTK_POPOVER(popover));

                                        // Trigger UI refresh
                                        mgr->triggerRefresh();
                                        return;
                                    }
                                }
                            }
                        }
                    }), nullptr);
                    gtk_box_append(GTK_BOX(menuBox), deleteBtn);

                    gtk_popover_popup(GTK_POPOVER(popover));
                }), nullptr);

                gtk_widget_add_controller(itemBtn, GTK_EVENT_CONTROLLER(gesture));
                std::cout << "  → Added context menu to folder item: " << item.name << std::endl;
            }

            gtk_box_append(GTK_BOX(parentBox), itemBtn);
        }
    }
}

void BrayaBookmarks::updateBookmarksBar(GtkWidget* bookmarksBar,
                                        gpointer windowPtr,
                                        GCallback clickCallback,
                                        GCallback addCallback,
                                        GCallback rightClickCallback) {

    std::cout << "✓ Updating bookmarks bar with " << bookmarks.size() << " bookmarks" << std::endl;
    std::cout << "  Widget type: " << G_OBJECT_TYPE_NAME(bookmarksBar) << std::endl;

    // Verify we have a box widget
    if (!GTK_IS_BOX(bookmarksBar)) {
        std::cerr << "ERROR: bookmarksBar is not a GtkBox! It's a " << G_OBJECT_TYPE_NAME(bookmarksBar) << std::endl;
        return;
    }

    // Store callbacks and bookmarks manager in the bar for drag & drop
    g_object_set_data(G_OBJECT(bookmarksBar), "window-ptr", windowPtr);
    g_object_set_data(G_OBJECT(bookmarksBar), "click-callback", (gpointer)clickCallback);
    g_object_set_data(G_OBJECT(bookmarksBar), "right-click-callback", (gpointer)rightClickCallback);
    g_object_set_data(G_OBJECT(bookmarksBar), "bookmarks-manager", this);

    // Clear existing bookmarks with proper cleanup
    GtkWidget* child = gtk_widget_get_first_child(bookmarksBar);
    while (child) {
        GtkWidget* next = gtk_widget_get_next_sibling(child);

        // If this is a button, check for and destroy any popover children
        if (GTK_IS_BUTTON(child)) {
            GtkWidget* popover_child = gtk_widget_get_first_child(child);
            while (popover_child) {
                GtkWidget* popover_next = gtk_widget_get_next_sibling(popover_child);
                if (GTK_IS_POPOVER(popover_child)) {
                    gtk_widget_unparent(popover_child);
                }
                popover_child = popover_next;
            }
        }

        gtk_box_remove(GTK_BOX(bookmarksBar), child);
        child = next;
    }

    // Add bookmarks from root folder (empty folder = bookmarks bar)
    for (const auto& bookmark : bookmarks) {
        std::cout << "  → Checking bookmark: " << bookmark.name
                  << " | folder='" << bookmark.folder << "' | isFolder=" << bookmark.isFolder << std::endl;

        if (bookmark.folder.empty() || bookmark.folder == "Bookmarks Bar") {
            std::cout << "    ✓ Will appear on bar" << std::endl;

            // Check if this is a folder
            if (bookmark.isFolder) {
                std::cout << "    📁 This is a FOLDER - creating folder button" << std::endl;
                // Create folder button with dropdown (using regular button for better drag support)
                GtkWidget* btn = gtk_button_new();
                gtk_widget_add_css_class(btn, "bookmark-bar-item");
                gtk_widget_add_css_class(btn, "bookmark-folder");

                // Create box for folder icon + text
                GtkWidget* btnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

                // Folder icon
                GtkWidget* icon = gtk_image_new_from_icon_name("folder-symbolic");
                gtk_image_set_pixel_size(GTK_IMAGE(icon), 16);
                gtk_box_append(GTK_BOX(btnBox), icon);

                // Truncate long folder names
                std::string displayName = bookmark.name;
                if (displayName.length() > 20) {
                    displayName = displayName.substr(0, 17) + "...";
                }

                GtkWidget* label = gtk_label_new(displayName.c_str());
                gtk_box_append(GTK_BOX(btnBox), label);

                // Dropdown arrow
                GtkWidget* arrow = gtk_image_new_from_icon_name("pan-down-symbolic");
                gtk_image_set_pixel_size(GTK_IMAGE(arrow), 12);
                gtk_box_append(GTK_BOX(btnBox), arrow);

                gtk_button_set_child(GTK_BUTTON(btn), btnBox);
                gtk_widget_set_tooltip_text(btn, ("Folder: " + bookmark.name).c_str());

                // Create popover menu for folder contents
                GtkWidget* popover = gtk_popover_new();

                // Configure popover to allow drag operations
                gtk_popover_set_cascade_popdown(GTK_POPOVER(popover), FALSE);

                GtkWidget* menuBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
                gtk_widget_add_css_class(menuBox, "folder-menu");

                // Recursively render folder contents (supports nested folders)
                if (!bookmark.children.empty()) {
                    renderFolderContents(menuBox, bookmark.children, windowPtr, clickCallback, this);
                } else {
                    // If folder is empty, show placeholder
                    GtkWidget* emptyLabel = gtk_label_new("(Empty folder)");
                    gtk_widget_add_css_class(emptyLabel, "dim-label");
                    gtk_widget_set_margin_start(emptyLabel, 10);
                    gtk_widget_set_margin_end(emptyLabel, 10);
                    gtk_widget_set_margin_top(emptyLabel, 5);
                    gtk_widget_set_margin_bottom(emptyLabel, 5);
                    gtk_box_append(GTK_BOX(menuBox), emptyLabel);
                }

                gtk_popover_set_child(GTK_POPOVER(popover), menuBox);
                gtk_widget_set_parent(popover, btn);

                // Add click handler to open popover
                g_signal_connect_swapped(btn, "clicked", G_CALLBACK(gtk_popover_popup), popover);

                // Store folder identifier for drag & drop (use folder name as identifier)
                g_object_set_data_full(G_OBJECT(btn), "url", g_strdup(("folder:" + bookmark.name).c_str()), g_free);
                g_object_set_data_full(G_OBJECT(btn), "bookmark-name", g_strdup(bookmark.name.c_str()), g_free);
                g_object_set_data(G_OBJECT(btn), "is-folder", GINT_TO_POINTER(1));

                // Add drag source for folder reordering
                GtkDragSource* dragSource = gtk_drag_source_new();
                gtk_drag_source_set_actions(dragSource, GDK_ACTION_MOVE);
                g_signal_connect(dragSource, "prepare", G_CALLBACK(onDragPrepare), this);
                g_signal_connect(dragSource, "drag-begin", G_CALLBACK(onDragBegin), this);
                g_signal_connect(dragSource, "drag-end", G_CALLBACK(onDragEnd), this);
                gtk_widget_add_controller(btn, GTK_EVENT_CONTROLLER(dragSource));

                // Add drop target for folder reordering
                GtkDropTarget* dropTarget = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_MOVE);
                g_signal_connect(dropTarget, "accept", G_CALLBACK(onDropAccept), this);
                g_signal_connect(dropTarget, "leave", G_CALLBACK(onDropLeave), this);
                g_signal_connect(dropTarget, "drop", G_CALLBACK(onDrop), this);
                gtk_widget_add_controller(btn, GTK_EVENT_CONTROLLER(dropTarget));

                // Add right-click gesture for folder management
                if (windowPtr && rightClickCallback) {
                    GtkGesture* gesture = gtk_gesture_click_new();
                    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), GDK_BUTTON_SECONDARY);
                    g_signal_connect(gesture, "pressed", rightClickCallback, btn);
                    gtk_widget_add_controller(btn, GTK_EVENT_CONTROLLER(gesture));
                }

                gtk_box_append(GTK_BOX(bookmarksBar), btn);

            } else {
                // Regular bookmark (not a folder)
                GtkWidget* btn = gtk_button_new();
                gtk_widget_add_css_class(btn, "bookmark-bar-item");

                // Create box for favicon + text
                GtkWidget* btnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

                // Try to load favicon
                if (!bookmark.faviconPath.empty()) {
                    GtkWidget* favicon = gtk_image_new_from_file(bookmark.faviconPath.c_str());
                    gtk_image_set_pixel_size(GTK_IMAGE(favicon), 16);
                    gtk_box_append(GTK_BOX(btnBox), favicon);
                } else {
                    // Default icon
                    GtkWidget* icon = gtk_image_new_from_icon_name("text-html-symbolic");
                    gtk_image_set_pixel_size(GTK_IMAGE(icon), 16);
                    gtk_box_append(GTK_BOX(btnBox), icon);
                }

                // Truncate long titles
                std::string displayName = bookmark.name;
                if (displayName.length() > 20) {
                    displayName = displayName.substr(0, 17) + "...";
                }

                GtkWidget* label = gtk_label_new(displayName.c_str());
                gtk_box_append(GTK_BOX(btnBox), label);

                gtk_button_set_child(GTK_BUTTON(btn), btnBox);
                gtk_widget_set_tooltip_text(btn, (bookmark.name + "\n" + bookmark.url).c_str());

                // Store URL and name in button data
                g_object_set_data_full(G_OBJECT(btn), "url", g_strdup(bookmark.url.c_str()), g_free);
                g_object_set_data_full(G_OBJECT(btn), "bookmark-name", g_strdup(bookmark.name.c_str()), g_free);

                // Connect click handler if callback provided
                if (windowPtr && clickCallback) {
                    g_signal_connect(btn, "clicked", clickCallback, windowPtr);
                }

                // Add right-click gesture for edit/delete if callback provided
                if (windowPtr && rightClickCallback) {
                    GtkGesture* gesture = gtk_gesture_click_new();
                    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), GDK_BUTTON_SECONDARY);
                    g_signal_connect(gesture, "pressed", rightClickCallback, btn);
                    gtk_widget_add_controller(btn, GTK_EVENT_CONTROLLER(gesture));
                }

                // Add drag source for reordering
                GtkDragSource* dragSource = gtk_drag_source_new();
                gtk_drag_source_set_actions(dragSource, GDK_ACTION_MOVE);
                g_signal_connect(dragSource, "prepare", G_CALLBACK(onDragPrepare), this);
                g_signal_connect(dragSource, "drag-begin", G_CALLBACK(onDragBegin), this);
                g_signal_connect(dragSource, "drag-end", G_CALLBACK(onDragEnd), this);
                gtk_widget_add_controller(btn, GTK_EVENT_CONTROLLER(dragSource));

                // Add drop target for reordering
                GtkDropTarget* dropTarget = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_MOVE);
                g_signal_connect(dropTarget, "accept", G_CALLBACK(onDropAccept), this);
                g_signal_connect(dropTarget, "leave", G_CALLBACK(onDropLeave), this);
                g_signal_connect(dropTarget, "drop", G_CALLBACK(onDrop), this);
                gtk_widget_add_controller(btn, GTK_EVENT_CONTROLLER(dropTarget));

                gtk_box_append(GTK_BOX(bookmarksBar), btn);
            }
        }
    }

    // Note: Add bookmark button removed for cleaner look
    // Users can use Ctrl+D or the star icon in the headerbar

    std::cout << "✓ Bookmarks bar updated" << std::endl;
}

void BrayaBookmarks::filterBookmarksBar(GtkWidget* bookmarksBar, const std::string& query) {
    if (!GTK_IS_BOX(bookmarksBar)) return;

    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

    GtkWidget* child = gtk_widget_get_first_child(bookmarksBar);
    while (child) {
        const char* name = (const char*)g_object_get_data(G_OBJECT(child), "bookmark-name");
        const char* url = (const char*)g_object_get_data(G_OBJECT(child), "url");
        gboolean isFolder = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "is-folder"));

        if (query.empty()) {
            // Show all items when query is empty
            gtk_widget_set_visible(child, TRUE);
        } else if (name || url) {
            // Search in name and URL
            std::string lowerName = name ? name : "";
            std::string lowerUrl = url ? url : "";
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            std::transform(lowerUrl.begin(), lowerUrl.end(), lowerUrl.begin(), ::tolower);

            bool matches = (lowerName.find(lowerQuery) != std::string::npos ||
                           lowerUrl.find(lowerQuery) != std::string::npos);

            gtk_widget_set_visible(child, matches ? TRUE : FALSE);
        }

        child = gtk_widget_get_next_sibling(child);
    }
}

void BrayaBookmarks::addCurrentPage(const std::string& title, const std::string& url, GdkTexture* favicon) {
    // Save favicon if provided
    std::string faviconPath;
    if (favicon) {
        // Create favicons directory
        const char* homeDir = g_get_home_dir();
        std::string faviconsDir = std::string(homeDir) + "/.config/braya-browser/favicons";
        mkdir(faviconsDir.c_str(), 0755);
        
        // Generate filename from URL
        std::string filename = url;
        // Replace invalid chars
        for (char& c : filename) {
            if (c == '/' || c == ':' || c == '?' || c == '&') c = '_';
        }
        filename = filename.substr(0, 50) + ".png";
        
        faviconPath = faviconsDir + "/" + filename;
        gdk_texture_save_to_png(favicon, faviconPath.c_str());
    }
    
    addBookmark(title, url, "Bookmarks Bar");
    
    // Update the last bookmark with favicon path
    if (!faviconPath.empty() && !bookmarks.empty()) {
        bookmarks.back().faviconPath = faviconPath;
        saveToFile();
    }
    
    std::cout << "✓ Added bookmark: " << title << std::endl;
}

// Speed Dial Implementation

GtkWidget* BrayaBookmarks::createSpeedDial() {
    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_widget_set_hexpand(scrolled, TRUE);
    
    // Grid layout for speed dial
    GtkWidget* grid = gtk_grid_new();
    gtk_widget_add_css_class(grid, "speed-dial");
    gtk_grid_set_row_spacing(GTK_GRID(grid), 20);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 20);
    gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(grid, GTK_ALIGN_START);
    gtk_widget_set_margin_start(grid, 40);
    gtk_widget_set_margin_end(grid, 40);
    gtk_widget_set_margin_top(grid, 40);
    gtk_widget_set_margin_bottom(grid, 40);
    
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), grid);
    
    // Get favorite bookmarks
    auto favorites = getFavoriteBookmarks(12);
    
    // Create speed dial tiles (4 columns)
    int row = 0, col = 0;
    for (const auto& bookmark : favorites) {
        GtkWidget* tile = gtk_button_new();
        gtk_widget_add_css_class(tile, "speed-dial-tile");
        gtk_widget_set_size_request(tile, 180, 180);
        
        GtkWidget* tileBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_widget_set_halign(tileBox, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(tileBox, GTK_ALIGN_CENTER);
        
        // Large favicon or icon
        if (!bookmark.faviconPath.empty()) {
            GtkWidget* favicon = gtk_image_new_from_file(bookmark.faviconPath.c_str());
            gtk_image_set_pixel_size(GTK_IMAGE(favicon), 64);
            gtk_box_append(GTK_BOX(tileBox), favicon);
        } else {
            GtkWidget* icon = gtk_image_new_from_icon_name("text-html-symbolic");
            gtk_image_set_pixel_size(GTK_IMAGE(icon), 64);
            gtk_box_append(GTK_BOX(tileBox), icon);
        }
        
        // Title
        std::string displayName = bookmark.name;
        if (displayName.length() > 25) {
            displayName = displayName.substr(0, 22) + "...";
        }
        GtkWidget* label = gtk_label_new(displayName.c_str());
        gtk_widget_add_css_class(label, "speed-dial-label");
        gtk_label_set_wrap(GTK_LABEL(label), TRUE);
        gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
        gtk_label_set_max_width_chars(GTK_LABEL(label), 20);
        gtk_box_append(GTK_BOX(tileBox), label);
        
        gtk_button_set_child(GTK_BUTTON(tile), tileBox);
        gtk_widget_set_tooltip_text(tile, bookmark.url.c_str());
        
        // Store URL
        g_object_set_data_full(G_OBJECT(tile), "url", g_strdup(bookmark.url.c_str()), g_free);
        
        gtk_grid_attach(GTK_GRID(grid), tile, col, row, 1, 1);
        
        col++;
        if (col >= 4) {
            col = 0;
            row++;
        }
    }
    
    // Add "Add bookmark" tile
    GtkWidget* addTile = gtk_button_new();
    gtk_widget_add_css_class(addTile, "speed-dial-tile");
    gtk_widget_add_css_class(addTile, "speed-dial-add");
    gtk_widget_set_size_request(addTile, 180, 180);
    
    GtkWidget* addBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(addBox, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(addBox, GTK_ALIGN_CENTER);
    
    GtkWidget* addIcon = gtk_image_new_from_icon_name("list-add-symbolic");
    gtk_image_set_pixel_size(GTK_IMAGE(addIcon), 64);
    gtk_box_append(GTK_BOX(addBox), addIcon);
    
    GtkWidget* addLabel = gtk_label_new("Add Bookmark");
    gtk_box_append(GTK_BOX(addBox), addLabel);
    
    gtk_button_set_child(GTK_BUTTON(addTile), addBox);
    gtk_grid_attach(GTK_GRID(grid), addTile, col, row, 1, 1);
    
    return scrolled;
}

std::vector<Bookmark> BrayaBookmarks::getFavoriteBookmarks(int count) {
    std::vector<Bookmark> favorites;

    // Get most recent bookmarks (or could be most visited)
    for (const auto& bookmark : bookmarks) {
        if (favorites.size() >= count) break;
        favorites.push_back(bookmark);
    }

    return favorites;
}

// Import/Export Functions

// Static callbacks for import/export dialog
static void onImportChromeClicked(GtkButton*, BrayaBookmarks* bm) {
    GtkFileDialog* fd = gtk_file_dialog_new();
    gtk_file_dialog_set_title(fd, "Import from Chrome");
    gtk_file_dialog_open(fd, nullptr, nullptr,
        [](GObject* src, GAsyncResult* res, gpointer data) {
            GError* err = nullptr;
            GFile* file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(src), res, &err);
            if (file) {
                char* path = g_file_get_path(file);
                static_cast<BrayaBookmarks*>(data)->importFromChrome(path);
                g_free(path);
                g_object_unref(file);
            }
            if (err) g_error_free(err);
            g_object_unref(src);
        }, bm);
}

static void onImportFirefoxClicked(GtkButton*, BrayaBookmarks* bm) {
    GtkFileDialog* fd = gtk_file_dialog_new();
    gtk_file_dialog_set_title(fd, "Import from Firefox");
    gtk_file_dialog_open(fd, nullptr, nullptr,
        [](GObject* src, GAsyncResult* res, gpointer data) {
            GError* err = nullptr;
            GFile* file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(src), res, &err);
            if (file) {
                char* path = g_file_get_path(file);
                static_cast<BrayaBookmarks*>(data)->importFromFirefox(path);
                g_free(path);
                g_object_unref(file);
            }
            if (err) g_error_free(err);
            g_object_unref(src);
        }, bm);
}

static void onImportHTMLClicked(GtkButton*, BrayaBookmarks* bm) {
    GtkFileDialog* fd = gtk_file_dialog_new();
    gtk_file_dialog_set_title(fd, "Import from HTML");
    gtk_file_dialog_open(fd, nullptr, nullptr,
        [](GObject* src, GAsyncResult* res, gpointer data) {
            GError* err = nullptr;
            GFile* file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(src), res, &err);
            if (file) {
                char* path = g_file_get_path(file);
                static_cast<BrayaBookmarks*>(data)->importFromHTML(path);
                g_free(path);
                g_object_unref(file);
            }
            if (err) g_error_free(err);
            g_object_unref(src);
        }, bm);
}

static void onExportHTMLClicked(GtkButton*, BrayaBookmarks* bm) {
    GtkFileDialog* fd = gtk_file_dialog_new();
    gtk_file_dialog_set_title(fd, "Export to HTML");
    gtk_file_dialog_set_initial_name(fd, "bookmarks.html");
    gtk_file_dialog_save(fd, nullptr, nullptr,
        [](GObject* src, GAsyncResult* res, gpointer data) {
            GError* err = nullptr;
            GFile* file = gtk_file_dialog_save_finish(GTK_FILE_DIALOG(src), res, &err);
            if (file) {
                char* path = g_file_get_path(file);
                static_cast<BrayaBookmarks*>(data)->exportToHTML(path);
                g_free(path);
                g_object_unref(file);
            }
            if (err) g_error_free(err);
            g_object_unref(src);
        }, bm);
}

static void onImportExportClose(GtkDialog* dialog, int response, gpointer data) {
    gtk_window_destroy(GTK_WINDOW(dialog));
}

// Helper to write HTML bookmarks recursively
static void writeBookmarkHTML(std::ofstream& file, const Bookmark& bm, int indent = 4) {
    std::string indentStr(indent, ' ');

    if (bm.isFolder) {
        file << indentStr << "<DT><H3>" << bm.name << "</H3>\n";
        file << indentStr << "<DL><p>\n";
        for (const auto& child : bm.children) {
            writeBookmarkHTML(file, child, indent + 4);
        }
        file << indentStr << "</DL><p>\n";
    } else {
        file << indentStr << "<DT><A HREF=\"" << bm.url << "\"";
        if (bm.timestamp > 0) {
            file << " ADD_DATE=\"" << bm.timestamp << "\"";
        }
        file << ">" << bm.name << "</A>\n";
    }
}

bool BrayaBookmarks::exportToHTML(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for export: " << filepath << std::endl;
        return false;
    }

    // Write Netscape Bookmark File header
    file << "<!DOCTYPE NETSCAPE-Bookmark-file-1>\n";
    file << "<!-- This is an automatically generated file.\n";
    file << "     It will be read and overwritten.\n";
    file << "     DO NOT EDIT! -->\n";
    file << "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=UTF-8\">\n";
    file << "<TITLE>Bookmarks</TITLE>\n";
    file << "<H1>Bookmarks</H1>\n";
    file << "<DL><p>\n";

    // Write all bookmarks
    for (const auto& bookmark : bookmarks) {
        writeBookmarkHTML(file, bookmark);
    }

    file << "</DL><p>\n";
    file.close();

    std::cout << "✓ Exported " << bookmarks.size() << " bookmarks to " << filepath << std::endl;
    return true;
}

bool BrayaBookmarks::importFromHTML(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for import: " << filepath << std::endl;
        return false;
    }

    std::string line;
    std::string currentFolder;
    int importCount = 0;

    while (std::getline(file, line)) {
        // Parse <H3> for folder names
        size_t h3Pos = line.find("<H3>");
        if (h3Pos != std::string::npos) {
            size_t h3End = line.find("</H3>");
            if (h3End != std::string::npos) {
                currentFolder = line.substr(h3Pos + 4, h3End - h3Pos - 4);
                addFolder(currentFolder);
                continue;
            }
        }

        // Parse <A HREF="..."> for bookmarks
        size_t aPos = line.find("<A HREF=\"");
        if (aPos != std::string::npos) {
            size_t urlStart = aPos + 9;
            size_t urlEnd = line.find("\"", urlStart);
            if (urlEnd != std::string::npos) {
                std::string url = line.substr(urlStart, urlEnd - urlStart);

                size_t nameStart = line.find(">", urlEnd) + 1;
                size_t nameEnd = line.find("</A>", nameStart);
                if (nameEnd != std::string::npos) {
                    std::string name = line.substr(nameStart, nameEnd - nameStart);

                    if (!currentFolder.empty()) {
                        addBookmarkToFolder(name, url, currentFolder);
                    } else {
                        addBookmark(name, url);
                    }
                    importCount++;
                }
            }
        }

        // Reset folder on </DL>
        if (line.find("</DL>") != std::string::npos) {
            currentFolder.clear();
        }
    }

    file.close();
    std::cout << "✓ Imported " << importCount << " bookmarks from " << filepath << std::endl;
    return true;
}

// Helper to parse Chrome/Firefox JSON recursively
static void parseChromiumBookmark(JsonObject* obj, BrayaBookmarks* manager, const std::string& parentFolder = "") {
    if (!obj) return;

    const char* type = json_object_has_member(obj, "type") ? json_object_get_string_member(obj, "type") : "";
    const char* name = json_object_has_member(obj, "name") ? json_object_get_string_member(obj, "name") : "";

    if (strcmp(type, "folder") == 0) {
        // It's a folder
        std::string folderName = name;
        manager->addFolder(folderName, parentFolder);

        // Process children
        if (json_object_has_member(obj, "children")) {
            JsonArray* children = json_object_get_array_member(obj, "children");
            guint len = json_array_get_length(children);
            for (guint i = 0; i < len; i++) {
                JsonObject* child = json_array_get_object_element(children, i);
                parseChromiumBookmark(child, manager, folderName);
            }
        }
    } else if (strcmp(type, "url") == 0) {
        // It's a bookmark
        const char* url = json_object_has_member(obj, "url") ? json_object_get_string_member(obj, "url") : "";
        if (url && strlen(url) > 0) {
            if (!parentFolder.empty()) {
                manager->addBookmarkToFolder(name, url, parentFolder);
            } else {
                manager->addBookmark(name, url);
            }
        }
    }
}

bool BrayaBookmarks::importFromChrome(const std::string& filepath) {
    GError* error = nullptr;
    JsonParser* parser = json_parser_new();

    if (!json_parser_load_from_file(parser, filepath.c_str(), &error)) {
        if (error) {
            std::cerr << "Failed to parse Chrome bookmarks: " << error->message << std::endl;
            g_error_free(error);
        }
        g_object_unref(parser);
        return false;
    }

    JsonNode* root = json_parser_get_root(parser);
    if (!root || !JSON_NODE_HOLDS_OBJECT(root)) {
        g_object_unref(parser);
        return false;
    }

    JsonObject* rootObj = json_node_get_object(root);

    // Chrome bookmarks structure: {"roots": {"bookmark_bar": {...}, "other": {...}}}
    if (json_object_has_member(rootObj, "roots")) {
        JsonObject* roots = json_object_get_object_member(rootObj, "roots");

        // Import from bookmark bar
        if (json_object_has_member(roots, "bookmark_bar")) {
            JsonObject* bookmarkBar = json_object_get_object_member(roots, "bookmark_bar");
            if (json_object_has_member(bookmarkBar, "children")) {
                JsonArray* children = json_object_get_array_member(bookmarkBar, "children");
                guint len = json_array_get_length(children);
                for (guint i = 0; i < len; i++) {
                    JsonObject* child = json_array_get_object_element(children, i);
                    parseChromiumBookmark(child, this);
                }
            }
        }

        // Import from "Other Bookmarks"
        if (json_object_has_member(roots, "other")) {
            JsonObject* other = json_object_get_object_member(roots, "other");
            if (json_object_has_member(other, "children")) {
                JsonArray* children = json_object_get_array_member(other, "children");
                guint len = json_array_get_length(children);
                for (guint i = 0; i < len; i++) {
                    JsonObject* child = json_array_get_object_element(children, i);
                    parseChromiumBookmark(child, this, "Other Bookmarks");
                }
            }
        }
    }

    g_object_unref(parser);
    std::cout << "✓ Imported bookmarks from Chrome" << std::endl;
    return true;
}

bool BrayaBookmarks::importFromFirefox(const std::string& filepath) {
    // Firefox uses similar JSON structure to Chrome
    return importFromChrome(filepath);  // Same logic works for both
}

void BrayaBookmarks::showImportExportDialog(GtkWindow* parent) {
    GtkWidget* dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Import/Export Bookmarks");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 380, -1);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);

    GtkWidget* importLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(importLabel), "<b>Import Bookmarks</b>");
    gtk_widget_set_halign(importLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), importLabel);

    GtkWidget* importBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget* importChromeBtn  = gtk_button_new_with_label("From Chrome");
    GtkWidget* importFirefoxBtn = gtk_button_new_with_label("From Firefox");
    GtkWidget* importHTMLBtn    = gtk_button_new_with_label("From HTML");
    gtk_box_append(GTK_BOX(importBox), importChromeBtn);
    gtk_box_append(GTK_BOX(importBox), importFirefoxBtn);
    gtk_box_append(GTK_BOX(importBox), importHTMLBtn);
    gtk_box_append(GTK_BOX(box), importBox);

    GtkWidget* exportLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(exportLabel), "<b>Export Bookmarks</b>");
    gtk_widget_set_halign(exportLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), exportLabel);

    GtkWidget* exportBtn = gtk_button_new_with_label("Export to HTML");
    gtk_box_append(GTK_BOX(box), exportBtn);

    GtkWidget* closeBtn = gtk_button_new_with_label("Close");
    gtk_widget_add_css_class(closeBtn, "pill");
    gtk_widget_set_halign(closeBtn, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(box), closeBtn);

    g_signal_connect(importChromeBtn,  "clicked", G_CALLBACK(onImportChromeClicked),  this);
    g_signal_connect(importFirefoxBtn, "clicked", G_CALLBACK(onImportFirefoxClicked), this);
    g_signal_connect(importHTMLBtn,    "clicked", G_CALLBACK(onImportHTMLClicked),    this);
    g_signal_connect(exportBtn,        "clicked", G_CALLBACK(onExportHTMLClicked),    this);
    g_signal_connect_swapped(closeBtn, "clicked", G_CALLBACK(gtk_window_destroy), dialog);

    gtk_window_set_child(GTK_WINDOW(dialog), box);
    gtk_window_present(GTK_WINDOW(dialog));
}
