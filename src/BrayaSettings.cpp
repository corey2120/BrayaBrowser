#include "BrayaSettings.h"
#include "BrayaCustomization.h"
#include "BrayaPasswordManager.h"
#include "extensions/BrayaExtensionManager.h"
#include "extensions/BrayaWebExtension.h"
#include "extensions/ExtensionInstaller.h"
#include "adblocker/BrayaAdBlocker.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <memory>
#include <json-glib/json-glib.h>

namespace {
struct WebsiteDataFetchContext {
    BrayaSettings* settings;
    GtkWidget* label;
    std::string prefix;
    WebKitWebsiteDataTypes types;
};

struct WebsiteDataClearContext {
    BrayaSettings* settings;
    GtkWidget* label;
    std::string successMessage;
    WebKitWebsiteDataTypes types;
};
} // namespace

BrayaSettings::BrayaSettings()
    : theme(DARK), fontSize(13), fontFamily("Sans"), showBookmarks(true),
      blockTrackers(true), blockAds(false), httpsOnly(false),
      enableJavaScript(true), enableWebGL(true), enablePlugins(false),
      showTabPreviews(true), memoryIndicatorEnabled(true),
      downloadPath(""), homePage("about:braya"), searchEngine("DuckDuckGo"),
      dialog(nullptr), notebook(nullptr), searchEntry(nullptr), navList(nullptr),
      webContext(nullptr),
      m_extensionManager(nullptr), m_adBlocker(nullptr), m_passwordManager(nullptr),
      extensionsList(nullptr), cookieStatusLabel(nullptr), siteDataStatusLabel(nullptr) {
    
    downloadPath = std::string(g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD));
    loadSettings();
}

void BrayaSettings::populatePermissionsList() {
    if (!permissionsList) return;

    GtkListBox* list = GTK_LIST_BOX(permissionsList);
    GtkWidget* child = gtk_widget_get_first_child(GTK_WIDGET(list));
    while (child) {
        GtkWidget* next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(list, child);
        child = next;
    }

    WebKitNetworkSession* session = webkit_network_session_get_default();
    WebKitWebsiteDataManager* manager = session ? webkit_network_session_get_website_data_manager(session) : nullptr;
    if (!manager) {
        GtkWidget* row = gtk_label_new("Website data manager unavailable.");
        gtk_list_box_append(list, row);
        return;
    }

    struct PermissionsContext {
        GtkListBox* list;
        WebKitWebsiteDataManager* manager;
    };
    auto* ctx = new PermissionsContext{list, manager};

    webkit_website_data_manager_fetch(manager,
        WEBKIT_WEBSITE_DATA_COOKIES,
        nullptr,
        +[](GObject* source_object, GAsyncResult* res, gpointer user_data) {
            std::unique_ptr<PermissionsContext> ctx(static_cast<PermissionsContext*>(user_data));
            GtkListBox* list = ctx->list;
            WebKitWebsiteDataManager* manager = ctx->manager;

            GError* error = nullptr;
            GList* data = webkit_website_data_manager_fetch_finish(WEBKIT_WEBSITE_DATA_MANAGER(source_object), res, &error);
            if (error) {
                GtkWidget* row = gtk_label_new("Failed to load site data.");
                gtk_list_box_append(list, row);
                g_error_free(error);
                return;
            }

            if (!data) {
                GtkWidget* row = gtk_label_new("No site data stored.");
                gtk_list_box_append(list, row);
                return;
            }

            for (GList* l = data; l; l = l->next) {
                WebKitWebsiteData* d = static_cast<WebKitWebsiteData*>(l->data);
                const gchar* name = webkit_website_data_get_name(d);
                GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
                gtk_widget_set_margin_top(row, 4);
                gtk_widget_set_margin_bottom(row, 4);
                gtk_widget_set_halign(row, GTK_ALIGN_FILL);

                GtkWidget* label = gtk_label_new(name ? name : "(unknown site)");
                gtk_widget_set_hexpand(label, TRUE);
                gtk_widget_set_halign(label, GTK_ALIGN_START);
                gtk_box_append(GTK_BOX(row), label);

                GtkWidget* clearBtn = gtk_button_new_with_label("Clear");
                g_object_set_data_full(G_OBJECT(clearBtn), "data", webkit_website_data_ref(d), (GDestroyNotify)webkit_website_data_unref);
                g_object_set_data(G_OBJECT(clearBtn), "list", list);
                g_object_set_data(G_OBJECT(clearBtn), "manager", manager);
                g_signal_connect(clearBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer) {
                    auto* wd = static_cast<WebKitWebsiteData*>(g_object_get_data(G_OBJECT(btn), "data"));
                    auto* list = GTK_LIST_BOX(g_object_get_data(G_OBJECT(btn), "list"));
                    auto* mgr = static_cast<WebKitWebsiteDataManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
                    if (!wd || !list || !mgr) return;
                    GList* toRemoveSingle = nullptr;
                    toRemoveSingle = g_list_prepend(toRemoveSingle, wd);
                    webkit_website_data_manager_remove(
                        mgr,
                        WEBKIT_WEBSITE_DATA_COOKIES,
                        toRemoveSingle,
                        nullptr,
                        nullptr,
                        nullptr);
                    g_list_free(toRemoveSingle);
                    gtk_list_box_remove(list, gtk_widget_get_parent(GTK_WIDGET(btn)));
                }), nullptr);
                gtk_box_append(GTK_BOX(row), clearBtn);

                gtk_list_box_append(list, row);
            }

            g_list_free_full(data, (GDestroyNotify)webkit_website_data_unref);
        },
        ctx);
}
void BrayaSettings::setWebContext(WebKitWebContext* context) {
    webContext = context;
    refreshWebsiteDataStatus(WEBKIT_WEBSITE_DATA_COOKIES, cookieStatusLabel, "Stored cookies");
    const WebKitWebsiteDataTypes siteTypes = static_cast<WebKitWebsiteDataTypes>(WEBKIT_WEBSITE_DATA_ALL & ~WEBKIT_WEBSITE_DATA_COOKIES);
    refreshWebsiteDataStatus(siteTypes, siteDataStatusLabel, "Stored site data entries");
}

void BrayaSettings::show(GtkWindow* parent) {
    if (dialog && GTK_IS_WINDOW(dialog)) {
        gtk_window_present(GTK_WINDOW(dialog));
        return;
    }

    createDialog(parent);
    updateUIFromSettings();

    // Connect close signal to reset dialog pointer
    g_signal_connect(dialog, "close-request", G_CALLBACK(+[](GtkWindow* window, gpointer data) -> gboolean {
        BrayaSettings* settings = static_cast<BrayaSettings*>(data);
        settings->dialog = nullptr;
        settings->notebook = nullptr;
        settings->searchEntry = nullptr;
        settings->navList = nullptr;
        settings->navRowMap.clear();
        return FALSE; // Allow the window to close
    }), this);

    gtk_window_present(GTK_WINDOW(dialog));
}

void BrayaSettings::showTab(GtkWindow* parent, const std::string& tabName) {
    if (dialog && GTK_IS_WINDOW(dialog)) {
        // Dialog already exists, just switch tab and present
        if (notebook) {
            gtk_stack_set_visible_child_name(GTK_STACK(notebook), tabName.c_str());
        }
        gtk_window_present(GTK_WINDOW(dialog));
        return;
    }

    // Create dialog first
    createDialog(parent);
    updateUIFromSettings();

    // Switch to requested tab
    if (notebook) {
        gtk_stack_set_visible_child_name(GTK_STACK(notebook), tabName.c_str());
    }

    // Connect close signal to reset dialog pointer
    g_signal_connect(dialog, "close-request", G_CALLBACK(+[](GtkWindow* window, gpointer data) -> gboolean {
        BrayaSettings* settings = static_cast<BrayaSettings*>(data);
        settings->dialog = nullptr;
        settings->notebook = nullptr;
        settings->searchEntry = nullptr;
        settings->navList = nullptr;
        settings->navRowMap.clear();
        return FALSE; // Allow the window to close
    }), this);

    gtk_window_present(GTK_WINDOW(dialog));
}

void BrayaSettings::createDialog(GtkWindow* parent) {
    dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Braya Settings");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 900, 650);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    
    GtkWidget* mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(dialog), mainBox);
    
    // Header with search
    GtkWidget* headerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(headerBox, 20);
    gtk_widget_set_margin_end(headerBox, 20);
    gtk_widget_set_margin_top(headerBox, 20);
    gtk_widget_set_margin_bottom(headerBox, 15);
    gtk_box_append(GTK_BOX(mainBox), headerBox);
    
    GtkWidget* titleLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(titleLabel), "<span size='x-large' weight='bold'>⚙️ Settings</span>");
    gtk_box_append(GTK_BOX(headerBox), titleLabel);
    
    // Search entry
    searchEntry = gtk_search_entry_new();
    gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(searchEntry), "Search settings...");
    gtk_widget_set_size_request(searchEntry, 250, -1);
    gtk_box_append(GTK_BOX(headerBox), searchEntry);

    g_signal_connect(searchEntry, "search-changed", G_CALLBACK(+[](GtkSearchEntry* entry, gpointer data) {
        auto* settings = static_cast<BrayaSettings*>(data);
        if (!settings) return;
        const char* text = gtk_editable_get_text(GTK_EDITABLE(entry));
        settings->handleSearchQuery(text ? text : "");
    }), this);

    g_signal_connect(searchEntry, "stop-search", G_CALLBACK(+[](GtkSearchEntry* entry, gpointer data) {
        auto* settings = static_cast<BrayaSettings*>(data);
        gtk_editable_set_text(GTK_EDITABLE(entry), "");
        if (settings) {
            settings->navigateToSection("general");
        }
    }), this);

    GtkWidget* hero = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_start(hero, 20);
    gtk_widget_set_margin_end(hero, 20);
    gtk_widget_set_margin_bottom(hero, 10);
    gtk_box_append(GTK_BOX(mainBox), hero);

    GtkWidget* heroTitle = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(heroTitle), "<span size='large' weight='bold'>Design Braya your way</span>");
    gtk_widget_set_halign(heroTitle, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(hero), heroTitle);

    GtkWidget* heroSubtitle = gtk_label_new("Switch themes, tighten privacy, and fine‑tune every aspect of your browser experience.");
    gtk_widget_add_css_class(heroSubtitle, "dim-label");
    gtk_widget_set_halign(heroSubtitle, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(hero), heroSubtitle);

    GtkWidget* heroActions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(hero), heroActions);

    auto addHeroShortcut = [&](const char* label, const char* targetSection) {
        GtkWidget* button = gtk_button_new_with_label(label);
        gtk_widget_add_css_class(button, "pill");
        g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
            auto* settings = static_cast<BrayaSettings*>(data);
            const char* sectionId = static_cast<const char*>(g_object_get_data(G_OBJECT(btn), "section-id"));
            if (settings && sectionId) {
                settings->navigateToSection(sectionId);
            }
        }), this);
        g_object_set_data_full(G_OBJECT(button), "section-id", g_strdup(targetSection), g_free);
        gtk_box_append(GTK_BOX(heroActions), button);
    };

    addHeroShortcut("🎨 Appearance Studio", "appearance");
    addHeroShortcut("🔌 Manage Extensions", "extensions");
    addHeroShortcut("🛡️ Privacy & Security", "privacy");
    
    gtk_box_append(GTK_BOX(mainBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // Content area: Sidebar + Stack
    GtkWidget* contentBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_vexpand(contentBox, TRUE);
    gtk_box_append(GTK_BOX(mainBox), contentBox);
    
    struct NavSection {
        const char* id;
        const char* title;
        const char* icon;
        const char* subtitle;
    };
    const std::vector<NavSection> navSections = {
        {"general", "General", "🏠", "Startup, downloads, home"},
        {"appearance", "Appearance", "🎨", "Themes, fonts, layouts"},
        {"privacy", "Privacy", "🔒", "Tracking protection"},
        {"security", "Security", "🛡️", "HTTPS & warnings"},
        {"passwords", "Passwords", "🔑", "Saved logins & import/export"},
        {"adblocker", "Ad-Blocker", "🚫", "Filters & shields"},
        {"extensions", "Extensions", "🔌", "Manage add-ons"},
        {"advanced", "Advanced", "⚡", "Web features & about"}
    };
    
    // Create stack first
    notebook = gtk_stack_new();
    gtk_widget_set_hexpand(notebook, TRUE);
    gtk_widget_set_vexpand(notebook, TRUE);
    gtk_stack_set_transition_type(GTK_STACK(notebook), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(GTK_STACK(notebook), 200);
    
    // Add all pages to stack
    gtk_stack_add_titled(GTK_STACK(notebook), createGeneralTab(), "general", "🏠 General");
    gtk_stack_add_titled(GTK_STACK(notebook), createAppearanceTab(), "appearance", "🎨 Appearance");
    gtk_stack_add_titled(GTK_STACK(notebook), createPrivacyTab(), "privacy", "🔒 Privacy");
    gtk_stack_add_titled(GTK_STACK(notebook), createSecurityTab(), "security", "🛡️ Security");
    gtk_stack_add_titled(GTK_STACK(notebook), createPasswordsTab(), "passwords", "🔑 Passwords");
    gtk_stack_add_titled(GTK_STACK(notebook), createAdBlockerTab(), "adblocker", "🛡️ Ad-Blocker");
    gtk_stack_add_titled(GTK_STACK(notebook), createExtensionsTab(), "extensions", "🔌 Extensions");
    gtk_stack_add_titled(GTK_STACK(notebook), createAdvancedTab(), "advanced", "⚡ Advanced");
    buildSearchIndex();
    
    // Navigation rail
    GtkWidget* navContainer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_size_request(navContainer, 260, -1);
    gtk_widget_set_margin_start(navContainer, 10);
    gtk_widget_set_margin_end(navContainer, 10);
    gtk_widget_set_margin_top(navContainer, 10);
    gtk_widget_set_margin_bottom(navContainer, 10);
    gtk_widget_add_css_class(navContainer, "settings-nav");
    gtk_box_append(GTK_BOX(contentBox), navContainer);

    GtkWidget* navTitle = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(navTitle), "<span weight='bold' size='large'>Control Center</span>");
    gtk_widget_set_halign(navTitle, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(navContainer), navTitle);

    GtkWidget* navSubtitle = gtk_label_new("Jump between sections instantly.");
    gtk_widget_add_css_class(navSubtitle, "dim-label");
    gtk_widget_set_halign(navSubtitle, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(navContainer), navSubtitle);

    navList = gtk_list_box_new();
    gtk_widget_add_css_class(navList, "navigation-sidebar");
    gtk_widget_set_vexpand(navList, TRUE);
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(navList), GTK_SELECTION_SINGLE);
    gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX(navList), TRUE);
    gtk_box_append(GTK_BOX(navContainer), navList);

    navRowMap.clear();
    for (const auto& section : navSections) {
        GtkWidget* row = gtk_list_box_row_new();
        GtkWidget* rowBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_widget_set_margin_start(rowBox, 12);
        gtk_widget_set_margin_end(rowBox, 12);
        gtk_widget_set_margin_top(rowBox, 10);
        gtk_widget_set_margin_bottom(rowBox, 10);

        GtkWidget* iconLabel = gtk_label_new(section.icon);
        gtk_widget_set_halign(iconLabel, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(iconLabel, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(rowBox), iconLabel);

        GtkWidget* textBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        gtk_widget_set_hexpand(textBox, TRUE);
        GtkWidget* titleLabel = gtk_label_new(section.title);
        gtk_widget_set_halign(titleLabel, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(textBox), titleLabel);

        if (section.subtitle && strlen(section.subtitle) > 0) {
            GtkWidget* subtitleLabel = gtk_label_new(section.subtitle);
            gtk_widget_add_css_class(subtitleLabel, "dim-label");
            gtk_widget_set_halign(subtitleLabel, GTK_ALIGN_START);
            gtk_box_append(GTK_BOX(textBox), subtitleLabel);
        }

        gtk_box_append(GTK_BOX(rowBox), textBox);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), rowBox);
        g_object_set_data_full(G_OBJECT(row), "section-id", g_strdup(section.id), g_free);
        navRowMap[section.id] = GTK_LIST_BOX_ROW(row);
        gtk_list_box_append(GTK_LIST_BOX(navList), row);
    }

    g_signal_connect(navList, "row-selected", G_CALLBACK(+[](GtkListBox* list, GtkListBoxRow* row, gpointer data) {
        if (!row) return;
        auto* settings = static_cast<BrayaSettings*>(data);
        const char* sectionId = static_cast<const char*>(g_object_get_data(G_OBJECT(row), "section-id"));
        if (settings && sectionId) {
            settings->navigateToSection(sectionId);
        }
    }), this);
    selectNavRow("general");

    GtkWidget* navSpacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_vexpand(navSpacer, TRUE);
    gtk_box_append(GTK_BOX(navContainer), navSpacer);

    GtkWidget* themeStudioBtn = gtk_button_new_with_label("🎨 Open Theme Studio");
    gtk_widget_add_css_class(themeStudioBtn, "suggested-action");
    g_signal_connect(themeStudioBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        auto* settings = static_cast<BrayaSettings*>(data);
        if (!settings) return;
        settings->navigateToSection("appearance");
        settings->selectNavRow("appearance");
        static BrayaCustomization* customization = nullptr;
        if (!customization) customization = new BrayaCustomization();
        customization->show(GTK_WINDOW(settings->dialog));
    }), this);
    gtk_box_append(GTK_BOX(navContainer), themeStudioBtn);

    GtkWidget* shortcutsBtn = gtk_button_new_with_label("⌨️ Keyboard Shortcuts");
    gtk_widget_add_css_class(shortcutsBtn, "flat");
    g_signal_connect(shortcutsBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        auto* settings = static_cast<BrayaSettings*>(data);
        if (!settings) return;
        settings->navigateToSection("advanced");
        settings->selectNavRow("advanced");
    }), this);
    gtk_box_append(GTK_BOX(navContainer), shortcutsBtn);

    gtk_box_append(GTK_BOX(contentBox), gtk_separator_new(GTK_ORIENTATION_VERTICAL));
    
    gtk_widget_set_margin_start(notebook, 10);
    gtk_widget_set_margin_end(notebook, 10);
    gtk_widget_set_margin_top(notebook, 10);
    gtk_widget_set_margin_bottom(notebook, 10);
    gtk_box_append(GTK_BOX(contentBox), notebook);
    
    gtk_box_append(GTK_BOX(mainBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // Buttons
    GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_start(buttonBox, 20);
    gtk_widget_set_margin_end(buttonBox, 20);
    gtk_widget_set_margin_top(buttonBox, 15);
    gtk_widget_set_margin_bottom(buttonBox, 15);
    gtk_widget_set_halign(buttonBox, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(mainBox), buttonBox);
    
    GtkWidget* resetBtn = gtk_button_new_with_label("Reset to Defaults");
    gtk_widget_add_css_class(resetBtn, "destructive-action");
    gtk_box_append(GTK_BOX(buttonBox), resetBtn);
    
    GtkWidget* spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(buttonBox), spacer);
    
    GtkWidget* applyBtn = gtk_button_new_with_label("✓ Apply");
    gtk_widget_add_css_class(applyBtn, "suggested-action");
    g_signal_connect(applyBtn, "clicked", G_CALLBACK(onApplyClicked), this);
    gtk_box_append(GTK_BOX(buttonBox), applyBtn);
    
    GtkWidget* closeBtn = gtk_button_new_with_label("Close");
    g_signal_connect(closeBtn, "clicked", G_CALLBACK(onCloseClicked), this);
    gtk_box_append(GTK_BOX(buttonBox), closeBtn);
}

