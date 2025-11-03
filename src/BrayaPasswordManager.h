#ifndef BRAYA_PASSWORD_MANAGER_H                                                                                                                   
#define BRAYA_PASSWORD_MANAGER_H                                                                                                                   
                                                                                                                                                   
#include <gtk/gtk.h>                                                                                                                               
#include <webkit/webkit.h>                                                                                                                         
#include <string>
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
    
    // Set the master password
    void setMasterPassword(const std::string& password);

    // Check if master key is set
    bool isMasterKeySet() const { return masterKeySet; }
    
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

    // Manual password management
    void addPasswordManually(GtkWindow* parent);
    void editPassword(const std::string& url, const std::string& username, GtkWindow* parent);
    void updatePassword(const std::string& url, const std::string& oldUsername, const std::string& newUsername, const std::string& newPassword);

    // Import/Export
    bool exportToCSV(const std::string& filepath);
    bool importFromCSV(const std::string& filepath);

    // Bitwarden integration
    bool syncWithBitwarden();
    bool exportToBitwarden();
    bool importFromBitwarden();
    bool isBitwardenCliAvailable();

    // Enable/disable password saving
    void setPasswordSavingEnabled(bool enabled);
    
private:
    bool passwordSavingEnabled;
    std::string storageFile;
    std::vector<unsigned char> masterKey; // Store the derived master key
    bool masterKeySet = false;
    
    void loadPasswords();
    void savePasswords();
    
    // Enhanced encryption (TODO: migrate to libsodium for production)
    std::string encrypt(const std::string& data);
    std::string decrypt(const std::string& data);
    void deriveKey(const std::string& password);

    std::string getDomain(const std::string& url);
    void showErrorDialog(GtkWindow* parent, const std::string& title, const std::string& message);
    void showSuccessDialog(GtkWindow* parent, const std::string& title, const std::string& message);

    std::vector<PasswordEntry> passwords;
};                                                                                                                                                   
#endif // BRAYA_PASSWORD_MANAGER_H