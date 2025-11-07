# Extension Installation - User Guide

## 🎉 Seamless Extension Installation (Like Firefox!)

Braya Browser now supports seamless extension installation just like Firefox and Chrome!

---

## 🚀 How to Install Extensions

### **Method 1: From Firefox Add-ons (Recommended)**

1. **Navigate to** https://addons.mozilla.org/en-US/firefox/
2. **Search for** any extension you want
3. **Click "Add to Braya Browser"** (button text automatically changes!)
4. **Extension installs automatically** - done!

### **Method 2: From Settings (Manual URL)**

1. **Go to** Settings → Extensions
2. **Click** "🔍 Browse Extensions"
3. **Paste URL** from:
   - Firefox Add-ons: `https://addons.mozilla.org/...`
   - Chrome Web Store: `https://chrome.google.com/webstore/...`
   - Direct .xpi/.crx file URL
4. **Click** "Install from URL"
5. **Wait** for download and extraction
6. **Done!** Extension appears in your toolbar

### **Method 3: Load Unpacked (Developers)**

1. **Go to** Settings → Extensions
2. **Click** "Load Unpacked"
3. **Select** extension folder
4. **Done!**

---

## 🔍 Supported Extension Sources

### ✅ **Firefox Add-ons**
- Full support for .xpi files
- Direct installation from addons.mozilla.org
- Example: `https://addons.mozilla.org/en-US/firefox/addon/ublock-origin/`

### ✅ **Chrome Web Store**
- Support for .crx files
- Auto-conversion of Web Store URLs to download URLs
- Example: `https://chrome.google.com/webstore/detail/extension-id`

### ✅ **Direct File URLs**
- Any direct .xpi or .crx file link
- Example: `https://example.com/extension.xpi`

---

## 🎯 What Happens Behind the Scenes

When you install an extension:

1. **Download**: Braya downloads the .xpi or .crx file
2. **Extract**: The archive is extracted to `~/.config/braya-browser/extensions/`
3. **Validate**: The manifest.json is checked for validity
4. **Load**: The extension is loaded into the browser
5. **Enable**: Extension appears in toolbar and starts working
6. **Save**: Installation is remembered for next time

---

## 📦 Extension File Formats

### **.xpi (Firefox Extensions)**
- ZIP archive with manifest.json
- Used by Firefox Add-ons
- Fully supported by Braya

### **.crx (Chrome Extensions)**
- ZIP archive with Chrome-specific header
- Used by Chrome Web Store
- Braya automatically strips the header and extracts

---

## 🛠️ Technical Details

### Download Method
- Uses `curl` to download files
- Follows redirects automatically
- Shows progress in terminal

### Extraction Method
- Uses standard `unzip` command
- Handles both .xpi and .crx formats
- Automatically strips CRX headers if present

### Installation Location
```
~/.config/braya-browser/extensions/
├── extension_1234567890/
│   ├── manifest.json
│   ├── background.js
│   ├── content.js
│   └── icons/
└── extension_1234567891/
    └── ...
```

---

## 🔒 Security Notes

### Safe Installation
- Extensions are sandboxed in WebKit contexts
- No system-level permissions granted
- Extensions cannot access your file system outside their sandbox

### Recommended Sources
- ✅ Firefox Add-ons (https://addons.mozilla.org)
- ✅ Chrome Web Store (https://chrome.google.com/webstore)
- ⚠️  Only install from trusted sources!

---

## 🐛 Troubleshooting

### "Failed to download extension"
- Check your internet connection
- Verify the URL is correct
- Make sure you have `curl` installed

### "Failed to extract extension"
- Make sure you have `unzip` installed
- Check disk space in `~/.config/braya-browser/`

### "Failed to load extension"
- Extension may not have a valid manifest.json
- Check terminal output for error messages
- Try downloading and loading as unpacked

### Extension doesn't appear in toolbar
- Only extensions with `browser_action` show toolbar buttons
- Check Settings → Extensions to verify it's enabled
- Some extensions don't have toolbar buttons (they work in background)

---

## 📝 Example: Installing uBlock Origin

1. **Visit**: https://addons.mozilla.org/en-US/firefox/addon/ublock-origin/
2. **Click**: "Add to Braya Browser"
3. **Wait**: ~5 seconds for download and installation
4. **See**: 🧩 uBlock icon appears in toolbar
5. **Done**: You now have ad blocking!

---

## 🌟 Popular Extensions to Try

### Privacy & Security
- **uBlock Origin** - Best ad blocker
- **Privacy Badger** - Tracker blocker
- **HTTPS Everywhere** - Force HTTPS

### Productivity
- **Dark Reader** - Dark mode for all websites
- **Grammarly** - Writing assistant
- **LastPass** - Password manager

### Development
- **React DevTools** - React debugging
- **Vue.js DevTools** - Vue debugging
- **ColorZilla** - Color picker

---

## 🎉 You're Ready!

Braya Browser now supports extensions just like Firefox and Chrome. Enjoy the full power of browser extensions!

**Have fun customizing your browsing experience!** 🚀
