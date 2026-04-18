#include "BrayaCommandPalette.h"
#include "BrayaHistory.h"
#include "BrayaBookmarks.h"
#include <algorithm>
#include <cctype>

BrayaCommandPalette::BrayaCommandPalette() {}

BrayaCommandPalette::~BrayaCommandPalette() {
    if (m_window) {
        gtk_window_destroy(GTK_WINDOW(m_window));
        m_window = nullptr;
    }
}

void BrayaCommandPalette::addCommand(const std::string& icon, const std::string& title,
                                      const std::string& subtitle, std::function<void()> action) {
    PaletteResult r;
    r.type     = PaletteResult::COMMAND;
    r.icon     = icon;
    r.title    = title;
    r.subtitle = subtitle;
    r.action   = std::move(action);
    m_commands.push_back(std::move(r));
}

bool BrayaCommandPalette::isVisible() const {
    return m_window && gtk_widget_get_visible(m_window);
}

void BrayaCommandPalette::show(GtkWindow* parent) {
    m_parent = parent;
    if (!m_window) {
        buildUI(parent);
    }

    gtk_window_set_default_size(GTK_WINDOW(m_window), 620, -1);

    gtk_editable_set_text(GTK_EDITABLE(m_entry), "");
    gtk_widget_set_visible(m_window, TRUE);
    gtk_window_present(GTK_WINDOW(m_window));
    gtk_widget_grab_focus(m_entry);
    updateResults("");
}

void BrayaCommandPalette::hide() {
    if (m_window) gtk_widget_set_visible(m_window, FALSE);
}

void BrayaCommandPalette::buildUI(GtkWindow* parent) {
    m_window = gtk_window_new();
    gtk_window_set_decorated(GTK_WINDOW(m_window), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(m_window), FALSE);
    gtk_window_set_transient_for(GTK_WINDOW(m_window), parent);
    gtk_window_set_modal(GTK_WINDOW(m_window), FALSE);
    gtk_widget_add_css_class(m_window, "command-palette");

    // Outer frame for border + shadow
    GtkWidget* frame = gtk_frame_new(nullptr);
    gtk_widget_add_css_class(frame, "palette-frame");
    gtk_window_set_child(GTK_WINDOW(m_window), frame);

    GtkWidget* outerBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_frame_set_child(GTK_FRAME(frame), outerBox);

    // Search entry row
    m_entry = gtk_search_entry_new();
    gtk_widget_set_margin_start(m_entry, 12);
    gtk_widget_set_margin_end(m_entry, 12);
    gtk_widget_set_margin_top(m_entry, 10);
    gtk_widget_set_margin_bottom(m_entry, 8);
    gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(m_entry),
        "Search bookmarks, history, or type a command...");
    gtk_widget_add_css_class(m_entry, "palette-search");
    gtk_box_append(GTK_BOX(outerBox), m_entry);

    // Separator
    GtkWidget* sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_append(GTK_BOX(outerBox), sep);

    // Results list
    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled, -1, 360);
    gtk_box_append(GTK_BOX(outerBox), scrolled);

    m_listBox = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(m_listBox), GTK_SELECTION_SINGLE);
    gtk_widget_add_css_class(m_listBox, "palette-list");
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), m_listBox);

    // CSS
    GtkCssProvider* css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css, R"CSS(
