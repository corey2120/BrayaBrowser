# Bookmarks Management System - Implementation Guide

**Status**: Dialogs implemented, wiring in progress
**Date**: November 3, 2024

---

## ✅ What's Implemented

### 1. Edit Bookmark Dialog
- Right-click bookmark → "Edit Bookmark"
- Shows dialog with 3 fields:
  - **Name**: Bookmark title
  - **URL**: Bookmark address
  - **Folder**: Organize into folders
- Save/Cancel buttons
- Pre-filled with current values

### 2. Delete Bookmark Dialog
- Right-click bookmark → "Delete Bookmark"
- Shows confirmation dialog
- Displays bookmark name and URL
- Delete/Cancel buttons

### 3. Backend Methods (BrayaBookmarks)
```cpp
// New methods added:
void editBookmarkByUrl(oldUrl, name, url, folder);
void deleteBookmarkByUrl(url);
int findBookmarkByUrl(url);
```

### 4. Copy URL
- ✅ **Already working!**
- Right-click → "Copy URL"
- Copies to clipboard instantly

---

## 🚧 What Needs Completion

The dialogs appear correctly but need to actually call the BrayaBookmarks methods.

### Architecture Challenge

The problem: The dialog callbacks are lambdas that don't have direct access to:
1. The `BrayaWindow` instance
2. The `bookmarksManager` instance

### Solution Options

#### Option A: Store Window Pointer (Recommended)
```cpp
// In the edit dialog callback:
g_object_set_data(G_OBJECT(saveBtn), "braya-window", window);

// In save callback:
BrayaWindow* window = (BrayaWindow*)g_object_get_data(..., "braya-window");
window->bookmarksManager->editBookmarkByUrl(...);
window->refreshBookmarksBar();
```

#### Option B: Create Dedicated Methods
Add to BrayaWindow.h:
```cpp
void editBookmarkFromBar(oldUrl, newName, newUrl, newFolder);
void deleteBookmarkFromBar(url);
void refreshBookmarksBar();
```

---

## 📋 Folder Support

### Current State
- Bookmarks have a `folder` field
- Only "Bookmarks Bar" or empty folders show in bar
- Other folders are hidden

### To Implement

1. **Folder Buttons** - Show folders in the bar as clickable buttons
2. **Dropdown Menus** - Click folder → shows bookmarks inside
3. **Folder Creation** - When editing/adding, can type new folder name
4. **Nested Folders** - Support subfolders (optional)

### Example Folder Bar
```
[🍎 Apple] [G Google] [📁 Work ▼] [📁 Dev ▼] [+]
```

Click "Work ▼" → dropdown shows:
```
├─ Email
├─ Calendar
└─ Slack
```

---

## 🎯 Completing the Implementation

### Step 1: Wire Up Edit Dialog

File: `src/BrayaWindow.cpp` around line 1496

Replace:
```cpp
std::cout << "✓ Editing bookmark: " << oldUrl << " → " << newUrl << std::endl;
gtk_window_destroy(GTK_WINDOW(dialog));
```

With:
```cpp
// Get BrayaWindow instance (need to store it)
BrayaWindow* window = ...;  // TODO: Get from dialog data

// Edit the bookmark
window->bookmarksManager->editBookmarkByUrl(oldUrl, newName, newUrl, newFolder);

// Refresh the bookmarks bar
window->refreshBookmarksBar();

std::cout << "✓ Edited bookmark: " << oldUrl << " → " << newUrl << std::endl;
gtk_window_destroy(GTK_WINDOW(dialog));
```

### Step 2: Wire Up Delete Dialog

File: `src/BrayaWindow.cpp` around line 1577

Replace:
```cpp
std::cout << "✓ Deleting bookmark: " << url << std::endl;
gtk_window_destroy(GTK_WINDOW(dialog));
```

With:
```cpp
// Get BrayaWindow instance
BrayaWindow* window = ...;  // TODO: Get from dialog data

// Delete the bookmark
window->bookmarksManager->deleteBookmarkByUrl(url);

// Refresh the bookmarks bar
window->refreshBookmarksBar();

std::cout << "✓ Deleted bookmark: " << url << std::endl;
gtk_window_destroy(GTK_WINDOW(dialog));
```

### Step 3: Add Helper Method

File: `src/BrayaWindow.h`

```cpp
// Add to BrayaWindow class:
void refreshBookmarksBar();
```

File: `src/BrayaWindow.cpp`

```cpp
void BrayaWindow::refreshBookmarksBar() {
    if (!bookmarksBar || !bookmarksManager) return;

    // Get the box widget
    GtkWidget* child = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(bookmarksBar));
    if (!child) return;

    // Handle viewport wrapping
    GtkWidget* boxWidget = child;
    if (GTK_IS_VIEWPORT(child)) {
        boxWidget = gtk_viewport_get_child(GTK_VIEWPORT(child));
    }

    if (boxWidget && GTK_IS_BOX(boxWidget)) {
        bookmarksManager->updateBookmarksBar(
            boxWidget,
            this,
            G_CALLBACK(onBookmarkBarItemClicked),
            G_CALLBACK(onBookmarkBarAddClicked),
            G_CALLBACK(onBookmarkBarItemRightClick)
        );
    }
}
```

