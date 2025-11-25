// Password Manager Extensions - New features for enhanced functionality
// This file contains implementations for the improved password manager

#include "BrayaPasswordManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <map>
#include <openssl/rand.h>

// 🆕 Passphrase Generator using EFF wordlist
std::string BrayaPasswordManager::generatePassphrase(int wordCount, char separator, bool includeNumber, bool capitalize) {
    // Load EFF wordlist
    std::vector<std::string> wordlist;

    // Try multiple locations for wordlist
    std::vector<std::string> wordlistPaths = {
        configDirectory + "/../share/braya-browser/eff-wordlist-short.txt",
        "/usr/share/braya-browser/eff-wordlist-short.txt",
        "./resources/eff-wordlist-short.txt",
        "../resources/eff-wordlist-short.txt"
    };

    std::string wordlistPath;
    for (const auto& path : wordlistPaths) {
        std::ifstream test(path);
        if (test.good()) {
            wordlistPath = path;
            test.close();
            break;
        }
    }

    if (wordlistPath.empty()) {
        std::cerr << "⚠️  EFF wordlist not found, falling back to random password" << std::endl;
        return generatePassword(16, true, true, true, true);
    }

    // Load words from file
    std::ifstream file(wordlistPath);
    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;
        wordlist.push_back(line);
    }
    file.close();

    if (wordlist.empty()) {
        std::cerr << "⚠️  No words loaded from wordlist" << std::endl;
        return generatePassword(16, true, true, true, true);
    }

    std::cout << "📖 Loaded " << wordlist.size() << " words from EFF wordlist" << std::endl;

    // Generate random indices using cryptographically secure random
    std::vector<std::string> selectedWords;
    unsigned char randomBytes[32];
    RAND_bytes(randomBytes, sizeof(randomBytes));

    for (int i = 0; i < wordCount; i++) {
        // Use secure random to select word
        uint32_t randomValue = 0;
        memcpy(&randomValue, &randomBytes[i * 4 % 32], sizeof(uint32_t));
        size_t index = randomValue % wordlist.size();

        std::string word = wordlist[index];

        // Capitalize if requested
        if (capitalize && !word.empty()) {
            word[0] = toupper(word[0]);
        }

        selectedWords.push_back(word);
    }

    // Build passphrase
    std::string passphrase;
    for (size_t i = 0; i < selectedWords.size(); i++) {
        passphrase += selectedWords[i];
        if (i < selectedWords.size() - 1) {
            passphrase += separator;
        }
    }

    // Add random number if requested
    if (includeNumber) {
        unsigned char numByte;
        RAND_bytes(&numByte, 1);
        int randomNum = (numByte % 90) + 10; // 10-99
        passphrase += separator;
        passphrase += std::to_string(randomNum);
    }

    std::cout << "🎲 Generated passphrase: " << wordCount << " words, "
              << passphrase.length() << " characters" << std::endl;

    return passphrase;
}

// 🆕 Toggle favorite status
void BrayaPasswordManager::toggleFavorite(const std::string& url, const std::string& username) {
    if (masterPasswordConfigured && (vaultLocked || isUnlockExpired())) {
        lockVault();
        std::cerr << "✗ Cannot toggle favorite while vault is locked" << std::endl;
        return;
    }

    refreshUnlockTimer();
    std::string domain = getDomain(url);

    for (auto& entry : passwords) {
        if (entry.url == domain && entry.username == username) {
            entry.favorite = !entry.favorite;
            savePasswords();
            std::cout << (entry.favorite ? "⭐" : "☆") << " " << username << " @ " << domain << std::endl;
            return;
        }
    }
}

// 🆕 Set category
void BrayaPasswordManager::setCategory(const std::string& url, const std::string& username, const std::string& category) {
    if (masterPasswordConfigured && (vaultLocked || isUnlockExpired())) {
        lockVault();
        std::cerr << "✗ Cannot set category while vault is locked" << std::endl;
        return;
    }

    refreshUnlockTimer();
    std::string domain = getDomain(url);

    for (auto& entry : passwords) {
        if (entry.url == domain && entry.username == username) {
            entry.category = category;
            savePasswords();
            std::cout << "📁 Set category to '" << category << "' for " << username << " @ " << domain << std::endl;
            return;
        }
    }
}

// 🆕 Add tag
void BrayaPasswordManager::addTag(const std::string& url, const std::string& username, const std::string& tag) {
    if (masterPasswordConfigured && (vaultLocked || isUnlockExpired())) {
        lockVault();
        std::cerr << "✗ Cannot add tag while vault is locked" << std::endl;
        return;
    }

    refreshUnlockTimer();
    std::string domain = getDomain(url);

    for (auto& entry : passwords) {
        if (entry.url == domain && entry.username == username) {
            // Check if tag already exists
            if (std::find(entry.tags.begin(), entry.tags.end(), tag) == entry.tags.end()) {
                entry.tags.push_back(tag);
                savePasswords();
                std::cout << "🏷️  Added tag '" << tag << "' to " << username << " @ " << domain << std::endl;
            }
            return;
        }
    }
}

