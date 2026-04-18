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
#include <sodium.h>
#include <cstdio>
#include <cctype>

namespace {
constexpr const char* kMasterVerifierContext = "BrayaVaultVerifier";

GtkWindow* create_modal_dialog(const char* title, GtkWindow* parent, int width, int height) {
    GtkWindow* window = GTK_WINDOW(gtk_window_new());
    gtk_window_set_modal(window, TRUE);
    if (parent) {
        gtk_window_set_transient_for(window, parent);
    }
    if (width > 0 && height > 0) {
        gtk_window_set_default_size(window, width, height);
    }

    GtkWidget* header = gtk_header_bar_new();
    gtk_widget_add_css_class(header, "dialog-header");

    GtkWidget* titleLabel = gtk_label_new(title);
    gtk_widget_add_css_class(titleLabel, "dialog-header-title");
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header), titleLabel);

    GtkWidget* closeBtn = gtk_button_new_from_icon_name("window-close-symbolic");
    gtk_widget_add_css_class(closeBtn, "flat");
    g_signal_connect_swapped(closeBtn, "clicked", G_CALLBACK(gtk_window_close), window);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), closeBtn);

    gtk_window_set_titlebar(window, header);
    gtk_window_set_title(window, title);
    gtk_widget_add_css_class(GTK_WIDGET(window), "braya-dialog");
    return window;
}

void clear_container(GtkWidget* container) {
    if (!container) {
        return;
    }
    GtkWidget* child = gtk_widget_get_first_child(container);
    while (child) {
        GtkWidget* next = gtk_widget_get_next_sibling(child);
        gtk_widget_unparent(child);
        child = next;
    }
}
} // namespace

BrayaPasswordManager::BrayaPasswordManager() 
    : passwordSavingEnabled(true) {
    
    // Storage in user config directory
    const char* homeDir = g_get_home_dir();
    std::string configDir = std::string(homeDir) + "/.config/braya-browser";
    
    // Create config directory if it doesn't exist
    mkdir(configDir.c_str(), 0700);
    
    configDirectory = configDir;
    storageFile = configDir + "/passwords.dat";
    blockedListFile = configDir + "/passwords.ignore";
    settingsFile = configDir + "/passwords.config";
    initializeCrypto();
    loadSettings();
    loadBlockedDomains();
    
    if (!masterPasswordConfigured) {
        // Derive a fallback key so existing installs keep working until a master password is created
        std::string keyMaterial = std::string(g_get_user_name()) + "_braya_browser";
        deriveKey(keyMaterial);
        fallbackKeyActive = masterKeySet;
        vaultLocked = false;
        refreshUnlockTimer();
    } else {
        lockVault();
    }
    
    loadPasswords();
}

void BrayaPasswordManager::deriveKey(const std::string& password) {
    initializeCrypto();
    masterKey.resize(crypto_aead_xchacha20poly1305_ietf_KEYBYTES);

    if (crypto_pwhash(masterKey.data(), masterKey.size(),
                      password.c_str(), password.size(),
                      masterSalt.data(),
                      crypto_pwhash_OPSLIMIT_MODERATE,
                      crypto_pwhash_MEMLIMIT_MODERATE,
                      crypto_pwhash_ALG_DEFAULT) != 0) {
        std::cerr << "✗ Failed to derive master key" << std::endl;
        masterKeySet = false;
        return;
    }

    masterKeySet = true;
}

void BrayaPasswordManager::setMasterPassword(const std::string& password) {
    if (password.empty()) {
        std::cerr << "✗ Master password cannot be empty" << std::endl;
        return;
    }

    if (initializeMasterPassword(password)) {
        std::cout << "✓ Master password set" << std::endl;
    } else {
        std::cerr << "✗ Failed to set master password" << std::endl;
    }
}

BrayaPasswordManager::~BrayaPasswordManager() {
    if (clipboardClearSource != 0) {
        g_source_remove(clipboardClearSource);
        clipboardClearSource = 0;
    }
    clearIdleConcealTimer();
    saveBlockedDomains();
    saveSettings();
    savePasswords();
}

void BrayaPasswordManager::persistMasterMetadata() {
    const std::string saltPath = configDirectory + "/master.key";
    std::ofstream out(saltPath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        std::cerr << "✗ Failed to persist master key metadata" << std::endl;
        return;
    }

    if (!masterSalt.empty()) {
        out.write(reinterpret_cast<const char*>(masterSalt.data()), masterSalt.size());
    }

    if (!masterVerifier.empty()) {
        out.write(reinterpret_cast<const char*>(masterVerifier.data()), masterVerifier.size());
    }

    out.close();
    chmod(saltPath.c_str(), S_IRUSR | S_IWUSR);
}

void BrayaPasswordManager::lockVault() {
    if (!masterKey.empty()) {
        sodium_memzero(masterKey.data(), masterKey.size());
        masterKey.clear();
    }
    masterKeySet = false;

    if (masterPasswordConfigured) {
        vaultLocked = true;
        passwords.clear();
    } else {
        vaultLocked = false;
    }
    lastSensitiveUnlockTimestamp = 0;
    clearIdleConcealTimer();
}

void BrayaPasswordManager::refreshUnlockTimer() {
    lastUnlockTimestamp = time(nullptr);
}

bool BrayaPasswordManager::isUnlockExpired() const {
    if (!masterPasswordConfigured) {
        return false;
    }

    if (vaultLocked || lastUnlockTimestamp == 0) {
        return true;
    }

    time_t now = time(nullptr);
    return difftime(now, lastUnlockTimestamp) >= lockTimeoutSeconds;
}

bool BrayaPasswordManager::authenticateMasterPassword(const std::string& password, bool reloadPasswords) {
    if (!masterPasswordConfigured || password.empty() || masterSalt.size() != crypto_pwhash_SALTBYTES ||
        masterVerifier.size() != MASTER_VERIFIER_BYTES) {
        return false;
    }

    std::vector<unsigned char> derived(crypto_aead_xchacha20poly1305_ietf_KEYBYTES);
    if (crypto_pwhash(derived.data(), derived.size(),
                      password.c_str(), password.size(),
                      masterSalt.data(),
                      crypto_pwhash_OPSLIMIT_MODERATE,
                      crypto_pwhash_MEMLIMIT_MODERATE,
                      crypto_pwhash_ALG_DEFAULT) != 0) {
        return false;
    }

    std::vector<unsigned char> computed(masterVerifier.size());
    const unsigned char* context = reinterpret_cast<const unsigned char*>(kMasterVerifierContext);
    crypto_generichash(
        computed.data(),
        computed.size(),
        derived.data(),
        derived.size(),
        context,
        strlen(kMasterVerifierContext)
    );

    bool matches = sodium_memcmp(computed.data(), masterVerifier.data(), masterVerifier.size()) == 0;
    if (!matches) {
        sodium_memzero(derived.data(), derived.size());
        return false;
    }

    masterKey.assign(derived.begin(), derived.end());
    sodium_memzero(derived.data(), derived.size());
    masterKeySet = true;
    fallbackKeyActive = false;
    vaultLocked = false;
    refreshUnlockTimer();
    if (reloadPasswords) {
        loadPasswords();
    }
    return true;
}

bool BrayaPasswordManager::verifyPassword(const std::string& password) const {
    if (masterSalt.size() != crypto_pwhash_SALTBYTES || masterVerifier.size() != MASTER_VERIFIER_BYTES) {
        return false;
    }

    std::vector<unsigned char> derived(crypto_aead_xchacha20poly1305_ietf_KEYBYTES);
    if (crypto_pwhash(derived.data(), derived.size(),
                      password.c_str(), password.size(),
                      masterSalt.data(),
                      crypto_pwhash_OPSLIMIT_MODERATE,
                      crypto_pwhash_MEMLIMIT_MODERATE,
                      crypto_pwhash_ALG_DEFAULT) != 0) {
        return false;
    }

    std::vector<unsigned char> digest(masterVerifier.size());
    crypto_generichash(digest.data(), digest.size(),
                       derived.data(), derived.size(),
                       reinterpret_cast<const unsigned char*>(kMasterVerifierContext),
                       strlen(kMasterVerifierContext));

    bool ok = sodium_memcmp(digest.data(), masterVerifier.data(), masterVerifier.size()) == 0;
    sodium_memzero(derived.data(), derived.size());
    return ok;
}

bool BrayaPasswordManager::unlockWithMasterPassword(const std::string& password) {
    if (authenticateMasterPassword(password, true)) {
        return true;
    }
    lockVault();
    return false;
}

bool BrayaPasswordManager::initializeMasterPassword(const std::string& password) {
    if (password.size() < 8) {
        return false;
    }

    if (masterSalt.size() != crypto_pwhash_SALTBYTES) {
        masterSalt.resize(crypto_pwhash_SALTBYTES);
    }

    randombytes_buf(masterSalt.data(), masterSalt.size());
    deriveKey(password);
    if (!masterKeySet) {
        return false;
    }

    masterVerifier.resize(MASTER_VERIFIER_BYTES);
    const unsigned char* context = reinterpret_cast<const unsigned char*>(kMasterVerifierContext);
    crypto_generichash(
        masterVerifier.data(),
        masterVerifier.size(),
        masterKey.data(),
        masterKey.size(),
        context,
        strlen(kMasterVerifierContext)
    );

    masterPasswordConfigured = true;
    fallbackKeyActive = false;
    vaultLocked = false;
    refreshUnlockTimer();
    persistMasterMetadata();
    savePasswords();

    return true;
}

bool BrayaPasswordManager::promptForMasterUnlock(GtkWindow* parent) {
    GtkWindow* dialog = create_modal_dialog("Unlock Password Vault", parent, 420, 240);
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_add_css_class(box, "dialog-body");
    gtk_window_set_child(dialog, box);

    GtkWidget* intro = gtk_label_new("Enter your master password to decrypt saved credentials.");
    gtk_label_set_wrap(GTK_LABEL(intro), TRUE);
    gtk_widget_set_halign(intro, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), intro);

    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Master password");
    gtk_box_append(GTK_BOX(box), entry);

    GtkWidget* errorLabel = gtk_label_new("");
    gtk_widget_add_css_class(errorLabel, "error");
    gtk_widget_set_halign(errorLabel, GTK_ALIGN_START);
    gtk_label_set_wrap(GTK_LABEL(errorLabel), TRUE);
    gtk_box_append(GTK_BOX(box), errorLabel);

    GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(buttonBox, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(box), buttonBox);

    GtkWidget* cancelBtn = gtk_button_new_with_label("Cancel");
    gtk_box_append(GTK_BOX(buttonBox), cancelBtn);

    GtkWidget* unlockBtn = gtk_button_new_with_label("Unlock");
    gtk_widget_add_css_class(unlockBtn, "suggested-action");
    gtk_box_append(GTK_BOX(buttonBox), unlockBtn);

    struct UnlockContext {
        BrayaPasswordManager* manager;
        GtkEntry* entry;
        GtkWidget* errorLabel;
        GtkWindow* dialog;
        GMainLoop* loop;
        bool success = false;
    } context{this, GTK_ENTRY(entry), errorLabel, dialog, g_main_loop_new(nullptr, FALSE), false};

    auto attemptUnlock = +[](GtkWidget*, gpointer data) {
        auto* ctx = static_cast<UnlockContext*>(data);
        const char* text = gtk_editable_get_text(GTK_EDITABLE(ctx->entry));
        if (!text || strlen(text) == 0) {
            gtk_label_set_text(GTK_LABEL(ctx->errorLabel), "Master password is required.");
            return;
        }
        std::string password(text);
        bool ok = ctx->manager->unlockWithMasterPassword(password);
        sodium_memzero(password.data(), password.size());
        if (!ok) {
            gtk_label_set_text(GTK_LABEL(ctx->errorLabel), "Incorrect master password. Try again.");
            return;
        }

        ctx->success = true;
        gtk_window_close(ctx->dialog);
    };

    g_signal_connect(unlockBtn, "clicked", G_CALLBACK(attemptUnlock), &context);
    g_signal_connect(entry, "activate", G_CALLBACK(attemptUnlock), &context);
    g_signal_connect_swapped(cancelBtn, "clicked", G_CALLBACK(gtk_window_close), dialog);

    g_signal_connect(dialog, "close-request", G_CALLBACK(+[](GtkWindow*, gpointer data) {
        auto* ctx = static_cast<UnlockContext*>(data);
        if (ctx->loop && g_main_loop_is_running(ctx->loop)) {
            g_main_loop_quit(ctx->loop);
        }
        return FALSE;
    }), &context);

    gtk_window_present(dialog);
    g_main_loop_run(context.loop);
    g_main_loop_unref(context.loop);

    return context.success;
}