GtkWidget* BrayaSettings::createGeneralTab() {
    GtkWidget* scrolled = gtk_scrolled_window_new();
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(box, 25);
    gtk_widget_set_margin_end(box, 25);
    gtk_widget_set_margin_top(box, 25);
    gtk_widget_set_margin_bottom(box, 25);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), box);
    
    GtkWidget* startupBody = nullptr;
    GtkWidget* startupCard = createSettingsCard("Startup & Home", "Decide what Braya opens to every time", &startupBody);
    gtk_box_append(GTK_BOX(box), startupCard);

    homePageEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(homePageEntry), "about:braya or https://example.com");
    gtk_widget_set_hexpand(homePageEntry, TRUE);
    gtk_box_append(GTK_BOX(startupBody), createSettingsRow("Home Page", "Shown when clicking the Home button", homePageEntry));

    GtkWidget* sessionSwitch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(sessionSwitch), TRUE);
    gtk_box_append(GTK_BOX(startupBody), createSettingsRow("Restore Session", "Reopen tabs from your last browsing session", sessionSwitch));

    GtkWidget* downloadsBody = nullptr;
    GtkWidget* downloadsCard = createSettingsCard("Downloads & toolbar", "Keep frequently used items close", &downloadsBody);
    gtk_box_append(GTK_BOX(box), downloadsCard);

    downloadPathEntry = gtk_entry_new();
    gtk_widget_set_hexpand(downloadPathEntry, TRUE);
    gtk_box_append(GTK_BOX(downloadsBody), createSettingsRow("Download Location", "Files will be saved here by default", downloadPathEntry));

    bookmarksSwitch = gtk_switch_new();
    g_signal_connect(bookmarksSwitch, "state-set", G_CALLBACK(onBookmarksToggled), this);
    gtk_box_append(GTK_BOX(downloadsBody), createSettingsRow("Show Bookmarks Bar", "Always display the bookmarks toolbar under the address bar", bookmarksSwitch));

    GtkWidget* searchBody = nullptr;
    GtkWidget* searchCard = createSettingsCard("Search & suggestions", "Choose how the address bar behaves", &searchBody);
    gtk_box_append(GTK_BOX(box), searchCard);

    searchEngineCombo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(searchEngineCombo), "DuckDuckGo");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(searchEngineCombo), "Google");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(searchEngineCombo), "Bing");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(searchEngineCombo), "Brave Search");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(searchEngineCombo), "Ecosia");
    gtk_widget_set_hexpand(searchEngineCombo, TRUE);
    gtk_box_append(GTK_BOX(searchBody), createSettingsRow("Default Search Engine", "Used in the address bar and search box", searchEngineCombo));

    GtkWidget* suggestionsSwitch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(suggestionsSwitch), TRUE);
    gtk_box_append(GTK_BOX(searchBody), createSettingsRow("Show Suggestions", "Show history and search suggestions as you type", suggestionsSwitch));
    
    return scrolled;
}

GtkWidget* BrayaSettings::createAppearanceTab() {
    GtkWidget* scrolled = gtk_scrolled_window_new();
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(box, 25);
    gtk_widget_set_margin_end(box, 25);
    gtk_widget_set_margin_top(box, 25);
    gtk_widget_set_margin_bottom(box, 25);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), box);
    
    GtkWidget* themeBody = nullptr;
    GtkWidget* themeCard = createSettingsCard("Themes & presets", "Blend in with your desktop or stand out", &themeBody);
    gtk_box_append(GTK_BOX(box), themeCard);
    
    themeCombo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(themeCombo), "🌙 Dark");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(themeCombo), "🏭 Industrial");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(themeCombo), "🎨 Custom");
    gtk_widget_set_hexpand(themeCombo, TRUE);
    g_signal_connect(themeCombo, "changed", G_CALLBACK(onThemeChanged), this);
    gtk_box_append(GTK_BOX(themeBody), createSettingsRow("Browser Theme", "Pick the base chrome style", themeCombo));

    GtkWidget* presetCombo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(presetCombo), "Zen (Cyan)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(presetCombo), "Arc (Purple)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(presetCombo), "Nord (Arctic Blue)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(presetCombo), "Dracula (Purple/Pink)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(presetCombo), "Tokyo Night (Blue)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(presetCombo), "Gruvbox (Warm Yellow)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(presetCombo), "Catppuccin (Pastel Blue)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(presetCombo), "One Dark (VS Code)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(presetCombo), "Solarized (Classic)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(presetCombo), "Monokai (Cyan)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(presetCombo), 0);
    gtk_widget_set_hexpand(presetCombo, TRUE);

    static auto onPresetChanged = [](GtkComboBox* combo, gpointer user_data) {
        static BrayaCustomization* customization = nullptr;
        if (!customization) customization = new BrayaCustomization();

        int active = gtk_combo_box_get_active(combo);
        const char* presetNames[] = {
            "Zen", "Arc", "Nord", "Dracula", "Tokyo Night",
            "Gruvbox", "Catppuccin", "One Dark", "Solarized", "Monokai"
        };
        if (active >= 0 && active < 10) {
            customization->loadPreset(presetNames[active]);
            customization->applyTheme(GTK_WIDGET(gtk_widget_get_root(GTK_WIDGET(combo))));
            customization->save();
            std::cout << "✓ Applied and saved preset: " << presetNames[active] << std::endl;
        }
    };

    g_signal_connect(presetCombo, "changed", G_CALLBACK(+onPresetChanged), this);
    gtk_box_append(GTK_BOX(themeBody), createSettingsRow("Preset Palette", "Apply curated color schemes instantly", presetCombo));

    GtkWidget* advancedBtn = gtk_button_new_with_label("🎨 Open Theme Studio");
    g_signal_connect_swapped(advancedBtn, "clicked", G_CALLBACK(+[](BrayaSettings* settings) {
        static BrayaCustomization* customization = nullptr;
        if (!customization) customization = new BrayaCustomization();
        customization->show(GTK_WINDOW(settings->dialog));
    }), this);
    gtk_box_append(GTK_BOX(themeBody), createSettingsActionRow("Advanced Customization", "Fine-tune every color and radius", advancedBtn));
    
    GtkWidget* typographyBody = nullptr;
    GtkWidget* typographyCard = createSettingsCard("Typography & density", "Tailor readability for tabs and menus", &typographyBody);
    gtk_box_append(GTK_BOX(box), typographyCard);
    
    fontFamilyEntry = gtk_entry_new();
    gtk_widget_set_hexpand(fontFamilyEntry, TRUE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(fontFamilyEntry), "Sans, Inter, etc.");
    gtk_box_append(GTK_BOX(typographyBody), createSettingsRow("Font Family", "Affects tabs, sidebar, and settings", fontFamilyEntry));
    
    fontSpinner = gtk_spin_button_new_with_range(10, 24, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(fontSpinner), fontSize);
    gtk_box_append(GTK_BOX(typographyBody), createSettingsRow("Font Size", "UI font size in points", fontSpinner));
    
    GtkWidget* uiBody = nullptr;
    GtkWidget* uiCard = createSettingsCard("Interface layout", "Quick toggles for layout ergonomics", &uiBody);
    gtk_box_append(GTK_BOX(box), uiCard);

    tabPreviewsSwitch = gtk_switch_new();
    gtk_box_append(GTK_BOX(uiBody), createSettingsRow("Tab Previews on Hover", "Show page preview when hovering over tabs", tabPreviewsSwitch));

    GtkWidget* compactSwitch = gtk_switch_new();
    gtk_box_append(GTK_BOX(uiBody), createSettingsRow("Compact Mode", "Reduce padding for smaller screens", compactSwitch));

    GtkWidget* showTitleSwitch = gtk_switch_new();
    gtk_box_append(GTK_BOX(uiBody), createSettingsRow("Show Window Titlebar", "Use system controls instead of Braya's headerbar", showTitleSwitch));

    return scrolled;
}

