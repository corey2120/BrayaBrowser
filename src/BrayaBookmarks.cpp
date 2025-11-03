#include "BrayaBookmarks.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>

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
    bookmarks.push_back(Bookmark(name, url, folder));
    saveToFile();
}

void BrayaBookmarks::editBookmark(int index, const std::string& name, const std::string& url, const std::string& folder) {
    if (index >= 0 && index < (int)bookmarks.size()) {
        bookmarks[index].name = name;
        bookmarks[index].url = url;
        bookmarks[index].folder = folder;
        saveToFile();
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
    int index = findBookmarkByUrl(url);
    if (index >= 0) {
        deleteBookmark(index);
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

void BrayaBookmarks::saveToFile() {
    std::ofstream file(bookmarksFilePath);
    if (!file.is_open()) {
        std::cerr << "Failed to save bookmarks" << std::endl;
        return;
    }
    
    file << "[\n";
    for (size_t i = 0; i < bookmarks.size(); i++) {
        const auto& bm = bookmarks[i];
        file << "  {\n";
        file << "    \"name\": \"" << bm.name << "\",\n";
        file << "    \"url\": \"" << bm.url << "\",\n";
        file << "    \"folder\": \"" << bm.folder << "\",\n";
        file << "    \"timestamp\": " << bm.timestamp << "\n";
        file << "  }" << (i < bookmarks.size() - 1 ? "," : "") << "\n";
    }
    file << "]\n";
    file.close();
}

void BrayaBookmarks::loadFromFile() {
    std::ifstream file(bookmarksFilePath);
    if (!file.is_open()) {
        return;
    }
    
    bookmarks.clear();
    std::string line;
    std::string name, url, folder;
    time_t timestamp = 0;
    
    while (std::getline(file, line)) {
        if (line.find("\"name\":") != std::string::npos) {
            size_t start = line.find("\"", line.find(":")) + 1;
            size_t end = line.rfind("\"");
            name = line.substr(start, end - start);
        }
        else if (line.find("\"url\":") != std::string::npos) {
            size_t start = line.find("\"", line.find(":")) + 1;
            size_t end = line.rfind("\"");
            url = line.substr(start, end - start);
        }
        else if (line.find("\"folder\":") != std::string::npos) {
            size_t start = line.find("\"", line.find(":")) + 1;
            size_t end = line.rfind("\"");
            folder = line.substr(start, end - start);
        }
        else if (line.find("\"timestamp\":") != std::string::npos) {
            size_t start = line.find(":") + 1;
            timestamp = std::stoll(line.substr(start));
            
            if (!name.empty() && !url.empty()) {
                Bookmark bm(name, url, folder);
                bm.timestamp = timestamp;
                bookmarks.push_back(bm);
                name.clear();
                url.clear();
                folder.clear();
                timestamp = 0;
            }
        }
    }
    file.close();
}

void BrayaBookmarks::showBookmarksManager(GtkWindow* parent) {
    if (managerDialog) {
        gtk_window_present(GTK_WINDOW(managerDialog));
        return;
    }
    
    createManagerDialog(parent);
    refreshBookmarksList();
    gtk_window_present(GTK_WINDOW(managerDialog));
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
    
    // Get bookmarks to display (filtered or all)
    std::vector<Bookmark> displayBookmarks;
    if (searchEntry) {
        const char* searchText = gtk_editable_get_text(GTK_EDITABLE(searchEntry));
        if (strlen(searchText) > 0) {
            displayBookmarks = searchBookmarks(searchText);
        } else {
            displayBookmarks = bookmarks;
        }
    } else {
        displayBookmarks = bookmarks;
    }
    
    // Add bookmarks
    for (size_t i = 0; i < displayBookmarks.size(); i++) {
        const auto& bm = displayBookmarks[i];
        
        GtkWidget* row = gtk_list_box_row_new();
        g_object_set_data(G_OBJECT(row), "bookmark-index", GINT_TO_POINTER(i));
        
        GtkWidget* rowBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_widget_set_margin_start(rowBox, 15);
        gtk_widget_set_margin_end(rowBox, 15);
        gtk_widget_set_margin_top(rowBox, 10);
        gtk_widget_set_margin_bottom(rowBox, 10);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), rowBox);
        
        // Name
        GtkWidget* nameLabel = gtk_label_new(bm.name.c_str());
        gtk_widget_set_halign(nameLabel, GTK_ALIGN_START);
        gtk_label_set_ellipsize(GTK_LABEL(nameLabel), PANGO_ELLIPSIZE_END);
        gtk_widget_add_css_class(nameLabel, "bookmark-name");
        gtk_box_append(GTK_BOX(rowBox), nameLabel);
        
        // URL
        GtkWidget* urlLabel = gtk_label_new(bm.url.c_str());
        gtk_widget_set_halign(urlLabel, GTK_ALIGN_START);
        gtk_label_set_ellipsize(GTK_LABEL(urlLabel), PANGO_ELLIPSIZE_END);
        gtk_widget_add_css_class(urlLabel, "bookmark-url");
        gtk_box_append(GTK_BOX(rowBox), urlLabel);
        
        // Folder (if exists)
        if (!bm.folder.empty()) {
            std::string folderText = "📁 " + bm.folder;
            GtkWidget* folderLabel = gtk_label_new(folderText.c_str());
            gtk_widget_set_halign(folderLabel, GTK_ALIGN_START);
            gtk_widget_add_css_class(folderLabel, "bookmark-folder");
            gtk_box_append(GTK_BOX(rowBox), folderLabel);
        }
        
        gtk_list_box_append(GTK_LIST_BOX(bookmarksList), row);
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
    
    GtkWidget* folderEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(folderEntry), "Folder (optional)");
    gtk_box_append(GTK_BOX(box), folderEntry);
    
    GtkWidget* btnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(box), btnBox);
    
    GtkWidget* saveBtn = gtk_button_new_with_label("Save");
    gtk_widget_set_hexpand(saveBtn, TRUE);
    gtk_box_append(GTK_BOX(btnBox), saveBtn);
    
    GtkWidget* cancelBtn = gtk_button_new_with_label("Cancel");
    gtk_widget_set_hexpand(cancelBtn, TRUE);
    gtk_box_append(GTK_BOX(btnBox), cancelBtn);
    
    g_signal_connect(saveBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        GtkWidget** widgets = (GtkWidget**)data;
        BrayaBookmarks* bookmarks = (BrayaBookmarks*)g_object_get_data(G_OBJECT(btn), "bookmarks");
        
        const char* name = gtk_editable_get_text(GTK_EDITABLE(widgets[0]));
        const char* url = gtk_editable_get_text(GTK_EDITABLE(widgets[1]));
        const char* folder = gtk_editable_get_text(GTK_EDITABLE(widgets[2]));
        
        if (strlen(name) > 0 && strlen(url) > 0) {
            bookmarks->addBookmark(name, url, folder);
            bookmarks->refreshBookmarksList();
        }
        
        gtk_window_destroy(GTK_WINDOW(widgets[3]));
        g_free(widgets);
    }), nullptr);
    
    GtkWidget** widgets = (GtkWidget**)g_malloc(sizeof(GtkWidget*) * 4);
    widgets[0] = nameEntry;
    widgets[1] = urlEntry;
    widgets[2] = folderEntry;
    widgets[3] = dialog;
    g_object_set_data(G_OBJECT(saveBtn), "bookmarks", bookmarks);
    g_signal_connect(saveBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        GtkWidget** widgets = (GtkWidget**)data;
        BrayaBookmarks* bookmarks = (BrayaBookmarks*)g_object_get_data(G_OBJECT(btn), "bookmarks");
        
        const char* name = gtk_editable_get_text(GTK_EDITABLE(widgets[0]));
        const char* url = gtk_editable_get_text(GTK_EDITABLE(widgets[1]));
        const char* folder = gtk_editable_get_text(GTK_EDITABLE(widgets[2]));
        
        if (strlen(name) > 0 && strlen(url) > 0) {
            bookmarks->addBookmark(name, url, folder);
            bookmarks->refreshBookmarksList();
        }
        
        gtk_window_destroy(GTK_WINDOW(widgets[3]));
        g_free(widgets);
    }), widgets);
    
    g_signal_connect(cancelBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        gtk_window_destroy(GTK_WINDOW(data));
    }), dialog);
    
    gtk_window_present(GTK_WINDOW(dialog));
}