bool BrayaPasswordManager::promptForSensitiveUnlock(GtkWindow* parent) {
    GtkWindow* dialog = create_modal_dialog("Confirm Identity", parent, 420, 240);
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_add_css_class(box, "dialog-body");
    gtk_window_set_child(dialog, box);

    GtkWidget* intro = gtk_label_new("Re-enter your master password to reveal this credential.");
    gtk_label_set_wrap(GTK_LABEL(intro), TRUE);
    gtk_widget_set_halign(intro, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), intro);

    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Master password");
    gtk_box_append(GTK_BOX(box), entry);

    GtkWidget* errorLabel = gtk_label_new("");
    gtk_widget_add_css_class(errorLabel, "error");
    gtk_widget_set_halign(errorLabel, GTK_ALIGN_START);
    gtk_label_set_wrap(GTK_LABEL(errorLabel), TRUE);
    gtk_box_append(GTK_BOX(box), errorLabel);

    GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(buttonBox, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(box), buttonBox);

    GtkWidget* cancelBtn = gtk_button_new_with_label("Cancel");
    gtk_box_append(GTK_BOX(buttonBox), cancelBtn);

    GtkWidget* confirmBtn = gtk_button_new_with_label("Reveal");
    gtk_widget_add_css_class(confirmBtn, "suggested-action");
    gtk_box_append(GTK_BOX(buttonBox), confirmBtn);

    struct SensitiveContext {
        BrayaPasswordManager* manager;
        GtkEntry* entry;
        GtkWidget* errorLabel;
        GtkWindow* dialog;
        GMainLoop* loop;
        bool success = false;
    } context{this, GTK_ENTRY(entry), errorLabel, dialog, g_main_loop_new(nullptr, FALSE), false};

    auto attemptConfirm = +[](GtkWidget*, gpointer data) {
        auto* ctx = static_cast<SensitiveContext*>(data);
        const char* text = gtk_editable_get_text(GTK_EDITABLE(ctx->entry));
        if (!text || strlen(text) == 0) {
            gtk_label_set_text(GTK_LABEL(ctx->errorLabel), "Master password is required.");
            return;
        }
        std::string password(text);
        bool ok = ctx->manager->authenticateMasterPassword(password, false);
        sodium_memzero(password.data(), password.size());
        if (!ok) {
            gtk_label_set_text(GTK_LABEL(ctx->errorLabel), "Incorrect password. Try again.");
            return;
        }
        ctx->success = true;
        gtk_window_close(ctx->dialog);
    };

    g_signal_connect(confirmBtn, "clicked", G_CALLBACK(attemptConfirm), &context);
    g_signal_connect(entry, "activate", G_CALLBACK(attemptConfirm), &context);
    g_signal_connect_swapped(cancelBtn, "clicked", G_CALLBACK(gtk_window_close), dialog);
    g_signal_connect(dialog, "close-request", G_CALLBACK(+[](GtkWindow*, gpointer data) {
        auto* ctx = static_cast<SensitiveContext*>(data);
        if (ctx->loop && g_main_loop_is_running(ctx->loop)) {
            g_main_loop_quit(ctx->loop);
        }
        return FALSE;
    }), &context);

    gtk_window_present(dialog);
    g_main_loop_run(context.loop);
    g_main_loop_unref(context.loop);
    return context.success;
}

bool BrayaPasswordManager::promptToCreateMasterPassword(GtkWindow* parent) {
    if (!fallbackKeyActive) {
        return masterKeySet;
    }

    GtkWindow* dialog = create_modal_dialog("Secure Your Password Vault", parent, 480, 360);
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_add_css_class(box, "dialog-body");
    gtk_window_set_child(dialog, box);

    GtkWidget* intro = gtk_label_new(
        "Create a master password to encrypt your saved passwords. "
        "You can skip for now, but auto-fill and viewing passwords will remain less secure.");
    gtk_label_set_wrap(GTK_LABEL(intro), TRUE);
    gtk_widget_set_halign(intro, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), intro);

    GtkWidget* passwordLabel = gtk_label_new("New master password");
    gtk_widget_set_halign(passwordLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), passwordLabel);

    GtkWidget* passwordEntry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(passwordEntry), FALSE);
    gtk_box_append(GTK_BOX(box), passwordEntry);

    GtkWidget* confirmLabel = gtk_label_new("Confirm password");
    gtk_widget_set_halign(confirmLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), confirmLabel);

    GtkWidget* confirmEntry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(confirmEntry), FALSE);
    gtk_box_append(GTK_BOX(box), confirmEntry);

    GtkWidget* hint = gtk_label_new("Use at least 8 characters with letters, numbers, and symbols.");
    gtk_widget_add_css_class(hint, "dim-label");
    gtk_widget_set_halign(hint, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), hint);

    GtkWidget* errorLabel = gtk_label_new("");
    gtk_widget_add_css_class(errorLabel, "error");
    gtk_label_set_wrap(GTK_LABEL(errorLabel), TRUE);
    gtk_widget_set_halign(errorLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), errorLabel);

    GtkWidget* buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(buttonBox, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(box), buttonBox);

    GtkWidget* skipBtn = gtk_button_new_with_label("Not Now");
    gtk_box_append(GTK_BOX(buttonBox), skipBtn);

    GtkWidget* createBtn = gtk_button_new_with_label("Create Master Password");
    gtk_widget_add_css_class(createBtn, "suggested-action");
    gtk_box_append(GTK_BOX(buttonBox), createBtn);

    struct SetupContext {
        BrayaPasswordManager* manager;
        GtkEntry* passwordEntry;
        GtkEntry* confirmEntry;
        GtkWidget* errorLabel;
        GtkWindow* dialog;
        GMainLoop* loop;
        bool success = false;
    } context{this, GTK_ENTRY(passwordEntry), GTK_ENTRY(confirmEntry), errorLabel, dialog, g_main_loop_new(nullptr, FALSE), masterKeySet};

    auto attemptCreate = +[](GtkWidget*, gpointer data) {
        auto* ctx = static_cast<SetupContext*>(data);
        const char* pass = gtk_editable_get_text(GTK_EDITABLE(ctx->passwordEntry));
        const char* confirm = gtk_editable_get_text(GTK_EDITABLE(ctx->confirmEntry));
        if (!pass || strlen(pass) < 8) {
            gtk_label_set_text(GTK_LABEL(ctx->errorLabel), "Master password must be at least 8 characters long.");
            return;
        }
        if (!confirm || strcmp(pass, confirm) != 0) {
            gtk_label_set_text(GTK_LABEL(ctx->errorLabel), "Passwords do not match.");
            return;
        }

        std::string password(pass);
        bool ok = ctx->manager->initializeMasterPassword(password);
        sodium_memzero(password.data(), password.size());

        if (!ok) {
            gtk_label_set_text(GTK_LABEL(ctx->errorLabel), "Could not create master password. Try a different value.");
            return;
        }

        ctx->success = true;
        gtk_window_close(ctx->dialog);
    };

    g_signal_connect(createBtn, "clicked", G_CALLBACK(attemptCreate), &context);
    g_signal_connect(confirmEntry, "activate", G_CALLBACK(attemptCreate), &context);
    g_signal_connect_swapped(skipBtn, "clicked", G_CALLBACK(gtk_window_close), dialog);

    g_signal_connect(dialog, "close-request", G_CALLBACK(+[](GtkWindow*, gpointer data) {
        auto* ctx = static_cast<SetupContext*>(data);
        if (ctx->loop && g_main_loop_is_running(ctx->loop)) {
            g_main_loop_quit(ctx->loop);
        }
        return FALSE;
    }), &context);

    gtk_window_present(dialog);
    g_main_loop_run(context.loop);
    g_main_loop_unref(context.loop);

    if (!context.success && masterKeySet) {
        refreshUnlockTimer();
    }

    return context.success || masterKeySet;
}

bool BrayaPasswordManager::ensureUnlocked(GtkWindow* parent) {
    if (!masterPasswordConfigured) {
        return promptToCreateMasterPassword(parent);
    }

    if (isUnlockExpired()) {
        lockVault();
    }

    if (vaultLocked || !masterKeySet) {
        return promptForMasterUnlock(parent);
    }

    refreshUnlockTimer();
    return true;
}

bool BrayaPasswordManager::requireSensitiveUnlock(GtkWindow* parent) {
    if (!masterPasswordConfigured || fallbackKeyActive) {
        return true;
    }

    if (vaultLocked || isUnlockExpired()) {
        if (!promptForMasterUnlock(parent)) {
            return false;
        }
    }

    time_t now = time(nullptr);
    if (lastSensitiveUnlockTimestamp != 0 &&
        difftime(now, lastSensitiveUnlockTimestamp) < SENSITIVE_UNLOCK_TIMEOUT_SECONDS) {
        lastSensitiveUnlockTimestamp = now;
        armIdleConcealTimer(parent);
        return true;
    }

    if (!promptForSensitiveUnlock(parent)) {
        return false;
    }

    lastSensitiveUnlockTimestamp = time(nullptr);
    armIdleConcealTimer(parent);
    return true;
}

std::string BrayaPasswordManager::getDomain(const std::string& url) const {
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

std::string BrayaPasswordManager::buildPasswordPreview(const std::string& password) const {
    if (password.empty()) {
        return "(Empty password)";
    }

    const std::string bullet = u8"\u2022";
    const size_t maxDots = std::min<size_t>(password.size(), 10);
    std::string preview;
    preview.reserve(maxDots * bullet.size() + 16);

    for (size_t i = 0; i < maxDots; ++i) {
        preview += bullet;
    }

    if (password.size() > maxDots) {
        preview += "…";
    }

    preview += " (" + std::to_string(password.size()) + " chars)";
    return preview;
}

std::string BrayaPasswordManager::formatTimestampRelative(long timestamp) const {
    if (timestamp <= 0) {
        return "Saved just now";
    }

    const time_t now = time(nullptr);
    long diff = std::max<long>(0, now - timestamp);
    
    auto pluralize = [](long value, const char* unit) {
        return std::to_string(value) + " " + unit + (value == 1 ? "" : "s");
    };

    if (diff < 60) {
        return "Saved moments ago";
    }
    if (diff < 3600) {
        long minutes = diff / 60;
        return "Saved " + pluralize(minutes, "minute") + " ago";
    }
    if (diff < 86400) {
        long hours = diff / 3600;
        return "Saved " + pluralize(hours, "hour") + " ago";
    }
    if (diff < 2592000) {
        long days = diff / 86400;
        return "Saved " + pluralize(days, "day") + " ago";
    }
    if (diff < 31536000) {
        long months = diff / 2592000;
        return "Saved " + pluralize(months, "month") + " ago";
    }

    long years = diff / 31536000;
    return "Saved " + pluralize(years, "year") + " ago";
}

void BrayaPasswordManager::copyToClipboard(GtkWidget* widget, const std::string& value, const std::string& label) {
    if (!widget || value.empty()) {
        return;
    }

    if (GdkClipboard* clipboard = gtk_widget_get_clipboard(widget)) {
        gdk_clipboard_set_text(clipboard, value.c_str());
        std::cout << "✓ Copied " << label << " to clipboard" << std::endl;
        if (label == "password") {
            scheduleClipboardClear(clipboard, label);
        }
    }
}

void BrayaPasswordManager::scheduleClipboardClear(GdkClipboard* clipboard, const std::string& label) {
    if (!clipboard || clipboardTimeoutSeconds <= 0) {
        return;
    }

    if (clipboardClearSource != 0) {
        g_source_remove(clipboardClearSource);
        clipboardClearSource = 0;
    }

    struct ClipboardClearContext {
        BrayaPasswordManager* manager;
        GdkClipboard* clipboard;
        std::string label;
    };

    auto* context = new ClipboardClearContext{
        this,
        GDK_CLIPBOARD(g_object_ref(clipboard)),
        label
    };

    clipboardClearSource = g_timeout_add_seconds_full(
        G_PRIORITY_DEFAULT,
        clipboardTimeoutSeconds,
        +[](gpointer data) -> gboolean {
            auto* ctx = static_cast<ClipboardClearContext*>(data);
            if (!ctx) {
                return G_SOURCE_REMOVE;
            }
            if (ctx->clipboard) {
                gdk_clipboard_set_text(ctx->clipboard, "");
            }
            if (ctx->manager) {
                ctx->manager->clipboardClearSource = 0;
            }
            std::cout << "✓ Cleared clipboard for " << (ctx->label.empty() ? "secret" : ctx->label)
                      << " after timeout" << std::endl;
            return G_SOURCE_REMOVE;
        },
        context,
        +[](gpointer data) {
            auto* ctx = static_cast<ClipboardClearContext*>(data);
            if (!ctx) {
                return;
            }
            if (ctx->clipboard) {
                g_object_unref(ctx->clipboard);
            }
            delete ctx;
        }
    );
}

void BrayaPasswordManager::clearIdleConcealTimer() {
    if (concealTimerSource != 0) {
        g_source_remove(concealTimerSource);
        concealTimerSource = 0;
    }
}

void BrayaPasswordManager::changeMasterPassword(GtkWindow* parent) {
    if (!ensureUnlocked(parent)) {
        return;
    }

    GtkWindow* dialog = create_modal_dialog("Change Master Password", parent, 460, 320);
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_add_css_class(box, "dialog-body");
    gtk_window_set_child(dialog, box);

    GtkWidget* intro = gtk_label_new(
        "Enter your current master password and a new one (8+ characters). "
        "Existing vault items will be re-encrypted.");
    gtk_label_set_wrap(GTK_LABEL(intro), TRUE);
    gtk_widget_set_halign(intro, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), intro);

    GtkWidget* currentLabel = gtk_label_new("Current master password");
    gtk_widget_set_halign(currentLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), currentLabel);

    GtkWidget* currentEntry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(currentEntry), FALSE);
    gtk_box_append(GTK_BOX(box), currentEntry);

    GtkWidget* newLabel = gtk_label_new("New master password");
    gtk_widget_set_halign(newLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), newLabel);

    GtkWidget* newEntry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(newEntry), FALSE);
    gtk_box_append(GTK_BOX(box), newEntry);

    GtkWidget* confirmLabel = gtk_label_new("Confirm new password");
    gtk_widget_set_halign(confirmLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), confirmLabel);

    GtkWidget* confirmEntry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(confirmEntry), FALSE);
    gtk_box_append(GTK_BOX(box), confirmEntry);

    GtkWidget* errorLabel = gtk_label_new("");
    gtk_widget_add_css_class(errorLabel, "error");
    gtk_label_set_wrap(GTK_LABEL(errorLabel), TRUE);
    gtk_widget_set_halign(errorLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), errorLabel);

    GtkWidget* buttonRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(buttonRow, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(box), buttonRow);

    GtkWidget* cancelBtn = gtk_button_new_with_label("Cancel");
    gtk_box_append(GTK_BOX(buttonRow), cancelBtn);

    GtkWidget* saveBtn = gtk_button_new_with_label("Change");
    gtk_widget_add_css_class(saveBtn, "suggested-action");
    gtk_box_append(GTK_BOX(buttonRow), saveBtn);

    struct ChangeContext {
        BrayaPasswordManager* manager;
        GtkEntry* current;
        GtkEntry* next;
        GtkEntry* confirm;
        GtkWidget* error;
        GtkWindow* dialog;
        GMainLoop* loop;
        bool success = false;
    } ctx{this, GTK_ENTRY(currentEntry), GTK_ENTRY(newEntry), GTK_ENTRY(confirmEntry),
          errorLabel, dialog, g_main_loop_new(nullptr, FALSE), false};

    auto attemptChange = +[](GtkWidget*, gpointer data) {
        auto* c = static_cast<ChangeContext*>(data);
        std::string current = gtk_editable_get_text(GTK_EDITABLE(c->current));
        std::string next = gtk_editable_get_text(GTK_EDITABLE(c->next));
        std::string confirm = gtk_editable_get_text(GTK_EDITABLE(c->confirm));

        if (current.empty()) {
            gtk_label_set_text(GTK_LABEL(c->error), "Current master password is required.");
            return;
        }
        if (next.size() < 8) {
            gtk_label_set_text(GTK_LABEL(c->error), "New master password must be at least 8 characters.");
            return;
        }
        if (next != confirm) {
            gtk_label_set_text(GTK_LABEL(c->error), "New passwords do not match.");
            return;
        }
        if (!c->manager->ensureUnlocked(c->dialog)) {
            gtk_label_set_text(GTK_LABEL(c->error), "Unlock the vault before changing the master password.");
            return;
        }
        if (!c->manager->verifyPassword(current)) {
            gtk_label_set_text(GTK_LABEL(c->error), "Current master password is incorrect.");
            return;
        }
        if (!c->manager->initializeMasterPassword(next)) {
            gtk_label_set_text(GTK_LABEL(c->error), "Failed to set the new master password. Try again.");
            return;
        }
        c->success = true;
        gtk_window_close(c->dialog);
    };

    g_signal_connect(saveBtn, "clicked", G_CALLBACK(attemptChange), &ctx);
    g_signal_connect(currentEntry, "activate", G_CALLBACK(attemptChange), &ctx);
    g_signal_connect(confirmEntry, "activate", G_CALLBACK(attemptChange), &ctx);
    g_signal_connect_swapped(cancelBtn, "clicked", G_CALLBACK(gtk_window_close), dialog);
    g_signal_connect(dialog, "close-request", G_CALLBACK(+[](GtkWindow*, gpointer data) {
        auto* c = static_cast<ChangeContext*>(data);
        if (c->loop && g_main_loop_is_running(c->loop)) {
            g_main_loop_quit(c->loop);
        }
        return FALSE;
    }), &ctx);

    gtk_window_present(dialog);
    g_main_loop_run(ctx.loop);
    g_main_loop_unref(ctx.loop);

    if (ctx.success) {
        showSuccessDialog(parent, "Master Password Changed",
                          "Your vault has been re-encrypted with the new master password.");
    }
}

