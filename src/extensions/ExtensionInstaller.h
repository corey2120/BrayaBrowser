#ifndef EXTENSION_INSTALLER_H
#define EXTENSION_INSTALLER_H

#include <string>
#include <functional>

class BrayaExtensionManager;

// Handles downloading and installing extensions from URLs
class ExtensionInstaller {
public:
    ExtensionInstaller(BrayaExtensionManager* manager);

    // Download and install extension from URL (supports .xpi, .crx, or web store URLs)
    void installFromUrl(const std::string& url,
                       std::function<void(bool success, const std::string& message)> callback);

    // Download and install extension from file path
    void installFromFile(const std::string& filePath,
                        std::function<void(bool success, const std::string& message)> callback);

private:
    BrayaExtensionManager* m_extensionManager;

    // Extract .xpi or .crx file to a directory
    bool extractExtension(const std::string& archivePath, const std::string& destDir);

    // Download file from URL
    bool downloadFile(const std::string& url, const std::string& destPath);

    // Convert Chrome Web Store URL to direct download URL
    std::string getChromeExtensionDownloadUrl(const std::string& webStoreUrl);

    // Convert Firefox Add-ons URL to direct download URL
    std::string getFirefoxExtensionDownloadUrl(const std::string& addonsUrl);
};

#endif // EXTENSION_INSTALLER_H