void BrayaBookmarks::onEditBookmarkClicked(GtkButton* button, gpointer data) {
    BrayaBookmarks* bookmarks = static_cast<BrayaBookmarks*>(data);
    
    // Get selected row
    GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(bookmarks->bookmarksList));
    if (!row) return;
    
    int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "bookmark-index"));
    if (index < 0 || index >= (int)bookmarks->bookmarks.size()) return;
    
    const Bookmark& bm = bookmarks->bookmarks[index];
    
    // Create edit dialog (similar to add)
    GtkWidget* dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Edit Bookmark");
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
    gtk_editable_set_text(GTK_EDITABLE(nameEntry), bm.name.c_str());
    gtk_box_append(GTK_BOX(box), nameEntry);
    
    GtkWidget* urlEntry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(urlEntry), bm.url.c_str());
    gtk_box_append(GTK_BOX(box), urlEntry);
    
    GtkWidget* folderEntry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(folderEntry), bm.folder.c_str());
    gtk_box_append(GTK_BOX(box), folderEntry);
    
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
    widgets[2] = folderEntry;
    widgets[3] = dialog;
    g_object_set_data(G_OBJECT(saveBtn), "bookmarks", bookmarks);
    g_object_set_data(G_OBJECT(saveBtn), "index", GINT_TO_POINTER(index));
    
    g_signal_connect(saveBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        GtkWidget** widgets = (GtkWidget**)data;
        BrayaBookmarks* bookmarks = (BrayaBookmarks*)g_object_get_data(G_OBJECT(btn), "bookmarks");
        int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "index"));
        
        const char* name = gtk_editable_get_text(GTK_EDITABLE(widgets[0]));
        const char* url = gtk_editable_get_text(GTK_EDITABLE(widgets[1]));
        const char* folder = gtk_editable_get_text(GTK_EDITABLE(widgets[2]));
        
        if (strlen(name) > 0 && strlen(url) > 0) {
            bookmarks->editBookmark(index, name, url, folder);
            bookmarks->refreshBookmarksList();
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
    if (!row) return;
    
    int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "bookmark-index"));
    bookmarks->deleteBookmark(index);
    bookmarks->refreshBookmarksList();
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