void BrayaPasswordManager::resetVault(GtkWindow* parent) {
    GtkWindow* dialog = create_modal_dialog("Reset Password Vault", parent, 480, 260);
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(box, "dialog-body");
    gtk_window_set_child(dialog, box);

    GtkWidget* intro = gtk_label_new(
        "This will delete all saved passwords and reset the master password. "
        "You cannot recover existing entries. Type RESET to confirm.");
    gtk_label_set_wrap(GTK_LABEL(intro), TRUE);
    gtk_widget_set_halign(intro, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), intro);

    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Type RESET to confirm");
    gtk_box_append(GTK_BOX(box), entry);

    GtkWidget* errorLabel = gtk_label_new("");
    gtk_widget_add_css_class(errorLabel, "error");
    gtk_widget_set_halign(errorLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), errorLabel);

    GtkWidget* buttonRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(buttonRow, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(box), buttonRow);

    GtkWidget* cancelBtn = gtk_button_new_with_label("Cancel");
    gtk_box_append(GTK_BOX(buttonRow), cancelBtn);

    GtkWidget* resetBtn = gtk_button_new_with_label("Reset Vault");
    gtk_widget_add_css_class(resetBtn, "destructive-action");
    gtk_box_append(GTK_BOX(buttonRow), resetBtn);

    struct ResetContext {
        BrayaPasswordManager* manager;
        GtkEntry* entry;
        GtkWidget* error;
        GtkWindow* dialog;
        GMainLoop* loop;
        bool success = false;
    } ctx{this, GTK_ENTRY(entry), errorLabel, dialog, g_main_loop_new(nullptr, FALSE), false};

    auto attemptReset = +[](GtkWidget*, gpointer data) {
        auto* c = static_cast<ResetContext*>(data);
        const char* text = gtk_editable_get_text(GTK_EDITABLE(c->entry));
        if (!text || strcmp(text, "RESET") != 0) {
            gtk_label_set_text(GTK_LABEL(c->error), "Type RESET to confirm.");
            return;
        }

        c->manager->lockVault();
        c->manager->blockedDomains.clear();
        c->manager->passwords.clear();

        std::remove(c->manager->storageFile.c_str());
        std::remove((c->manager->configDirectory + "/master.key").c_str());
        std::remove(c->manager->blockedListFile.c_str());
        std::remove(c->manager->settingsFile.c_str());

        c->manager->masterPasswordConfigured = false;
        c->manager->fallbackKeyActive = false;
        c->manager->loadOrCreateMasterSalt();
        std::string fallback = std::string(g_get_user_name()) + "_braya_browser";
        c->manager->deriveKey(fallback);
        c->manager->fallbackKeyActive = c->manager->masterKeySet;
        c->manager->vaultLocked = false;
        c->manager->refreshUnlockTimer();
        c->manager->saveBlockedDomains();
        c->manager->saveSettings();
        c->manager->savePasswords();
        c->success = true;
        gtk_window_close(c->dialog);
    };

    g_signal_connect(resetBtn, "clicked", G_CALLBACK(attemptReset), &ctx);
    g_signal_connect(entry, "activate", G_CALLBACK(attemptReset), &ctx);
    g_signal_connect_swapped(cancelBtn, "clicked", G_CALLBACK(gtk_window_close), dialog);
    g_signal_connect(dialog, "close-request", G_CALLBACK(+[](GtkWindow*, gpointer data) {
        auto* c = static_cast<ResetContext*>(data);
        if (c->loop && g_main_loop_is_running(c->loop)) {
            g_main_loop_quit(c->loop);
        }
        return FALSE;
    }), &ctx);

    gtk_window_present(dialog);
    g_main_loop_run(ctx.loop);
    g_main_loop_unref(ctx.loop);

    if (ctx.success) {
        showSuccessDialog(parent, "Vault Reset",
                          "All saved passwords were deleted and the vault was reset.");
    }
}

void BrayaPasswordManager::updateDetailPane(GtkWidget* detailPane, GtkListBoxRow* row) {
    std::cout << "updateDetailPane called" << std::endl;

    if (!detailPane) {
        std::cout << "  ✗ No detail pane widget" << std::endl;
        return;
    }
    std::cout << "  ✓ Detail pane widget exists" << std::endl;

    if (!row) {
        std::cout << "  ! No row selected, showing placeholder" << std::endl;
        renderDetailPlaceholder(detailPane);
        return;
    }
    std::cout << "  ✓ Row selected" << std::endl;

    // Get the child widget (the actual box we created) from the ListBoxRow
    GtkWidget* child = gtk_list_box_row_get_child(row);
    if (!child) {
        std::cout << "  ✗ No child widget in row" << std::endl;
        renderDetailPlaceholder(detailPane);
        return;
    }
    std::cout << "  ✓ Child widget found" << std::endl;

    auto* entry = static_cast<PasswordEntry*>(g_object_get_data(G_OBJECT(child), "entry_data"));
    if (!entry) {
        std::cout << "  ✗ No entry_data on child widget" << std::endl;
        renderDetailPlaceholder(detailPane);
        return;
    }
    std::cout << "  ✓ Entry data found: " << entry->url << std::endl;

    renderDetailForEntry(detailPane, *entry);
}