GtkWidget* BrayaSettings::createPrivacyTab() {
    GtkWidget* scrolled = gtk_scrolled_window_new();
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(box, 25);
    gtk_widget_set_margin_end(box, 25);
    gtk_widget_set_margin_top(box, 25);
    gtk_widget_set_margin_bottom(box, 25);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), box);
    
    GtkWidget* trackingBody = nullptr;
    GtkWidget* trackingCard = createSettingsCard("Tracking protection", "Choose how aggressively Braya shields you", &trackingBody);
    gtk_box_append(GTK_BOX(box), trackingCard);
    
    blockTrackersSwitch = gtk_switch_new();
    gtk_box_append(GTK_BOX(trackingBody), createSettingsRow("Block Trackers", "Prevent sites from following you around the web", blockTrackersSwitch));
    
    blockAdsSwitch = gtk_switch_new();
    gtk_box_append(GTK_BOX(trackingBody), createSettingsRow("Block Ads", "Use built-in blocking to stop ads and pop-ups", blockAdsSwitch));
    
    GtkWidget* cookiesBody = nullptr;
    GtkWidget* cookiesCard = createSettingsCard("Cookies & site data", "Clear trackers and signed-in sessions", &cookiesBody);
    gtk_box_append(GTK_BOX(box), cookiesCard);

    GtkWidget* clearCookiesBtn = gtk_button_new_with_label("Clear Cookies");
    gtk_box_append(GTK_BOX(cookiesBody), createSettingsActionRow("Cookies", "Remove all stored cookies across profiles", clearCookiesBtn));

    cookieStatusLabel = gtk_label_new("Cookies: calculating…");
    gtk_widget_add_css_class(cookieStatusLabel, "settings-row-subtitle");
    gtk_widget_set_halign(cookieStatusLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(cookiesBody), cookieStatusLabel);

    GtkWidget* clearStorageBtn = gtk_button_new_with_label("Clear Site Data");
    gtk_box_append(GTK_BOX(cookiesBody), createSettingsActionRow("Site storage", "Clears cache, local storage, IndexedDB, etc.", clearStorageBtn));

    siteDataStatusLabel = gtk_label_new("Site data: calculating…");
    gtk_widget_add_css_class(siteDataStatusLabel, "settings-row-subtitle");
    gtk_widget_set_halign(siteDataStatusLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(cookiesBody), siteDataStatusLabel);

    g_signal_connect(clearCookiesBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        auto* settings = static_cast<BrayaSettings*>(data);
        if (!settings) return;
        settings->clearWebsiteData(WEBKIT_WEBSITE_DATA_COOKIES, settings->cookieStatusLabel, "Cookies cleared.");
    }), this);

    g_signal_connect(clearStorageBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        auto* settings = static_cast<BrayaSettings*>(data);
        if (!settings) return;
        const WebKitWebsiteDataTypes siteTypes = static_cast<WebKitWebsiteDataTypes>(WEBKIT_WEBSITE_DATA_ALL & ~WEBKIT_WEBSITE_DATA_COOKIES);
        settings->clearWebsiteData(siteTypes, settings->siteDataStatusLabel, "Site data cleared.");
    }), this);

    refreshWebsiteDataStatus(WEBKIT_WEBSITE_DATA_COOKIES, cookieStatusLabel, "Stored cookies");
    const WebKitWebsiteDataTypes siteTypes = static_cast<WebKitWebsiteDataTypes>(WEBKIT_WEBSITE_DATA_ALL & ~WEBKIT_WEBSITE_DATA_COOKIES);
    refreshWebsiteDataStatus(siteTypes, siteDataStatusLabel, "Stored site data entries");

    GtkWidget* privacyBody = nullptr;
    GtkWidget* privacyCard = createSettingsCard("Privacy signals", "Control what identity hints get sent", &privacyBody);
    gtk_box_append(GTK_BOX(box), privacyCard);
    
    GtkWidget* dntSwitch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(dntSwitch), TRUE);
    gtk_box_append(GTK_BOX(privacyBody), createSettingsRow("Send Do Not Track", "Ask websites politely not to track you", dntSwitch));
    
    GtkWidget* permissionsBtn = gtk_button_new_with_label("Manage Site Permissions");
    g_signal_connect(permissionsBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data){
        auto* settings = static_cast<BrayaSettings*>(data);
        if (!settings || !settings->dialog) return;
        if (settings->permissionsDialog) {
            gtk_window_present(GTK_WINDOW(settings->permissionsDialog));
            return;
        }

        GtkWindow* dialog = GTK_WINDOW(gtk_window_new());
        settings->permissionsDialog = GTK_WIDGET(dialog);
        gtk_window_set_modal(dialog, TRUE);
        gtk_window_set_transient_for(dialog, GTK_WINDOW(settings->dialog));
        gtk_window_set_title(dialog, "Manage Site Permissions");
        gtk_window_set_default_size(dialog, 600, 420);

        GtkWidget* content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_widget_set_margin_start(content, 16);
        gtk_widget_set_margin_end(content, 16);
        gtk_widget_set_margin_top(content, 16);
        gtk_widget_set_margin_bottom(content, 16);
        gtk_window_set_child(dialog, content);

        GtkWidget* desc = gtk_label_new("Review cookies and site permissions per domain. Select a site to clear data or reset permissions.");
        gtk_label_set_wrap(GTK_LABEL(desc), TRUE);
        gtk_widget_set_halign(desc, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(content), desc);

        GtkWidget* scrolled = gtk_scrolled_window_new();
        gtk_widget_set_vexpand(scrolled, TRUE);
        gtk_box_append(GTK_BOX(content), scrolled);

        GtkWidget* list = gtk_list_box_new();
        gtk_list_box_set_selection_mode(GTK_LIST_BOX(list), GTK_SELECTION_NONE);
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), list);
        settings->permissionsList = list;

        GtkWidget* footer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_widget_set_halign(footer, GTK_ALIGN_END);
        gtk_box_append(GTK_BOX(content), footer);

        GtkWidget* closeBtn = gtk_button_new_with_label("Close");
        g_signal_connect_swapped(closeBtn, "clicked", G_CALLBACK(gtk_window_close), dialog);
        gtk_box_append(GTK_BOX(footer), closeBtn);

        g_signal_connect(dialog, "close-request", G_CALLBACK(+[](GtkWindow*, gpointer d) {
            auto* settings = static_cast<BrayaSettings*>(d);
            settings->permissionsDialog = nullptr;
            settings->permissionsList = nullptr;
            return FALSE;
        }), settings);

        settings->populatePermissionsList();
        gtk_window_present(dialog);
    }), this);
    gtk_box_append(GTK_BOX(privacyBody), createSettingsActionRow("Per-site rules", "Camera, microphone, and location controls", permissionsBtn));

    return scrolled;
}

GtkWidget* BrayaSettings::createSecurityTab() {
    GtkWidget* scrolled = gtk_scrolled_window_new();
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(box, 25);
    gtk_widget_set_margin_end(box, 25);
    gtk_widget_set_margin_top(box, 25);
    gtk_widget_set_margin_bottom(box, 25);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), box);
    
    GtkWidget* httpsBody = nullptr;
    GtkWidget* httpsCard = createSettingsCard("HTTPS & certificates", "Lock down every connection", &httpsBody);
    gtk_box_append(GTK_BOX(box), httpsCard);
    
    httpsOnlySwitch = gtk_switch_new();
    gtk_box_append(GTK_BOX(httpsBody), createSettingsRow("HTTPS-Only Mode", "Automatically upgrade to secure connections", httpsOnlySwitch));
    
    GtkWidget* certButton = gtk_button_new_with_label("View Certificates");
    g_signal_connect(certButton, "clicked", G_CALLBACK(+[](GtkButton*, gpointer){
        g_print("Certificate viewer not implemented yet.\n");
    }), nullptr);
    gtk_box_append(GTK_BOX(httpsBody), createSettingsActionRow("Certificates", "Inspect trusted authorities and pinning", certButton));
    
    GtkWidget* warningsBody = nullptr;
    GtkWidget* warningsCard = createSettingsCard("Safe browsing", "Get alerts before visiting risky pages", &warningsBody);
    gtk_box_append(GTK_BOX(box), warningsCard);
    
    GtkWidget* safeSwitch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(safeSwitch), TRUE);
    gtk_box_append(GTK_BOX(warningsBody), createSettingsRow("Warn about dangerous sites", "Show interstitials for malware or phishing", safeSwitch));
    
    return scrolled;
}

GtkWidget* BrayaSettings::createAdvancedTab() {
    GtkWidget* scrolled = gtk_scrolled_window_new();
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(box, 25);
    gtk_widget_set_margin_end(box, 25);
    gtk_widget_set_margin_top(box, 25);
    gtk_widget_set_margin_bottom(box, 25);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), box);
    
    GtkWidget* webBody = nullptr;
    GtkWidget* webCard = createSettingsCard("Web capabilities", "Toggle features developers love (or trackers abuse)", &webBody);
    gtk_box_append(GTK_BOX(box), webCard);
    
    jsSwitch = gtk_switch_new();
    gtk_box_append(GTK_BOX(webBody), createSettingsRow("Enable JavaScript", "Disable to browse distraction-free", jsSwitch));
    
    webglSwitch = gtk_switch_new();
    gtk_box_append(GTK_BOX(webBody), createSettingsRow("Enable WebGL", "Allow advanced 3D and graphics content", webglSwitch));
    
    pluginsSwitch = gtk_switch_new();
    gtk_box_append(GTK_BOX(webBody), createSettingsRow("Enable Plugins", "Legacy NPAPI-style plugin support", pluginsSwitch));
    
    GtkWidget* aboutBody = nullptr;
    GtkWidget* aboutCard = createSettingsCard("About Braya", "Build metadata & debugging shortcuts", &aboutBody);
    gtk_box_append(GTK_BOX(box), aboutCard);
    
    GtkWidget* versionLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(versionLabel),
        "<span>🐕 <b>Braya Browser</b> v1.0.2<br/>Built with WebKit2GTK 6.0 &amp; GTK 4</span>");
    gtk_widget_set_halign(versionLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(aboutBody), versionLabel);
    
    GtkWidget* profileBtn = gtk_button_new_with_label("Open Profile Folder");
    g_signal_connect(profileBtn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer){
        g_print("Profile folder shortcut coming soon.\n");
    }), nullptr);
    gtk_box_append(GTK_BOX(aboutBody), createSettingsActionRow("Profile directory", "Inspect logs, extensions, and saved data", profileBtn));
    
    return scrolled;
}

void BrayaSettings::updateUIFromSettings() {
    if (!dialog) return;
    
    if (homePageEntry) gtk_editable_set_text(GTK_EDITABLE(homePageEntry), homePage.c_str());
    if (downloadPathEntry) gtk_editable_set_text(GTK_EDITABLE(downloadPathEntry), downloadPath.c_str());
    if (fontFamilyEntry) gtk_editable_set_text(GTK_EDITABLE(fontFamilyEntry), fontFamily.c_str());
    if (fontSpinner) gtk_spin_button_set_value(GTK_SPIN_BUTTON(fontSpinner), fontSize);

    // Map Theme enum to dropdown index (Light removed from dropdown)
    // Dropdown: 0=Dark, 1=Industrial, 2=Custom
    if (themeCombo) {
        int dropdownIndex;
        if (theme == DARK || theme == LIGHT) {
            dropdownIndex = 0;  // Both DARK and LIGHT map to Dark (LIGHT removed)
        } else if (theme == INDUSTRIAL) {
            dropdownIndex = 1;
        } else {  // CUSTOM
            dropdownIndex = 2;
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(themeCombo), dropdownIndex);
    }

    if (bookmarksSwitch) gtk_switch_set_active(GTK_SWITCH(bookmarksSwitch), showBookmarks);
    if (blockTrackersSwitch) gtk_switch_set_active(GTK_SWITCH(blockTrackersSwitch), blockTrackers);
    if (blockAdsSwitch) gtk_switch_set_active(GTK_SWITCH(blockAdsSwitch), blockAds);
    if (httpsOnlySwitch) gtk_switch_set_active(GTK_SWITCH(httpsOnlySwitch), httpsOnly);
    if (jsSwitch) gtk_switch_set_active(GTK_SWITCH(jsSwitch), enableJavaScript);
    if (webglSwitch) gtk_switch_set_active(GTK_SWITCH(webglSwitch), enableWebGL);
    if (pluginsSwitch) gtk_switch_set_active(GTK_SWITCH(pluginsSwitch), enablePlugins);
    if (tabPreviewsSwitch) gtk_switch_set_active(GTK_SWITCH(tabPreviewsSwitch), showTabPreviews);
    
    if (searchEngineCombo) {
        if (searchEngine == "DuckDuckGo") gtk_combo_box_set_active(GTK_COMBO_BOX(searchEngineCombo), 0);
        else if (searchEngine == "Google") gtk_combo_box_set_active(GTK_COMBO_BOX(searchEngineCombo), 1);
        else if (searchEngine == "Bing") gtk_combo_box_set_active(GTK_COMBO_BOX(searchEngineCombo), 2);
        else if (searchEngine == "Brave Search") gtk_combo_box_set_active(GTK_COMBO_BOX(searchEngineCombo), 3);
        else gtk_combo_box_set_active(GTK_COMBO_BOX(searchEngineCombo), 4);
    }

    // Update ad-blocker UI from current state
    updateAdBlockerUI();
}

void BrayaSettings::navigateToSection(const std::string& sectionId) {
    if (!notebook || sectionId.empty()) return;
    gtk_stack_set_visible_child_name(GTK_STACK(notebook), sectionId.c_str());
    selectNavRow(sectionId);
}

void BrayaSettings::selectNavRow(const std::string& sectionId) {
    if (!navList || sectionId.empty()) return;
    auto it = navRowMap.find(sectionId);
    if (it == navRowMap.end()) return;

    GtkListBoxRow* targetRow = it->second;
    if (!targetRow) return;

    GtkListBoxRow* current = gtk_list_box_get_selected_row(GTK_LIST_BOX(navList));
    if (current == targetRow) return;

    gtk_list_box_select_row(GTK_LIST_BOX(navList), targetRow);
    gtk_list_box_row_set_activatable(targetRow, TRUE);
}
void BrayaSettings::buildSearchIndex() {
    sectionSearchIndex = {
        {"general", "General", {"home", "homepage", "startup", "downloads", "default", "session", "search"}},
        {"appearance", "Appearance", {"theme", "color", "font", "layout", "accent", "light", "dark"}},
        {"privacy", "Privacy", {"tracking", "cookies", "permissions", "site", "history"}},
        {"security", "Security", {"https", "sandbox", "warnings", "certificates"}},
        {"passwords", "Passwords", {"credentials", "login", "manager", "bitwarden", "import", "export"}},
        {"adblocker", "Ad-Blocker", {"ads", "filters", "blocking", "whitelist", "shield"}},
        {"extensions", "Extensions", {"addons", "extensions", "store", "apis", "permissions"}},
        {"advanced", "Advanced", {"developer", "experiments", "debug", "shortcuts"}}
    };

    // Normalize keywords to lowercase for faster comparisons
    for (auto& section : sectionSearchIndex) {
        for (auto& keyword : section.keywords) {
            std::transform(keyword.begin(), keyword.end(), keyword.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
        }
    }
}

void BrayaSettings::handleSearchQuery(const std::string& query) {
    if (!notebook || query.empty() || sectionSearchIndex.empty()) return;

    std::string trimmed = query;
    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), notSpace));
    trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), notSpace).base(), trimmed.end());

    if (trimmed.empty()) {
        return;
    }

    auto toLower = [](std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return value;
    };

    std::string needle = toLower(trimmed);
    int bestScore = 0;
    std::string bestMatch;

    for (const auto& section : sectionSearchIndex) {
        std::string titleLower = toLower(section.title);
        int score = 0;

        if (titleLower.find(needle) != std::string::npos) {
            score += 5;
        }

        for (const auto& keyword : section.keywords) {
            if (keyword.find(needle) != std::string::npos) {
                score += 2;
            }
        }

        if (score > bestScore) {
            bestScore = score;
            bestMatch = section.id;
        }
    }

    if (bestScore > 0) {
        navigateToSection(bestMatch);
    }
}