// 🆕 Remove tag
void BrayaPasswordManager::removeTag(const std::string& url, const std::string& username, const std::string& tag) {
    if (masterPasswordConfigured && (vaultLocked || isUnlockExpired())) {
        lockVault();
        std::cerr << "✗ Cannot remove tag while vault is locked" << std::endl;
        return;
    }

    refreshUnlockTimer();
    std::string domain = getDomain(url);

    for (auto& entry : passwords) {
        if (entry.url == domain && entry.username == username) {
            auto it = std::find(entry.tags.begin(), entry.tags.end(), tag);
            if (it != entry.tags.end()) {
                entry.tags.erase(it);
                savePasswords();
                std::cout << "🏷️  Removed tag '" << tag << "' from " << username << " @ " << domain << std::endl;
            }
            return;
        }
    }
}

// 🆕 Get favorite passwords
std::vector<PasswordEntry> BrayaPasswordManager::getFavorites() {
    if (masterPasswordConfigured && (vaultLocked || isUnlockExpired())) {
        lockVault();
        return {};
    }

    refreshUnlockTimer();

    std::vector<PasswordEntry> favorites;
    for (const auto& entry : passwords) {
        if (entry.favorite) {
            favorites.push_back(entry);
        }
    }

    return favorites;
}

// 🆕 Get passwords by category
std::vector<PasswordEntry> BrayaPasswordManager::getByCategory(const std::string& category) {
    if (masterPasswordConfigured && (vaultLocked || isUnlockExpired())) {
        lockVault();
        return {};
    }

    refreshUnlockTimer();

    std::vector<PasswordEntry> result;
    for (const auto& entry : passwords) {
        if (entry.category == category) {
            result.push_back(entry);
        }
    }

    return result;
}

// 🆕 Get weak passwords (strength < 40)
std::vector<PasswordEntry> BrayaPasswordManager::getWeakPasswords() {
    if (masterPasswordConfigured && (vaultLocked || isUnlockExpired())) {
        lockVault();
        return {};
    }

    refreshUnlockTimer();

    std::vector<PasswordEntry> weak;
    for (const auto& entry : passwords) {
        if (entry.strengthScore < 40) {
            weak.push_back(entry);
        }
    }

    std::cout << "⚠️  Found " << weak.size() << " weak passwords" << std::endl;
    return weak;
}

// 🆕 Get reused passwords
std::vector<PasswordEntry> BrayaPasswordManager::getReusedPasswords() {
    if (masterPasswordConfigured && (vaultLocked || isUnlockExpired())) {
        lockVault();
        return {};
    }

    refreshUnlockTimer();

    // Build map of password -> list of entries
    std::map<std::string, std::vector<PasswordEntry>> passwordMap;
    for (const auto& entry : passwords) {
        passwordMap[entry.password].push_back(entry);
    }

    // Find passwords used on multiple sites
    std::vector<PasswordEntry> reused;
    for (const auto& pair : passwordMap) {
        if (pair.second.size() > 1) {
            // This password is reused
            for (const auto& entry : pair.second) {
                reused.push_back(entry);
            }
        }
    }

    std::cout << "🔁 Found " << reused.size() << " reused passwords across "
              << passwordMap.size() << " unique passwords" << std::endl;

    return reused;
}

// 🆕 Search passwords
std::vector<PasswordEntry> BrayaPasswordManager::searchPasswords(const std::string& query) {
    if (masterPasswordConfigured && (vaultLocked || isUnlockExpired())) {
        lockVault();
        return {};
    }

    refreshUnlockTimer();

    if (query.empty()) {
        return passwords; // Return all if no query
    }

    // Convert query to lowercase for case-insensitive search
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

    std::vector<PasswordEntry> results;
    for (const auto& entry : passwords) {
        // Search in URL
        std::string lowerUrl = entry.url;
        std::transform(lowerUrl.begin(), lowerUrl.end(), lowerUrl.begin(), ::tolower);
        if (lowerUrl.find(lowerQuery) != std::string::npos) {
            results.push_back(entry);
            continue;
        }

        // Search in username
        std::string lowerUsername = entry.username;
        std::transform(lowerUsername.begin(), lowerUsername.end(), lowerUsername.begin(), ::tolower);
        if (lowerUsername.find(lowerQuery) != std::string::npos) {
            results.push_back(entry);
            continue;
        }

        // Search in category
        std::string lowerCategory = entry.category;
        std::transform(lowerCategory.begin(), lowerCategory.end(), lowerCategory.begin(), ::tolower);
        if (lowerCategory.find(lowerQuery) != std::string::npos) {
            results.push_back(entry);
            continue;
        }

        // Search in tags
        for (const auto& tag : entry.tags) {
            std::string lowerTag = tag;
            std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::tolower);
            if (lowerTag.find(lowerQuery) != std::string::npos) {
                results.push_back(entry);
                break;
            }
        }
    }

    std::cout << "🔍 Found " << results.size() << " results for '" << query << "'" << std::endl;
    return results;
}