void BrayaPasswordManager::renderDetailPlaceholder(GtkWidget* detailPane) {
    if (!detailPane) {
        return;
    }

    clear_container(detailPane);
    GtkWidget* placeholder = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(placeholder, 40);
    gtk_widget_set_halign(placeholder, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(placeholder, GTK_ALIGN_CENTER);

    GtkWidget* icon = gtk_label_new("🔍");
    gtk_widget_set_halign(icon, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(icon, "nav-rail-title");
    gtk_box_append(GTK_BOX(placeholder), icon);

    GtkWidget* title = gtk_label_new("Select a login to view details");
    gtk_widget_set_halign(title, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(title, "nav-rail-title");
    gtk_box_append(GTK_BOX(placeholder), title);

    GtkWidget* subtitle = gtk_label_new("Choose an entry to see metadata, strength, and quick actions.");
    gtk_widget_add_css_class(subtitle, "dim-label");
    gtk_label_set_wrap(GTK_LABEL(subtitle), TRUE);
    gtk_label_set_justify(GTK_LABEL(subtitle), GTK_JUSTIFY_CENTER);
    gtk_widget_set_halign(subtitle, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_start(subtitle, 12);
    gtk_widget_set_margin_end(subtitle, 12);
    gtk_box_append(GTK_BOX(placeholder), subtitle);

    gtk_box_append(GTK_BOX(detailPane), placeholder);
}

void BrayaPasswordManager::renderDetailForEntry(GtkWidget* detailPane, const PasswordEntry& entry) {
    std::cout << "renderDetailForEntry called for: " << entry.url << std::endl;
    if (!detailPane) {
        std::cout << "  ✗ No detail pane!" << std::endl;
        return;
    }

    clear_container(detailPane);
    std::cout << "  ✓ Cleared detail pane, adding widgets..." << std::endl;

    auto describeTime = [&](long timestamp, const char* fallback) -> std::string {
        if (timestamp <= 0) {
            return fallback;
        }
        return formatTimestampRelative(timestamp);
    };

    GtkWidget* header = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_bottom(header, 10);
    gtk_box_append(GTK_BOX(detailPane), header);

    gchar* escapedUrl = g_markup_escape_text(entry.url.c_str(), -1);
    std::string titleMarkup = std::string("<span size='large' weight='bold'>") +
                              (escapedUrl ? escapedUrl : "unknown") + "</span>";
    g_free(escapedUrl);
    GtkWidget* title = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(title), titleMarkup.c_str());
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_label_set_wrap(GTK_LABEL(title), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(title), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_max_width_chars(GTK_LABEL(title), 40);
    gtk_box_append(GTK_BOX(header), title);

    GtkWidget* subtitle = gtk_label_new(entry.username.empty() ? "No username saved" : entry.username.c_str());
    gtk_widget_set_halign(subtitle, GTK_ALIGN_START);
    gtk_widget_add_css_class(subtitle, "dim-label");
    gtk_label_set_wrap(GTK_LABEL(subtitle), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(subtitle), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_max_width_chars(GTK_LABEL(subtitle), 40);
    gtk_box_append(GTK_BOX(header), subtitle);

    GtkWidget* metaGrid = gtk_grid_new();
    gtk_widget_add_css_class(metaGrid, "detail-meta-grid");
    gtk_grid_set_column_spacing(GTK_GRID(metaGrid), 16);
    gtk_grid_set_row_spacing(GTK_GRID(metaGrid), 6);
    gtk_box_append(GTK_BOX(detailPane), metaGrid);

    auto appendMetaRow = [&](const char* label, const std::string& value, int rowIndex) {
        GtkWidget* keyLabel = gtk_label_new(label);
        gtk_widget_set_halign(keyLabel, GTK_ALIGN_START);
        gtk_widget_add_css_class(keyLabel, "dim-label");
        gtk_grid_attach(GTK_GRID(metaGrid), keyLabel, 0, rowIndex, 1, 1);

        GtkWidget* valueLabel = gtk_label_new(value.c_str());
        gtk_widget_set_halign(valueLabel, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(metaGrid), valueLabel, 1, rowIndex, 1, 1);
    };

    appendMetaRow("Last updated", describeTime(entry.updatedAt, "Never"), 0);
    appendMetaRow("Created", describeTime(entry.createdAt, "Unknown"), 1);
    appendMetaRow("Last used", describeTime(entry.lastUsedAt, "Not used yet"), 2);

    std::string strengthLabelText = "Strength: " + getPasswordStrengthLabel(entry.strengthScore) +
                                    " (" + std::to_string(entry.strengthScore) + "/100)";
    if (entry.breached) {
        strengthLabelText += " • Breach flagged";
    }
    GtkWidget* strengthLabel = gtk_label_new(strengthLabelText.c_str());
    gtk_widget_set_halign(strengthLabel, GTK_ALIGN_START);
    if (entry.breached) {
        gtk_widget_add_css_class(strengthLabel, "error");
    }
    gtk_box_append(GTK_BOX(detailPane), strengthLabel);

    GtkWidget* passwordBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(detailPane), passwordBox);

    GtkWidget* passwordPreview = gtk_label_new(buildPasswordPreview(entry.password).c_str());
    gtk_widget_set_halign(passwordPreview, GTK_ALIGN_START);
    gtk_widget_add_css_class(passwordPreview, "dim-label");
    gtk_box_append(GTK_BOX(passwordBox), passwordPreview);

    GtkWidget* revealBtn = gtk_toggle_button_new_with_label("Show password");
    g_object_set_data(G_OBJECT(revealBtn), "manager", this);
    g_object_set_data(G_OBJECT(revealBtn), "parent_window",
                      g_object_get_data(G_OBJECT(detailPane), "parent_window"));
    g_object_set_data(G_OBJECT(revealBtn), "password_label", passwordPreview);
    g_object_set_data_full(G_OBJECT(revealBtn), "password_plain", g_strdup(entry.password.c_str()), g_free);
    g_object_set_data_full(G_OBJECT(revealBtn), "password_masked",
                           g_strdup(buildPasswordPreview(entry.password).c_str()), g_free);

    g_signal_connect(revealBtn, "toggled", G_CALLBACK(+[](GtkToggleButton* btn, gpointer) {
        auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
        GtkWidget* label = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "password_label"));
        GtkWidget* parent = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "parent_window"));
        const char* plain = static_cast<const char*>(g_object_get_data(G_OBJECT(btn), "password_plain"));
        const char* masked = static_cast<const char*>(g_object_get_data(G_OBJECT(btn), "password_masked"));
        if (!label) {
            return;
        }

        if (gtk_toggle_button_get_active(btn)) {
            if (manager && !manager->requireSensitiveUnlock(GTK_WINDOW(parent))) {
                gtk_toggle_button_set_active(btn, FALSE);
                gtk_label_set_text(GTK_LABEL(label), masked ? masked : "");
                return;
            }
            gtk_label_set_text(GTK_LABEL(label), plain ? plain : "");
        } else {
            gtk_label_set_text(GTK_LABEL(label), masked ? masked : "");
        }
    }), nullptr);

    gtk_box_append(GTK_BOX(passwordBox), revealBtn);

    GtkWidget* buttonRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(detailPane), buttonRow);

    GtkWidget* copyUserBtn = gtk_button_new_with_label("Copy username");
    g_object_set_data(G_OBJECT(copyUserBtn), "manager", this);
    g_object_set_data_full(G_OBJECT(copyUserBtn), "value", g_strdup(entry.username.c_str()), g_free);
    g_signal_connect(copyUserBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer) {
        auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
        const char* value = static_cast<const char*>(g_object_get_data(G_OBJECT(btn), "value"));
        if (manager && value) {
            manager->copyToClipboard(GTK_WIDGET(btn), value, "username");
        }
    }), nullptr);
    gtk_box_append(GTK_BOX(buttonRow), copyUserBtn);

    GtkWidget* copyPassBtn = gtk_button_new_with_label("Copy password");
    g_object_set_data(G_OBJECT(copyPassBtn), "manager", this);
    g_object_set_data_full(G_OBJECT(copyPassBtn), "value", g_strdup(entry.password.c_str()), g_free);
    g_object_set_data(G_OBJECT(copyPassBtn), "parent_window",
                      g_object_get_data(G_OBJECT(detailPane), "parent_window"));
    g_signal_connect(copyPassBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer) {
        auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
        const char* value = static_cast<const char*>(g_object_get_data(G_OBJECT(btn), "value"));
        GtkWidget* parent = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "parent_window"));
        if (!manager || !value) {
            return;
        }
        if (!manager->requireSensitiveUnlock(GTK_WINDOW(parent))) {
            return;
        }
        manager->copyToClipboard(GTK_WIDGET(btn), value, "password");
    }), nullptr);
    gtk_box_append(GTK_BOX(buttonRow), copyPassBtn);

    // Edit/Delete buttons
    GtkWidget* actionRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_top(actionRow, 12);
    gtk_box_append(GTK_BOX(detailPane), actionRow);

    GtkWidget* editBtn = gtk_button_new_with_label("✏️ Edit");
    gtk_widget_add_css_class(editBtn, "suggested-action");
    g_object_set_data(G_OBJECT(editBtn), "manager", this);
    g_object_set_data_full(G_OBJECT(editBtn), "url", g_strdup(entry.url.c_str()), g_free);
    g_object_set_data_full(G_OBJECT(editBtn), "username", g_strdup(entry.username.c_str()), g_free);
    g_object_set_data(G_OBJECT(editBtn), "parent_window",
                      g_object_get_data(G_OBJECT(detailPane), "parent_window"));
    g_signal_connect(editBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer) {
        auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
        const char* url = static_cast<const char*>(g_object_get_data(G_OBJECT(btn), "url"));
        const char* username = static_cast<const char*>(g_object_get_data(G_OBJECT(btn), "username"));
        GtkWidget* parent = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "parent_window"));
        if (manager && url && username && parent) {
            manager->editPassword(url, username, GTK_WINDOW(parent));
        }
    }), nullptr);
    gtk_box_append(GTK_BOX(actionRow), editBtn);

    GtkWidget* deleteBtn = gtk_button_new_with_label("🗑️ Delete");
    gtk_widget_add_css_class(deleteBtn, "destructive-action");
    g_object_set_data(G_OBJECT(deleteBtn), "manager", this);
    g_object_set_data_full(G_OBJECT(deleteBtn), "url", g_strdup(entry.url.c_str()), g_free);
    g_object_set_data_full(G_OBJECT(deleteBtn), "username", g_strdup(entry.username.c_str()), g_free);
    g_object_set_data(G_OBJECT(deleteBtn), "parent_window",
                      g_object_get_data(G_OBJECT(detailPane), "parent_window"));
    g_signal_connect(deleteBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer) {
        auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
        const char* url = static_cast<const char*>(g_object_get_data(G_OBJECT(btn), "url"));
        const char* username = static_cast<const char*>(g_object_get_data(G_OBJECT(btn), "username"));
        GtkWidget* parent = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "parent_window"));
        if (manager && url && username && parent) {
            manager->deletePassword(url, username);
            // Close and reopen password manager to show updated list
            gtk_window_close(GTK_WINDOW(parent));
            manager->showPasswordManager(GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(btn))));
        }
    }), nullptr);
    gtk_box_append(GTK_BOX(actionRow), deleteBtn);
}

bool BrayaPasswordManager::isSavingSuppressedForUrl(const std::string& url) const {
    std::string domain = getDomain(url);
    return blockedDomains.find(domain) != blockedDomains.end();
}

void BrayaPasswordManager::blockSavingForUrl(const std::string& url) {
    std::string domain = getDomain(url);
    if (domain.empty()) {
        return;
    }
    blockedDomains.insert(domain);
    saveBlockedDomains();
}

void BrayaPasswordManager::unblockDomain(const std::string& domain) {
    blockedDomains.erase(domain);
    saveBlockedDomains();
}

void BrayaPasswordManager::loadSettings() {
    multiStepCaptureEnabled = true;
    std::ifstream file(settingsFile);
    if (!file.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        key.erase(std::remove_if(key.begin(), key.end(), ::isspace), key.end());
        value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());

        if (key == "multi_step") {
            multiStepCaptureEnabled = !(value == "0" || value == "false");
        }
    }
}

void BrayaPasswordManager::saveSettings() const {
    std::ofstream file(settingsFile, std::ios::trunc);
    if (!file.is_open()) {
        return;
    }
    file << "multi_step=" << (multiStepCaptureEnabled ? "1" : "0") << "\n";
    file.close();
    chmod(settingsFile.c_str(), S_IRUSR | S_IWUSR);
}

void BrayaPasswordManager::loadBlockedDomains() {
    blockedDomains.clear();
    std::ifstream file(blockedListFile);
    if (!file.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
        if (!line.empty()) {
            blockedDomains.insert(line);
        }
    }
}

void BrayaPasswordManager::saveBlockedDomains() const {
    std::ofstream file(blockedListFile, std::ios::trunc);
    if (!file.is_open()) {
        return;
    }
    for (const auto& domain : blockedDomains) {
        file << domain << "\n";
    }
    file.close();
    chmod(blockedListFile.c_str(), S_IRUSR | S_IWUSR);
}

void BrayaPasswordManager::armIdleConcealTimer(GtkWindow* parent) {
    clearIdleConcealTimer();

    GtkWindow* target = parent ? parent : activeVaultDialog;
    if (!target) {
        return;
    }

    struct ConcealContext {
        BrayaPasswordManager* manager;
        GtkWindow* window;
    };

    auto* context = new ConcealContext{this, GTK_WINDOW(g_object_ref(target))};

    concealTimerSource = g_timeout_add_seconds_full(
        G_PRIORITY_DEFAULT,
        SENSITIVE_UNLOCK_TIMEOUT_SECONDS,
        +[](gpointer data) -> gboolean {
            auto* ctx = static_cast<ConcealContext*>(data);
            if (!ctx || !ctx->manager) {
                return G_SOURCE_REMOVE;
            }
            ctx->manager->lockVault();
            if (ctx->window) {
                gtk_window_close(ctx->window);
            }
            ctx->manager->concealTimerSource = 0;
            return G_SOURCE_REMOVE;
        },
        context,
        +[](gpointer data) {
            auto* ctx = static_cast<ConcealContext*>(data);
            if (!ctx) {
                return;
            }
            if (ctx->window) {
                g_object_unref(ctx->window);
                ctx->window = nullptr;
            }
            delete ctx;
        }
    );
}