GtkWidget* BrayaSettings::createSettingsCard(const std::string& title, const std::string& subtitle, GtkWidget** bodyOut) {
    GtkWidget* frame = gtk_frame_new(nullptr);
    gtk_widget_add_css_class(frame, "settings-card");

    GtkWidget* wrapper = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(wrapper, 16);
    gtk_widget_set_margin_end(wrapper, 16);
    gtk_widget_set_margin_top(wrapper, 16);
    gtk_widget_set_margin_bottom(wrapper, 16);
    gtk_frame_set_child(GTK_FRAME(frame), wrapper);

    GtkWidget* header = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_box_append(GTK_BOX(wrapper), header);

    GtkWidget* titleLabel = gtk_label_new(nullptr);
    gchar* markup = g_markup_printf_escaped("<span weight='bold' size='large'>%s</span>", title.c_str());
    gtk_label_set_markup(GTK_LABEL(titleLabel), markup);
    g_free(markup);
    gtk_widget_set_halign(titleLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(header), titleLabel);

    if (!subtitle.empty()) {
        GtkWidget* subtitleLabel = gtk_label_new(subtitle.c_str());
        gtk_widget_add_css_class(subtitleLabel, "dim-label");
        gtk_widget_set_halign(subtitleLabel, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(header), subtitleLabel);
    }

    GtkWidget* body = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_box_append(GTK_BOX(wrapper), body);

    if (bodyOut) {
        *bodyOut = body;
    }
    return frame;
}

GtkWidget* BrayaSettings::createSettingsRow(const std::string& title, const std::string& subtitle, GtkWidget* control) {
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_widget_add_css_class(row, "settings-row");

    GtkWidget* textBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_hexpand(textBox, TRUE);
    gtk_box_append(GTK_BOX(row), textBox);

    GtkWidget* titleLabel = gtk_label_new(title.c_str());
    gtk_widget_add_css_class(titleLabel, "settings-row-title");
    gtk_widget_set_halign(titleLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(textBox), titleLabel);

    if (!subtitle.empty()) {
        GtkWidget* subtitleLabel = gtk_label_new(subtitle.c_str());
        gtk_widget_add_css_class(subtitleLabel, "settings-row-subtitle");
        gtk_widget_set_halign(subtitleLabel, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(textBox), subtitleLabel);
    }

    if (control) {
        gtk_widget_set_valign(control, GTK_ALIGN_CENTER);
        gtk_box_append(GTK_BOX(row), control);
    }

    return row;
}

GtkWidget* BrayaSettings::createSettingsActionRow(const std::string& title, const std::string& subtitle, GtkWidget* actionButton) {
    gtk_widget_add_css_class(actionButton, "pill");
    return createSettingsRow(title, subtitle, actionButton);
}

void BrayaSettings::refreshWebsiteDataStatus(WebKitWebsiteDataTypes types, GtkWidget* label, const std::string& prefix) {
    if (!label) return;
    gtk_label_set_text(GTK_LABEL(label), "Updating…");
    WebKitNetworkSession* session = webkit_network_session_get_default();
    WebKitWebsiteDataManager* manager = session ? webkit_network_session_get_website_data_manager(session) : nullptr;
    if (!manager) {
        gtk_label_set_text(GTK_LABEL(label), "Website data manager unavailable");
        return;
    }
    auto* ctx = new WebsiteDataFetchContext{this, label, prefix, types};
    webkit_website_data_manager_fetch(manager, types, nullptr, onWebsiteDataFetchFinished, ctx);
}

void BrayaSettings::clearWebsiteData(WebKitWebsiteDataTypes types, GtkWidget* statusLabel, const std::string& successMessage) {
    if (!statusLabel) return;
    gtk_label_set_text(GTK_LABEL(statusLabel), "Clearing…");
    WebKitNetworkSession* session = webkit_network_session_get_default();
    WebKitWebsiteDataManager* manager = session ? webkit_network_session_get_website_data_manager(session) : nullptr;
    if (!manager) {
        gtk_label_set_text(GTK_LABEL(statusLabel), "Website data manager unavailable");
        return;
    }
    auto* ctx = new WebsiteDataClearContext{this, statusLabel, successMessage, types};
    webkit_website_data_manager_clear(manager, types, 0, nullptr, onWebsiteDataClearFinished, ctx);
}

void BrayaSettings::onWebsiteDataFetchFinished(GObject* source, GAsyncResult* result, gpointer user_data) {
    std::unique_ptr<WebsiteDataFetchContext> ctx(static_cast<WebsiteDataFetchContext*>(user_data));
    GError* error = nullptr;
    GList* data = webkit_website_data_manager_fetch_finish(WEBKIT_WEBSITE_DATA_MANAGER(source), result, &error);

    std::string message;
    if (error) {
        message = "Unable to fetch data";
        g_warning("Failed to fetch website data: %s", error->message);
        g_error_free(error);
    } else {
        gsize count = g_list_length(data);
        message = ctx->prefix + ": " + std::to_string(count);
    }

    if (ctx->label) {
        gtk_label_set_text(GTK_LABEL(ctx->label), message.c_str());
    }

    if (data) {
        g_list_free_full(data, reinterpret_cast<GDestroyNotify>(webkit_website_data_unref));
    }
}

void BrayaSettings::onWebsiteDataClearFinished(GObject* source, GAsyncResult* result, gpointer user_data) {
    std::unique_ptr<WebsiteDataClearContext> ctx(static_cast<WebsiteDataClearContext*>(user_data));
    GError* error = nullptr;
    gboolean ok = webkit_website_data_manager_clear_finish(WEBKIT_WEBSITE_DATA_MANAGER(source), result, &error);

    if (!ok || error) {
        if (ctx->label) {
            gtk_label_set_text(GTK_LABEL(ctx->label), "Failed to clear data");
        }
        if (error) {
            g_warning("Failed to clear website data: %s", error->message);
            g_error_free(error);
        } else {
            g_warning("Failed to clear website data for unknown reasons");
        }
        return;
    }

    if (ctx->label) {
        gtk_label_set_text(GTK_LABEL(ctx->label), ctx->successMessage.c_str());
    }

    const bool clearingCookies = ctx->types == WEBKIT_WEBSITE_DATA_COOKIES;
    const WebKitWebsiteDataTypes siteTypes = static_cast<WebKitWebsiteDataTypes>(WEBKIT_WEBSITE_DATA_ALL & ~WEBKIT_WEBSITE_DATA_COOKIES);
    const std::string prefix = clearingCookies ? "Stored cookies" : "Stored site data entries";
    ctx->settings->refreshWebsiteDataStatus(clearingCookies ? WEBKIT_WEBSITE_DATA_COOKIES : siteTypes,
                                            clearingCookies ? ctx->settings->cookieStatusLabel : ctx->settings->siteDataStatusLabel,
                                            prefix);
}

void BrayaSettings::updateAdBlockerUI() {
    if (!m_adBlocker || !dialog) return;

    // Update enabled switch
    if (adBlockerEnabledSwitch) {
        gtk_switch_set_active(GTK_SWITCH(adBlockerEnabledSwitch), m_adBlocker->isEnabled());
    }

    // Update security level combo
    if (securityLevelCombo) {
        SecurityLevel level = m_adBlocker->getSecurityLevel();
        switch (level) {
            case SecurityLevel::OFF:
                gtk_combo_box_set_active_id(GTK_COMBO_BOX(securityLevelCombo), "off");
                break;
            case SecurityLevel::MINIMAL:
                gtk_combo_box_set_active_id(GTK_COMBO_BOX(securityLevelCombo), "minimal");
                break;
            case SecurityLevel::STANDARD:
                gtk_combo_box_set_active_id(GTK_COMBO_BOX(securityLevelCombo), "standard");
                break;
            case SecurityLevel::STRICT:
                gtk_combo_box_set_active_id(GTK_COMBO_BOX(securityLevelCombo), "strict");
                break;
            case SecurityLevel::CUSTOM:
                gtk_combo_box_set_active_id(GTK_COMBO_BOX(securityLevelCombo), "custom");
                break;
        }
    }

    // Update feature checkboxes
    BlockingFeatures features = m_adBlocker->getFeatures();
    if (blockAdsCheck) gtk_check_button_set_active(GTK_CHECK_BUTTON(blockAdsCheck), features.block_ads);
    if (blockTrackersCheck) gtk_check_button_set_active(GTK_CHECK_BUTTON(blockTrackersCheck), features.block_trackers);
    if (blockSocialCheck) gtk_check_button_set_active(GTK_CHECK_BUTTON(blockSocialCheck), features.block_social);
    if (blockCryptominersCheck) gtk_check_button_set_active(GTK_CHECK_BUTTON(blockCryptominersCheck), features.block_cryptominers);
    if (blockPopupsCheck) gtk_check_button_set_active(GTK_CHECK_BUTTON(blockPopupsCheck), features.block_popups);
    if (blockAutoplayCheck) gtk_check_button_set_active(GTK_CHECK_BUTTON(blockAutoplayCheck), features.block_autoplay);
    if (removeCookieWarningsCheck) gtk_check_button_set_active(GTK_CHECK_BUTTON(removeCookieWarningsCheck), features.remove_cookie_warnings);
    if (blockNSFWCheck) gtk_check_button_set_active(GTK_CHECK_BUTTON(blockNSFWCheck), features.block_nsfw);

    // Statistics removed in v1.0.8

    // Update whitelist display
    if (notebook) {
        GtkWidget* adBlockerTab = gtk_stack_get_child_by_name(GTK_STACK(notebook), "adblocker");
        if (adBlockerTab) {
            GtkWidget* scrolled = gtk_widget_get_first_child(adBlockerTab);
            if (scrolled && GTK_IS_SCROLLED_WINDOW(scrolled)) {
                GtkWidget* box = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(scrolled));
                if (box) {
                    GtkWidget* whitelistBox = GTK_WIDGET(g_object_get_data(G_OBJECT(box), "whitelist_box"));
                    if (whitelistBox) {
                        // Clear existing items
                        GtkWidget* child = gtk_widget_get_first_child(whitelistBox);
                        while (child) {
                            GtkWidget* next = gtk_widget_get_next_sibling(child);
                            gtk_box_remove(GTK_BOX(whitelistBox), child);
                            child = next;
                        }

                        // Add whitelisted domains
                        std::vector<std::string> whitelist = m_adBlocker->getWhitelist();
                        for (const auto& domain : whitelist) {
                            GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

                            GtkWidget* label = gtk_label_new(domain.c_str());
                            gtk_label_set_xalign(GTK_LABEL(label), 0);
                            gtk_widget_set_hexpand(label, TRUE);
                            gtk_box_append(GTK_BOX(row), label);

                            GtkWidget* removeBtn = gtk_button_new_with_label("Remove");
                            g_object_set_data_full(G_OBJECT(removeBtn), "domain",
                                                   g_strdup(domain.c_str()), g_free);
                            g_signal_connect(removeBtn, "clicked", G_CALLBACK(onRemoveFromWhitelist), this);
                            gtk_box_append(GTK_BOX(row), removeBtn);

                            gtk_box_append(GTK_BOX(whitelistBox), row);
                        }
                    }

                    // Update filter lists
                    GtkWidget* filterListsBox = GTK_WIDGET(g_object_get_data(G_OBJECT(box), "filter_lists_box"));
                    if (filterListsBox) {
                        // Clear existing items
                        GtkWidget* child = gtk_widget_get_first_child(filterListsBox);
                        while (child) {
                            GtkWidget* next = gtk_widget_get_next_sibling(child);
                            gtk_box_remove(GTK_BOX(filterListsBox), child);
                            child = next;
                        }

                        // Add filter lists
                        std::vector<FilterList> filterLists = m_adBlocker->getFilterLists();
                        for (const auto& list : filterLists) {
                            GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);

                            GtkWidget* nameLabel = gtk_label_new(list.name.c_str());
                            gtk_label_set_xalign(GTK_LABEL(nameLabel), 0);
                            gtk_widget_set_hexpand(nameLabel, TRUE);
                            gtk_box_append(GTK_BOX(row), nameLabel);

                            GtkWidget* toggleSwitch = gtk_switch_new();
                            gtk_switch_set_active(GTK_SWITCH(toggleSwitch), list.enabled);
                            g_object_set_data_full(G_OBJECT(toggleSwitch), "list_name",
                                                   g_strdup(list.name.c_str()), g_free);
                            g_signal_connect(toggleSwitch, "state-set", G_CALLBACK(onFilterListToggled), this);
                            gtk_box_append(GTK_BOX(row), toggleSwitch);

                            gtk_box_append(GTK_BOX(filterListsBox), row);
                        }
                    }

                    // Update custom rules display
                    GtkWidget* customRulesBox = GTK_WIDGET(g_object_get_data(G_OBJECT(box), "custom_rules_box"));
                    if (customRulesBox) {
                        // Clear existing items
                        GtkWidget* child = gtk_widget_get_first_child(customRulesBox);
                        while (child) {
                            GtkWidget* next = gtk_widget_get_next_sibling(child);
                            gtk_box_remove(GTK_BOX(customRulesBox), child);
                            child = next;
                        }

                        // Add custom rules
                        std::vector<std::string> customRules = m_adBlocker->getCustomRules();
                        for (const auto& rule : customRules) {
                            GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

                            GtkWidget* label = gtk_label_new(rule.c_str());
                            gtk_label_set_xalign(GTK_LABEL(label), 0);
                            gtk_widget_set_hexpand(label, TRUE);
                            gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
                            gtk_box_append(GTK_BOX(row), label);

                            GtkWidget* removeBtn = gtk_button_new_with_label("Remove");
                            g_object_set_data_full(G_OBJECT(removeBtn), "rule",
                                                   g_strdup(rule.c_str()), g_free);
                            g_signal_connect(removeBtn, "clicked", G_CALLBACK(onRemoveCustomRule), this);
                            gtk_box_append(GTK_BOX(row), removeBtn);

                            gtk_box_append(GTK_BOX(customRulesBox), row);
                        }
                    }

                    // Update advanced statistics
                    GtkWidget* adsBlockedLabel = GTK_WIDGET(g_object_get_data(G_OBJECT(box), "ads_blocked_label"));
                    GtkWidget* trackersBlockedLabel = GTK_WIDGET(g_object_get_data(G_OBJECT(box), "trackers_blocked_label"));
                    GtkWidget* malwareBlockedLabel = GTK_WIDGET(g_object_get_data(G_OBJECT(box), "malware_blocked_label"));

                    BlockingStats stats = m_adBlocker->getStats();
                    if (adsBlockedLabel) {
                        std::string text = "Ads Blocked: " + std::to_string(stats.ads_blocked);
                        gtk_label_set_text(GTK_LABEL(adsBlockedLabel), text.c_str());
                    }
                    if (trackersBlockedLabel) {
                        std::string text = "Trackers Blocked: " + std::to_string(stats.trackers_blocked);
                        gtk_label_set_text(GTK_LABEL(trackersBlockedLabel), text.c_str());
                    }
                    if (malwareBlockedLabel) {
                        std::string text = "Malware Blocked: " + std::to_string(stats.malware_blocked);
                        gtk_label_set_text(GTK_LABEL(malwareBlockedLabel), text.c_str());
                    }
                }
            }
        }
    }
}