.command-palette {
    background: transparent;
}
.palette-frame {
    background: #13191f;
    border: 1px solid rgba(255,255,255,0.12);
    border-radius: 10px;
    box-shadow: 0 24px 64px rgba(0,0,0,0.7);
    padding: 0;
}
.palette-search {
    font-size: 15px;
    background: transparent;
    border: none;
    box-shadow: none;
    outline: none;
    color: #e0e6ed;
    padding: 6px 4px;
    min-height: 0;
}
.palette-search:focus {
    box-shadow: none;
    border: none;
    outline: none;
}
.palette-list {
    background: transparent;
}
.palette-list row {
    padding: 8px 14px;
    border-radius: 6px;
    margin: 2px 6px;
    background: transparent;
}
.palette-list row:hover,
.palette-list row:selected {
    background: rgba(0,217,255,0.1);
}
.palette-list row:selected {
    background: rgba(0,217,255,0.15);
}
.palette-icon {
    font-size: 16px;
    min-width: 28px;
}
.palette-title {
    font-size: 13px;
    color: #e0e6ed;
    font-weight: 500;
}
.palette-subtitle {
    font-size: 11px;
    color: #7a8494;
}
.palette-section {
    font-size: 10px;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: .06em;
    color: #4a5568;
    padding: 8px 20px 4px;
}
)CSS");
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    // Signals
    g_signal_connect(m_entry, "search-changed", G_CALLBACK(onSearchChanged), this);
    g_signal_connect(m_listBox, "row-activated", G_CALLBACK(onRowActivated), this);

    GtkEventController* keyCtrl = gtk_event_controller_key_new();
    g_signal_connect(keyCtrl, "key-pressed", G_CALLBACK(onKeyPress), this);
    gtk_widget_add_controller(m_window, keyCtrl);

    // Close on click outside
    GtkGesture* clickOutside = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(clickOutside), 0);
    g_signal_connect(clickOutside, "pressed", G_CALLBACK(onClickOutside), this);
    gtk_widget_add_controller(GTK_WIDGET(parent), GTK_EVENT_CONTROLLER(clickOutside));
}

// ── Result building ──────────────────────────────────────────────────────────

static std::string toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

static bool matchesQuery(const std::string& needle, const std::string& title, const std::string& sub) {
    return toLower(title).find(needle) != std::string::npos ||
           toLower(sub).find(needle) != std::string::npos;
}