void BrayaPasswordManager::savePassword(const std::string& url, const std::string& username, const std::string& password) {
    if (!passwordSavingEnabled) return; 
    if (masterPasswordConfigured && (vaultLocked || isUnlockExpired())) {
        std::cerr << "✗ Cannot save password while vault is locked" << std::endl;
        lockVault();
        return;
    }
    refreshUnlockTimer();
    std::string domain = getDomain(url);
    time_t now = time(nullptr);
    int strength = calculatePasswordStrength(password);
    
    // Check if already exists
    for (auto& entry : passwords) {
        if (entry.url == domain && entry.username == username) {
            // Update existing
            entry.password = password;
            entry.updatedAt = now;
            entry.strengthScore = strength;
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
    entry.createdAt = now;
    entry.updatedAt = now;
    entry.lastUsedAt = 0;
    entry.strengthScore = strength;
    entry.breached = false;
    
    passwords.push_back(entry);
    savePasswords();
    
    std::cout << "✓ Password saved for " << username << " @ " << domain << std::endl;
}

std::vector<PasswordEntry> BrayaPasswordManager::getPasswordsForUrl(const std::string& url) {
    if (masterPasswordConfigured) {
        if (vaultLocked || isUnlockExpired()) {
            lockVault();
            return {};
        }
        refreshUnlockTimer();
    }

    std::string domain = getDomain(url);
    std::cout << "🔍 Searching passwords: URL domain='" << domain << "'" << std::endl;

    std::vector<PasswordEntry> matches;

    for (const auto& entry : passwords) {
        std::cout << "  Comparing with saved: '" << entry.url << "'" << std::endl;
        if (entry.url == domain) {
            std::cout << "  ✅ MATCH!" << std::endl;
            matches.push_back(entry);
        }
    }

    std::cout << "🔍 Found " << matches.size() << " matches for domain '" << domain << "'" << std::endl;
    return matches;
}

std::vector<PasswordEntry> BrayaPasswordManager::getAllPasswords() {
    if (masterPasswordConfigured) {
        if (vaultLocked || isUnlockExpired()) {
            lockVault();
            return {};
        }
        refreshUnlockTimer();
    }

    return passwords;
}

void BrayaPasswordManager::deletePassword(const std::string& url, const std::string& username) {
    if (masterPasswordConfigured && (vaultLocked || isUnlockExpired())) {
        lockVault();
        std::cerr << "✗ Cannot delete password while vault is locked" << std::endl;
        return;
    }
    refreshUnlockTimer();

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
    if (masterPasswordConfigured && (vaultLocked || isUnlockExpired())) {
        lockVault();
        std::cerr << "✗ Cannot clear passwords while vault is locked" << std::endl;
        return;
    }

    refreshUnlockTimer();
    passwords.clear();
    savePasswords();
    std::cout << "✓ All passwords cleared" << std::endl;
}

void BrayaPasswordManager::initializeCrypto() {
    if (cryptoInitialized)
        return;

    if (sodium_init() < 0) {
        std::cerr << "✗ Failed to initialize libsodium" << std::endl;
        return;
    }

    loadOrCreateMasterSalt();
    cryptoInitialized = true;
}

void BrayaPasswordManager::loadOrCreateMasterSalt() {
    masterSalt.resize(crypto_pwhash_SALTBYTES);
    const std::string saltPath = configDirectory + "/master.key";
    std::ifstream saltFile(saltPath, std::ios::binary);
    if (saltFile.is_open()) {
        saltFile.read(reinterpret_cast<char*>(masterSalt.data()), masterSalt.size());
        if (saltFile.gcount() == static_cast<std::streamsize>(masterSalt.size())) {
            std::vector<unsigned char> verifier(MASTER_VERIFIER_BYTES);
            saltFile.read(reinterpret_cast<char*>(verifier.data()), verifier.size());
            if (saltFile.gcount() == static_cast<std::streamsize>(verifier.size())) {
                masterVerifier = verifier;
                masterPasswordConfigured = true;
            } else {
                masterVerifier.clear();
                masterPasswordConfigured = false;
            }
            saltFile.close();
            return;
        }
        saltFile.close();
    }

    randombytes_buf(masterSalt.data(), masterSalt.size());
    masterVerifier.clear();
    masterPasswordConfigured = false;
    persistMasterMetadata();
}

std::string BrayaPasswordManager::encrypt(const std::string& data) {
    return encryptModern(data);
}

std::string BrayaPasswordManager::decrypt(const std::string& data) {
    std::string modern = decryptModern(data);
    if (!modern.empty()) {
        return modern;
    }

    std::string legacy = decryptLegacy(data);
    if (!legacy.empty()) {
        legacyDataMigrated = true;
    }
    return legacy;
}

std::string BrayaPasswordManager::encryptModern(const std::string& data) {
    if (!masterKeySet || masterKey.empty()) {
        std::cerr << "✗ Master key not set, cannot encrypt" << std::endl;
        return "";
    }

    std::vector<unsigned char> nonce(crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
    randombytes_buf(nonce.data(), nonce.size());

    std::vector<unsigned char> ciphertext(data.size() + crypto_aead_xchacha20poly1305_ietf_ABYTES);
    unsigned long long ciphertext_len = 0;

    if (crypto_aead_xchacha20poly1305_ietf_encrypt(
            ciphertext.data(), &ciphertext_len,
            reinterpret_cast<const unsigned char*>(data.data()), data.size(),
            nullptr, 0, nullptr, nonce.data(), masterKey.data()) != 0) {
        std::cerr << "✗ Encryption failed" << std::endl;
        return "";
    }

    size_t totalLen = nonce.size() + ciphertext_len;
    std::vector<unsigned char> combined(totalLen);
    memcpy(combined.data(), nonce.data(), nonce.size());
    memcpy(combined.data() + nonce.size(), ciphertext.data(), ciphertext_len);

    size_t encodedLen = sodium_base64_encoded_len(totalLen, sodium_base64_VARIANT_ORIGINAL);
    std::string encoded(encodedLen, '\0');
    sodium_bin2base64(encoded.data(), encoded.size(),
                      combined.data(), combined.size(),
                      sodium_base64_VARIANT_ORIGINAL);
    encoded.resize(strlen(encoded.c_str()));
    return encoded;
}

std::string BrayaPasswordManager::decryptModern(const std::string& data) {
    if (!masterKeySet || masterKey.empty()) {
        return "";
    }

    size_t decodedCapacity = (data.size() * 3) / 4 + 4;
    std::vector<unsigned char> decoded(decodedCapacity);
    size_t decodedLen = 0;
    if (sodium_base642bin(decoded.data(), decoded.size(),
                          data.c_str(), data.size(),
                          nullptr, &decodedLen, nullptr,
                          sodium_base64_VARIANT_ORIGINAL) != 0) {
        return "";
    }
    decoded.resize(decodedLen);
    if (decodedLen <= crypto_aead_xchacha20poly1305_ietf_NPUBBYTES) {
        return "";
    }

    const unsigned char* nonce = decoded.data();
    const unsigned char* ciphertext = decoded.data() + crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
    size_t ciphertextLen = decodedLen - crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;

    if (ciphertextLen < crypto_aead_xchacha20poly1305_ietf_ABYTES) {
        return "";
    }

    std::vector<unsigned char> plaintext(ciphertextLen);
    unsigned long long plaintextLen = 0;
    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
            plaintext.data(), &plaintextLen,
            nullptr,
            ciphertext, ciphertextLen,
            nullptr, 0, nonce, masterKey.data()) != 0) {
        return "";
    }

    return std::string(reinterpret_cast<char*>(plaintext.data()), plaintextLen);
}

std::string BrayaPasswordManager::decryptLegacy(const std::string& data) {
    if (!masterKeySet || masterKey.empty()) {
        return "";
    }

    if (data.size() < 16) {
        return "";
    }

    unsigned char iv[16];
    memcpy(iv, data.data(), 16);

    const unsigned char* ciphertext = reinterpret_cast<const unsigned char*>(data.data()) + 16;
    size_t ciphertext_len = data.size() - 16;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return "";

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, masterKey.data(), iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }

    std::vector<unsigned char> plaintext(ciphertext_len + 16);
    int len = 0;
    int plaintext_len = 0;

    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext, ciphertext_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    plaintext_len = len;

    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    return std::string(reinterpret_cast<char*>(plaintext.data()), plaintext_len);
}

void BrayaPasswordManager::loadPasswords() {
    std::ifstream file(storageFile, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "📂 No password file found at: " << storageFile << std::endl;
        return; // No passwords saved yet
    }

    std::cout << "📂 Loading passwords from: " << storageFile << std::endl;
    passwords.clear();

    int lineNum = 0;
    std::string line;
    while (std::getline(file, line)) {
        lineNum++;
        if (line.empty()) continue;

        // Decrypt the line
        std::string decrypted = decrypt(line);
        if (decrypted.empty()) {
            std::cout << "❌ Failed to decrypt password entry #" << lineNum << " (key mismatch or corrupted)" << std::endl;
            continue;
        }

        std::vector<std::string> parts;
        size_t start = 0;
        while (start <= decrypted.size()) {
            size_t pos = decrypted.find('|', start);
            if (pos == std::string::npos) {
                parts.emplace_back(decrypted.substr(start));
                break;
            }
            parts.emplace_back(decrypted.substr(start, pos - start));
            start = pos + 1;
        }

        if (parts.size() < 4) {
            continue;
        }

        PasswordEntry entry;
        entry.url = parts[0];
        entry.username = parts[1];
        entry.password = parts[2];

        auto parseLongField = [](const std::string& text, long fallback) -> long {
            if (text.empty()) {
                return fallback;
            }
            try {
                return std::stol(text);
            } catch (...) {
                return fallback;
            }
        };

        auto parseIntField = [](const std::string& text, int fallback) -> int {
            if (text.empty()) {
                return fallback;
            }
            try {
                return std::stoi(text);
            } catch (...) {
                return fallback;
            }
        };

        auto parseBoolField = [](const std::string& text, bool fallback) -> bool {
            if (text.empty()) {
                return fallback;
            }
            return text == "1" || text == "true" || text == "TRUE";
        };

        long legacyTimestamp = parseLongField(parts[3], 0);
        entry.createdAt = legacyTimestamp;
        entry.updatedAt = legacyTimestamp;
        entry.lastUsedAt = 0;
        entry.strengthScore = calculatePasswordStrength(entry.password);
        entry.breached = false;

        if (parts.size() > 4) {
            entry.updatedAt = parseLongField(parts[4], entry.createdAt);
        }
        if (parts.size() > 5) {
            entry.lastUsedAt = parseLongField(parts[5], 0);
        }
        if (parts.size() > 6) {
            entry.strengthScore = parseIntField(parts[6], entry.strengthScore);
        }
        if (parts.size() > 7) {
            entry.breached = parseBoolField(parts[7], false);
        }

        // 🆕 Load new fields (backward compatible - defaults if not present)
        if (parts.size() > 8) {
            entry.favorite = parseBoolField(parts[8], false);
        }
        if (parts.size() > 9) {
            entry.category = parts[9].empty() ? "Personal" : parts[9];
        }
        if (parts.size() > 10 && !parts[10].empty()) {
            // Split tags by comma
            std::string tagsStr = parts[10];
            size_t pos = 0;
            while ((pos = tagsStr.find(',')) != std::string::npos) {
                std::string tag = tagsStr.substr(0, pos);
                if (!tag.empty()) entry.tags.push_back(tag);
                tagsStr.erase(0, pos + 1);
            }
            if (!tagsStr.empty()) entry.tags.push_back(tagsStr);
        }
        if (parts.size() > 11) {
            entry.usageCount = parseIntField(parts[11], 0);
        }
        if (parts.size() > 12) {
            entry.lastBreachCheck = parseLongField(parts[12], 0);
        }
        if (parts.size() > 13) {
            entry.notes = parts[13];
        }

        if (entry.strengthScore <= 0 && !entry.password.empty()) {
            entry.strengthScore = calculatePasswordStrength(entry.password);
        }

        passwords.push_back(entry);
    }
    
    file.close();
    if (legacyDataMigrated) {
        savePasswords();
        legacyDataMigrated = false;
        std::cout << "✓ Migrated legacy passwords to new encryption format" << std::endl;
    }
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
        // 🆕 Extended format: url|username|password|created|updated|lastUsed|strength|breached|favorite|category|tags|usageCount|lastBreachCheck|notes

        // Join tags with commas
        std::string tagsStr;
        for (size_t i = 0; i < entry.tags.size(); i++) {
            tagsStr += entry.tags[i];
            if (i < entry.tags.size() - 1) tagsStr += ",";
        }

        std::string line = entry.url + "|" + entry.username + "|" +
                          entry.password + "|" + std::to_string(entry.createdAt) + "|" +
                          std::to_string(entry.updatedAt) + "|" + std::to_string(entry.lastUsedAt) + "|" +
                          std::to_string(entry.strengthScore) + "|" + (entry.breached ? "1" : "0") + "|" +
                          (entry.favorite ? "1" : "0") + "|" + entry.category + "|" + tagsStr + "|" +
                          std::to_string(entry.usageCount) + "|" + std::to_string(entry.lastBreachCheck) + "|" +
                          entry.notes;

        // Encrypt and save
        std::string encrypted = encrypt(line);
        if (!encrypted.empty()) {
            file << encrypted << "\n";
        }
    }
    
    file.close();
}

void BrayaPasswordManager::setPasswordSavingEnabled(bool enabled) {
    passwordSavingEnabled = enabled;
}

void BrayaPasswordManager::setMultiStepCaptureEnabled(bool enabled) {
    multiStepCaptureEnabled = enabled;
    saveSettings();
}

bool BrayaPasswordManager::requestUnlock(GtkWindow* parent) {
    return ensureUnlocked(parent);
}

bool BrayaPasswordManager::isVaultLocked() const {
    return masterPasswordConfigured && (vaultLocked || isUnlockExpired());
}

struct PwImportCtx { BrayaPasswordManager* mgr; GtkWidget* parent; };
struct PwExportCtx { BrayaPasswordManager* mgr; GtkWidget* parent; };

