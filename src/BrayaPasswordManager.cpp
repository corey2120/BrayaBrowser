#include "BrayaPasswordManager.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <algorithm>
#include <sys/stat.h>
#include <cstring>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

BrayaPasswordManager::BrayaPasswordManager() 
    : passwordSavingEnabled(true) {
    
    // Storage in user config directory
    const char* homeDir = g_get_home_dir();
    std::string configDir = std::string(homeDir) + "/.config/braya-browser";
    
    // Create config directory if it doesn't exist
    mkdir(configDir.c_str(), 0700);
    
    storageFile = configDir + "/passwords.dat";
    
    // Generate a default master key from username+machine ID
    // In production, should prompt user for master password
    std::string keyMaterial = std::string(g_get_user_name()) + "_braya_browser";
    deriveKey(keyMaterial);
    
    loadPasswords();
}

void BrayaPasswordManager::deriveKey(const std::string& password) {
    // Derive a 32-byte key from password using SHA-256
    masterKey.resize(32);
    SHA256((unsigned char*)password.c_str(), password.size(), masterKey.data());
    masterKeySet = true;
}

void BrayaPasswordManager::setMasterPassword(const std::string& password) {
    if (password.empty()) {
        std::cerr << "✗ Master password cannot be empty" << std::endl;
        return;
    }

    deriveKey(password);
    std::cout << "✓ Master password set" << std::endl;
    
    // Reload passwords with new key
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

    // Remove port if present
    pos = domain.find(":");
    if (pos != std::string::npos) {
        domain = domain.substr(0, pos);
    }

    // Normalize: remove www. prefix for better matching
    if (domain.substr(0, 4) == "www.") {
        domain = domain.substr(4);
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
    if (!masterKeySet || masterKey.empty()) {
        std::cerr << "✗ Master key not set, cannot encrypt" << std::endl;
        return data;
    }

    // Generate random IV (16 bytes for AES)
    unsigned char iv[16];
    RAND_bytes(iv, 16);

    // Create cipher context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return "";

    // Initialize encryption
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, masterKey.data(), iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }

    // Allocate output buffer (plaintext + block size)
    std::vector<unsigned char> ciphertext(data.size() + 16);
    int len = 0;
    int ciphertext_len = 0;

    // Encrypt
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, (unsigned char*)data.c_str(), data.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    ciphertext_len = len;

    // Finalize
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    // Combine IV + ciphertext
    std::string result;
    result.reserve(16 + ciphertext_len);
    result.append((char*)iv, 16);
    result.append((char*)ciphertext.data(), ciphertext_len);

    return result;
}