void BrayaCommandPalette::updateResults(const std::string& rawQuery) {
    // Clear existing rows
    GtkWidget* child = gtk_widget_get_first_child(m_listBox);
    while (child) {
        GtkWidget* next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(GTK_LIST_BOX(m_listBox), child);
        child = next;
    }

    std::string query = toLower(rawQuery);
    bool empty = query.empty();

    // ── Commands ────────────────────────────────────────────────────────────
    std::vector<PaletteResult> matched;
    for (const auto& cmd : m_commands) {
        if (empty || matchesQuery(query, cmd.title, cmd.subtitle))
            matched.push_back(cmd);
    }
    if (!matched.empty()) {
        GtkWidget* sectionLabel = gtk_label_new(empty ? "Commands" : "Commands");
        gtk_widget_add_css_class(sectionLabel, "palette-section");
        gtk_widget_set_halign(sectionLabel, GTK_ALIGN_START);
        gtk_list_box_append(GTK_LIST_BOX(m_listBox), sectionLabel);
        for (const auto& r : matched) appendRow(r);
    }

    if (empty) {
        // Select first item so keyboard works immediately
        GtkListBoxRow* first = gtk_list_box_get_row_at_index(GTK_LIST_BOX(m_listBox), 1);
        if (first) gtk_list_box_select_row(GTK_LIST_BOX(m_listBox), first);
        return;
    }

    // ── Bookmarks ────────────────────────────────────────────────────────────
    if (m_bookmarks) {
        std::vector<PaletteResult> bResults;
        auto bookmarks = m_bookmarks->getBookmarks();
        for (const auto& bm : bookmarks) {
            if (bm.isFolder) continue;
            if (matchesQuery(query, bm.name, bm.url)) {
                PaletteResult r;
                r.type     = PaletteResult::BOOKMARK;
                r.icon     = "🔖";
                r.title    = bm.name;
                r.subtitle = bm.url;
                std::string url = bm.url;
                r.action = [this, url]() { if (m_navigateCb) m_navigateCb(url); };
                bResults.push_back(std::move(r));
                if (bResults.size() >= 5) break;
            }
        }
        if (!bResults.empty()) {
            GtkWidget* lbl = gtk_label_new("Bookmarks");
            gtk_widget_add_css_class(lbl, "palette-section");
            gtk_widget_set_halign(lbl, GTK_ALIGN_START);
            gtk_list_box_append(GTK_LIST_BOX(m_listBox), lbl);
            for (const auto& r : bResults) appendRow(r);
        }
    }

    // ── History ───────────────────────────────────────────────────────────────
    if (m_history) {
        auto entries = m_history->getHistory(300);
        std::vector<PaletteResult> hResults;
        for (const auto& e : entries) {
            if (matchesQuery(query, e.title, e.url)) {
                PaletteResult r;
                r.type     = PaletteResult::HISTORY;
                r.icon     = "🕒";
                r.title    = e.title.empty() ? e.url : e.title;
                r.subtitle = e.url;
                std::string url = e.url;
                r.action = [this, url]() { if (m_navigateCb) m_navigateCb(url); };
                hResults.push_back(std::move(r));
                if (hResults.size() >= 5) break;
            }
        }
        if (!hResults.empty()) {
            GtkWidget* lbl = gtk_label_new("History");
            gtk_widget_add_css_class(lbl, "palette-section");
            gtk_widget_set_halign(lbl, GTK_ALIGN_START);
            gtk_list_box_append(GTK_LIST_BOX(m_listBox), lbl);
            for (const auto& r : hResults) appendRow(r);
        }
    }

    // ── Navigate / Search fallback ────────────────────────────────────────────
    {
        PaletteResult r;
        r.type = PaletteResult::NAVIGATE;
        bool looksLikeUrl = rawQuery.find("://") != std::string::npos ||
            (rawQuery.find('.') != std::string::npos && rawQuery.find(' ') == std::string::npos);
        if (looksLikeUrl) {
            r.icon     = "🌐";
            r.title    = "Go to " + rawQuery;
            r.subtitle = rawQuery;
            std::string url = rawQuery;
            r.action = [this, url]() { if (m_navigateCb) m_navigateCb(url); };
        } else {
            r.icon     = "🔍";
            r.title    = "Search for \"" + rawQuery + "\"";
            r.subtitle = "DuckDuckGo";
            std::string q = rawQuery;
            r.action = [this, q]() { if (m_navigateCb) m_navigateCb(q); };
        }
        GtkWidget* lbl = gtk_label_new("Search");
        gtk_widget_add_css_class(lbl, "palette-section");
        gtk_widget_set_halign(lbl, GTK_ALIGN_START);
        gtk_list_box_append(GTK_LIST_BOX(m_listBox), lbl);
        appendRow(r);
    }

    // Select first actionable row
    for (int i = 0; ; i++) {
        GtkListBoxRow* row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(m_listBox), i);
        if (!row) break;
        if (GTK_IS_LIST_BOX_ROW(row) && gtk_list_box_row_is_selected(GTK_LIST_BOX_ROW(row)) == FALSE) {
            GtkWidget* child2 = gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(row));
            if (child2 && !GTK_IS_LABEL(child2)) {
                gtk_list_box_select_row(GTK_LIST_BOX(m_listBox), row);
                break;
            }
        }
    }
}

void BrayaCommandPalette::appendRow(const PaletteResult& result) {
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_valign(row, GTK_ALIGN_CENTER);

    GtkWidget* iconLabel = gtk_label_new(result.icon.c_str());
    gtk_widget_add_css_class(iconLabel, "palette-icon");
    gtk_widget_set_valign(iconLabel, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(row), iconLabel);

    GtkWidget* textBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
    gtk_widget_set_hexpand(textBox, TRUE);
    gtk_widget_set_valign(textBox, GTK_ALIGN_CENTER);

    GtkWidget* title = gtk_label_new(result.title.c_str());
    gtk_widget_add_css_class(title, "palette-title");
    gtk_label_set_xalign(GTK_LABEL(title), 0);
    gtk_label_set_ellipsize(GTK_LABEL(title), PANGO_ELLIPSIZE_END);
    gtk_box_append(GTK_BOX(textBox), title);

    if (!result.subtitle.empty()) {
        GtkWidget* sub = gtk_label_new(result.subtitle.c_str());
        gtk_widget_add_css_class(sub, "palette-subtitle");
        gtk_label_set_xalign(GTK_LABEL(sub), 0);
        gtk_label_set_ellipsize(GTK_LABEL(sub), PANGO_ELLIPSIZE_END);
        gtk_box_append(GTK_BOX(textBox), sub);
    }

    gtk_box_append(GTK_BOX(row), textBox);

    // Store action on the row widget
    auto* actionPtr = new std::function<void()>(result.action);
    g_object_set_data_full(G_OBJECT(row), "palette-action", actionPtr,
                           [](gpointer p) { delete static_cast<std::function<void()>*>(p); });

    gtk_list_box_append(GTK_LIST_BOX(m_listBox), row);
}