void BrayaSettings::applySettings() {
    if (homePageEntry) homePage = gtk_editable_get_text(GTK_EDITABLE(homePageEntry));
    if (downloadPathEntry) downloadPath = gtk_editable_get_text(GTK_EDITABLE(downloadPathEntry));
    if (fontFamilyEntry) fontFamily = gtk_editable_get_text(GTK_EDITABLE(fontFamilyEntry));
    if (fontSpinner) fontSize = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(fontSpinner));
    if (bookmarksSwitch) showBookmarks = gtk_switch_get_active(GTK_SWITCH(bookmarksSwitch));
    if (blockTrackersSwitch) blockTrackers = gtk_switch_get_active(GTK_SWITCH(blockTrackersSwitch));
    if (blockAdsSwitch) blockAds = gtk_switch_get_active(GTK_SWITCH(blockAdsSwitch));
    if (httpsOnlySwitch) httpsOnly = gtk_switch_get_active(GTK_SWITCH(httpsOnlySwitch));
    if (jsSwitch) enableJavaScript = gtk_switch_get_active(GTK_SWITCH(jsSwitch));
    if (webglSwitch) enableWebGL = gtk_switch_get_active(GTK_SWITCH(webglSwitch));
    if (pluginsSwitch) enablePlugins = gtk_switch_get_active(GTK_SWITCH(pluginsSwitch));
    if (tabPreviewsSwitch) showTabPreviews = gtk_switch_get_active(GTK_SWITCH(tabPreviewsSwitch));
    
    if (searchEngineCombo) {
        int active = gtk_combo_box_get_active(GTK_COMBO_BOX(searchEngineCombo));
        const char* engines[] = {"DuckDuckGo", "Google", "Bing", "Brave Search", "Ecosia"};
        if (active >= 0 && active < 5) searchEngine = engines[active];
    }
    
    saveSettings();
    applyTheme();
    std::cout << "✓ Settings applied!" << std::endl;
}

void BrayaSettings::applyTheme() {
    std::cout << "Applying theme: " << theme << std::endl;
}

void BrayaSettings::saveSettings() {
    std::string configDir = std::string(g_get_user_config_dir()) + "/braya";
    g_mkdir_with_parents(configDir.c_str(), 0755);
    std::string settingsPath = configDir + "/settings.json";
    
    std::ofstream file(settingsPath);
    if (!file.is_open()) return;
    
    file << "{\n";
    file << "  \"theme\": " << theme << ",\n";
    file << "  \"fontSize\": " << fontSize << ",\n";
    file << "  \"fontFamily\": \"" << fontFamily << "\",\n";
    file << "  \"showBookmarks\": " << (showBookmarks ? "true" : "false") << ",\n";
    file << "  \"blockTrackers\": " << (blockTrackers ? "true" : "false") << ",\n";
    file << "  \"blockAds\": " << (blockAds ? "true" : "false") << ",\n";
    file << "  \"httpsOnly\": " << (httpsOnly ? "true" : "false") << ",\n";
    file << "  \"enableJavaScript\": " << (enableJavaScript ? "true" : "false") << ",\n";
    file << "  \"enableWebGL\": " << (enableWebGL ? "true" : "false") << ",\n";
    file << "  \"enablePlugins\": " << (enablePlugins ? "true" : "false") << ",\n";
    file << "  \"showTabPreviews\": " << (showTabPreviews ? "true" : "false") << ",\n";
    file << "  \"downloadPath\": \"" << downloadPath << "\",\n";
    file << "  \"homePage\": \"" << homePage << "\",\n";
    file << "  \"searchEngine\": \"" << searchEngine << "\"\n";
    file << "}\n";
    file.close();
}

void BrayaSettings::loadSettings() {
    std::string configDir = std::string(g_get_user_config_dir()) + "/braya";
    std::string settingsPath = configDir + "/settings.json";
    
    std::ifstream file(settingsPath);
    if (!file.is_open()) return;
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("\"theme\":") != std::string::npos) {
            size_t pos = line.find(":") + 1;
            theme = static_cast<Theme>(std::stoi(line.substr(pos)));
        }
        else if (line.find("\"fontSize\":") != std::string::npos) {
            size_t pos = line.find(":") + 1;
            fontSize = std::stoi(line.substr(pos));
        }
        else if (line.find("\"fontFamily\":") != std::string::npos) {
            size_t start = line.find("\"", line.find(":")) + 1;
            size_t end = line.rfind("\"");
            fontFamily = line.substr(start, end - start);
        }
        else if (line.find("\"homePage\":") != std::string::npos) {
            size_t start = line.find("\"", line.find(":")) + 1;
            size_t end = line.rfind("\"");
            homePage = line.substr(start, end - start);
        }
        else if (line.find("\"searchEngine\":") != std::string::npos) {
            size_t start = line.find("\"", line.find(":")) + 1;
            size_t end = line.rfind("\"");
            searchEngine = line.substr(start, end - start);
        }
        else if (line.find("\"showBookmarks\":") != std::string::npos) {
            showBookmarks = line.find("true") != std::string::npos;
        }
        else if (line.find("\"blockTrackers\":") != std::string::npos) {
            blockTrackers = line.find("true") != std::string::npos;
        }
        else if (line.find("\"blockAds\":") != std::string::npos) {
            blockAds = line.find("true") != std::string::npos;
        }
        else if (line.find("\"httpsOnly\":") != std::string::npos) {
            httpsOnly = line.find("true") != std::string::npos;
        }
        else if (line.find("\"enableJavaScript\":") != std::string::npos) {
            enableJavaScript = line.find("true") != std::string::npos;
        }
        else if (line.find("\"enableWebGL\":") != std::string::npos) {
            enableWebGL = line.find("true") != std::string::npos;
        }
        else if (line.find("\"enablePlugins\":") != std::string::npos) {
            enablePlugins = line.find("true") != std::string::npos;
        }
        else if (line.find("\"showTabPreviews\":") != std::string::npos) {
            showTabPreviews = line.find("true") != std::string::npos;
        }
    }
    file.close();
}

// Setters
void BrayaSettings::setTheme(Theme t) { theme = t; }
void BrayaSettings::setColors(const ColorScheme& c) { colors = c; }
void BrayaSettings::setFontSize(int size) { fontSize = size; }
void BrayaSettings::setFontFamily(const std::string& family) { fontFamily = family; }
void BrayaSettings::setShowBookmarks(bool show) { showBookmarks = show; }
void BrayaSettings::setBlockTrackers(bool block) { blockTrackers = block; }
void BrayaSettings::setBlockAds(bool block) { blockAds = block; }
void BrayaSettings::setHttpsOnly(bool https) { httpsOnly = https; }
void BrayaSettings::setEnableJavaScript(bool enable) { enableJavaScript = enable; }
void BrayaSettings::setEnableWebGL(bool enable) { enableWebGL = enable; }
void BrayaSettings::setEnablePlugins(bool enable) { enablePlugins = enable; }
void BrayaSettings::setShowTabPreviews(bool show) { showTabPreviews = show; }
void BrayaSettings::setDownloadPath(const std::string& path) { downloadPath = path; }
void BrayaSettings::setHomePage(const std::string& page) { homePage = page; }
void BrayaSettings::setSearchEngine(const std::string& engine) { searchEngine = engine; }

// Callbacks
void BrayaSettings::onThemeChanged(GtkComboBox* combo, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    int dropdownIndex = gtk_combo_box_get_active(combo);

    // Map dropdown index to Theme enum (Light removed from dropdown)
    // Dropdown: 0=Dark, 1=Industrial, 2=Custom
    // Enum: DARK=0, LIGHT=1, INDUSTRIAL=2, CUSTOM=3
    Theme themeId;
    if (dropdownIndex == 0) {
        themeId = DARK;
    } else if (dropdownIndex == 1) {
        themeId = INDUSTRIAL;
    } else {
        themeId = CUSTOM;
    }

    settings->setTheme(themeId);

    // Call the callback immediately to apply theme
    if (settings->themeCallback) {
        settings->themeCallback(static_cast<int>(themeId));
    }
}

void BrayaSettings::onFontSizeChanged(GtkSpinButton* button, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    settings->setFontSize(gtk_spin_button_get_value_as_int(button));
}

void BrayaSettings::onBookmarksToggled(GtkSwitch* widget, gboolean state, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    settings->setShowBookmarks(state);
}

void BrayaSettings::onApplyClicked(GtkButton* button, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    settings->applySettings();
}

void BrayaSettings::onCloseClicked(GtkButton* button, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    settings->applySettings();
    gtk_window_destroy(GTK_WINDOW(settings->dialog));
    settings->dialog = nullptr;
}

void BrayaSettings::onColorButtonClicked(GtkButton* button, gpointer data) {
    std::cout << "Color picker coming soon!" << std::endl;
}

// =============================================================================
// EXTENSIONS TAB
// =============================================================================

// Helper struct for extension row data
struct ExtensionRowData {
    BrayaSettings* settings;
    std::string extensionId;
};

GtkWidget* BrayaSettings::createExtensionsTab() {
    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    GtkWidget* mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), mainBox);

    // Header area
    GtkWidget* headerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_start(headerBox, 20);
    gtk_widget_set_margin_end(headerBox, 20);
    gtk_widget_set_margin_top(headerBox, 20);
    gtk_widget_set_margin_bottom(headerBox, 10);
    gtk_box_append(GTK_BOX(mainBox), headerBox);

    GtkWidget* headerLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(headerLabel), "<span size='large' weight='bold'>Browser Extensions</span>");
    gtk_widget_set_halign(headerLabel, GTK_ALIGN_START);
    gtk_widget_set_hexpand(headerLabel, TRUE);
    gtk_box_append(GTK_BOX(headerBox), headerLabel);

    // Install buttons box
    GtkWidget* installBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_append(GTK_BOX(headerBox), installBox);

    // Quick install box - URL entry right in the header
    GtkWidget* quickInstallBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_hexpand(quickInstallBox, TRUE);
    gtk_box_append(GTK_BOX(installBox), quickInstallBox);

    GtkWidget* urlEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(urlEntry), "Paste extension URL from addons.mozilla.org or chrome.google.com/webstore");
    gtk_widget_set_hexpand(urlEntry, TRUE);
    gtk_box_append(GTK_BOX(quickInstallBox), urlEntry);

    GtkWidget* installButton = gtk_button_new_with_label("Install");
    gtk_widget_add_css_class(installButton, "suggested-action");
    gtk_widget_set_tooltip_text(installButton, "Install extension from URL");
    g_object_set_data(G_OBJECT(installButton), "settings", this);
    g_object_set_data(G_OBJECT(installButton), "url-entry", urlEntry);
    g_signal_connect(installButton, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        BrayaSettings* s = static_cast<BrayaSettings*>(g_object_get_data(G_OBJECT(btn), "settings"));
        GtkEntry* entry = GTK_ENTRY(g_object_get_data(G_OBJECT(btn), "url-entry"));

        const char* url = gtk_editable_get_text(GTK_EDITABLE(entry));
        if (!url || strlen(url) == 0) {
            std::cerr << "No URL provided" << std::endl;
            return;
        }

        std::cout << "📦 Installing extension from URL: " << url << std::endl;

        // Disable button during installation
        gtk_widget_set_sensitive(GTK_WIDGET(btn), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(entry), FALSE);
        gtk_button_set_label(btn, "Installing...");

        // Create installer and install
        ExtensionInstaller installer(s->m_extensionManager);
        installer.installFromUrl(url, [s, btn, entry](bool success, const std::string& message) {
            // Re-enable controls
            gtk_button_set_label(GTK_BUTTON(btn), "Install");
            gtk_widget_set_sensitive(GTK_WIDGET(btn), TRUE);
            gtk_widget_set_sensitive(GTK_WIDGET(entry), TRUE);

            if (success) {
                std::cout << "✓ " << message << std::endl;

                // Clear the URL entry
                gtk_editable_set_text(GTK_EDITABLE(entry), "");

                // Save extension states
                s->saveExtensionStates();

                // Refresh the extensions list
                s->refreshExtensionsList();

                // Notify window to update extension buttons
                if (s->m_extensionChangeCallback) {
                    s->m_extensionChangeCallback();
                }

                // Show success notification
                std::cout << "✅ Extension installed and ready to use!" << std::endl;
            } else {
                std::cerr << "✗ " << message << std::endl;
            }
        });
    }), nullptr);
    gtk_box_append(GTK_BOX(quickInstallBox), installButton);

    // Add note about where to find extensions
    GtkWidget* noteLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(noteLabel),
        "<span size='small' style='italic'>💡 Just browse https://addons.mozilla.org in the browser and click install!</span>");
    gtk_label_set_wrap(GTK_LABEL(noteLabel), TRUE);
    gtk_widget_add_css_class(noteLabel, "dim-label");
    gtk_widget_set_halign(noteLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(installBox), noteLabel);

    // Load Unpacked button (for developers)
    GtkWidget* loadButton = gtk_button_new_with_label("Load Unpacked");
    gtk_widget_set_tooltip_text(loadButton, "Load an unpacked extension folder (for development)");
    g_signal_connect(loadButton, "clicked", G_CALLBACK(onLoadUnpackedClicked), this);
    gtk_box_append(GTK_BOX(installBox), loadButton);

    // Subtitle
    GtkWidget* subtitle = gtk_label_new("Install, enable, and manage your browser extensions");
    gtk_widget_set_halign(subtitle, GTK_ALIGN_START);
    gtk_widget_add_css_class(subtitle, "dim-label");
    gtk_widget_set_margin_start(subtitle, 20);
    gtk_widget_set_margin_end(subtitle, 20);
    gtk_widget_set_margin_bottom(subtitle, 15);
    gtk_box_append(GTK_BOX(mainBox), subtitle);

    // Extensions list
    extensionsList = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(extensionsList), GTK_SELECTION_NONE);
    gtk_widget_add_css_class(extensionsList, "boxed-list");
    gtk_widget_set_margin_start(extensionsList, 20);
    gtk_widget_set_margin_end(extensionsList, 20);
    gtk_widget_set_margin_bottom(extensionsList, 20);
    gtk_box_append(GTK_BOX(mainBox), extensionsList);

    // Populate list
    refreshExtensionsList();

    return scrolled;
}