std::string BrayaPasswordManager::decrypt(const std::string& data) {
    if (!masterKeySet || masterKey.empty()) {
        std::cerr << "✗ Master key not set, cannot decrypt" << std::endl;
        return data;
    }

    // Need at least IV
    if (data.size() < 16) {
        std::cerr << "✗ Invalid encrypted data (too short)" << std::endl;
        return "";
    }

    // Extract IV
    unsigned char iv[16];
    memcpy(iv, data.data(), 16);

    // Extract ciphertext
    const unsigned char* ciphertext = (unsigned char*)data.data() + 16;
    size_t ciphertext_len = data.size() - 16;

    // Create cipher context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return "";

    // Initialize decryption
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, masterKey.data(), iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }

    // Allocate output buffer
    std::vector<unsigned char> plaintext(ciphertext_len + 16);
    int len = 0;
    int plaintext_len = 0;

    // Decrypt
    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext, ciphertext_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    plaintext_len = len;

    // Finalize
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    return std::string((char*)plaintext.data(), plaintext_len);
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

            // Parse timestamp with error handling
            std::string timestampStr = decrypted.substr(pos3 + 1);
            try {
                entry.timestamp = timestampStr.empty() ? 0 : std::stol(timestampStr);
            } catch (const std::invalid_argument& e) {
                std::cerr << "⚠ Warning: Invalid timestamp for " << entry.url << ", using 0" << std::endl;
                entry.timestamp = 0;
            } catch (const std::out_of_range& e) {
                std::cerr << "⚠ Warning: Timestamp out of range for " << entry.url << ", using 0" << std::endl;
                entry.timestamp = 0;
            }

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
        // Escape special characters for markup
        gchar* escapedUrl = g_markup_escape_text(entry.url.c_str(), -1);
        std::string urlMarkup = std::string("<b>") + escapedUrl + "</b>";
        g_free(escapedUrl);
        gtk_label_set_markup(GTK_LABEL(urlLabel), urlMarkup.c_str());
        gtk_widget_set_halign(urlLabel, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(infoBox), urlLabel);
        
        GtkWidget* userLabel = gtk_label_new(entry.username.c_str());
        gtk_widget_set_halign(userLabel, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(infoBox), userLabel);
        
        gtk_box_append(GTK_BOX(row), infoBox);

        // Edit button
        GtkWidget* editBtn = gtk_button_new_with_label("✏️ Edit");
        gtk_widget_add_css_class(editBtn, "flat");

        g_object_set_data_full(G_OBJECT(editBtn), "url", g_strdup(entry.url.c_str()), g_free);
        g_object_set_data_full(G_OBJECT(editBtn), "username", g_strdup(entry.username.c_str()), g_free);
        g_object_set_data(G_OBJECT(editBtn), "manager", this);
        g_object_set_data(G_OBJECT(editBtn), "dialog", dialog);

        g_signal_connect(editBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
            auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
            const char* url = (const char*)g_object_get_data(G_OBJECT(btn), "url");
            const char* username = (const char*)g_object_get_data(G_OBJECT(btn), "username");
            GtkWidget* dialog = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "dialog"));

            manager->editPassword(url, username, GTK_WINDOW(dialog));
        }), nullptr);

        gtk_box_append(GTK_BOX(row), editBtn);

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

    // Action buttons row
    GtkWidget* actionBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_top(actionBox, 10);
    gtk_box_append(GTK_BOX(mainBox), actionBox);

    // Left side buttons (actions)
    GtkWidget* leftBtnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_hexpand(leftBtnBox, TRUE);
    gtk_box_append(GTK_BOX(actionBox), leftBtnBox);

    GtkWidget* addBtn = gtk_button_new_with_label("➕ Add");
    gtk_widget_add_css_class(addBtn, "suggested-action");
    g_object_set_data(G_OBJECT(addBtn), "manager", this);
    g_object_set_data(G_OBJECT(addBtn), "parent", dialog);
    g_signal_connect(addBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
        GtkWidget* parent = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "parent"));
        manager->addPasswordManually(GTK_WINDOW(parent));
    }), nullptr);
    gtk_box_append(GTK_BOX(leftBtnBox), addBtn);

    GtkWidget* importBtn = gtk_button_new_with_label("📥 Import");
    g_object_set_data(G_OBJECT(importBtn), "manager", this);
    g_object_set_data(G_OBJECT(importBtn), "parent", dialog);
    g_signal_connect(importBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
        GtkWidget* parent = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "parent"));

        GtkFileChooserNative* chooser = gtk_file_chooser_native_new(
            "Import Passwords",
            GTK_WINDOW(parent),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            "Import",
            "Cancel"
        );

        GtkFileFilter* filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, "CSV Files");
        gtk_file_filter_add_pattern(filter, "*.csv");
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);

        g_object_set_data(G_OBJECT(chooser), "parent", parent);
        g_signal_connect(chooser, "response", G_CALLBACK(+[](GtkFileChooserNative* chooser, int response, gpointer data) {
            if (response == GTK_RESPONSE_ACCEPT) {
                auto* manager = static_cast<BrayaPasswordManager*>(data);
                GtkWidget* parent = GTK_WIDGET(g_object_get_data(G_OBJECT(chooser), "parent"));
                GFile* file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(chooser));
                char* path = g_file_get_path(file);
                if (path) {
                    if (manager->importFromCSV(path)) {
                        manager->showSuccessDialog(GTK_WINDOW(parent), "Import Successful",
                            "Successfully imported passwords from CSV file!");
                    } else {
                        manager->showErrorDialog(GTK_WINDOW(parent), "Import Failed",
                            "Could not import passwords. Check the CSV file format.");
                    }
                    g_free(path);
                }
                g_object_unref(file);
            }
        }), manager);

        gtk_native_dialog_show(GTK_NATIVE_DIALOG(chooser));
    }), nullptr);
    gtk_box_append(GTK_BOX(leftBtnBox), importBtn);

    GtkWidget* exportBtn = gtk_button_new_with_label("📤 Export");
    g_object_set_data(G_OBJECT(exportBtn), "manager", this);
    g_object_set_data(G_OBJECT(exportBtn), "parent", dialog);
    g_signal_connect(exportBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
        GtkWidget* parent = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "parent"));

        GtkFileChooserNative* chooser = gtk_file_chooser_native_new(
            "Export Passwords",
            GTK_WINDOW(parent),
            GTK_FILE_CHOOSER_ACTION_SAVE,
            "Export",
            "Cancel"
        );

        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(chooser), "braya-passwords.csv");

        g_object_set_data(G_OBJECT(chooser), "parent", parent);
        g_signal_connect(chooser, "response", G_CALLBACK(+[](GtkFileChooserNative* chooser, int response, gpointer data) {
            if (response == GTK_RESPONSE_ACCEPT) {
                auto* manager = static_cast<BrayaPasswordManager*>(data);
                GtkWidget* parent = GTK_WIDGET(g_object_get_data(G_OBJECT(chooser), "parent"));
                GFile* file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(chooser));
                char* path = g_file_get_path(file);
                if (path) {
                    if (manager->exportToCSV(path)) {
                        std::string msg = "Successfully exported passwords to:\n" + std::string(path);
                        manager->showSuccessDialog(GTK_WINDOW(parent), "Export Successful", msg);
                    } else {
                        manager->showErrorDialog(GTK_WINDOW(parent), "Export Failed",
                            "Could not export passwords to the selected file.");
                    }
                    g_free(path);
                }
                g_object_unref(file);
            }
        }), manager);

        gtk_native_dialog_show(GTK_NATIVE_DIALOG(chooser));
    }), nullptr);
    gtk_box_append(GTK_BOX(leftBtnBox), exportBtn);

    // Bitwarden button
    GtkWidget* bitwardenBtn = gtk_button_new_with_label("🔐 Bitwarden");
    g_object_set_data(G_OBJECT(bitwardenBtn), "manager", this);
    g_object_set_data(G_OBJECT(bitwardenBtn), "parent", dialog);
    g_signal_connect(bitwardenBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
        GtkWidget* parent = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "parent"));

        // Show Bitwarden options menu
        GtkWidget* menu = gtk_popover_new();
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_widget_set_margin_start(box, 10);
        gtk_widget_set_margin_end(box, 10);
        gtk_widget_set_margin_top(box, 10);
        gtk_widget_set_margin_bottom(box, 10);

        GtkWidget* syncBtn = gtk_button_new_with_label("🔄 Sync from Bitwarden");
        g_object_set_data(G_OBJECT(syncBtn), "manager", manager);
        g_object_set_data(G_OBJECT(syncBtn), "menu", menu);
        g_object_set_data(G_OBJECT(syncBtn), "parent", parent);
        g_signal_connect(syncBtn, "clicked", G_CALLBACK(+[](GtkButton* b, gpointer d) {
            auto* mgr = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(b), "manager"));
            GtkWidget* m = GTK_WIDGET(g_object_get_data(G_OBJECT(b), "menu"));
            GtkWidget* parent = GTK_WIDGET(g_object_get_data(G_OBJECT(b), "parent"));

            gtk_popover_popdown(GTK_POPOVER(m));

            if (mgr->syncWithBitwarden()) {
                mgr->showSuccessDialog(GTK_WINDOW(parent), "Bitwarden Sync",
                    "Successfully imported passwords from Bitwarden!");
            } else {
                mgr->showErrorDialog(GTK_WINDOW(parent), "Bitwarden Sync Failed",
                    "Could not sync with Bitwarden. Check console for details.");
            }
        }), nullptr);
        gtk_box_append(GTK_BOX(box), syncBtn);

        GtkWidget* importBWBtn = gtk_button_new_with_label("📥 Import from Bitwarden");
        g_object_set_data(G_OBJECT(importBWBtn), "manager", manager);
        g_object_set_data(G_OBJECT(importBWBtn), "menu", menu);
        g_object_set_data(G_OBJECT(importBWBtn), "parent", parent);
        g_signal_connect(importBWBtn, "clicked", G_CALLBACK(+[](GtkButton* b, gpointer d) {
            auto* mgr = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(b), "manager"));
            GtkWidget* m = GTK_WIDGET(g_object_get_data(G_OBJECT(b), "menu"));
            GtkWidget* parent = GTK_WIDGET(g_object_get_data(G_OBJECT(b), "parent"));

            gtk_popover_popdown(GTK_POPOVER(m));

            if (mgr->importFromBitwarden()) {
                mgr->showSuccessDialog(GTK_WINDOW(parent), "Import Successful",
                    "Successfully imported passwords from Bitwarden!");
            } else {
                mgr->showErrorDialog(GTK_WINDOW(parent), "Import Failed",
                    "Could not import from Bitwarden. Check console for details.");
            }
        }), nullptr);
        gtk_box_append(GTK_BOX(box), importBWBtn);

        GtkWidget* exportBWBtn = gtk_button_new_with_label("📤 Export to Bitwarden");
        g_object_set_data(G_OBJECT(exportBWBtn), "manager", manager);
        g_object_set_data(G_OBJECT(exportBWBtn), "menu", menu);
        g_object_set_data(G_OBJECT(exportBWBtn), "parent", parent);
        g_signal_connect(exportBWBtn, "clicked", G_CALLBACK(+[](GtkButton* b, gpointer d) {
            auto* mgr = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(b), "manager"));
            GtkWidget* m = GTK_WIDGET(g_object_get_data(G_OBJECT(b), "menu"));
            GtkWidget* parent = GTK_WIDGET(g_object_get_data(G_OBJECT(b), "parent"));

            gtk_popover_popdown(GTK_POPOVER(m));

            if (mgr->exportToBitwarden()) {
                mgr->showSuccessDialog(GTK_WINDOW(parent), "Export Successful",
                    "Passwords exported to ~/braya-passwords-for-bitwarden.csv\n" 
                    "Import this file in Bitwarden: Tools > Import Data");
            } else {
                mgr->showErrorDialog(GTK_WINDOW(parent), "Export Failed",
                    "Could not export passwords. Check console for details.");
            }
        }), nullptr);
        gtk_box_append(GTK_BOX(box), exportBWBtn);

        gtk_popover_set_child(GTK_POPOVER(menu), box);
        gtk_widget_set_parent(menu, GTK_WIDGET(btn));
        gtk_popover_popup(GTK_POPOVER(menu));
    }), nullptr);
    gtk_box_append(GTK_BOX(leftBtnBox), bitwardenBtn);

    // Right side buttons
    GtkWidget* rightBtnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(actionBox), rightBtnBox);

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
        gtk_box_append(GTK_BOX(rightBtnBox), clearBtn);
    }

    GtkWidget* closeBtn = gtk_button_new_with_label("Close");
    g_signal_connect_swapped(closeBtn, "clicked", G_CALLBACK(gtk_window_close), dialog);
    gtk_box_append(GTK_BOX(rightBtnBox), closeBtn);

    gtk_window_present(GTK_WINDOW(dialog));
}