void BrayaPasswordManager::showPasswordManager(GtkWindow* parent) {
    if (!requestUnlock(parent)) {
        return;
    }

    GtkWindow* dialog = create_modal_dialog("🔑 Password Manager", parent, 1100, 650);
    GtkWidget* dialogWidget = GTK_WIDGET(dialog);
    activeVaultDialog = dialog;
    
    GtkWidget* mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_add_css_class(mainBox, "dialog-body");
    gtk_window_set_child(dialog, mainBox);
    
    // Header
    GtkWidget* headerLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(headerLabel), "<span size='x-large' weight='bold'>🔑 Saved Passwords</span>");
    gtk_box_append(GTK_BOX(mainBox), headerLabel);

    if (fallbackKeyActive) {
        GtkWidget* warningBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_widget_add_css_class(warningBox, "warning-banner");
        gtk_widget_set_margin_bottom(warningBox, 8);

        GtkWidget* warningIcon = gtk_image_new_from_icon_name("dialog-warning-symbolic");
        gtk_box_append(GTK_BOX(warningBox), warningIcon);

        GtkWidget* warningLabel = gtk_label_new(
            "Vault is using a temporary key. Create a master password to fully encrypt your credentials.");
        gtk_label_set_wrap(GTK_LABEL(warningLabel), TRUE);
        gtk_widget_set_halign(warningLabel, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(warningBox), warningLabel);

        gtk_box_append(GTK_BOX(mainBox), warningBox);
    }

    enum class VaultFilter {
        All = 0,
        Favorites,
        Recent,
        Weak,
        Breached
    };

    GtkWidget* contentRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_widget_set_hexpand(contentRow, TRUE);
    gtk_box_append(GTK_BOX(mainBox), contentRow);

    GtkWidget* navRail = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_add_css_class(navRail, "dialog-card");
    gtk_widget_add_css_class(navRail, "nav-rail");
    gtk_box_append(GTK_BOX(contentRow), navRail);

    GtkWidget* mainContent = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_widget_set_hexpand(mainContent, TRUE);
    gtk_widget_set_vexpand(mainContent, TRUE);
    gtk_box_append(GTK_BOX(contentRow), mainContent);

    GtkWidget* listPanel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_size_request(listPanel, 400, -1);  // Fixed width for list panel
    gtk_widget_set_vexpand(listPanel, TRUE);
    gtk_box_append(GTK_BOX(mainContent), listPanel);

    // Search box
    GtkWidget* searchEntry = gtk_search_entry_new();
    gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(searchEntry), "Search passwords...");
    gtk_widget_add_css_class(searchEntry, "dialog-search");
    gtk_box_append(GTK_BOX(listPanel), searchEntry);

    // Results count label
    std::string countText = std::to_string(passwords.size()) + (passwords.size() == 1 ? " password" : " passwords");
    GtkWidget* countLabel = gtk_label_new(countText.c_str());
    gtk_widget_set_halign(countLabel, GTK_ALIGN_START);
    gtk_widget_add_css_class(countLabel, "dim-label");
    gtk_widget_set_margin_top(countLabel, 4);
    gtk_widget_set_margin_bottom(countLabel, 4);
    gtk_box_append(GTK_BOX(listPanel), countLabel);

    // Scrolled window for password list
    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_box_append(GTK_BOX(listPanel), scrolled);

    GtkWidget* listBox = gtk_list_box_new();
    gtk_widget_add_css_class(listBox, "password-list");
    gtk_widget_add_css_class(listBox, "dialog-card");
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(listBox), GTK_SELECTION_SINGLE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), listBox);

    // Create scrolled window for detail pane to prevent expanding
    GtkWidget* detailScrolled = gtk_scrolled_window_new();
    gtk_widget_set_size_request(detailScrolled, 320, -1);  // Min width 320px
    gtk_scrolled_window_set_max_content_width(GTK_SCROLLED_WINDOW(detailScrolled), 450);  // Max width 450px
    gtk_widget_set_hexpand(detailScrolled, TRUE);
    gtk_widget_set_vexpand(detailScrolled, TRUE);
    gtk_box_append(GTK_BOX(mainContent), detailScrolled);

    GtkWidget* detailPane = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_add_css_class(detailPane, "dialog-card");
    gtk_widget_set_margin_start(detailPane, 12);
    gtk_widget_set_margin_end(detailPane, 12);
    gtk_widget_set_margin_top(detailPane, 12);
    gtk_widget_set_margin_bottom(detailPane, 12);
    gtk_widget_set_visible(detailPane, TRUE);  // Ensure it's visible
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(detailScrolled), detailPane);

    std::cout << "✓ Detail pane created and added to layout" << std::endl;
    g_object_set_data(G_OBJECT(detailPane), "parent_window", dialogWidget);
    g_object_set_data(G_OBJECT(listBox), "detail-pane", detailPane);
    renderDetailPlaceholder(detailPane);

    struct VaultFilterContext {
        GtkSearchEntry* search;
        GtkListBox* list;
        BrayaPasswordManager* manager;
        VaultFilter filter = VaultFilter::All;
    };

    auto* filterContext = new VaultFilterContext{GTK_SEARCH_ENTRY(searchEntry), GTK_LIST_BOX(listBox), this, VaultFilter::All};
    g_object_set_data_full(G_OBJECT(listBox), "filter-context", filterContext,
        +[](gpointer data) { delete static_cast<VaultFilterContext*>(data); });

    // Set up search filtering
    gtk_list_box_set_filter_func(GTK_LIST_BOX(listBox),
        +[](GtkListBoxRow* row, gpointer user_data) -> gboolean {
            auto* ctx = static_cast<VaultFilterContext*>(user_data);
            if (!ctx) {
                return TRUE;
            }

            // Get the child widget (the actual box we created) from the ListBoxRow
            GtkWidget* child = gtk_list_box_row_get_child(row);
            if (!child) {
                return TRUE;
            }

            const char* searchText = gtk_editable_get_text(GTK_EDITABLE(ctx->search));
            auto* entryData = static_cast<PasswordEntry*>(g_object_get_data(G_OBJECT(child), "entry_data"));

            auto matchesSearch = [&](const char* url, const char* username) {
                if (!searchText || strlen(searchText) == 0) {
                    return true;
                }
                if (!url && !username) {
                    return true;
                }
                std::string searchLower = searchText;
                std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

                auto contains = [&](const std::string& value) {
                    std::string lower = value;
                    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                    return lower.find(searchLower) != std::string::npos;
                };

                return contains(url ? url : "") || contains(username ? username : "");
            };

            if (!matchesSearch(
                    static_cast<const char*>(g_object_get_data(G_OBJECT(child), "url")),
                    static_cast<const char*>(g_object_get_data(G_OBJECT(child), "username")))) {
                return FALSE;
            }

            if (!entryData) {
                return TRUE;
            }

            switch (ctx->filter) {
                case VaultFilter::All:
                    return TRUE;
                case VaultFilter::Favorites:
                    return entryData->favorite;
                case VaultFilter::Recent: {
                    const long now = time(nullptr);
                    const long diff = now - entryData->updatedAt;
                    return entryData->updatedAt > 0 && diff <= 14 * 24 * 3600;
                }
                case VaultFilter::Weak:
                    return entryData->strengthScore > 0 && entryData->strengthScore < 50;
                case VaultFilter::Breached:
                    return entryData->breached;
            }
            return TRUE;
        }, filterContext, nullptr);

    // Trigger filter on search change
    g_signal_connect(searchEntry, "search-changed", G_CALLBACK(+[](GtkSearchEntry* entry, gpointer user_data) {
        GtkListBox* list = GTK_LIST_BOX(user_data);
        gtk_list_box_invalidate_filter(list);
    }), listBox);
    
    // Add password entries
    for (const auto& entry : passwords) {
        GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
        gtk_widget_set_margin_start(row, 10);
        gtk_widget_set_margin_end(row, 10);
        gtk_widget_set_margin_top(row, 10);
        gtk_widget_set_margin_bottom(row, 10);

        // Store data for search filtering
        g_object_set_data_full(G_OBJECT(row), "url", g_strdup(entry.url.c_str()), g_free);
        g_object_set_data_full(G_OBJECT(row), "username", g_strdup(entry.username.c_str()), g_free);
        auto* entryCopy = new PasswordEntry(entry);
        g_object_set_data_full(G_OBJECT(row), "entry_data", entryCopy,
            +[](gpointer data) { delete static_cast<PasswordEntry*>(data); });
        
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

        std::string previewText = buildPasswordPreview(entry.password);
        GtkWidget* passwordPreview = gtk_label_new(previewText.c_str());
        gtk_widget_set_halign(passwordPreview, GTK_ALIGN_START);
        gtk_widget_add_css_class(passwordPreview, "dim-label");
        gtk_box_append(GTK_BOX(infoBox), passwordPreview);

        GtkWidget* timestampLabel = gtk_label_new(formatTimestampRelative(entry.updatedAt).c_str());
        gtk_widget_set_halign(timestampLabel, GTK_ALIGN_START);
        gtk_widget_add_css_class(timestampLabel, "dim-label");
        gtk_box_append(GTK_BOX(infoBox), timestampLabel);

        std::string strengthSummary = "Strength: " + getPasswordStrengthLabel(entry.strengthScore) +
                                      " (" + std::to_string(entry.strengthScore) + "/100)";
        if (entry.breached) {
            strengthSummary += " • ⚠ Breach suspected";
        }
        GtkWidget* strengthLabel = gtk_label_new(strengthSummary.c_str());
        gtk_widget_set_halign(strengthLabel, GTK_ALIGN_START);
        gtk_widget_add_css_class(strengthLabel, "dim-label");

        // 🆕 Add color coding for strength
        if (entry.strengthScore >= 60) {
            gtk_widget_add_css_class(strengthLabel, "success");
        } else if (entry.strengthScore >= 40) {
            gtk_widget_add_css_class(strengthLabel, "warning");
        } else {
            gtk_widget_add_css_class(strengthLabel, "error");
        }
        gtk_box_append(GTK_BOX(infoBox), strengthLabel);

        gtk_box_append(GTK_BOX(row), infoBox);

        // 🆕 Add favorites button
        GtkWidget* favBtn = gtk_button_new_from_icon_name(entry.favorite ? "starred-symbolic" : "non-starred-symbolic");
        gtk_widget_set_tooltip_text(favBtn, entry.favorite ? "Remove from favorites" : "Add to favorites");
        gtk_widget_add_css_class(favBtn, "flat");
        gtk_widget_set_valign(favBtn, GTK_ALIGN_CENTER);

        // Store data for favorite toggle
        g_object_set_data_full(G_OBJECT(favBtn), "url", g_strdup(entry.url.c_str()), g_free);
        g_object_set_data_full(G_OBJECT(favBtn), "username", g_strdup(entry.username.c_str()), g_free);
        g_object_set_data(G_OBJECT(favBtn), "manager", this);
        g_object_set_data(G_OBJECT(favBtn), "list_box", listBox);

        g_signal_connect(favBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
            auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
            const char* url = static_cast<const char*>(g_object_get_data(G_OBJECT(btn), "url"));
            const char* username = static_cast<const char*>(g_object_get_data(G_OBJECT(btn), "username"));
            GtkListBox* list = GTK_LIST_BOX(g_object_get_data(G_OBJECT(btn), "list_box"));

            if (manager && url && username) {
                manager->toggleFavorite(url, username);

                // 🔄 Refresh the list to show updated star
                // Find parent window and refresh vault dialog
                GtkWidget* parent = gtk_widget_get_ancestor(GTK_WIDGET(btn), GTK_TYPE_WINDOW);
                if (parent) {
                    gtk_window_close(GTK_WINDOW(parent));
                    manager->showPasswordManager(GTK_WINDOW(gtk_widget_get_root(parent)));
                }
            }
        }), nullptr);

        gtk_box_append(GTK_BOX(row), favBtn);

        // 🆕 Add weak password warning icon
        if (entry.strengthScore < 40) {
            GtkWidget* warnIcon = gtk_image_new_from_icon_name("dialog-warning-symbolic");
            gtk_widget_set_tooltip_text(warnIcon, "⚠️ Weak password");
            gtk_widget_set_valign(warnIcon, GTK_ALIGN_CENTER);
            gtk_widget_add_css_class(warnIcon, "warning");
            gtk_box_append(GTK_BOX(row), warnIcon);
        }

        GtkWidget* buttonColumn = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_widget_set_halign(buttonColumn, GTK_ALIGN_END);

        GtkWidget* quickActionRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_widget_set_halign(quickActionRow, GTK_ALIGN_END);
        gtk_box_append(GTK_BOX(buttonColumn), quickActionRow);

        GtkWidget* copyUserBtn = gtk_button_new_with_label("Copy User");
        gtk_widget_add_css_class(copyUserBtn, "flat");
        g_object_set_data(G_OBJECT(copyUserBtn), "manager", this);
        g_object_set_data_full(G_OBJECT(copyUserBtn), "value", g_strdup(entry.username.c_str()), g_free);
        g_signal_connect(copyUserBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
            auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
            const char* value = (const char*)g_object_get_data(G_OBJECT(btn), "value");
            if (manager && value && strlen(value) > 0) {
                manager->copyToClipboard(GTK_WIDGET(btn), value, "username");
            }
        }), nullptr);
        gtk_widget_set_sensitive(copyUserBtn, !entry.username.empty());
        gtk_box_append(GTK_BOX(quickActionRow), copyUserBtn);

        GtkWidget* copyPasswordBtn = gtk_button_new_with_label("Copy Pass");
        gtk_widget_add_css_class(copyPasswordBtn, "flat");
        g_object_set_data(G_OBJECT(copyPasswordBtn), "manager", this);
        g_object_set_data(G_OBJECT(copyPasswordBtn), "parent_window", dialogWidget);
        g_object_set_data_full(G_OBJECT(copyPasswordBtn), "value", g_strdup(entry.password.c_str()), g_free);
        g_signal_connect(copyPasswordBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
            auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
            const char* value = (const char*)g_object_get_data(G_OBJECT(btn), "value");
            GtkWidget* parent = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "parent_window"));
            if (manager && value && strlen(value) > 0) {
                if (!manager->requireSensitiveUnlock(GTK_WINDOW(parent))) {
                    return;
                }
                manager->copyToClipboard(GTK_WIDGET(btn), value, "password");
            }
        }), nullptr);
        gtk_widget_set_sensitive(copyPasswordBtn, !entry.password.empty());
        gtk_box_append(GTK_BOX(quickActionRow), copyPasswordBtn);

        GtkWidget* revealBtn = gtk_toggle_button_new_with_label("Show");
        gtk_widget_add_css_class(revealBtn, "flat");
        g_object_set_data(G_OBJECT(revealBtn), "password_label", passwordPreview);
        g_object_set_data(G_OBJECT(revealBtn), "manager", this);
        g_object_set_data(G_OBJECT(revealBtn), "parent_window", dialogWidget);
        g_object_set_data_full(G_OBJECT(revealBtn), "password_value", g_strdup(entry.password.c_str()), g_free);
        g_object_set_data_full(G_OBJECT(revealBtn), "password_masked", g_strdup(previewText.c_str()), g_free);
        g_signal_connect(revealBtn, "toggled", G_CALLBACK(+[](GtkToggleButton* btn, gpointer data) {
            GtkWidget* label = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "password_label"));
            const char* plain = (const char*)g_object_get_data(G_OBJECT(btn), "password_value");
            const char* masked = (const char*)g_object_get_data(G_OBJECT(btn), "password_masked");
            auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
            GtkWidget* parent = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "parent_window"));
            if (!label) {
                return;
            }

            if (gtk_toggle_button_get_active(btn)) {
                if (manager && !manager->requireSensitiveUnlock(GTK_WINDOW(parent))) {
                    gtk_toggle_button_set_active(btn, FALSE);
                    gtk_label_set_text(GTK_LABEL(label), masked ? masked : "");
                    gtk_button_set_label(GTK_BUTTON(btn), "Show");
                    return;
                }
                gtk_label_set_text(GTK_LABEL(label), plain ? plain : "");
                gtk_button_set_label(GTK_BUTTON(btn), "Hide");
            } else {
                gtk_label_set_text(GTK_LABEL(label), masked ? masked : "");
                gtk_button_set_label(GTK_BUTTON(btn), "Show");
            }
        }), nullptr);
        gtk_widget_set_sensitive(revealBtn, !entry.password.empty());
        gtk_box_append(GTK_BOX(quickActionRow), revealBtn);

        GtkWidget* manageRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_widget_set_halign(manageRow, GTK_ALIGN_END);
        gtk_box_append(GTK_BOX(buttonColumn), manageRow);

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
        gtk_box_append(GTK_BOX(manageRow), editBtn);

        GtkWidget* deleteBtn = gtk_button_new_with_label("🗑️ Delete");
        gtk_widget_add_css_class(deleteBtn, "destructive-action");
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
        gtk_box_append(GTK_BOX(manageRow), deleteBtn);

        gtk_box_append(GTK_BOX(row), buttonColumn);
        
        gtk_list_box_append(GTK_LIST_BOX(listBox), row);
    }

    // Connect row-selected signal BEFORE auto-selecting
    auto* detailList = GTK_LIST_BOX(listBox);
    g_signal_connect(detailList, "row-selected", G_CALLBACK(+[](GtkListBox* lb, GtkListBoxRow* row, gpointer data) {
        auto* manager = static_cast<BrayaPasswordManager*>(data);
        GtkWidget* pane = GTK_WIDGET(g_object_get_data(G_OBJECT(lb), "detail-pane"));
        manager->updateDetailPane(pane, row);
    }), this);

    // Auto-select first row to show details immediately
    if (!passwords.empty()) {
        GtkListBoxRow* firstRow = gtk_list_box_get_row_at_index(GTK_LIST_BOX(listBox), 0);
        if (firstRow) {
            gtk_list_box_select_row(GTK_LIST_BOX(listBox), firstRow);
        }
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
    gtk_widget_add_css_class(actionBox, "dialog-action-bar");
    gtk_box_append(GTK_BOX(mainBox), actionBox);

    auto createNavButton = [&](const char* label, const char* description, VaultFilter filterValue) {
        GtkWidget* btn = gtk_toggle_button_new();
        gtk_widget_add_css_class(btn, "nav-rail-button");
        gtk_widget_set_halign(btn, GTK_ALIGN_FILL);
        gtk_widget_set_hexpand(btn, TRUE);

        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        gtk_widget_set_margin_start(inner, 10);
        gtk_widget_set_margin_end(inner, 10);
        gtk_widget_set_margin_top(inner, 8);
        gtk_widget_set_margin_bottom(inner, 8);
        gtk_button_set_child(GTK_BUTTON(btn), inner);

        GtkWidget* title = gtk_label_new(label);
        gtk_widget_set_halign(title, GTK_ALIGN_START);
        gtk_label_set_xalign(GTK_LABEL(title), 0.0);
        gtk_widget_add_css_class(title, "nav-rail-title");
        gtk_box_append(GTK_BOX(inner), title);

        if (description && strlen(description) > 0) {
            GtkWidget* subtitle = gtk_label_new(description);
            gtk_widget_set_halign(subtitle, GTK_ALIGN_START);
            gtk_label_set_xalign(GTK_LABEL(subtitle), 0.0);
            gtk_widget_add_css_class(subtitle, "dim-label");
            gtk_box_append(GTK_BOX(inner), subtitle);
        }

        g_object_set_data(G_OBJECT(btn), "filter-context", filterContext);
        g_object_set_data(G_OBJECT(btn), "filter-value",
                          GINT_TO_POINTER(static_cast<int>(filterValue)));

        g_signal_connect(btn, "toggled", G_CALLBACK(+[](GtkToggleButton* button, gpointer) {
            if (!gtk_toggle_button_get_active(button)) {
                return;
            }
            auto* ctx = static_cast<VaultFilterContext*>(g_object_get_data(G_OBJECT(button), "filter-context"));
            if (!ctx) {
                return;
            }
            auto filterValue = static_cast<VaultFilter>(GPOINTER_TO_INT(
                g_object_get_data(G_OBJECT(button), "filter-value")));
            ctx->filter = filterValue;
            gtk_list_box_unselect_all(ctx->list);
            gtk_list_box_invalidate_filter(ctx->list);
            GtkWidget* pane = GTK_WIDGET(g_object_get_data(G_OBJECT(ctx->list), "detail-pane"));
            if (pane && ctx->manager) {
                ctx->manager->renderDetailPlaceholder(pane);
            }
        }), nullptr);

        return btn;
    };

    GtkToggleButton* navGroup = nullptr;
    auto appendNavButton = [&](GtkWidget* button) {
        if (!navGroup) {
            navGroup = GTK_TOGGLE_BUTTON(button);
        } else {
            gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(button), navGroup);
        }
        gtk_box_append(GTK_BOX(navRail), button);
    };

    appendNavButton(createNavButton("All logins", "Everything in your vault", VaultFilter::All));
    appendNavButton(createNavButton("Favorites", "Your starred passwords", VaultFilter::Favorites));
    appendNavButton(createNavButton("Recently updated", "Changed in the last 14 days", VaultFilter::Recent));
    appendNavButton(createNavButton("Weak passwords", "Needs attention", VaultFilter::Weak));
    appendNavButton(createNavButton("Breached", "Marked as compromised", VaultFilter::Breached));
    gtk_toggle_button_set_active(navGroup, TRUE);

    // Left side buttons (actions)
    GtkWidget* leftBtnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_hexpand(leftBtnBox, TRUE);
    gtk_box_append(GTK_BOX(actionBox), leftBtnBox);

    GtkWidget* addBtn = gtk_button_new_with_label("➕ Add");
    gtk_widget_add_css_class(addBtn, "suggested-action");
    g_object_set_data(G_OBJECT(addBtn), "manager", this);
    g_object_set_data(G_OBJECT(addBtn), "parent", dialogWidget);
    g_signal_connect(addBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
        GtkWidget* parent = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "parent"));
        manager->addPasswordManually(GTK_WINDOW(parent));
    }), nullptr);
    gtk_box_append(GTK_BOX(leftBtnBox), addBtn);

    GtkWidget* importBtn = gtk_button_new_with_label("📥 Import");
    g_object_set_data(G_OBJECT(importBtn), "manager", this);
    g_object_set_data(G_OBJECT(importBtn), "parent", dialogWidget);
    g_signal_connect(importBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
        GtkWidget* parent = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "parent"));

        auto* ctx = new PwImportCtx();
        ctx->mgr = manager; ctx->parent = parent;

        GtkFileDialog* fd = gtk_file_dialog_new();
        gtk_file_dialog_set_title(fd, "Import Passwords");
        GtkFileFilter* filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, "CSV Files");
        gtk_file_filter_add_pattern(filter, "*.csv");
        GListStore* filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
        g_list_store_append(filters, filter);
        g_object_unref(filter);
        gtk_file_dialog_set_filters(fd, G_LIST_MODEL(filters));
        g_object_unref(filters);
        gtk_file_dialog_open(fd, GTK_WINDOW(parent), nullptr,
            [](GObject* src, GAsyncResult* res, gpointer data) {
                auto* ctx = static_cast<PwImportCtx*>(data);
                GError* err = nullptr;
                GFile* file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(src), res, &err);
                if (file) {
                    char* path = g_file_get_path(file);
                    if (path) {
                        if (ctx->mgr->importFromCSV(path)) {
                            ctx->mgr->showSuccessDialog(GTK_WINDOW(ctx->parent),
                                "Import Successful", "Successfully imported passwords from CSV file!");
                        } else {
                            ctx->mgr->showErrorDialog(GTK_WINDOW(ctx->parent),
                                "Import Failed", "Could not import passwords. Check the CSV file format.");
                        }
                        g_free(path);
                    }
                    g_object_unref(file);
                }
                if (err) g_error_free(err);
                g_object_unref(src);
                delete ctx;
            }, ctx);
    }), nullptr);
    gtk_box_append(GTK_BOX(leftBtnBox), importBtn);

    GtkWidget* exportBtn = gtk_button_new_with_label("📤 Export");
    g_object_set_data(G_OBJECT(exportBtn), "manager", this);
    g_object_set_data(G_OBJECT(exportBtn), "parent", dialogWidget);
    g_signal_connect(exportBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
        GtkWidget* parent = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "parent"));

        if (!manager->requireSensitiveUnlock(GTK_WINDOW(parent))) {
            return;
        }

        auto* ctx = new PwExportCtx();
        ctx->mgr = manager; ctx->parent = parent;

        GtkFileDialog* fd = gtk_file_dialog_new();
        gtk_file_dialog_set_title(fd, "Export Passwords");
        gtk_file_dialog_set_initial_name(fd, "braya-passwords.csv");
        gtk_file_dialog_save(fd, GTK_WINDOW(parent), nullptr,
            [](GObject* src, GAsyncResult* res, gpointer data) {
                auto* ctx = static_cast<PwExportCtx*>(data);
                GError* err = nullptr;
                GFile* file = gtk_file_dialog_save_finish(GTK_FILE_DIALOG(src), res, &err);
                if (file) {
                    char* path = g_file_get_path(file);
                    if (path) {
                        if (ctx->mgr->exportToCSV(path)) {
                            std::string msg = "Successfully exported passwords to:\n" + std::string(path);
                            ctx->mgr->showSuccessDialog(GTK_WINDOW(ctx->parent), "Export Successful", msg);
                        } else {
                            ctx->mgr->showErrorDialog(GTK_WINDOW(ctx->parent), "Export Failed",
                                "Could not export passwords to the selected file.");
                        }
                        g_free(path);
                    }
                    g_object_unref(file);
                }
                if (err) g_error_free(err);
                g_object_unref(src);
                delete ctx;
            }, ctx);
    }), nullptr);
    gtk_box_append(GTK_BOX(leftBtnBox), exportBtn);

    // Right side buttons
    GtkWidget* rightBtnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(actionBox), rightBtnBox);

    if (!passwords.empty()) {
        GtkWidget* clearBtn = gtk_button_new_with_label("Clear All");
        gtk_widget_add_css_class(clearBtn, "destructive-action");
        g_object_set_data(G_OBJECT(clearBtn), "manager", this);
        g_object_set_data(G_OBJECT(clearBtn), "dialog", dialogWidget);
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

    g_signal_connect(dialog, "close-request", G_CALLBACK(+[](GtkWindow* dialog, gpointer data) {
        auto* manager = static_cast<BrayaPasswordManager*>(data);
        if (!manager) {
            return FALSE;
        }
        manager->activeVaultDialog = nullptr;
        manager->clearIdleConcealTimer();
        return FALSE;
    }), this);

    gtk_window_present(dialog);
}

