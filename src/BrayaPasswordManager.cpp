#include "BrayaPasswordManager.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <algorithm>
#include <sys/stat.h>

BrayaPasswordManager::BrayaPasswordManager() 
    : passwordSavingEnabled(true) {
    
    // Storage in user config directory
    const char* homeDir = g_get_home_dir();
    std::string configDir = std::string(homeDir) + "/.config/braya-browser";
    
    // Create config directory if it doesn't exist
    mkdir(configDir.c_str(), 0700);
    
    storageFile = configDir + "/passwords.dat";
    loadPasswords();
}

BrayaPasswordManager::~BrayaPasswordManager() {
    savePasswords();
}

std::string BrayaPasswordManager::getDomain(const std::string& url) {
    // Extract domain from URL
    std::string domain = url;
    
    // Remove protocol
    size_t pos = domain.find("://");
    if (pos != std::string::npos) {
        domain = domain.substr(pos + 3);
    }
    
    // Remove path
    pos = domain.find("/");
    if (pos != std::string::npos) {
        domain = domain.substr(0, pos);
    }
    
    return domain;
}

void BrayaPasswordManager::savePassword(const std::string& url, const std::string& username, const std::string& password) {
    if (!passwordSavingEnabled) return;
    
    std::string domain = getDomain(url);
    
    // Check if already exists
    for (auto& entry : passwords) {
        if (entry.url == domain && entry.username == username) {
            // Update existing
            entry.password = password;
            entry.timestamp = time(nullptr);
            savePasswords();
            std::cout << "✓ Password updated for " << username << " @ " << domain << std::endl;
            return;
        }
    }
    
    // Add new entry
    PasswordEntry entry;
    entry.url = domain;
    entry.username = username;
    entry.password = password;
    entry.timestamp = time(nullptr);
    
    passwords.push_back(entry);
    savePasswords();
    
    std::cout << "✓ Password saved for " << username << " @ " << domain << std::endl;
}

std::vector<PasswordEntry> BrayaPasswordManager::getPasswordsForUrl(const std::string& url) {
    std::string domain = getDomain(url);
    std::vector<PasswordEntry> matches;
    
    for (const auto& entry : passwords) {
        if (entry.url == domain) {
            matches.push_back(entry);
        }
    }
    
    return matches;
}

std::vector<PasswordEntry> BrayaPasswordManager::getAllPasswords() {
    return passwords;
}

void BrayaPasswordManager::deletePassword(const std::string& url, const std::string& username) {
    std::string domain = getDomain(url);
    
    passwords.erase(
        std::remove_if(passwords.begin(), passwords.end(),
            [&](const PasswordEntry& entry) {
                return entry.url == domain && entry.username == username;
            }),
        passwords.end()
    );
    
    savePasswords();
    std::cout << "✓ Password deleted for " << username << " @ " << domain << std::endl;
}

void BrayaPasswordManager::clearAll() {
    passwords.clear();
    savePasswords();
    std::cout << "✓ All passwords cleared" << std::endl;
}

std::string BrayaPasswordManager::encrypt(const std::string& data) {
    // Simple XOR encryption with a key (for v1.0.1)
    // TODO v2.0: Use proper encryption (libsodium, GPG, or system keyring)
    std::string key = "BrayaBrowserPasswordKey2024"; // In production, use system keyring
    std::string encrypted = data;
    
    for (size_t i = 0; i < encrypted.size(); i++) {
        encrypted[i] ^= key[i % key.size()];
    }
    
    return encrypted;
}

std::string BrayaPasswordManager::decrypt(const std::string& data) {
    // XOR encryption is symmetric
    return encrypt(data);
}

void BrayaPasswordManager::loadPasswords() {
    std::ifstream file(storageFile, std::ios::binary);
    if (!file.is_open()) {
        return; // No passwords saved yet
    }
    
    passwords.clear();
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        // Decrypt the line
        std::string decrypted = decrypt(line);
        
        // Parse: url|username|password|timestamp
        size_t pos1 = decrypted.find('|');
        size_t pos2 = decrypted.find('|', pos1 + 1);
        size_t pos3 = decrypted.find('|', pos2 + 1);
        
        if (pos1 != std::string::npos && pos2 != std::string::npos && pos3 != std::string::npos) {
            PasswordEntry entry;
            entry.url = decrypted.substr(0, pos1);
            entry.username = decrypted.substr(pos1 + 1, pos2 - pos1 - 1);
            entry.password = decrypted.substr(pos2 + 1, pos3 - pos2 - 1);
            entry.timestamp = std::stol(decrypted.substr(pos3 + 1));
            
            passwords.push_back(entry);
        }
    }
    
    file.close();
    std::cout << "✓ Loaded " << passwords.size() << " saved passwords" << std::endl;
}

void BrayaPasswordManager::savePasswords() {
    std::ofstream file(storageFile, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "✗ Failed to save passwords" << std::endl;
        return;
    }
    
    // Set file permissions to 600 (user read/write only)
    chmod(storageFile.c_str(), S_IRUSR | S_IWUSR);
    
    for (const auto& entry : passwords) {
        // Format: url|username|password|timestamp
        std::string line = entry.url + "|" + entry.username + "|" + 
                          entry.password + "|" + std::to_string(entry.timestamp);
        
        // Encrypt and save
        std::string encrypted = encrypt(line);
        file << encrypted << "\n";
    }
    
    file.close();
}