// Add password manually
void BrayaPasswordManager::addPasswordManually(GtkWindow* parent) {
    GtkWidget* dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Add Password");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 250);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_window_set_child(GTK_WINDOW(dialog), box);

    // URL field
    GtkWidget* urlLabel = gtk_label_new("URL or Domain:");
    gtk_widget_set_halign(urlLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), urlLabel);

    GtkWidget* urlEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(urlEntry), "example.com");
    gtk_box_append(GTK_BOX(box), urlEntry);

    // Username field
    GtkWidget* usernameLabel = gtk_label_new("Username or Email:");
    gtk_widget_set_halign(usernameLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), usernameLabel);

    GtkWidget* usernameEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(usernameEntry), "user@example.com");
    gtk_box_append(GTK_BOX(box), usernameEntry);

    // Password field
    GtkWidget* passwordLabel = gtk_label_new("Password:");
    gtk_widget_set_halign(passwordLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), passwordLabel);

    GtkWidget* passwordEntry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(passwordEntry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(passwordEntry), "••••••••");
    gtk_box_append(GTK_BOX(box), passwordEntry);

    // Buttons
    GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(buttonBox, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(box), buttonBox);

    GtkWidget* cancelBtn = gtk_button_new_with_label("Cancel");
    g_signal_connect_swapped(cancelBtn, "clicked", G_CALLBACK(gtk_window_close), dialog);
    gtk_box_append(GTK_BOX(buttonBox), cancelBtn);

    GtkWidget* saveBtn = gtk_button_new_with_label("Add");
    gtk_widget_add_css_class(saveBtn, "suggested-action");

    g_object_set_data(G_OBJECT(saveBtn), "manager", this);
    g_object_set_data(G_OBJECT(saveBtn), "url_entry", urlEntry);
    g_object_set_data(G_OBJECT(saveBtn), "username_entry", usernameEntry);
    g_object_set_data(G_OBJECT(saveBtn), "password_entry", passwordEntry);
    g_object_set_data(G_OBJECT(saveBtn), "dialog", dialog);
    g_object_set_data(G_OBJECT(saveBtn), "parent", parent);

    g_signal_connect(saveBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
        GtkEntry* urlEntry = GTK_ENTRY(g_object_get_data(G_OBJECT(btn), "url_entry"));
        GtkEntry* userEntry = GTK_ENTRY(g_object_get_data(G_OBJECT(btn), "username_entry"));
        GtkEntry* passEntry = GTK_ENTRY(g_object_get_data(G_OBJECT(btn), "password_entry"));
        GtkWidget* dialog = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "dialog"));
        GtkWindow* parent = GTK_WINDOW(g_object_get_data(G_OBJECT(btn), "parent"));

        const char* url = gtk_editable_get_text(GTK_EDITABLE(urlEntry));
        const char* username = gtk_editable_get_text(GTK_EDITABLE(userEntry));
        const char* password = gtk_editable_get_text(GTK_EDITABLE(passEntry));

        if (url && username && password && strlen(url) > 0 && strlen(username) > 0 && strlen(password) > 0) {
            manager->savePassword(url, username, password);
            gtk_window_close(GTK_WINDOW(dialog));
            manager->showPasswordManager(parent);
        }
    }), nullptr);

    gtk_box_append(GTK_BOX(buttonBox), saveBtn);

    gtk_window_present(GTK_WINDOW(dialog));
}

