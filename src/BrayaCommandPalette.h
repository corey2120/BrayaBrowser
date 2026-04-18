#ifndef BRAYA_COMMAND_PALETTE_H
#define BRAYA_COMMAND_PALETTE_H

#include <gtk/gtk.h>
#include <string>
#include <vector>
#include <functional>

class BrayaHistory;
class BrayaBookmarks;

struct PaletteResult {
    enum Type { COMMAND, BOOKMARK, HISTORY, NAVIGATE };
    Type type;
    std::string icon;
    std::string title;
    std::string subtitle;
    std::function<void()> action;
};

class BrayaCommandPalette {
public:
    BrayaCommandPalette();
    ~BrayaCommandPalette();

    void show(GtkWindow* parent);
    void hide();
    bool isVisible() const;

    void setHistory(BrayaHistory* h)           { m_history = h; }
    void setBookmarks(BrayaBookmarks* b)       { m_bookmarks = b; }
    void setNavigateCallback(std::function<void(const std::string&)> cb) { m_navigateCb = cb; }
    void addCommand(const std::string& icon, const std::string& title,
                    const std::string& subtitle, std::function<void()> action);

private:
    GtkWidget* m_window   = nullptr;
    GtkWidget* m_entry    = nullptr;
    GtkWidget* m_listBox  = nullptr;
    GtkWindow* m_parent   = nullptr;

    BrayaHistory*   m_history   = nullptr;
    BrayaBookmarks* m_bookmarks = nullptr;
    std::function<void(const std::string&)> m_navigateCb;
    std::vector<PaletteResult> m_commands;

    void buildUI(GtkWindow* parent);
    void updateResults(const std::string& query);
    void appendRow(const PaletteResult& result);
    void activateSelected();
    void moveSelection(int delta);
    int  getSelectedIndex() const;

    static void onSearchChanged(GtkSearchEntry* entry, gpointer data);
    static gboolean onKeyPress(GtkEventControllerKey* ctrl, guint keyval,
                               guint keycode, GdkModifierType state, gpointer data);
    static void onRowActivated(GtkListBox* box, GtkListBoxRow* row, gpointer data);
    static void onClickOutside(GtkGestureClick* gesture, int n, double x, double y, gpointer data);
};

#endif // BRAYA_COMMAND_PALETTE_H