---

## 🗂️ Adding Folder Support

### Step 1: Detect Folders

In `BrayaBookmarks.cpp`, modify `updateBookmarksBar()`:

```cpp
// Get unique folders
std::set<std::string> folders;
for (const auto& bookmark : bookmarks) {
    if (!bookmark.folder.empty() && bookmark.folder != "Bookmarks Bar") {
        folders.insert(bookmark.folder);
    }
}

// Add folder buttons BEFORE individual bookmarks
for (const auto& folder : folders) {
    GtkWidget* folderBtn = gtk_menu_button_new();
    gtk_widget_add_css_class(folderBtn, "bookmark-folder-btn");

    // Create popover with bookmarks in this folder
    GtkWidget* popover = gtk_popover_new();
    GtkWidget* folderBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // Add bookmarks from this folder to popover
    for (const auto& bm : bookmarks) {
        if (bm.folder == folder) {
            GtkWidget* item = gtk_button_new_with_label(bm.name.c_str());
            // ... connect signals ...
            gtk_box_append(GTK_BOX(folderBox), item);
        }
    }

    gtk_popover_set_child(GTK_POPOVER(popover), folderBox);
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(folderBtn), popover);

    gtk_box_append(GTK_BOX(bookmarksBar), folderBtn);
}
```

### Step 2: Style Folders

In `resources/style.css`:

```css
.bookmark-folder-btn {
    padding: 2px 12px;
    margin: 0 2px;
    border-radius: 3px;
    background: rgba(0, 0, 0, 0.03);
    font-size: 12px;
    font-weight: 600;
}

.bookmark-folder-btn:hover {
    background: rgba(0, 0, 0, 0.08);
}
```

---

## 📖 Usage Guide

### For End Users

#### Edit a Bookmark
1. Right-click bookmark in bar
2. Click "✏️ Edit Bookmark"
3. Change name, URL, or folder
4. Click "Save"

#### Delete a Bookmark
1. Right-click bookmark in bar
2. Click "🗑️ Delete Bookmark"
3. Confirm deletion
4. Bookmark removed

#### Organize into Folders
1. Right-click bookmark → Edit
2. Type folder name (e.g., "Work", "Dev", "News")
3. Save
4. Bookmark moves to that folder
5. Folder button appears in bar

#### Copy Bookmark URL
1. Right-click bookmark
2. Click "📋 Copy URL"
3. URL copied to clipboard

---

## 🎨 UI/UX Design

### Bookmarks Bar Layout

```
┌─────────────────────────────────────────────────────┐
│ [🍎 Apple] [📁 Work ▼] [📁 Dev ▼] [G Google] [+]   │
└─────────────────────────────────────────────────────┘
```

### Folder Dropdown

```
📁 Work ▼
├─────────────┐
│ 📧 Email    │
│ 📅 Calendar │
│ 💬 Slack    │
│ 📊 Analytics│
└─────────────┘
```

### Right-Click Menu

```
┌──────────────────────┐
│ ✏️ Edit Bookmark     │
│ 🗑️ Delete Bookmark   │
│ 📋 Copy URL          │
└──────────────────────┘
```

---

## 🧪 Testing Checklist

### Basic Operations
- [ ] Edit bookmark name
- [ ] Edit bookmark URL
- [ ] Move bookmark to folder
- [ ] Delete bookmark
- [ ] Copy bookmark URL

### Folder Operations
- [ ] Create new folder
- [ ] Folder appears in bar
- [ ] Click folder shows bookmarks
- [ ] Move bookmark between folders
- [ ] Delete last bookmark in folder (folder disappears)

### Edge Cases
- [ ] Edit bookmark to empty name (should prevent)
- [ ] Edit bookmark to invalid URL (should warn)
- [ ] Delete only bookmark (bar shows empty + button)
- [ ] Very long bookmark names (should truncate)
- [ ] Very long folder names (should truncate)
- [ ] Special characters in names

---

## 🚀 Quick Start Guide

### Current Status

Run the browser:
```bash
cd /home/cobrien/Projects/braya-browser-cpp
./build/braya-browser
```

What works NOW:
1. ✅ Right-click shows menu
2. ✅ Edit dialog appears
3. ✅ Delete confirmation appears
4. ✅ Copy URL works
5. ⚠️ Edit/Delete close dialogs but don't save changes yet

What's needed:
- Wire up edit/delete to actually call bookmarksManager methods
- Add `refreshBookmarksBar()` helper
- Implement folder dropdown support

---

## 📊 Progress

- [x] Edit dialog UI
- [x] Delete confirmation UI
- [x] Copy URL functionality
- [x] Backend edit/delete methods
- [ ] Wire edit to backend
- [ ] Wire delete to backend
- [ ] Add refresh helper
- [ ] Implement folder buttons
- [ ] Implement folder dropdowns
- [ ] Test all features

**Estimated Completion**: 1-2 hours of focused work

---

**Next Steps**: Wire up the dialogs to actually modify bookmarks and refresh the bar!
