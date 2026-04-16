#include "ExtensionInstaller.h"
#include "BrayaExtensionManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>

ExtensionInstaller::ExtensionInstaller(BrayaExtensionManager* manager)
    : m_extensionManager(manager) {
    std::cout << "📦 ExtensionInstaller created (manager: " << (manager ? "valid" : "NULL") << ")" << std::endl;

    if (!manager) {
        std::cerr << "❌ WARNING: ExtensionInstaller created with NULL manager!" << std::endl;
    }
}

void ExtensionInstaller::installFromUrl(const std::string& url,
                                        std::function<void(bool, const std::string&)> callback) {
    std::cout << "📦 Installing extension from URL: " << url << std::endl;

    // Determine the download URL
    std::string downloadUrl = url;

    // If it's a Chrome Web Store URL, convert it
    if (url.find("chrome.google.com/webstore") != std::string::npos) {
        downloadUrl = getChromeExtensionDownloadUrl(url);
        std::cout << "  → Converted to download URL: " << downloadUrl << std::endl;
    }
    // If it's a Firefox Add-ons URL, convert it
    else if (url.find("addons.mozilla.org") != std::string::npos) {
        downloadUrl = getFirefoxExtensionDownloadUrl(url);
        std::cout << "  → Converted to download URL: " << downloadUrl << std::endl;
    }

    // Create temp directory for download
    std::string homeDir = getenv("HOME") ? getenv("HOME") : "/tmp";
    std::string tempDir = homeDir + "/.cache/braya-browser/extension-downloads";

    // Create directory if it doesn't exist
    system(("mkdir -p " + tempDir).c_str());

    // Determine file extension
    std::string fileExt = ".zip";
    if (url.find(".xpi") != std::string::npos || downloadUrl.find(".xpi") != std::string::npos) {
        fileExt = ".xpi";
    } else if (url.find(".crx") != std::string::npos || downloadUrl.find(".crx") != std::string::npos) {
        fileExt = ".crx";
    }

    // Download to temp file
    std::string tempFile = tempDir + "/extension_" + std::to_string(time(nullptr)) + fileExt;

    std::cout << "  → Downloading to: " << tempFile << std::endl;

    if (!downloadFile(downloadUrl, tempFile)) {
        callback(false, "Failed to download extension from URL");
        return;
    }

    // Install from downloaded file
    installFromFile(tempFile, callback);

    // Clean up temp file
    std::remove(tempFile.c_str());
}

void ExtensionInstaller::installFromFile(const std::string& filePath,
                                         std::function<void(bool, const std::string&)> callback) {
    std::cout << "📦 Installing extension from file: " << filePath << std::endl;

    // Validate manager
    if (!m_extensionManager) {
        std::cerr << "❌ Cannot install: Extension manager is null!" << std::endl;
        callback(false, "Extension manager not initialized");
        return;
    }

    // Create destination directory
    std::string homeDir = getenv("HOME") ? getenv("HOME") : "/tmp";
    std::string extensionsDir = homeDir + "/.config/braya-browser/extensions";
    std::string destDir = extensionsDir + "/extension_" + std::to_string(time(nullptr));

    std::cout << "  → Creating directory: " << destDir << std::endl;

    // Create directory
    system(("mkdir -p " + destDir).c_str());

    // Extract extension
    if (!extractExtension(filePath, destDir)) {
        callback(false, "Failed to extract extension archive");
        system(("rm -rf " + destDir).c_str());
        return;
    }

    std::cout << "  → Loading extension into manager..." << std::endl;

    // Load the extension
    if (m_extensionManager->loadWebExtension(destDir)) {
        std::cout << "✓ Extension installed successfully!" << std::endl;
        callback(true, "Extension installed successfully!");
    } else {
        callback(false, "Failed to load extension - invalid manifest.json");
        system(("rm -rf " + destDir).c_str());
    }
}