// Edit password
void BrayaPasswordManager::editPassword(const std::string& url, const std::string& username, GtkWindow* parent) {
    // Find the existing entry
    PasswordEntry* entry = nullptr;
    for (auto& e : passwords) {
        if (e.url == url && e.username == username) {
            entry = &e;
            break;
        }
    }

    if (!entry) return;

    GtkWidget* dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Edit Password");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 250);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_window_set_child(GTK_WINDOW(dialog), box);

    // URL field (readonly)
    GtkWidget* urlLabel = gtk_label_new("URL or Domain:");
    gtk_widget_set_halign(urlLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), urlLabel);

    GtkWidget* urlEntry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(urlEntry), entry->url.c_str());
    gtk_editable_set_editable(GTK_EDITABLE(urlEntry), FALSE);
    gtk_box_append(GTK_BOX(box), urlEntry);

    // Username field
    GtkWidget* usernameLabel = gtk_label_new("Username or Email:");
    gtk_widget_set_halign(usernameLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), usernameLabel);

    GtkWidget* usernameEntry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(usernameEntry), entry->username.c_str());
    gtk_box_append(GTK_BOX(box), usernameEntry);

    // Password field
    GtkWidget* passwordLabel = gtk_label_new("Password:");
    gtk_widget_set_halign(passwordLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), passwordLabel);

    GtkWidget* passwordEntry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(passwordEntry), FALSE);
    gtk_editable_set_text(GTK_EDITABLE(passwordEntry), entry->password.c_str());
    gtk_box_append(GTK_BOX(box), passwordEntry);

    // Buttons
    GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(buttonBox, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(box), buttonBox);

    GtkWidget* cancelBtn = gtk_button_new_with_label("Cancel");
    g_signal_connect_swapped(cancelBtn, "clicked", G_CALLBACK(gtk_window_close), dialog);
    gtk_box_append(GTK_BOX(buttonBox), cancelBtn);

    GtkWidget* saveBtn = gtk_button_new_with_label("Save");
    gtk_widget_add_css_class(saveBtn, "suggested-action");

    g_object_set_data(G_OBJECT(saveBtn), "manager", this);
    g_object_set_data_full(G_OBJECT(saveBtn), "old_username", g_strdup(username.c_str()), g_free);
    g_object_set_data_full(G_OBJECT(saveBtn), "url", g_strdup(url.c_str()), g_free);
    g_object_set_data(G_OBJECT(saveBtn), "username_entry", usernameEntry);
    g_object_set_data(G_OBJECT(saveBtn), "password_entry", passwordEntry);
    g_object_set_data(G_OBJECT(saveBtn), "dialog", dialog);
    g_object_set_data(G_OBJECT(saveBtn), "parent", parent);

    g_signal_connect(saveBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
        const char* oldUsername = (const char*)g_object_get_data(G_OBJECT(btn), "old_username");
        const char* url = (const char*)g_object_get_data(G_OBJECT(btn), "url");
        GtkEntry* userEntry = GTK_ENTRY(g_object_get_data(G_OBJECT(btn), "username_entry"));
        GtkEntry* passEntry = GTK_ENTRY(g_object_get_data(G_OBJECT(btn), "password_entry"));
        GtkWidget* dialog = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "dialog"));
        GtkWindow* parent = GTK_WINDOW(g_object_get_data(G_OBJECT(btn), "parent"));

        const char* newUsername = gtk_editable_get_text(GTK_EDITABLE(userEntry));
        const char* newPassword = gtk_editable_get_text(GTK_EDITABLE(passEntry));

        if (newUsername && newPassword && strlen(newUsername) > 0 && strlen(newPassword) > 0) {
            manager->updatePassword(url, oldUsername, newUsername, newPassword);
            gtk_window_close(GTK_WINDOW(dialog));
            manager->showPasswordManager(parent);
        }
    }), nullptr);

    gtk_box_append(GTK_BOX(buttonBox), saveBtn);

    gtk_window_present(GTK_WINDOW(dialog));
}