void BrayaSettings::refreshExtensionsList() {
    if (!extensionsList || !m_extensionManager) {
        return;
    }

    // Clear existing rows
    GtkWidget* child = gtk_widget_get_first_child(extensionsList);
    while (child) {
        GtkWidget* next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(GTK_LIST_BOX(extensionsList), child);
        child = next;
    }

    // Get all extensions
    auto extensions = m_extensionManager->getWebExtensions();

    if (extensions.empty()) {
        // Show empty state
        GtkWidget* emptyBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
        gtk_widget_set_margin_top(emptyBox, 60);
        gtk_widget_set_margin_bottom(emptyBox, 60);

        GtkWidget* emptyLabel = gtk_label_new("No Extensions Installed");
        gtk_widget_add_css_class(emptyLabel, "title-2");
        gtk_box_append(GTK_BOX(emptyBox), emptyLabel);

        GtkWidget* emptyDesc = gtk_label_new("Click '🔍 Browse Extensions' to get started");
        gtk_widget_add_css_class(emptyDesc, "dim-label");
        gtk_box_append(GTK_BOX(emptyBox), emptyDesc);

        gtk_list_box_append(GTK_LIST_BOX(extensionsList), emptyBox);
    } else {
        // Add row for each extension
        for (auto* extension : extensions) {
            GtkWidget* row = createExtensionRow(extension);
            gtk_list_box_append(GTK_LIST_BOX(extensionsList), row);
        }
    }

    std::cout << "✓ Extension list refreshed (" << extensions.size() << " extensions)" << std::endl;
}

GtkWidget* BrayaSettings::createExtensionRow(void* ext) {
    auto* extension = static_cast<BrayaWebExtension*>(ext);

    // Main row box
    GtkWidget* rowBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_start(rowBox, 12);
    gtk_widget_set_margin_end(rowBox, 12);
    gtk_widget_set_margin_top(rowBox, 12);
    gtk_widget_set_margin_bottom(rowBox, 12);

    // Left side: Extension info
    GtkWidget* infoBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_hexpand(infoBox, TRUE);
    gtk_box_append(GTK_BOX(rowBox), infoBox);

    // Extension name
    GtkWidget* nameLabel = gtk_label_new(extension->getName().c_str());
    gtk_widget_set_halign(nameLabel, GTK_ALIGN_START);
    gtk_widget_add_css_class(nameLabel, "title-4");
    gtk_box_append(GTK_BOX(infoBox), nameLabel);

    // Extension version and ID
    std::string details = "Version " + extension->getVersion() + " • ID: " + extension->getId();
    GtkWidget* detailsLabel = gtk_label_new(details.c_str());
    gtk_widget_set_halign(detailsLabel, GTK_ALIGN_START);
    gtk_widget_add_css_class(detailsLabel, "dim-label");
    gtk_label_set_selectable(GTK_LABEL(detailsLabel), TRUE);
    gtk_box_append(GTK_BOX(infoBox), detailsLabel);

    // Extension path
    GtkWidget* pathLabel = gtk_label_new(extension->getPath().c_str());
    gtk_widget_set_halign(pathLabel, GTK_ALIGN_START);
    gtk_widget_add_css_class(pathLabel, "caption");
    gtk_widget_add_css_class(pathLabel, "dim-label");
    gtk_label_set_selectable(GTK_LABEL(pathLabel), TRUE);
    gtk_label_set_ellipsize(GTK_LABEL(pathLabel), PANGO_ELLIPSIZE_MIDDLE);
    gtk_label_set_max_width_chars(GTK_LABEL(pathLabel), 50);
    gtk_box_append(GTK_BOX(infoBox), pathLabel);

    // Permissions info
    auto permissions = extension->getPermissions();
    if (!permissions.empty()) {
        std::string permText = "Permissions: ";
        for (size_t i = 0; i < permissions.size() && i < 3; i++) {
            if (i > 0) permText += ", ";
            permText += permissions[i];
        }
        if (permissions.size() > 3) {
            permText += " (+" + std::to_string(permissions.size() - 3) + " more)";
        }

        GtkWidget* permLabel = gtk_label_new(permText.c_str());
        gtk_widget_set_halign(permLabel, GTK_ALIGN_START);
        gtk_widget_add_css_class(permLabel, "caption");
        gtk_box_append(GTK_BOX(infoBox), permLabel);
    }

    // Right side: Controls
    GtkWidget* controlsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_valign(controlsBox, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(rowBox), controlsBox);

    // Enable/Disable toggle
    GtkWidget* toggle = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(toggle), extension->isEnabled());
    gtk_widget_set_valign(toggle, GTK_ALIGN_CENTER);

    auto* toggleData = new ExtensionRowData{this, extension->getId()};
    g_signal_connect_data(toggle, "state-set", G_CALLBACK(onToggleExtension),
                         toggleData, [](gpointer data, GClosure*) { delete (ExtensionRowData*)data; }, (GConnectFlags)0);

    gtk_box_append(GTK_BOX(controlsBox), toggle);

    // Remove button
    GtkWidget* removeButton = gtk_button_new_with_label("Remove");
    gtk_widget_add_css_class(removeButton, "destructive-action");
    gtk_widget_set_valign(removeButton, GTK_ALIGN_CENTER);

    auto* removeData = new ExtensionRowData{this, extension->getId()};
    g_signal_connect_data(removeButton, "clicked", G_CALLBACK(onRemoveExtension),
                         removeData, [](gpointer data, GClosure*) { delete (ExtensionRowData*)data; }, (GConnectFlags)0);

    gtk_box_append(GTK_BOX(controlsBox), removeButton);

    return rowBox;
}

void BrayaSettings::onLoadUnpackedClicked(GtkButton* button, gpointer user_data) {
    auto* settings = static_cast<BrayaSettings*>(user_data);
    settings->loadUnpackedExtension();
}

void BrayaSettings::loadUnpackedExtension() {
    // Create file chooser dialog for selecting directory
    GtkWidget* fileDialog = gtk_file_chooser_dialog_new(
        "Select Extension Directory",
        GTK_WINDOW(dialog),
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Load", GTK_RESPONSE_ACCEPT,
        nullptr
    );

    // Set default folder to home directory
    const char* homeDir = g_get_home_dir();
    GFile* homeFile = g_file_new_for_path(homeDir);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fileDialog), homeFile, nullptr);
    g_object_unref(homeFile);

    // Show dialog
    g_signal_connect(fileDialog, "response", G_CALLBACK(+[](GtkDialog* dlg, int response, gpointer user_data) {
        auto* settings = static_cast<BrayaSettings*>(user_data);

        if (response == GTK_RESPONSE_ACCEPT) {
            GFile* folder = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dlg));
            char* path = g_file_get_path(folder);

            std::cout << "📦 Loading extension from: " << path << std::endl;

            // Load the extension
            if (settings->m_extensionManager->loadWebExtension(path)) {
                std::cout << "✓ Extension loaded successfully!" << std::endl;
                settings->saveExtensionStates();
                settings->refreshExtensionsList();
            } else {
                std::cerr << "❌ Failed to load extension" << std::endl;

                // Show error dialog
                GtkWidget* errorDialog = gtk_message_dialog_new(
                    GTK_WINDOW(settings->dialog),
                    GTK_DIALOG_MODAL,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_OK,
                    "Failed to Load Extension"
                );
                gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(errorDialog),
                    "Could not load extension from:\n%s\n\nMake sure the directory contains a valid manifest.json file.", path);
                gtk_window_present(GTK_WINDOW(errorDialog));
                g_signal_connect(errorDialog, "response", G_CALLBACK(gtk_window_destroy), nullptr);
            }

            g_free(path);
            g_object_unref(folder);
        }

        gtk_window_destroy(GTK_WINDOW(dlg));
    }), this);

    gtk_window_present(GTK_WINDOW(fileDialog));
}

void BrayaSettings::onToggleExtension(GtkSwitch* toggle, gboolean state, gpointer user_data) {
    auto* data = static_cast<ExtensionRowData*>(user_data);
    auto* extension = data->settings->m_extensionManager->getWebExtension(data->extensionId);

    if (extension) {
        extension->setEnabled(state);
        std::cout << (state ? "✓ Enabled" : "✗ Disabled") << " extension: " << extension->getName() << std::endl;
        data->settings->saveExtensionStates();

        // Update the extension buttons in the toolbar via callback
        if (data->settings->m_extensionChangeCallback) {
            data->settings->m_extensionChangeCallback();
        }
    }
}

void BrayaSettings::onRemoveExtension(GtkButton* button, gpointer user_data) {
    auto* data = static_cast<ExtensionRowData*>(user_data);
    data->settings->removeExtension(data->extensionId);
}

void BrayaSettings::removeExtension(const std::string& extensionId) {
    auto* extension = m_extensionManager->getWebExtension(extensionId);
    if (!extension) {
        return;
    }

    std::string extensionName = extension->getName();

    // Show confirmation dialog
    GtkWidget* confirmDialog = gtk_message_dialog_new(
        GTK_WINDOW(dialog),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_NONE,
        "Remove Extension?"
    );
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(confirmDialog),
        "Are you sure you want to remove '%s'?\n\nThe extension files will not be deleted from disk.",
        extensionName.c_str());

    gtk_dialog_add_buttons(GTK_DIALOG(confirmDialog),
        "Cancel", GTK_RESPONSE_CANCEL,
        "Remove", GTK_RESPONSE_ACCEPT,
        nullptr);

    // Make Remove button destructive
    GtkWidget* removeBtn = gtk_dialog_get_widget_for_response(GTK_DIALOG(confirmDialog), GTK_RESPONSE_ACCEPT);
    gtk_widget_add_css_class(removeBtn, "destructive-action");

    auto* data = new std::pair<BrayaSettings*, std::string>(this, extensionId);
    auto callback = +[](GtkDialog* dlg, int response, gpointer user_data) {
        auto* data = static_cast<std::pair<BrayaSettings*, std::string>*>(user_data);

        if (response == GTK_RESPONSE_ACCEPT) {
            std::cout << "🗑️  Removing extension: " << data->second << std::endl;
            data->first->m_extensionManager->removeWebExtension(data->second);
            data->first->saveExtensionStates();
            data->first->refreshExtensionsList();

            // Update the extension buttons in the toolbar via callback
            if (data->first->m_extensionChangeCallback) {
                data->first->m_extensionChangeCallback();
            }
        }

        delete data;
        gtk_window_destroy(GTK_WINDOW(dlg));
    };
    g_signal_connect_data(confirmDialog, "response", G_CALLBACK(callback), data, nullptr, (GConnectFlags)0);

    gtk_window_present(GTK_WINDOW(confirmDialog));
}

void BrayaSettings::saveExtensionStates() {
    if (!m_extensionManager) return;

    const char* configDir = g_get_user_config_dir();
    std::string configPath = std::string(configDir) + "/braya-browser";
    std::string extensionsFile = configPath + "/extensions.json";

    // Create JSON array of installed extensions
    JsonBuilder* builder = json_builder_new();
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "extensions");
    json_builder_begin_array(builder);

    auto extensions = m_extensionManager->getWebExtensions();
    for (auto* extension : extensions) {
        json_builder_begin_object(builder);
        json_builder_set_member_name(builder, "id");
        json_builder_add_string_value(builder, extension->getId().c_str());
        json_builder_set_member_name(builder, "path");
        json_builder_add_string_value(builder, extension->getPath().c_str());
        json_builder_set_member_name(builder, "enabled");
        json_builder_add_boolean_value(builder, extension->isEnabled());
        json_builder_end_object(builder);
    }

    json_builder_end_array(builder);
    json_builder_end_object(builder);

    // Generate JSON string
    JsonNode* root = json_builder_get_root(builder);
    JsonGenerator* generator = json_generator_new();
    json_generator_set_root(generator, root);
    json_generator_set_pretty(generator, TRUE);
    gchar* jsonData = json_generator_to_data(generator, nullptr);

    // Write to file
    std::ofstream file(extensionsFile);
    if (file.is_open()) {
        file << jsonData;
        file.close();
        std::cout << "✓ Saved extension states to: " << extensionsFile << std::endl;
    } else {
        std::cerr << "ERROR: Could not write to: " << extensionsFile << std::endl;
    }

    g_free(jsonData);
    g_object_unref(generator);
    json_node_free(root);
    g_object_unref(builder);
}