// Visual Bookmarks Bar Implementation

GtkWidget* BrayaBookmarks::createBookmarksBar() {
    std::cout << "BrayaBookmarks::createBookmarksBar() called" << std::endl;

    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    gtk_widget_set_vexpand(scrolled, FALSE);
    gtk_widget_add_css_class(scrolled, "bookmarks-bar-scroll");

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_add_css_class(box, "visual-bookmarks-bar");
    gtk_widget_set_margin_start(box, 10);
    gtk_widget_set_margin_end(box, 10);
    gtk_widget_set_margin_top(box, 5);
    gtk_widget_set_margin_bottom(box, 5);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), box);

    // Don't populate yet - will be done by BrayaWindow with callbacks
    std::cout << "✓ Bookmarks bar widget created (empty)" << std::endl;

    return scrolled;
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

    // Clear existing bookmarks
    GtkWidget* child = gtk_widget_get_first_child(bookmarksBar);
    while (child) {
        GtkWidget* next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(bookmarksBar), child);
        child = next;
    }

    // Add bookmarks from root folder (empty folder = bookmarks bar)
    for (const auto& bookmark : bookmarks) {
        if (bookmark.folder.empty() || bookmark.folder == "Bookmarks Bar") {
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

            gtk_box_append(GTK_BOX(bookmarksBar), btn);
        }
    }

    // Add "+" button to add bookmark
    GtkWidget* addBtn = gtk_button_new_from_icon_name("list-add-symbolic");
    gtk_widget_add_css_class(addBtn, "bookmark-add-btn");
    gtk_widget_set_tooltip_text(addBtn, "Add Current Page");

    // Connect add button click handler if callback provided
    if (windowPtr && addCallback) {
        g_signal_connect(addBtn, "clicked", addCallback, windowPtr);
    }

    gtk_box_append(GTK_BOX(bookmarksBar), addBtn);

    std::cout << "✓ Bookmarks bar updated" << std::endl;
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