// Update password
void BrayaPasswordManager::updatePassword(const std::string& url, const std::string& oldUsername,
                                         const std::string& newUsername, const std::string& newPassword) {
    for (auto& entry : passwords) {
        if (entry.url == url && entry.username == oldUsername) {
            entry.username = newUsername;
            entry.password = newPassword;
            entry.timestamp = time(nullptr);
            savePasswords();
            std::cout << "✓ Password updated for " << newUsername << " @ " << url << std::endl;
            return;
        }
    }
}

// Export to CSV
bool BrayaPasswordManager::exportToCSV(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "✗ Failed to open file for export: " << filepath << std::endl;
        return false;
    }

    // Write CSV header
    file << "url,username,password\n";

    // Write entries
    for (const auto& entry : passwords) {
        // Escape commas and quotes in fields
        auto escape = [](const std::string& field) {
            std::string escaped = field;
            if (escaped.find(',') != std::string::npos || escaped.find('"') != std::string::npos) {
                // Replace " with ""
                size_t pos = 0;
                while ((pos = escaped.find('"', pos)) != std::string::npos) {
                    escaped.replace(pos, 1, "\"\"");
                    pos += 2;
                }
                escaped = "\"" + escaped + "\"";
            }
            return escaped;
        };

        file << escape(entry.url) << ","
             << escape(entry.username) << ","
             << escape(entry.password) << "\n";
    }

    file.close();
    std::cout << "✓ Exported " << passwords.size() << " passwords to " << filepath << std::endl;
    return true;
}