bool ExtensionInstaller::extractExtension(const std::string& archivePath, const std::string& destDir) {
    std::cout << "  → Extracting extension..." << std::endl;

    // Use unzip for both .xpi and .crx (they're both ZIP archives)
    // Note: .crx files have a header, but unzip can usually handle them

    std::string cmd = "unzip -q -o \"" + archivePath + "\" -d \"" + destDir + "\" 2>&1";

    // Try standard unzip first
    int result = system(cmd.c_str());

    if (result != 0) {
        std::cout << "  → Standard unzip failed, trying to handle .crx header..." << std::endl;

        // For .crx files, we might need to strip the header first
        // CRX format has a header before the ZIP data
        std::string strippedFile = destDir + "/extension.zip";

        // Read the file and skip CRX header
        std::ifstream input(archivePath, std::ios::binary);
        if (!input.is_open()) {
            return false;
        }

        // CRX3 header: "Cr24" + version + header_size
        char magic[4];
        input.read(magic, 4);

        if (strncmp(magic, "Cr24", 4) == 0) {
            // It's a CRX file, skip header
            uint32_t version, headerSize;
            input.read((char*)&version, 4);
            input.read((char*)&headerSize, 4);

            // Skip the header
            input.seekg(16 + headerSize, std::ios::beg);

            // Write the ZIP portion
            std::ofstream output(strippedFile, std::ios::binary);
            output << input.rdbuf();
            output.close();
            input.close();

            // Try unzipping the stripped file
            cmd = "unzip -q -o \"" + strippedFile + "\" -d \"" + destDir + "\" 2>&1";
            result = system(cmd.c_str());

            // Clean up
            std::remove(strippedFile.c_str());
        } else {
            input.close();
        }
    }

    if (result != 0) {
        std::cerr << "  ✗ Failed to extract extension" << std::endl;
        return false;
    }

    std::cout << "  ✓ Extension extracted successfully" << std::endl;
    return true;
}

bool ExtensionInstaller::downloadFile(const std::string& url, const std::string& destPath) {
    std::cout << "  → Downloading file..." << std::endl;

    // Use curl to download
    std::string cmd = "curl -L -o \"" + destPath + "\" \"" + url + "\" 2>&1";
    int result = system(cmd.c_str());

    if (result != 0) {
        std::cerr << "  ✗ Download failed" << std::endl;
        return false;
    }

    // Check if file exists and has content
    struct stat st;
    if (stat(destPath.c_str(), &st) != 0 || st.st_size == 0) {
        std::cerr << "  ✗ Downloaded file is empty or doesn't exist" << std::endl;
        return false;
    }

    std::cout << "  ✓ Downloaded successfully (" << st.st_size << " bytes)" << std::endl;
    return true;
}

std::string ExtensionInstaller::getChromeExtensionDownloadUrl(const std::string& webStoreUrl) {
    // Extract extension ID from Chrome Web Store URL
    // Format: https://chrome.google.com/webstore/detail/extension-name/EXTENSION_ID

    size_t lastSlash = webStoreUrl.rfind('/');
    if (lastSlash == std::string::npos) {
        return webStoreUrl;
    }

    std::string extensionId = webStoreUrl.substr(lastSlash + 1);

    // Remove any query parameters
    size_t queryPos = extensionId.find('?');
    if (queryPos != std::string::npos) {
        extensionId = extensionId.substr(0, queryPos);
    }

    // Chrome Web Store download URL format
    // Note: This is a simplified approach. Real Chrome uses a more complex API.
    // For production, you'd want to use the official Chrome Web Store API

    std::string downloadUrl = "https://clients2.google.com/service/update2/crx?response=redirect"
                             "&prodversion=49.0"
                             "&acceptformat=crx2,crx3"
                             "&x=id%3D" + extensionId + "%26uc";

    return downloadUrl;
}

std::string ExtensionInstaller::getFirefoxExtensionDownloadUrl(const std::string& addonsUrl) {
    // Firefox Add-ons URL format is usually:
    // https://addons.mozilla.org/en-US/firefox/addon/ADDON_NAME/

    // The download URL is typically available as:
    // https://addons.mozilla.org/firefox/downloads/latest/ADDON_NAME/addon-VERSION.xpi

    // Extract addon name
    size_t addonPos = addonsUrl.find("/addon/");
    if (addonPos == std::string::npos) {
        return addonsUrl;
    }

    size_t nameStart = addonPos + 7; // length of "/addon/"
    size_t nameEnd = addonsUrl.find('/', nameStart);

    std::string addonName;
    if (nameEnd != std::string::npos) {
        addonName = addonsUrl.substr(nameStart, nameEnd - nameStart);
    } else {
        addonName = addonsUrl.substr(nameStart);
    }

    // Remove query parameters
    size_t queryPos = addonName.find('?');
    if (queryPos != std::string::npos) {
        addonName = addonName.substr(0, queryPos);
    }

    // Build download URL
    std::string downloadUrl = "https://addons.mozilla.org/firefox/downloads/latest/" + addonName + "/addon-latest.xpi";

    return downloadUrl;
}