// Add password manually
void BrayaPasswordManager::addPasswordManually(GtkWindow* parent) {
    if (!requestUnlock(parent)) {
        return;
    }

    GtkWindow* dialog = create_modal_dialog("Add Password", parent, 420, 320);
    GtkWidget* dialogWidget = GTK_WIDGET(dialog);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_add_css_class(box, "dialog-body");
    gtk_window_set_child(dialog, box);

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

    // Password field with mode selector
    GtkWidget* passwordHeaderBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(box), passwordHeaderBox);

    GtkWidget* passwordLabel = gtk_label_new("Password:");
    gtk_widget_set_halign(passwordLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(passwordHeaderBox), passwordLabel);

    const char* genModes[] = {"Random", "Passphrase", nullptr};
    GtkWidget* genModeDropdown = gtk_drop_down_new_from_strings(genModes);
    gtk_widget_set_tooltip_text(genModeDropdown, "Generator mode");
    gtk_box_append(GTK_BOX(passwordHeaderBox), genModeDropdown);

    // Password entry with controls
    GtkWidget* passwordBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_append(GTK_BOX(box), passwordBox);

    GtkWidget* passwordEntry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(passwordEntry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(passwordEntry), "••••••••");
    gtk_widget_set_hexpand(passwordEntry, TRUE);
    gtk_box_append(GTK_BOX(passwordBox), passwordEntry);

    // Show/Hide password toggle
    GtkWidget* toggleBtn = gtk_button_new_with_label("👁");
    gtk_widget_set_tooltip_text(toggleBtn, "Show/Hide Password");
    g_object_set_data(G_OBJECT(toggleBtn), "password_entry", passwordEntry);
    g_signal_connect(toggleBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        GtkEntry* entry = GTK_ENTRY(g_object_get_data(G_OBJECT(btn), "password_entry"));
        bool visible = gtk_entry_get_visibility(entry);
        gtk_entry_set_visibility(entry, !visible);
        gtk_button_set_label(btn, visible ? "👁" : "👁‍🗨");
    }), nullptr);
    gtk_box_append(GTK_BOX(passwordBox), toggleBtn);

    // Password generator button
    GtkWidget* generateBtn = gtk_button_new_with_label("🎲");
    gtk_widget_set_tooltip_text(generateBtn, "Generate Password");
    g_object_set_data(G_OBJECT(generateBtn), "manager", this);
    g_object_set_data(G_OBJECT(generateBtn), "password_entry", passwordEntry);
    g_object_set_data(G_OBJECT(generateBtn), "gen_mode", genModeDropdown);
    g_signal_connect(generateBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
        GtkEntry* entry = GTK_ENTRY(g_object_get_data(G_OBJECT(btn), "password_entry"));
        GtkDropDown* modeDropdown = GTK_DROP_DOWN(g_object_get_data(G_OBJECT(btn), "gen_mode"));

        std::string generated;
        guint mode = gtk_drop_down_get_selected(modeDropdown);
        if (mode == 1) {
            // Passphrase mode
            generated = manager->generatePassphrase(4, '-', true, true);
        } else {
            // Random password mode
            generated = manager->generatePassword(16, true, true, true, true);
        }
        gtk_editable_set_text(GTK_EDITABLE(entry), generated.c_str());

        // Temporarily show it
        gtk_entry_set_visibility(entry, TRUE);
    }), nullptr);
    gtk_box_append(GTK_BOX(passwordBox), generateBtn);

    // Password strength indicator
    GtkWidget* strengthLabel = gtk_label_new("");
    gtk_widget_set_halign(strengthLabel, GTK_ALIGN_START);
    gtk_widget_add_css_class(strengthLabel, "dim-label");
    gtk_box_append(GTK_BOX(box), strengthLabel);

    // Update strength on password change
    g_object_set_data(G_OBJECT(passwordEntry), "manager", this);
    g_object_set_data(G_OBJECT(passwordEntry), "strength_label", strengthLabel);
    g_signal_connect(passwordEntry, "changed", G_CALLBACK(+[](GtkEditable* editable, gpointer data) {
        auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(editable), "manager"));
        GtkLabel* label = GTK_LABEL(g_object_get_data(G_OBJECT(editable), "strength_label"));

        const char* password = gtk_editable_get_text(editable);
        if (password && strlen(password) > 0) {
            int strength = manager->calculatePasswordStrength(password);
            std::string strengthText = "Strength: " + manager->getPasswordStrengthLabel(strength) +
                                      " (" + std::to_string(strength) + "/100)";
            gtk_label_set_text(label, strengthText.c_str());

            // Color code the strength
            gtk_widget_remove_css_class(GTK_WIDGET(label), "error");
            gtk_widget_remove_css_class(GTK_WIDGET(label), "warning");
            gtk_widget_remove_css_class(GTK_WIDGET(label), "success");

            if (strength >= 60) {
                gtk_widget_add_css_class(GTK_WIDGET(label), "success");
            } else if (strength >= 40) {
                gtk_widget_add_css_class(GTK_WIDGET(label), "warning");
            } else {
                gtk_widget_add_css_class(GTK_WIDGET(label), "error");
            }
        } else {
            gtk_label_set_text(label, "");
        }
    }), nullptr);

    // Category dropdown
    GtkWidget* categoryLabel = gtk_label_new("Category:");
    gtk_widget_set_halign(categoryLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), categoryLabel);

    const char* categories[] = {"Personal", "Work", "Banking", "Social", "Shopping", "Other", nullptr};
    GtkWidget* categoryDropdown = gtk_drop_down_new_from_strings(categories);
    gtk_box_append(GTK_BOX(box), categoryDropdown);

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
    g_object_set_data(G_OBJECT(saveBtn), "category_dropdown", categoryDropdown);
    g_object_set_data(G_OBJECT(saveBtn), "dialog", dialogWidget);
    g_object_set_data(G_OBJECT(saveBtn), "parent", parent);

    g_signal_connect(saveBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
        GtkEntry* urlEntry = GTK_ENTRY(g_object_get_data(G_OBJECT(btn), "url_entry"));
        GtkEntry* userEntry = GTK_ENTRY(g_object_get_data(G_OBJECT(btn), "username_entry"));
        GtkEntry* passEntry = GTK_ENTRY(g_object_get_data(G_OBJECT(btn), "password_entry"));
        GtkDropDown* categoryDropdown = GTK_DROP_DOWN(g_object_get_data(G_OBJECT(btn), "category_dropdown"));
        GtkWidget* dialog = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "dialog"));
        GtkWindow* parent = GTK_WINDOW(g_object_get_data(G_OBJECT(btn), "parent"));

        const char* url = gtk_editable_get_text(GTK_EDITABLE(urlEntry));
        const char* username = gtk_editable_get_text(GTK_EDITABLE(userEntry));
        const char* password = gtk_editable_get_text(GTK_EDITABLE(passEntry));

        if (url && username && password && strlen(url) > 0 && strlen(username) > 0 && strlen(password) > 0) {
            manager->savePassword(url, username, password);

            // Set category
            guint selected = gtk_drop_down_get_selected(categoryDropdown);
            const char* categoryName = (selected == 0) ? "Personal" :
                                       (selected == 1) ? "Work" :
                                       (selected == 2) ? "Banking" :
                                       (selected == 3) ? "Social" :
                                       (selected == 4) ? "Shopping" : "Other";
            manager->setCategory(url, username, categoryName);

            gtk_window_close(GTK_WINDOW(dialog));
            manager->showPasswordManager(parent);
        }
    }), nullptr);

    gtk_box_append(GTK_BOX(buttonBox), saveBtn);

    gtk_window_present(dialog);
}