// Import from CSV
bool BrayaPasswordManager::importFromCSV(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "✗ Failed to open file for import: " << filepath << std::endl;
        return false;
    }

    std::string line;
    bool firstLine = true;
    int imported = 0;
    bool isChromeFormat = false;

    while (std::getline(file, line)) {
        // Check header to detect format
        if (firstLine) {
            firstLine = false;
            // Chrome format: name,url,username,password
            // Braya format: url,username,password
            if (line.find("name,url,username,password") != std::string::npos) {
                isChromeFormat = true;
                std::cout << "✓ Detected Chrome CSV format" << std::endl;
            } else {
                std::cout << "✓ Detected Braya CSV format" << std::endl;
            }
            continue;
        }

        if (line.empty()) continue;

        // Simple CSV parser
        std::vector<std::string> fields;
        std::string field;
        bool inQuotes = false;

        for (size_t i = 0; i < line.length(); i++) {
            char c = line[i];

            if (c == '"') {
                inQuotes = !inQuotes;
            } else if (c == ',' && !inQuotes) {
                fields.push_back(field);
                field.clear();
            } else {
                field += c;
            }
        }
        fields.push_back(field);

        // Parse based on format
        if (isChromeFormat && fields.size() >= 4) {
            // Chrome: name, url, username, password
            std::string url = fields[1];
            std::string username = fields[2];
            std::string password = fields[3];
            
            if (!url.empty() && !username.empty() && !password.empty()) {
                savePassword(url, username, password);
                imported++;
            }
        } else if (!isChromeFormat && fields.size() >= 3) {
            // Braya: url, username, password
            std::string url = fields[0];
            std::string username = fields[1];
            std::string password = fields[2];
            
            if (!url.empty() && !username.empty() && !password.empty()) {
                savePassword(url, username, password);
                imported++;
            }
        }
    }

    file.close();
    std::cout << "✓ Imported " << imported << " passwords from " << filepath << std::endl;
    return true;
}