void BrayaSettings::loadExtensionStates() {
    if (!m_extensionManager) return;

    const char* configDir = g_get_user_config_dir();
    std::string extensionsFile = std::string(configDir) + "/braya-browser/extensions.json";

    // Check if file exists
    if (!g_file_test(extensionsFile.c_str(), G_FILE_TEST_EXISTS)) {
        std::cout << "ℹ️  No saved extensions file found" << std::endl;
        return;
    }

    // Read file
    std::ifstream file(extensionsFile);
    if (!file.is_open()) {
        std::cerr << "ERROR: Could not open: " << extensionsFile << std::endl;
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    std::string jsonContent = buffer.str();

    // Parse JSON
    GError* error = nullptr;
    JsonParser* parser = json_parser_new();

    if (!json_parser_load_from_data(parser, jsonContent.c_str(), -1, &error)) {
        std::cerr << "ERROR: Failed to parse extensions.json: " << error->message << std::endl;
        g_error_free(error);
        g_object_unref(parser);
        return;
    }

    JsonNode* root = json_parser_get_root(parser);
    if (!JSON_NODE_HOLDS_OBJECT(root)) {
        g_object_unref(parser);
        return;
    }

    JsonObject* rootObj = json_node_get_object(root);
    if (!json_object_has_member(rootObj, "extensions")) {
        g_object_unref(parser);
        return;
    }

    JsonArray* extensionsArray = json_object_get_array_member(rootObj, "extensions");
    guint numExtensions = json_array_get_length(extensionsArray);

    std::cout << "📦 Loading " << numExtensions << " saved extensions..." << std::endl;

    for (guint i = 0; i < numExtensions; i++) {
        JsonObject* extObj = json_array_get_object_element(extensionsArray, i);

        if (json_object_has_member(extObj, "path")) {
            const char* path = json_object_get_string_member(extObj, "path");
            bool enabled = json_object_has_member(extObj, "enabled") ?
                          json_object_get_boolean_member(extObj, "enabled") : true;

            if (m_extensionManager->loadWebExtension(path)) {
                // Set enabled state
                const char* id = json_object_get_string_member(extObj, "id");
                auto* extension = m_extensionManager->getWebExtension(id);
                if (extension) {
                    extension->setEnabled(enabled);
                }
            }
        }
    }

    g_object_unref(parser);
    std::cout << "✓ Extensions loaded from config" << std::endl;
}

// Ad-Blocker Tab
GtkWidget* BrayaSettings::createAdBlockerTab() {
    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(box, 30);
    gtk_widget_set_margin_end(box, 30);
    gtk_widget_set_margin_top(box, 30);
    gtk_widget_set_margin_bottom(box, 30);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), box);

    // Header
    GtkWidget* headerLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(headerLabel), "<span size='large' weight='bold'>🛡️ Ad-Blocker Settings</span>");
    gtk_widget_set_halign(headerLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), headerLabel);

    GtkWidget* descLabel = gtk_label_new("Control ad-blocking, tracking protection, and privacy features");
    gtk_widget_add_css_class(descLabel, "dim-label");
    gtk_widget_set_halign(descLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), descLabel);

    // Enable/Disable
    GtkWidget* enableBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
    gtk_box_append(GTK_BOX(box), enableBox);

    GtkWidget* enableLabel = gtk_label_new("Enable Ad-Blocker");
    gtk_label_set_xalign(GTK_LABEL(enableLabel), 0);
    gtk_widget_set_hexpand(enableLabel, TRUE);
    gtk_box_append(GTK_BOX(enableBox), enableLabel);

    adBlockerEnabledSwitch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(adBlockerEnabledSwitch), TRUE);
    g_signal_connect(adBlockerEnabledSwitch, "state-set", G_CALLBACK(onAdBlockerToggled), this);
    gtk_box_append(GTK_BOX(enableBox), adBlockerEnabledSwitch);

    // Security Level
    GtkWidget* secLevelBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
    gtk_box_append(GTK_BOX(box), secLevelBox);

    GtkWidget* secLabel = gtk_label_new("Security Level");
    gtk_label_set_xalign(GTK_LABEL(secLabel), 0);
    gtk_widget_set_hexpand(secLabel, TRUE);
    gtk_box_append(GTK_BOX(secLevelBox), secLabel);

    securityLevelCombo = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(securityLevelCombo), "off", "Off");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(securityLevelCombo), "minimal", "Minimal");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(securityLevelCombo), "standard", "Standard");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(securityLevelCombo), "strict", "Strict");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(securityLevelCombo), "custom", "Custom");
    gtk_combo_box_set_active(GTK_COMBO_BOX(securityLevelCombo), 2); // Standard
    g_signal_connect(securityLevelCombo, "changed", G_CALLBACK(onSecurityLevelChanged), this);
    gtk_box_append(GTK_BOX(secLevelBox), securityLevelCombo);

    // Features grid
    GtkWidget* featuresLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(featuresLabel), "<span weight='bold'>Blocking Features</span>");
    gtk_widget_set_halign(featuresLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(featuresLabel, 10);
    gtk_box_append(GTK_BOX(box), featuresLabel);

    GtkWidget* featuresGrid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(featuresGrid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(featuresGrid), 20);
    gtk_box_append(GTK_BOX(box), featuresGrid);

    // Feature checkboxes
    blockAdsCheck = gtk_check_button_new_with_label("Block Ads");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(blockAdsCheck), TRUE);
    g_signal_connect(blockAdsCheck, "toggled", G_CALLBACK(onFeatureToggled), this);
    gtk_grid_attach(GTK_GRID(featuresGrid), blockAdsCheck, 0, 0, 1, 1);

    blockTrackersCheck = gtk_check_button_new_with_label("Block Trackers");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(blockTrackersCheck), TRUE);
    g_signal_connect(blockTrackersCheck, "toggled", G_CALLBACK(onFeatureToggled), this);
    gtk_grid_attach(GTK_GRID(featuresGrid), blockTrackersCheck, 1, 0, 1, 1);

    blockSocialCheck = gtk_check_button_new_with_label("Block Social Widgets");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(blockSocialCheck), FALSE);
    g_signal_connect(blockSocialCheck, "toggled", G_CALLBACK(onFeatureToggled), this);
    gtk_grid_attach(GTK_GRID(featuresGrid), blockSocialCheck, 0, 1, 1, 1);

    blockCryptominersCheck = gtk_check_button_new_with_label("Block Cryptominers");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(blockCryptominersCheck), TRUE);
    g_signal_connect(blockCryptominersCheck, "toggled", G_CALLBACK(onFeatureToggled), this);
    gtk_grid_attach(GTK_GRID(featuresGrid), blockCryptominersCheck, 1, 1, 1, 1);

    blockPopupsCheck = gtk_check_button_new_with_label("Block Pop-ups");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(blockPopupsCheck), TRUE);
    g_signal_connect(blockPopupsCheck, "toggled", G_CALLBACK(onFeatureToggled), this);
    gtk_grid_attach(GTK_GRID(featuresGrid), blockPopupsCheck, 0, 2, 1, 1);

    blockAutoplayCheck = gtk_check_button_new_with_label("Block Autoplay Videos");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(blockAutoplayCheck), FALSE);
    g_signal_connect(blockAutoplayCheck, "toggled", G_CALLBACK(onFeatureToggled), this);
    gtk_grid_attach(GTK_GRID(featuresGrid), blockAutoplayCheck, 1, 2, 1, 1);

    removeCookieWarningsCheck = gtk_check_button_new_with_label("Remove Cookie Warnings");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(removeCookieWarningsCheck), FALSE);
    g_signal_connect(removeCookieWarningsCheck, "toggled", G_CALLBACK(onFeatureToggled), this);
    gtk_grid_attach(GTK_GRID(featuresGrid), removeCookieWarningsCheck, 0, 3, 1, 1);

    blockNSFWCheck = gtk_check_button_new_with_label("Block NSFW Content");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(blockNSFWCheck), FALSE);
    g_signal_connect(blockNSFWCheck, "toggled", G_CALLBACK(onFeatureToggled), this);
    gtk_grid_attach(GTK_GRID(featuresGrid), blockNSFWCheck, 1, 3, 1, 1);

    // Import/Export Section
    GtkWidget* importExportLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(importExportLabel), "<span weight='bold'>Import/Export</span>");
    gtk_widget_set_halign(importExportLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(importExportLabel, 20);
    gtk_box_append(GTK_BOX(box), importExportLabel);

    GtkWidget* importExportBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(box), importExportBox);

    GtkWidget* exportBtn = gtk_button_new_with_label("Export Settings");
    g_signal_connect(exportBtn, "clicked", G_CALLBACK(onExportSettings), this);
    gtk_box_append(GTK_BOX(importExportBox), exportBtn);

    GtkWidget* importBtn = gtk_button_new_with_label("Import Settings");
    g_signal_connect(importBtn, "clicked", G_CALLBACK(onImportSettings), this);
    gtk_box_append(GTK_BOX(importExportBox), importBtn);

    return scrolled;
}

GtkWidget* BrayaSettings::createPasswordsTab() {
    GtkWidget* scrolled = gtk_scrolled_window_new();
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(box, 30);
    gtk_widget_set_margin_end(box, 30);
    gtk_widget_set_margin_top(box, 30);
    gtk_widget_set_margin_bottom(box, 30);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), box);

    // Header
    GtkWidget* headerLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(headerLabel), "<span size='large' weight='bold'>🔑 Password Manager</span>");
    gtk_widget_set_halign(headerLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), headerLabel);

    GtkWidget* descLabel = gtk_label_new("Manage your saved passwords and security settings");
    gtk_widget_add_css_class(descLabel, "dim-label");
    gtk_widget_set_halign(descLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), descLabel);

    // Open Password Manager button
    GtkWidget* openBtn = gtk_button_new_with_label("🔑 Open Password Manager");
    gtk_widget_add_css_class(openBtn, "suggested-action");
    gtk_widget_set_halign(openBtn, GTK_ALIGN_START);
    g_object_set_data(G_OBJECT(openBtn), "settings", this);
    g_signal_connect(openBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        BrayaSettings* settings = static_cast<BrayaSettings*>(g_object_get_data(G_OBJECT(btn), "settings"));
        if (settings && settings->m_passwordManager) {
            GtkWidget* dialog = GTK_WIDGET(settings->dialog);
            settings->m_passwordManager->showPasswordManager(GTK_WINDOW(dialog));
        }
    }), nullptr);
    gtk_box_append(GTK_BOX(box), openBtn);

    // Separator
    GtkWidget* separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_margin_top(separator, 10);
    gtk_widget_set_margin_bottom(separator, 10);
    gtk_box_append(GTK_BOX(box), separator);

    // Quick stats
    GtkWidget* statsLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(statsLabel), "<span weight='bold'>Password Statistics</span>");
    gtk_widget_set_halign(statsLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), statsLabel);

    if (m_passwordManager) {
        int passwordCount = m_passwordManager->getAllPasswords().size();
        std::string countText = "Saved passwords: " + std::to_string(passwordCount);
        GtkWidget* countLabel = gtk_label_new(countText.c_str());
        gtk_widget_set_halign(countLabel, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(box), countLabel);

        multiStepSwitch = gtk_switch_new();
        gtk_switch_set_active(GTK_SWITCH(multiStepSwitch), m_passwordManager->isMultiStepCaptureEnabled());
        g_signal_connect(multiStepSwitch, "state-set", G_CALLBACK(onMultiStepToggle), this);
        GtkWidget* multiStepRow = createSettingsRow(
            "Multi-step login capture",
            "Capture logins that ask for username and password on separate screens. Disable if login forms don't save properly on some sites.",
            multiStepSwitch);
        gtk_box_append(GTK_BOX(box), multiStepRow);

        GtkWidget* securityLabel = gtk_label_new(nullptr);
        gtk_label_set_markup(GTK_LABEL(securityLabel), "<span weight='bold'>Security controls</span>");
        gtk_widget_set_halign(securityLabel, GTK_ALIGN_START);
        gtk_widget_set_margin_top(securityLabel, 12);
        gtk_box_append(GTK_BOX(box), securityLabel);

        GtkWidget* actionsRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_box_append(GTK_BOX(box), actionsRow);

        GtkWidget* changeBtn = gtk_button_new_with_label("Change master password");
        g_object_set_data(G_OBJECT(changeBtn), "settings", this);
        gtk_box_append(GTK_BOX(actionsRow), changeBtn);
        g_signal_connect(changeBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer) {
            auto* settings = static_cast<BrayaSettings*>(g_object_get_data(G_OBJECT(btn), "settings"));
            if (settings && settings->m_passwordManager) {
                settings->m_passwordManager->changeMasterPassword(GTK_WINDOW(settings->dialog));
            }
        }), nullptr);

        GtkWidget* resetBtn = gtk_button_new_with_label("Reset vault");
        gtk_widget_add_css_class(resetBtn, "destructive-action");
        g_object_set_data(G_OBJECT(resetBtn), "settings", this);
        gtk_box_append(GTK_BOX(actionsRow), resetBtn);
        g_signal_connect(resetBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer) {
            auto* settings = static_cast<BrayaSettings*>(g_object_get_data(G_OBJECT(btn), "settings"));
            if (settings && settings->m_passwordManager) {
                settings->m_passwordManager->resetVault(GTK_WINDOW(settings->dialog));
            }
        }), nullptr);

        GtkWidget* exceptionsLabel = gtk_label_new(nullptr);
        gtk_label_set_markup(GTK_LABEL(exceptionsLabel), "<span weight='bold'>Site exceptions</span>");
        gtk_widget_set_halign(exceptionsLabel, GTK_ALIGN_START);
        gtk_widget_set_margin_top(exceptionsLabel, 12);
        gtk_box_append(GTK_BOX(box), exceptionsLabel);

        GtkWidget* exceptionsBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_widget_add_css_class(exceptionsBox, "dialog-card");
        gtk_widget_set_margin_bottom(exceptionsBox, 5);
        gtk_box_append(GTK_BOX(box), exceptionsBox);

        auto blocked = m_passwordManager->getBlockedDomains();
        if (blocked.empty()) {
            GtkWidget* empty = gtk_label_new("No sites are blocked from saving passwords.");
            gtk_widget_set_halign(empty, GTK_ALIGN_START);
            gtk_box_append(GTK_BOX(exceptionsBox), empty);
        } else {
            for (const auto& domain : blocked) {
                GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
                gtk_widget_set_halign(row, GTK_ALIGN_FILL);
                gtk_box_append(GTK_BOX(exceptionsBox), row);

                GtkWidget* label = gtk_label_new(domain.c_str());
                gtk_widget_set_hexpand(label, TRUE);
                gtk_widget_set_halign(label, GTK_ALIGN_START);
                gtk_box_append(GTK_BOX(row), label);

                GtkWidget* allowBtn = gtk_button_new_with_label("Allow saving");
                g_object_set_data(G_OBJECT(allowBtn), "settings", this);
                g_object_set_data_full(G_OBJECT(allowBtn), "domain", g_strdup(domain.c_str()), g_free);
                g_signal_connect(allowBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer) {
                    auto* settings = static_cast<BrayaSettings*>(g_object_get_data(G_OBJECT(btn), "settings"));
                    const char* dom = static_cast<const char*>(g_object_get_data(G_OBJECT(btn), "domain"));
                    if (settings && settings->m_passwordManager && dom) {
                        settings->m_passwordManager->unblockDomain(dom);
                        settings->createPasswordsTab(); // rebuild?
                    }
                }), nullptr);
                gtk_box_append(GTK_BOX(row), allowBtn);
            }
        }
    }

    // Import/Export section
    GtkWidget* importExportLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(importExportLabel), "<span weight='bold'>Import/Export</span>");
    gtk_widget_set_halign(importExportLabel, GTK_ALIGN_START);
    gtk_widget_set_margin_top(importExportLabel, 20);
    gtk_box_append(GTK_BOX(box), importExportLabel);

    GtkWidget* importExportBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(box), importExportBox);

    GtkWidget* exportBtn = gtk_button_new_with_label("📤 Export to CSV");
    g_object_set_data(G_OBJECT(exportBtn), "settings", this);
    g_signal_connect(exportBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        BrayaSettings* settings = static_cast<BrayaSettings*>(g_object_get_data(G_OBJECT(btn), "settings"));
        if (settings && settings->m_passwordManager && settings->dialog) {
            GtkFileChooserNative* chooser = gtk_file_chooser_native_new(
                "Export Passwords",
                GTK_WINDOW(settings->dialog),
                GTK_FILE_CHOOSER_ACTION_SAVE,
                "Export",
                "Cancel"
            );

            gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(chooser), "braya-passwords.csv");

            g_object_set_data(G_OBJECT(chooser), "settings", settings);
            g_signal_connect(chooser, "response", G_CALLBACK(+[](GtkFileChooserNative* chooser, int response, gpointer) {
                if (response == GTK_RESPONSE_ACCEPT) {
                    auto* settings = static_cast<BrayaSettings*>(g_object_get_data(G_OBJECT(chooser), "settings"));
                    GFile* file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(chooser));
                    char* path = g_file_get_path(file);
                    if (path && settings->m_passwordManager) {
                        if (settings->m_passwordManager->exportToCSV(path)) {
                            g_print("✓ Passwords exported to %s\n", path);
                        }
                        g_free(path);
                    }
                    g_object_unref(file);
                }
            }), nullptr);

            gtk_native_dialog_show(GTK_NATIVE_DIALOG(chooser));
        }
    }), nullptr);
    gtk_box_append(GTK_BOX(importExportBox), exportBtn);

    GtkWidget* importBtn = gtk_button_new_with_label("📥 Import from CSV");
    g_object_set_data(G_OBJECT(importBtn), "settings", this);
    g_signal_connect(importBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        BrayaSettings* settings = static_cast<BrayaSettings*>(g_object_get_data(G_OBJECT(btn), "settings"));
        if (settings && settings->m_passwordManager && settings->dialog) {
            GtkFileChooserNative* chooser = gtk_file_chooser_native_new(
                "Import Passwords",
                GTK_WINDOW(settings->dialog),
                GTK_FILE_CHOOSER_ACTION_OPEN,
                "Import",
                "Cancel"
            );

            GtkFileFilter* filter = gtk_file_filter_new();
            gtk_file_filter_set_name(filter, "CSV Files");
            gtk_file_filter_add_pattern(filter, "*.csv");
            gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);

            g_object_set_data(G_OBJECT(chooser), "settings", settings);
            g_signal_connect(chooser, "response", G_CALLBACK(+[](GtkFileChooserNative* chooser, int response, gpointer) {
                if (response == GTK_RESPONSE_ACCEPT) {
                    auto* settings = static_cast<BrayaSettings*>(g_object_get_data(G_OBJECT(chooser), "settings"));
                    GFile* file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(chooser));
                    char* path = g_file_get_path(file);
                    if (path && settings->m_passwordManager) {
                        if (settings->m_passwordManager->importFromCSV(path)) {
                            g_print("✓ Passwords imported from %s\n", path);
                        }
                        g_free(path);
                    }
                    g_object_unref(file);
                }
            }), nullptr);

            gtk_native_dialog_show(GTK_NATIVE_DIALOG(chooser));
        }
    }), nullptr);
    gtk_box_append(GTK_BOX(importExportBox), importBtn);

    return scrolled;
}