// Edit password
void BrayaPasswordManager::editPassword(const std::string& url, const std::string& username, GtkWindow* parent) {
    if (!requestUnlock(parent)) {
        return;
    }
    if (!requireSensitiveUnlock(parent)) {
        return;
    }

    // Find the existing entry
    PasswordEntry* entry = nullptr;
    for (auto& e : passwords) {
        if (e.url == url && e.username == username) {
            entry = &e;
            break;
        }
    }

    if (!entry) return;

    GtkWindow* dialog = create_modal_dialog("Edit Password", parent, 420, 280);
    GtkWidget* dialogWidget = GTK_WIDGET(dialog);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_add_css_class(box, "dialog-body");
    gtk_window_set_child(dialog, box);

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

    // Category dropdown
    GtkWidget* categoryLabel = gtk_label_new("Category:");
    gtk_widget_set_halign(categoryLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), categoryLabel);

    const char* editCategories[] = {"Personal", "Work", "Banking", "Social", "Shopping", "Other", nullptr};
    GtkWidget* categoryDropdown = gtk_drop_down_new_from_strings(editCategories);

    // Set current category
    for (guint i = 0; i < 6; i++) {
        if (entry->category == editCategories[i]) {
            gtk_drop_down_set_selected(GTK_DROP_DOWN(categoryDropdown), i);
            break;
        }
    }
    gtk_box_append(GTK_BOX(box), categoryDropdown);

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
    g_object_set_data(G_OBJECT(saveBtn), "category_dropdown", categoryDropdown);
    g_object_set_data(G_OBJECT(saveBtn), "dialog", dialogWidget);
    g_object_set_data(G_OBJECT(saveBtn), "parent", parent);

    g_signal_connect(saveBtn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer data) {
        auto* manager = static_cast<BrayaPasswordManager*>(g_object_get_data(G_OBJECT(btn), "manager"));
        const char* oldUsername = (const char*)g_object_get_data(G_OBJECT(btn), "old_username");
        const char* url = (const char*)g_object_get_data(G_OBJECT(btn), "url");
        GtkEntry* userEntry = GTK_ENTRY(g_object_get_data(G_OBJECT(btn), "username_entry"));
        GtkEntry* passEntry = GTK_ENTRY(g_object_get_data(G_OBJECT(btn), "password_entry"));
        GtkDropDown* categoryDropdown = GTK_DROP_DOWN(g_object_get_data(G_OBJECT(btn), "category_dropdown"));
        GtkWidget* dialog = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "dialog"));
        GtkWindow* parent = GTK_WINDOW(g_object_get_data(G_OBJECT(btn), "parent"));

        const char* newUsername = gtk_editable_get_text(GTK_EDITABLE(userEntry));
        const char* newPassword = gtk_editable_get_text(GTK_EDITABLE(passEntry));

        if (newUsername && newPassword && strlen(newUsername) > 0 && strlen(newPassword) > 0) {
            manager->updatePassword(url, oldUsername, newUsername, newPassword);

            // Update category
            guint selected = gtk_drop_down_get_selected(categoryDropdown);
            const char* categoryName = (selected == 0) ? "Personal" :
                                       (selected == 1) ? "Work" :
                                       (selected == 2) ? "Banking" :
                                       (selected == 3) ? "Social" :
                                       (selected == 4) ? "Shopping" : "Other";
            manager->setCategory(url, newUsername, categoryName);

            gtk_window_close(GTK_WINDOW(dialog));
            manager->showPasswordManager(parent);
        }
    }), nullptr);

    gtk_box_append(GTK_BOX(buttonBox), saveBtn);

    gtk_window_present(dialog);
}

// Update password
void BrayaPasswordManager::updatePassword(const std::string& url, const std::string& oldUsername,
                                         const std::string& newUsername, const std::string& newPassword) {
    if (masterPasswordConfigured && (vaultLocked || isUnlockExpired())) {
        lockVault();
        std::cerr << "✗ Cannot update password while vault is locked" << std::endl;
        return;
    }

    refreshUnlockTimer();

    for (auto& entry : passwords) {
        if (entry.url == url && entry.username == oldUsername) {
            entry.username = newUsername;
            entry.password = newPassword;
            entry.updatedAt = time(nullptr);
            entry.strengthScore = calculatePasswordStrength(newPassword);
            savePasswords();
            std::cout << "✓ Password updated for " << newUsername << " @ " << url << std::endl;
            return;
        }
    }
}

// Export to CSV
bool BrayaPasswordManager::exportToCSV(const std::string& filepath) {
    if (masterPasswordConfigured && (vaultLocked || isUnlockExpired())) {
        lockVault();
        std::cerr << "✗ Unlock the vault before exporting passwords" << std::endl;
        return false;
    }

    refreshUnlockTimer();

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
    if (masterPasswordConfigured && (vaultLocked || isUnlockExpired())) {
        lockVault();
        std::cerr << "✗ Unlock the vault before importing passwords" << std::endl;
        return false;
    }

    refreshUnlockTimer();

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

// Password Generator
std::string BrayaPasswordManager::generatePassword(int length, bool includeSymbols, bool includeNumbers, bool includeUppercase, bool includeLowercase) {
    std::string charset;

    if (includeLowercase) charset += "abcdefghijklmnopqrstuvwxyz";
    if (includeUppercase) charset += "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (includeNumbers) charset += "0123456789";
    if (includeSymbols) charset += "!@#$%^&*()_+-=[]{}|;:,.<>?";

    if (charset.empty()) {
        // Fallback to all characters if nothing selected
        charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=[]{}|;:,.<>?";
    }

    std::string password;
    password.reserve(length);

    // Use OpenSSL's RAND_bytes for cryptographically secure random
    unsigned char random_bytes[256];
    RAND_bytes(random_bytes, length);

    for (int i = 0; i < length; i++) {
        password += charset[random_bytes[i] % charset.size()];
    }

    return password;
}

int BrayaPasswordManager::calculatePasswordStrength(const std::string& password) {
    if (password.empty()) return 0;

    int score = 0;
    int length = password.length();

    // Length scoring (max 40 points)
    if (length >= 16) score += 40;
    else if (length >= 12) score += 30;
    else if (length >= 8) score += 20;
    else if (length >= 6) score += 10;
    else score += 5;

    // Character variety (max 40 points)
    bool hasLower = false, hasUpper = false, hasNumber = false, hasSymbol = false;
    for (char c : password) {
        if (islower(c)) hasLower = true;
        else if (isupper(c)) hasUpper = true;
        else if (isdigit(c)) hasNumber = true;
        else hasSymbol = true;
    }

    int variety = (hasLower ? 1 : 0) + (hasUpper ? 1 : 0) + (hasNumber ? 1 : 0) + (hasSymbol ? 1 : 0);
    score += variety * 10;

    // Penalize common patterns (max -20 points)
    std::string lower = password;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower.find("password") != std::string::npos) score -= 20;
    else if (lower.find("123456") != std::string::npos) score -= 15;
    else if (lower.find("qwerty") != std::string::npos) score -= 15;
    else if (lower.find("abc") != std::string::npos) score -= 10;

    // Bonus for very long passwords (max 20 points)
    if (length >= 20) score += 20;
    else if (length >= 18) score += 10;

    // Clamp to 0-100
    if (score < 0) score = 0;
    if (score > 100) score = 100;

    return score;
}

std::string BrayaPasswordManager::getPasswordStrengthLabel(int strength) {
    if (strength >= 80) return "Strong";
    else if (strength >= 60) return "Good";
    else if (strength >= 40) return "Fair";
    else if (strength >= 20) return "Weak";
    else return "Very Weak";
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