void BrayaPasswordManager::setPasswordSavingEnabled(bool enabled) {
    passwordSavingEnabled = enabled;
}

void BrayaPasswordManager::showPasswordManager(GtkWindow* parent) {
    GtkWidget* dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "🔑 Password Manager");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 700, 500);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    
    GtkWidget* mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(mainBox, 20);
    gtk_widget_set_margin_end(mainBox, 20);
    gtk_widget_set_margin_top(mainBox, 20);
    gtk_widget_set_margin_bottom(mainBox, 20);
    gtk_window_set_child(GTK_WINDOW(dialog), mainBox);
    
    // Header
    GtkWidget* headerLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(headerLabel), "<span size='x-large' weight='bold'>🔑 Saved Passwords</span>");
    gtk_box_append(GTK_BOX(mainBox), headerLabel);
    
    // Scrolled window for password list
    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_box_append(GTK_BOX(mainBox), scrolled);
    
    GtkWidget* listBox = gtk_list_box_new();
    gtk_widget_add_css_class(listBox, "password-list");
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), listBox);
    
    // Add password entries
    for (const auto& entry : passwords) {
        GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
        gtk_widget_set_margin_start(row, 10);
        gtk_widget_set_margin_end(row, 10);
        gtk_widget_set_margin_top(row, 10);
        gtk_widget_set_margin_bottom(row, 10);
        
        GtkWidget* infoBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_widget_set_hexpand(infoBox, TRUE);
        
        GtkWidget* urlLabel = gtk_label_new(nullptr);
        std::string urlMarkup = "<b>" + entry.url + "</b>";
        gtk_label_set_markup(GTK_LABEL(urlLabel), urlMarkup.c_str());
        gtk_widget_set_halign(urlLabel, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(infoBox), urlLabel);
        
        GtkWidget* userLabel = gtk_label_new(entry.username.c_str());
        gtk_widget_set_halign(userLabel, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(infoBox), userLabel);
        
        gtk_box_append(GTK_BOX(row), infoBox);
        
        // Delete button
        GtkWidget* deleteBtn = gtk_button_new_with_label("🗑️ Delete");
        gtk_widget_add_css_class(deleteBtn, "destructive-action");
        
        // Store entry info for delete callback
        g_object_set_data_full(G_OBJECT(deleteBtn), "url", g_strdup(entry.url.c_str()), g_free);
        g_object_set_data_full(G_OBJECT(deleteBtn), "username", g_strdup(entry.username.c_str()), g_free);
        g_object_set_data(G_OBJECT(deleteBtn), "manager", this);
        g_object_set_data(G_OBJECT(deleteBtn), "dialog", dialog);
        
        g_signal_connect(deleteBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
            auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
            const char* url = (const char*)g_object_get_data(G_OBJECT(btn), "url");
            const char* username = (const char*)g_object_get_data(G_OBJECT(btn), "username");
            GtkWidget* dialog = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "dialog"));
            
            manager->deletePassword(url, username);
            gtk_window_close(GTK_WINDOW(dialog));
            
            // Reopen to show updated list
            manager->showPasswordManager(GTK_WINDOW(gtk_widget_get_root(dialog)));
        }), nullptr);
        
        gtk_box_append(GTK_BOX(row), deleteBtn);
        
        gtk_list_box_append(GTK_LIST_BOX(listBox), row);
    }
    
    // Empty state
    if (passwords.empty()) {
        GtkWidget* emptyLabel = gtk_label_new("No saved passwords yet.\nPasswords will be saved automatically when you log in to websites.");
        gtk_widget_set_margin_top(emptyLabel, 50);
        gtk_widget_set_margin_bottom(emptyLabel, 50);
        gtk_list_box_append(GTK_LIST_BOX(listBox), emptyLabel);
    }
    
    gtk_box_append(GTK_BOX(mainBox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    // Buttons
    GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(buttonBox, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(mainBox), buttonBox);
    
    if (!passwords.empty()) {
        GtkWidget* clearBtn = gtk_button_new_with_label("Clear All");
        gtk_widget_add_css_class(clearBtn, "destructive-action");
        g_object_set_data(G_OBJECT(clearBtn), "manager", this);
        g_object_set_data(G_OBJECT(clearBtn), "dialog", dialog);
        g_signal_connect(clearBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
            auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
            GtkWidget* dialog = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "dialog"));
            
            manager->clearAll();
            gtk_window_close(GTK_WINDOW(dialog));
        }), nullptr);
        gtk_box_append(GTK_BOX(buttonBox), clearBtn);
    }
    
    GtkWidget* closeBtn = gtk_button_new_with_label("Close");
    g_signal_connect_swapped(closeBtn, "clicked", G_CALLBACK(gtk_window_close), dialog);
    gtk_box_append(GTK_BOX(buttonBox), closeBtn);
    
    gtk_window_present(GTK_WINDOW(dialog));
}