// Check if Bitwarden CLI is available
bool BrayaPasswordManager::isBitwardenCliAvailable() {
    int result = system("which bw > /dev/null 2>&1");
    return result == 0;
}

// Sync with Bitwarden
bool BrayaPasswordManager::syncWithBitwarden() {
    if (!isBitwardenCliAvailable()) {
        std::cerr << "✗ Bitwarden CLI (bw) is not installed" << std::endl;
        std::cerr << "  Install with: npm install -g @bitwarden/cli" << std::endl;
        return false;
    }

    std::cout << "🔄 Syncing with Bitwarden..." << std::endl;

    // First, sync the vault
    int syncResult = system("bw sync");
    if (syncResult != 0) {
        std::cerr << "✗ Failed to sync Bitwarden vault" << std::endl;
        std::cerr << "  Make sure you're logged in: bw login" << std::endl;
        return false;
    }

    // Import after sync
    return importFromBitwarden();
}

// Import from Bitwarden
bool BrayaPasswordManager::importFromBitwarden() {
    if (!isBitwardenCliAvailable()) {
        std::cerr << "✗ Bitwarden CLI (bw) is not installed" << std::endl;
        std::cerr << "  Install with: npm install -g @bitwarden/cli" << std::endl;
        std::cerr << "  Or download from: https://bitwarden.com/download/" << std::endl;
        return false;
    }

    // Check for jq (needed for JSON parsing)
    int jqCheck = system("which jq > /dev/null 2>&1");
    if (jqCheck != 0) {
        std::cerr << "✗ jq is not installed (needed for JSON parsing)" << std::endl;
        std::cerr << "  Install with: sudo dnf install jq" << std::endl;
        return false;
    }

    std::cout << "📥 Importing from Bitwarden..." << std::endl;

    // Get home directory and create cache directory if needed
    const char* homeDir = g_get_home_dir();
    std::string cacheDir = std::string(homeDir) + "/.cache/braya-browser";
    mkdir(cacheDir.c_str(), 0700);

    std::string tempFile = cacheDir + "/bw-export.csv";

    // Check if user is logged in
    std::cout << "  Checking Bitwarden login status..." << std::endl;
    int statusCheck = system("bw status 2>/dev/null | grep -q 'unauthenticated'");
    if (statusCheck == 0) {
        std::cerr << "✗ Not logged in to Bitwarden" << std::endl;
        std::cerr << "  Login with: bw login" << std::endl;
        return false;
    }

    // Try to get session key (will prompt for password if locked)
    std::cout << "  Getting Bitwarden session..." << std::endl;
    std::cout << "  You may be prompted to unlock your vault" << std::endl;

    std::string sessionFile = cacheDir + "/bw-session.tmp";
    std::string getSessionCmd = "bw unlock --raw 2>/dev/null > " + sessionFile;
    int sessionResult = system(getSessionCmd.c_str());

    if (sessionResult != 0) {
        std::cerr << "✗ Failed to unlock Bitwarden vault" << std::endl;
        std::cerr << "  Try running manually: bw unlock" << std::endl;
        remove(sessionFile.c_str());
        return false;
    }

    // Read session key
    std::ifstream sessionIn(sessionFile);
    std::string sessionKey;
    if (sessionIn.is_open()) {
        std::getline(sessionIn, sessionKey);
        sessionIn.close();
    }
    remove(sessionFile.c_str());

    if (sessionKey.empty()) {
        std::cerr << "✗ Failed to get Bitwarden session key" << std::endl;
        return false;
    }

    // Export items to CSV
    std::cout << "  Exporting login items..." << std::endl;
    std::string exportCmd = "bw list items --session '" + sessionKey + "' 2>/dev/null | "
                           "jq -r '.[] | select(.type == 1) | "
                           "[(.login.uris[0].uri // .name), .login.username, .login.password] | "
                           "@csv' > " + tempFile;

    int result = system(exportCmd.c_str());
    if (result != 0) {
        std::cerr << "✗ Failed to export from Bitwarden" << std::endl;
        remove(tempFile.c_str());
        return false;
    }

    // Check if file has content
    std::ifstream checkFile(tempFile);
    std::string firstLine;
    std::getline(checkFile, firstLine);
    checkFile.close();

    if (firstLine.empty()) {
        std::cerr << "✗ No passwords found in Bitwarden vault" << std::endl;
        remove(tempFile.c_str());
        return false;
    }

    // Import the CSV
    std::cout << "  Importing passwords..." << std::endl;
    bool success = importFromCSV(tempFile);

    // Clean up temp file
    remove(tempFile.c_str());

    if (success) {
        std::cout << "✓ Successfully imported from Bitwarden" << std::endl;
    }

    return success;
}

