#ifndef BRAYA_PASSWORD_MANAGER_H                                                                                                                   
#define BRAYA_PASSWORD_MANAGER_H                                                                                                                   
                                                                                                                                                   
#include <gtk/gtk.h>                                                                                                                               
#include <webkit/webkit.h>                                                                                                                         
#include <string>
#include <vector>
#include <ctime>
#include <set>

struct PasswordEntry {
    std::string url;
    std::string username;
    std::string password;
    long createdAt = 0;
    long updatedAt = 0;
    long lastUsedAt = 0;
    int strengthScore = 0;
    bool breached = false;
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

    // Vault lock helpers
    bool requestUnlock(GtkWindow* parent);
    bool isVaultLocked() const;
    
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

    // Password Generator
    std::string generatePassword(int length = 16, bool includeSymbols = true, bool includeNumbers = true, bool includeUppercase = true, bool includeLowercase = true);
    int calculatePasswordStrength(const std::string& password);
    std::string getPasswordStrengthLabel(int strength);

    // Utility methods
    std::string formatTimestampRelative(long timestamp) const;

    // Enable/disable password saving
    void setPasswordSavingEnabled(bool enabled);
    bool isMultiStepCaptureEnabled() const { return multiStepCaptureEnabled; }
    void setMultiStepCaptureEnabled(bool enabled);

    bool isSavingSuppressedForUrl(const std::string& url) const;
    void blockSavingForUrl(const std::string& url);
    std::set<std::string> getBlockedDomains() const { return blockedDomains; }
    void unblockDomain(const std::string& domain);
    void changeMasterPassword(GtkWindow* parent);
    void resetVault(GtkWindow* parent);
    
private:
    static constexpr int MASTER_VERIFIER_BYTES = 32; // libsodium generichash digest
    static constexpr int DEFAULT_LOCK_TIMEOUT_SECONDS = 15 * 60;
    static constexpr int SENSITIVE_UNLOCK_TIMEOUT_SECONDS = 90;

    bool passwordSavingEnabled;
    std::string storageFile;
    std::string configDirectory;
    std::vector<unsigned char> masterKey; // Store the derived master key
    bool masterKeySet = false;
    bool cryptoInitialized = false;
    bool legacyDataMigrated = false;
    std::vector<unsigned char> masterSalt;
    std::vector<unsigned char> masterVerifier;
    bool masterPasswordConfigured = false;
    bool vaultLocked = false;
    bool fallbackKeyActive = false;
    time_t lastUnlockTimestamp = 0;
    int lockTimeoutSeconds = DEFAULT_LOCK_TIMEOUT_SECONDS;
    time_t lastSensitiveUnlockTimestamp = 0;
    int clipboardTimeoutSeconds = 45;
    guint clipboardClearSource = 0;
    guint concealTimerSource = 0;
    GtkWindow* activeVaultDialog = nullptr;
    std::set<std::string> blockedDomains;
    std::string blockedListFile;
    std::string settingsFile;
    bool multiStepCaptureEnabled = true;
    
    void loadPasswords();
    void savePasswords();
    void initializeCrypto();
    void loadOrCreateMasterSalt();
    void persistMasterMetadata();
    
    // Enhanced encryption (TODO: migrate to libsodium for production)
    std::string encrypt(const std::string& data);
    std::string decrypt(const std::string& data);
    std::string encryptModern(const std::string& data);
    std::string decryptModern(const std::string& data);
    std::string decryptLegacy(const std::string& data);
    void deriveKey(const std::string& password);
    bool unlockWithMasterPassword(const std::string& password);
    bool promptForMasterUnlock(GtkWindow* parent);
    bool promptForSensitiveUnlock(GtkWindow* parent);
    bool promptToCreateMasterPassword(GtkWindow* parent);
    bool initializeMasterPassword(const std::string& password);
    void lockVault();
    bool isUnlockExpired() const;
    void refreshUnlockTimer();
    bool ensureUnlocked(GtkWindow* parent);
    bool requireSensitiveUnlock(GtkWindow* parent);
    bool authenticateMasterPassword(const std::string& password, bool reloadPasswords);
    bool verifyPassword(const std::string& password) const;
    void scheduleClipboardClear(GdkClipboard* clipboard, const std::string& label);
    void armIdleConcealTimer(GtkWindow* parent);
    void clearIdleConcealTimer();
    void loadSettings();
    void saveSettings() const;
    void loadBlockedDomains();
    void saveBlockedDomains() const;

    std::string getDomain(const std::string& url) const;
    void showErrorDialog(GtkWindow* parent, const std::string& title, const std::string& message);
    void showSuccessDialog(GtkWindow* parent, const std::string& title, const std::string& message);

    std::vector<PasswordEntry> passwords;

    std::string buildPasswordPreview(const std::string& password) const;
    void copyToClipboard(GtkWidget* widget, const std::string& value, const std::string& label);
    void updateDetailPane(GtkWidget* detailPane, GtkListBoxRow* row);
    void renderDetailPlaceholder(GtkWidget* detailPane);
    void renderDetailForEntry(GtkWidget* detailPane, const PasswordEntry& entry);
};
#endif // BRAYA_PASSWORD_MANAGER_H
