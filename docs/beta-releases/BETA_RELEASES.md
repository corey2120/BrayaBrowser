# Braya Browser - Beta Release Notes

## v1.0.1-beta5 (November 2, 2024) - LATEST ✅

### Password Manager Fixes:
- ✅ **Password Manager Opens** - Fixed GTK markup escaping bug preventing dialog from opening
- ✅ **Auto-Fill Works** - Fixed URL matching to work with www/non-www variations
- ✅ **Chrome Import Compatible** - Passwords imported from Chrome now auto-fill correctly
- ✅ **Better URL Matching** - Normalized domain matching (strips www., ignores ports)

### What Was Fixed:
1. **GTK Markup Escaping** - Special characters in URLs (like &) are now properly escaped
2. **Domain Matching** - URLs with/without "www." now match (e.g., vwhub.com = www.vwhub.com)
3. **Port Stripping** - URLs with ports now match correctly
4. **Edit/Delete Buttons** - Password entries can now be edited and deleted

### Installation:
```bash
cd /home/cobrien/Projects/braya-browser-cpp
sudo dnf upgrade ./rpm-output/braya-browser-1.0.1-0.5.beta5.fc43.x86_64.rpm
```

**All previous beta features working:**
- ✅ No startup crashes
- ✅ Home screen loads
- ✅ CSV import/export with dialogs
- ✅ Bitwarden integration
- ✅ Password auto-save/auto-fill

---

## v1.0.1-beta4 (November 2, 2024)

### Critical Fixes:
- ✅ **CRASH FIX** - Browser no longer crashes on startup with corrupted password data
- ✅ **Home Screen Fix** - Home screen now loads correctly in installed RPM version
- ✅ **Resource Paths** - Fixed resource loading for both development and installed environments
- ✅ **Error Handling** - Graceful handling of invalid timestamp data with warnings instead of crashes

### What Was Fixed:
1. **Startup Crash** - Added try-catch blocks for `std::stol()` timestamp parsing
2. **Home Page Loading** - Implemented proper resource path resolution that works in both dev and installed locations
3. **Password Script** - Fixed password-detect.js loading from correct install location

### Installation:
```bash
cd /home/cobrien/Projects/braya-browser-cpp
sudo dnf upgrade ./rpm-output/braya-browser-1.0.1-0.4.beta4.fc43.x86_64.rpm
```

**All beta3 features still working:**
- ✅ Password auto-save/auto-fill
- ✅ CSV import/export with dialogs
- ✅ Bitwarden integration
- ✅ All password management features

---

## v1.0.1-beta3 (November 2, 2024)

### What's Fixed:
- ✅ **CSV Import** - Now shows success/error dialogs with feedback
- ✅ **CSV Export** - Now shows success dialog with file path
- ✅ **All Import Methods** - Complete UI feedback for all operations

### All Import/Export Methods Working:
1. ✅ **CSV Import** - Upload passwords from CSV file
2. ✅ **CSV Export** - Download passwords to CSV file
3. ✅ **Bitwarden Sync** - Import from Bitwarden vault
4. ✅ **Bitwarden Import** - One-time import from Bitwarden
5. ✅ **Bitwarden Export** - Export to Bitwarden-compatible CSV

### Installation:
```bash
cd /home/cobrien/Projects/braya-browser-cpp
sudo dnf upgrade ./rpm-output/braya-browser-1.0.1-0.3.beta3.fc43.x86_64.rpm
```

---

## v1.0.1-beta2 (November 2, 2024)

### What's Fixed:
- ✅ **Bitwarden Integration** - Completely rewritten and working
- ✅ **Better Error Handling** - Shows helpful error messages
- ✅ **Dependency Checks** - Verifies bw and jq are installed
- ✅ **Session Handling** - Properly manages Bitwarden sessions
- ✅ **Login Verification** - Checks login status before import
- ✅ **User Dialogs** - Success/error popups instead of console-only

### Technical Improvements:
- Creates cache directory automatically
- Validates exported data before import
- Better error messages with installation instructions
- Temporary files properly cleaned up

---

## v1.0.1-beta1 (November 2, 2024)

### Initial Beta Release

#### New Features:
- ✅ **Advanced Password Manager** with encryption
- ✅ **Auto-Save Passwords** on form submission
- ✅ **Auto-Fill Passwords** on page load
- ✅ **Manual Password Management** (Add/Edit/Delete)
- ✅ **CSV Import/Export** for migration
- ✅ **Bitwarden Integration** (basic implementation)
- ✅ **Encrypted Storage** in `~/.config/braya-browser/passwords.dat`

#### Known Issues (Fixed in Beta 2):
- ⚠️ Bitwarden import not working reliably
- ⚠️ No user feedback for import/export operations
- ⚠️ Console-only error messages

---

## Testing Checklist

### Password Manager:
- [x] Auto-save prompts appear on login
- [x] Auto-fill works on returning to sites
- [x] Manual add password works
- [x] Manual edit password works
- [x] Manual delete password works
- [x] CSV import shows success dialog
- [x] CSV export shows success dialog with path
- [x] Bitwarden sync works (with bw + jq installed)
- [x] Error dialogs show helpful messages

### Expected Behavior:

**CSV Import:**
1. Click 📥 Import button
2. Select CSV file
3. See "Import Successful" dialog
4. Passwords appear in list

**CSV Export:**
1. Click 📤 Export button
2. Choose save location
3. See "Export Successful" dialog with file path
4. File created with all passwords

**Bitwarden Sync:**
1. Install: `npm install -g @bitwarden/cli && sudo dnf install jq`
2. Login: `bw login`
3. Click 🔐 Bitwarden > 🔄 Sync
4. See "Bitwarden Sync" success dialog
5. All passwords imported

---

## Upgrade Path

### From Beta 1 → Beta 3:
```bash
sudo dnf upgrade ./rpm-output/braya-browser-1.0.1-0.3.beta3.fc43.x86_64.rpm
```

### From Beta 2 → Beta 3:
```bash
sudo dnf upgrade ./rpm-output/braya-browser-1.0.1-0.3.beta3.fc43.x86_64.rpm
```

### From v1.0.0 → Beta 3:
```bash
sudo dnf upgrade ./rpm-output/braya-browser-1.0.1-0.3.beta3.fc43.x86_64.rpm
```

All user data (passwords, settings, bookmarks) preserved during upgrade!

---

## Known Limitations

### Current:
- Password encryption uses XOR (basic, will upgrade to libsodium in v1.1.0)
- Bitwarden export to vault requires manual CSV import (API coming later)
- Password strength meter not yet implemented

### Future Enhancements (v1.1.0+):
- Master password protection
- Password generator
- Breach detection (haveibeenpwned.com)
- System keyring integration
- Direct Bitwarden API (no CLI needed)

---

## Support

### Getting Help:
1. Check console output for detailed errors
2. Review `BITWARDEN_SETUP.md` for Bitwarden issues
3. Ensure dependencies installed (bw, jq)
4. Test with sample CSV file first

### Reporting Issues:
Include:
- Beta version (beta3)
- What you were doing
- Error dialog message
- Console output (if available)

---

**Recommended Version:** v1.0.1-beta3
**Status:** All password import/export methods working ✅
**Next Target:** v1.1.0 (UI overhaul)