gboolean BrayaSettings::onMultiStepToggle(GtkSwitch* toggle, gboolean state, gpointer data) {
    auto* settings = static_cast<BrayaSettings*>(data);
    if (settings && settings->m_passwordManager) {
        settings->m_passwordManager->setMultiStepCaptureEnabled(state);
    }
    return FALSE;
}

// Ad-Blocker Callbacks
void BrayaSettings::onAdBlockerToggled(GtkSwitch* toggle, gboolean state, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    if (!settings->m_adBlocker) return;

    if (state) {
        settings->m_adBlocker->enable();
        std::cout << "✓ Ad-blocker enabled" << std::endl;
    } else {
        settings->m_adBlocker->disable();
        std::cout << "✓ Ad-blocker disabled" << std::endl;
    }
}

void BrayaSettings::onSecurityLevelChanged(GtkComboBox* combo, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    if (!settings->m_adBlocker) return;

    const char* activeId = gtk_combo_box_get_active_id(combo);
    if (!activeId) return;

    SecurityLevel level;
    if (strcmp(activeId, "off") == 0) level = SecurityLevel::OFF;
    else if (strcmp(activeId, "minimal") == 0) level = SecurityLevel::MINIMAL;
    else if (strcmp(activeId, "standard") == 0) level = SecurityLevel::STANDARD;
    else if (strcmp(activeId, "strict") == 0) level = SecurityLevel::STRICT;
    else level = SecurityLevel::CUSTOM;

    settings->m_adBlocker->setSecurityLevel(level);
    std::cout << "✓ Security level changed to: " << activeId << std::endl;
}

void BrayaSettings::onFeatureToggled(GtkCheckButton* button, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    if (!settings->m_adBlocker) return;

    BlockingFeatures features;
    features.block_ads = gtk_check_button_get_active(GTK_CHECK_BUTTON(settings->blockAdsCheck));
    features.block_trackers = gtk_check_button_get_active(GTK_CHECK_BUTTON(settings->blockTrackersCheck));
    features.block_social = gtk_check_button_get_active(GTK_CHECK_BUTTON(settings->blockSocialCheck));
    features.block_cryptominers = gtk_check_button_get_active(GTK_CHECK_BUTTON(settings->blockCryptominersCheck));
    features.block_popups = gtk_check_button_get_active(GTK_CHECK_BUTTON(settings->blockPopupsCheck));
    features.block_autoplay = gtk_check_button_get_active(GTK_CHECK_BUTTON(settings->blockAutoplayCheck));
    features.remove_cookie_warnings = gtk_check_button_get_active(GTK_CHECK_BUTTON(settings->removeCookieWarningsCheck));
    features.block_nsfw = gtk_check_button_get_active(GTK_CHECK_BUTTON(settings->blockNSFWCheck));

    settings->m_adBlocker->setFeatures(features);
    std::cout << "✓ Ad-blocker features updated" << std::endl;
}

void BrayaSettings::onAddToWhitelist(GtkButton* button, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    if (!settings->m_adBlocker || !settings->notebook) return;

    // Get the entry widget
    GtkWidget* adBlockerTab = gtk_stack_get_child_by_name(GTK_STACK(settings->notebook), "adblocker");
    if (!adBlockerTab) return;

    GtkWidget* scrolled = gtk_widget_get_first_child(adBlockerTab);
    if (!scrolled || !GTK_IS_SCROLLED_WINDOW(scrolled)) return;

    GtkWidget* box = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(scrolled));
    if (!box) return;

    GtkWidget* whitelistEntry = GTK_WIDGET(g_object_get_data(G_OBJECT(box), "whitelist_entry"));
    if (!whitelistEntry) return;

    const char* domain = gtk_editable_get_text(GTK_EDITABLE(whitelistEntry));
    if (!domain || strlen(domain) == 0) return;

    // Add to whitelist
    settings->m_adBlocker->addToWhitelist(domain);

    // Clear entry
    gtk_editable_set_text(GTK_EDITABLE(whitelistEntry), "");

    // Refresh UI
    settings->updateAdBlockerUI();
}

void BrayaSettings::onRemoveFromWhitelist(GtkButton* button, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    if (!settings->m_adBlocker) return;

    const char* domain = (const char*)g_object_get_data(G_OBJECT(button), "domain");
    if (!domain) return;

    settings->m_adBlocker->removeFromWhitelist(domain);
    settings->updateAdBlockerUI();
}

void BrayaSettings::onFilterListToggled(GtkSwitch* toggle, gboolean state, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    if (!settings->m_adBlocker) return;

    const char* listName = (const char*)g_object_get_data(G_OBJECT(toggle), "list_name");
    if (!listName) return;

    settings->m_adBlocker->enableFilterList(listName, state);
    std::cout << "✓ Filter list " << listName << " " << (state ? "enabled" : "disabled") << std::endl;
}

void BrayaSettings::onAddCustomRule(GtkButton* button, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    if (!settings->m_adBlocker || !settings->notebook) return;

    // Get the entry widget
    GtkWidget* adBlockerTab = gtk_stack_get_child_by_name(GTK_STACK(settings->notebook), "adblocker");
    if (!adBlockerTab) return;

    GtkWidget* scrolled = gtk_widget_get_first_child(adBlockerTab);
    if (!scrolled || !GTK_IS_SCROLLED_WINDOW(scrolled)) return;

    GtkWidget* box = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(scrolled));
    if (!box) return;

    GtkWidget* customRuleEntry = GTK_WIDGET(g_object_get_data(G_OBJECT(box), "custom_rule_entry"));
    if (!customRuleEntry) return;

    const char* rule = gtk_editable_get_text(GTK_EDITABLE(customRuleEntry));
    if (!rule || strlen(rule) == 0) return;

    // Add custom rule
    settings->m_adBlocker->addCustomRule(rule);

    // Clear entry
    gtk_editable_set_text(GTK_EDITABLE(customRuleEntry), "");

    // Refresh UI
    settings->updateAdBlockerUI();
}

void BrayaSettings::onRemoveCustomRule(GtkButton* button, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    if (!settings->m_adBlocker) return;

    const char* rule = (const char*)g_object_get_data(G_OBJECT(button), "rule");
    if (!rule) return;

    settings->m_adBlocker->removeCustomRule(rule);
    settings->updateAdBlockerUI();
}

void BrayaSettings::onResetStats(GtkButton* button, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    if (!settings->m_adBlocker) return;

    settings->m_adBlocker->resetStats();
    settings->updateAdBlockerUI();
    std::cout << "✓ Ad-blocker statistics reset" << std::endl;
}

void BrayaSettings::onExportSettings(GtkButton* button, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    if (!settings->m_adBlocker || !settings->dialog) return;

    // Get home directory
    const char* home = getenv("HOME");
    if (!home) {
        std::cerr << "✗ Could not get home directory" << std::endl;
        return;
    }

    // Export to a timestamped file
    time_t now = time(nullptr);
    struct tm* tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);

    std::string exportPath = std::string(home) + "/braya-adblock-settings-" + timestamp + ".json";

    if (settings->m_adBlocker->saveSettings(exportPath)) {
        std::cout << "✓ Settings exported to: " << exportPath << std::endl;

        // Show success dialog
        GtkWidget* dialog = gtk_message_dialog_new(
            GTK_WINDOW(settings->dialog),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "Settings Exported"
        );
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
            "Settings exported to:\n%s", exportPath.c_str());
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
        gtk_window_present(GTK_WINDOW(dialog));
    } else {
        std::cerr << "✗ Failed to export settings" << std::endl;
    }
}

void BrayaSettings::onImportSettings(GtkButton* button, gpointer data) {
    BrayaSettings* settings = static_cast<BrayaSettings*>(data);
    if (!settings->m_adBlocker || !settings->dialog) return;

    // Create file chooser dialog
    GtkFileDialog* fileDialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(fileDialog, "Import Ad-Blocker Settings");

    // Set initial folder to home
    const char* home = getenv("HOME");
    if (home) {
        GFile* homeFile = g_file_new_for_path(home);
        gtk_file_dialog_set_initial_folder(fileDialog, homeFile);
        g_object_unref(homeFile);
    }

    // Add JSON filter
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "JSON files");
    gtk_file_filter_add_pattern(filter, "*.json");

    GListStore* filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filters, filter);
    gtk_file_dialog_set_filters(fileDialog, G_LIST_MODEL(filters));

    // Open file chooser
    gtk_file_dialog_open(fileDialog, GTK_WINDOW(settings->dialog), nullptr,
        +[](GObject* source, GAsyncResult* result, gpointer user_data) {
            BrayaSettings* settings = static_cast<BrayaSettings*>(user_data);
            GtkFileDialog* dialog = GTK_FILE_DIALOG(source);

            GError* error = nullptr;
            GFile* file = gtk_file_dialog_open_finish(dialog, result, &error);

            if (file) {
                char* path = g_file_get_path(file);
                if (settings->m_adBlocker->loadSettings(path)) {
                    std::cout << "✓ Settings imported from: " << path << std::endl;
                    settings->updateAdBlockerUI();

                    // Show success dialog
                    GtkWidget* successDialog = gtk_message_dialog_new(
                        GTK_WINDOW(settings->dialog),
                        GTK_DIALOG_MODAL,
                        GTK_MESSAGE_INFO,
                        GTK_BUTTONS_OK,
                        "Settings Imported"
                    );
                    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(successDialog),
                        "Settings imported successfully from:\n%s", path);
                    g_signal_connect(successDialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
                    gtk_window_present(GTK_WINDOW(successDialog));
                } else {
                    std::cerr << "✗ Failed to import settings from: " << path << std::endl;
                }
                g_free(path);
                g_object_unref(file);
            } else if (error) {
                if (error->code != GTK_DIALOG_ERROR_DISMISSED) {
                    std::cerr << "✗ Error opening file: " << error->message << std::endl;
                }
                g_error_free(error);
            }
        }, settings);

    g_object_unref(filter);
    g_object_unref(filters);
}