// ── Navigation helpers ───────────────────────────────────────────────────────

int BrayaCommandPalette::getSelectedIndex() const {
    GtkListBoxRow* sel = gtk_list_box_get_selected_row(GTK_LIST_BOX(m_listBox));
    return sel ? gtk_list_box_row_get_index(sel) : -1;
}

void BrayaCommandPalette::moveSelection(int delta) {
    int cur = getSelectedIndex();
    int total = 0;
    while (gtk_list_box_get_row_at_index(GTK_LIST_BOX(m_listBox), total)) total++;

    // Walk in direction, skipping section-label rows
    int next = cur;
    for (int i = 0; i < total; i++) {
        next += delta;
        if (next < 0) next = total - 1;
        if (next >= total) next = 0;
        GtkListBoxRow* row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(m_listBox), next);
        if (!row) break;
        GtkWidget* child = gtk_list_box_row_get_child(row);
        // Section labels are plain GtkLabel children; skip them
        if (child && GTK_IS_LABEL(child)) continue;
        gtk_list_box_select_row(GTK_LIST_BOX(m_listBox), row);
        // Scroll into view
        GtkWidget* scrolled = gtk_widget_get_parent(m_listBox);
        if (scrolled && GTK_IS_SCROLLED_WINDOW(scrolled)) {
            GtkAdjustment* adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled));
            double rowH = 52.0;
            gtk_adjustment_set_value(adj, next * rowH);
        }
        break;
    }
}

void BrayaCommandPalette::activateSelected() {
    GtkListBoxRow* sel = gtk_list_box_get_selected_row(GTK_LIST_BOX(m_listBox));
    if (!sel) return;
    GtkWidget* child = gtk_list_box_row_get_child(sel);
    if (!child) return;
    auto* action = static_cast<std::function<void()>*>(
        g_object_get_data(G_OBJECT(child), "palette-action"));
    hide();
    if (action && *action) (*action)();
}

// ── Signal handlers ──────────────────────────────────────────────────────────

void BrayaCommandPalette::onSearchChanged(GtkSearchEntry* entry, gpointer data) {
    auto* self = static_cast<BrayaCommandPalette*>(data);
    const char* text = gtk_editable_get_text(GTK_EDITABLE(entry));
    self->updateResults(text ? text : "");
}

gboolean BrayaCommandPalette::onKeyPress(GtkEventControllerKey* ctrl, guint keyval,
                                          guint keycode, GdkModifierType state, gpointer data) {
    auto* self = static_cast<BrayaCommandPalette*>(data);
    if (keyval == GDK_KEY_Escape) {
        self->hide();
        return TRUE;
    }
    if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
        self->activateSelected();
        return TRUE;
    }
    if (keyval == GDK_KEY_Down || keyval == GDK_KEY_Tab) {
        self->moveSelection(+1);
        return TRUE;
    }
    if (keyval == GDK_KEY_Up) {
        self->moveSelection(-1);
        return TRUE;
    }
    return FALSE;
}

void BrayaCommandPalette::onRowActivated(GtkListBox* box, GtkListBoxRow* row, gpointer data) {
    auto* self = static_cast<BrayaCommandPalette*>(data);
    if (!row) return;
    GtkWidget* child = gtk_list_box_row_get_child(row);
    if (!child) return;
    auto* action = static_cast<std::function<void()>*>(
        g_object_get_data(G_OBJECT(child), "palette-action"));
    self->hide();
    if (action && *action) (*action)();
}

void BrayaCommandPalette::onClickOutside(GtkGestureClick* gesture, int n,
                                          double x, double y, gpointer data) {
    static_cast<BrayaCommandPalette*>(data)->hide();
}
