#ifndef BRAYA_PASSWORD_MANAGER_H
#define BRAYA_PASSWORD_MANAGER_H

#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <string>
#include <map>
#include <vector>

struct PasswordEntry {
    std::string url;
    std::string username;
    std::string password;
    long timestamp;
};

class BrayaPasswordManager {
public:
    BrayaPasswordManager();
    ~BrayaPasswordManager();
    
    // Save a password
    void savePassword(const std::string& url, const std::string& username, const std::string& password);
    
    // Get saved credentials for a URL
    std::vector<PasswordEntry> getPasswordsForUrl(const std::string& url);
    
    // Get all saved passwords
    std::vector<PasswordEntry> getAllPasswords();
    
    // Delete a password
    void deletePassword(const std::string& url, const std::string& username);
    
    // Clear all passwords
    void clearAll();
    
    // Show password manager UI
    void showPasswordManager(GtkWindow* parent);
    
    // Enable/disable password saving
    void setPasswordSavingEnabled(bool enabled);
    bool isPasswordSavingEnabled() const { return passwordSavingEnabled; }
    
private:
    bool passwordSavingEnabled;
    std::string storageFile;
    
    void loadPasswords();
    void savePasswords();
    std::string encrypt(const std::string& data);
    std::string decrypt(const std::string& data);
    std::string getDomain(const std::string& url);
    
    std::vector<PasswordEntry> passwords;
};

#endif // BRAYA_PASSWORD_MANAGER_H