// Export to Bitwarden
bool BrayaPasswordManager::exportToBitwarden() {
    if (!isBitwardenCliAvailable()) {
        std::cerr << "✗ Bitwarden CLI (bw) is not installed" << std::endl;
        return false;
    }

    std::cout << "📤 Exporting to Bitwarden..." << std::endl;
    std::cout << "  This feature requires Bitwarden organization and API access" << std::endl;
    std::cout << "  For now, use Export to CSV and import manually in Bitwarden web vault" << std::endl;

    // For v1.0.1, we'll just export to CSV and let user import manually
    const char* homeDir = g_get_home_dir();
    std::string exportFile = std::string(homeDir) + "/braya-passwords-for-bitwarden.csv";

    if (exportToCSV(exportFile)) {
        std::cout << "✓ Exported to: " << exportFile << std::endl;
        std::cout << "  Import this file in Bitwarden: Tools > Import Data" << std::endl;
        return true;
    }

    return false;
}

// Show error dialog
void BrayaPasswordManager::showErrorDialog(GtkWindow* parent, const std::string& title, const std::string& message) {
    GtkWidget* dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), title.c_str());
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 150);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_window_set_child(GTK_WINDOW(dialog), box);

    GtkWidget* label = gtk_label_new(message.c_str());
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(label), 50);
    gtk_box_append(GTK_BOX(box), label);

    GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(buttonBox, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(box), buttonBox);

    GtkWidget* okBtn = gtk_button_new_with_label("OK");
    gtk_widget_add_css_class(okBtn, "suggested-action");
    g_signal_connect_swapped(okBtn, "clicked", G_CALLBACK(gtk_window_close), dialog);
    gtk_box_append(GTK_BOX(buttonBox), okBtn);

    gtk_window_present(GTK_WINDOW(dialog));
}

// Show success dialog
void BrayaPasswordManager::showSuccessDialog(GtkWindow* parent, const std::string& title, const std::string& message) {
    GtkWidget* dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), title.c_str());
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 150);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_window_set_child(GTK_WINDOW(dialog), box);

    GtkWidget* label = gtk_label_new(message.c_str());
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(label), 50);
    gtk_box_append(GTK_BOX(box), label);

    GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(buttonBox, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(box), buttonBox);

    GtkWidget* okBtn = gtk_button_new_with_label("OK");
    gtk_widget_add_css_class(okBtn, "suggested-action");
    g_signal_connect_swapped(okBtn, "clicked", G_CALLBACK(gtk_window_close), dialog);
    gtk_box_append(GTK_BOX(buttonBox), okBtn);

    gtk_window_present(GTK_WINDOW(dialog));
}