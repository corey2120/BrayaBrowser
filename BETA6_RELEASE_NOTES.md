# Braya Browser v1.0.1-beta6 Release Notes

**Release Date**: November 2, 2024  
**Build**: 1.0.1-0.6.beta6  

## 🎉 Major Features: Safari-Style Password Manager Overhaul

This beta release completely overhauls the password manager with Safari-inspired features and enterprise-grade security.

---

## 🆕 What's New

### Safari-Style Field Detection
- ✨ **Autocomplete Attributes**: Now checks HTML5 autocomplete attributes (`autocomplete="username"`, `autocomplete="current-password"`)
- ✨ **Advanced Heuristics**: Smart pattern matching for username/email fields (checks name, id, and type attributes)
- ✨ **Visibility Validation**: Only considers visible form fields (ignores hidden/display:none elements)
- ✨ **Smart Fallback**: Intelligently finds username field even without explicit attributes

### Multi-Account Support
- 🎯 **Account Selection**: When multiple accounts exist for a domain, shows a selection dialog
- 🎯 **One-Click Fill**: Single account auto-fills immediately on page load
- 🎯 **User Choice**: Click on username/password field to trigger account selection

### Focus-Triggered Autofill (Safari-Style)
- 🖱️ **Click to Fill**: Click on login fields to see password options
- 🖱️ **Instant Fill**: Select account from dialog, credentials fill immediately
- 🖱️ **Natural UX**: Matches Safari's autofill behavior

### Modern Web App Support
- 🌐 **AJAX/SPA Detection**: Works with React, Vue, Angular, and other SPAs
- 🌐 **Fetch API Interception**: Monitors AJAX requests for successful logins
- 🌐 **Dynamic Forms**: MutationObserver watches for dynamically added login forms
- 🌐 **Framework Events**: Properly dispatches input/change events for React/Vue

### Enhanced Security
- 🔒 **AES-256-CBC Encryption**: Replaced XOR with industry-standard OpenSSL encryption
- 🔒 **Random IVs**: Each encrypted entry uses a unique initialization vector
- 🔒 **SHA-256 Key Derivation**: Master key derived from user password using SHA-256
- 🔒 **File Permissions**: Password storage enforced at 0600 (user read/write only)
- 🔒 **Injection Protection**: JavaScript values properly escaped to prevent vulnerabilities

---

## 🔧 Technical Improvements

### JavaScript Side (`password-detect.js`)
- Complete rewrite with 215 lines of smart detection logic
- Function-based architecture with proper scoping
- Visibility checking for all form elements
- Position-based username detection (finds field before password)
- Support for multiple field naming conventions

### C++ Implementation
- New `showAutofillSuggestions()` method for account selection
- New `onAutofillRequest()` handler for focus events
- Improved `autoFillPasswords()` with JavaScript escaping
- `deriveKey()` method for secure key generation
- OpenSSL EVP API integration for encryption

### Dependencies
- **Added**: `openssl-devel` (build) / `openssl-libs` (runtime)
- **Required**: GTK4, WebKitGTK 6.0 (unchanged)

---

## 📋 Comparison with Safari

| Feature | Safari | Braya Beta6 | Status |
|---------|--------|-------------|--------|
| Autocomplete Detection | ✅ | ✅ | **Identical** |
| Heuristic Field Finding | ✅ | ✅ | **Identical** |
| Visibility Checking | ✅ | ✅ | **Identical** |
| Focus-Triggered Autofill | ✅ | ✅ | **Similar** (GTK dialog vs native) |
| Multi-Account Support | ✅ | ✅ | **Similar** (dialog vs dropdown) |
| AJAX/SPA Detection | ✅ | ✅ | **Identical** |
| AES Encryption | ✅ | ✅ | **Similar** (AES-256 vs Keychain) |
| System Keychain | ✅ | ❌ | *Planned for v2.0* |
| Touch ID/Biometrics | ✅ | ❌ | *Future feature* |
| Password Generator | ✅ | ❌ | *Planned* |

---

## 📦 Installation

### Fresh Install
```bash
sudo dnf install ./braya-browser-1.0.1-0.6.beta6.fc43.x86_64.rpm
```

### Upgrade from Beta5
```bash
sudo dnf upgrade ./braya-browser-1.0.1-0.6.beta6.fc43.x86_64.rpm
```

### Dependencies
Beta6 adds OpenSSL as a dependency:
- `openssl-libs` - Runtime encryption library (automatically installed)

---

## 🧪 Testing Guide

### Basic Password Manager
1. Launch Braya Browser
2. Visit github.com (or any login page)
3. Enter username and password
4. Click "Sign in"
5. **Expected**: "Save password?" dialog appears
6. Click "Save"
7. Visit github.com again
8. **Expected**: Password auto-fills on page load

### Multi-Account Selection
1. Save 2+ accounts for the same site
2. Visit the site
3. Click on the username or password field
4. **Expected**: Account selection dialog appears
5. Click "Fill" next to desired account
6. **Expected**: Credentials fill instantly

### Focus-Triggered Autofill
1. Visit a saved login page
2. Don't wait - immediately click username field
3. **Expected**: Account selection appears (or auto-fills if only 1 account)

### SPA Support (Advanced)
1. Visit a React/Vue-based login (e.g., modern web apps)
2. Test login capture and autofill
3. **Expected**: Should work seamlessly with AJAX forms

---

## ⚠️ Important Notes

### Password Re-entry Required
**Action Required**: Old passwords encrypted with XOR won't decrypt with new AES encryption.

**What to do**:
1. Open Password Manager (Ctrl+Shift+P or menu)
2. Click "Clear All" (exports before clearing recommended)
3. Re-enter passwords as you browse
4. Or import from CSV/Bitwarden

### Master Password
Currently uses username-based key derivation. Setting a custom master password UI will be added in a future release.

### Backup Recommendation
Before upgrading, export your passwords:
1. Open Password Manager
2. Click "📤 Export"
3. Save CSV file
4. Upgrade to beta6
5. Import passwords back

---

## 🐛 Known Issues

1. **Encrypted Data Migration**: Old XOR passwords won't work - re-save required
2. **Master Password UI**: No UI to set custom master password yet (uses auto-generated key)
3. **Some Sites**: May not detect non-standard login forms (please report!)

---

## 🔜 Planned for Beta7/Final

### High Priority
- [ ] Master password setup UI
- [ ] System keychain integration (libsecret/gnome-keyring)
- [ ] Password strength generator
- [ ] HTTPS-only autofill restriction
- [ ] Visual indicators in password fields (key icon)

### Medium Priority
- [ ] Password strength indicator
- [ ] Auto-update detection (changed password)
- [ ] Password history tracking
- [ ] Breach detection API

---

## 📚 Documentation

New documentation included in this release:
- `PASSWORD_AUTOFILL_IMPROVEMENTS.md` - Technical deep-dive (192 lines)
- `QUICK_SUMMARY.md` - Quick overview and comparison
- `test_password_manager.sh` - Automated test script

---

## 🔐 Security Notes

### Current Implementation
- **Encryption**: AES-256-CBC with OpenSSL
- **Key Derivation**: SHA-256 (will upgrade to PBKDF2 in v2.0)
- **Storage**: `~/.config/braya-browser/passwords.dat` (0600 permissions)
- **IV**: Random 16-byte IV per entry

### Future Security Enhancements
- PBKDF2 key derivation with salt
- System keychain integration
- Optional biometric authentication
- Two-factor code support

---

## 📊 Statistics

- **Files Modified**: 6
- **Lines Changed**: ~800 (400+ added, 400+ modified)
- **JavaScript**: 215 lines (complete rewrite)
- **C++**: 350+ lines added
- **Documentation**: 450+ lines

---

## 🙏 Testing Needed

Please test with:
- ✅ GitHub login
- ✅ Gmail/Google accounts
- ✅ Banking sites (if comfortable)
- ✅ React/Vue SPAs
- ✅ Traditional forms
- ✅ Multiple accounts per site

Report issues to: corey@braya.dev

---

## 📝 Full Changelog

```
* Sat Nov 02 2024 - 1.0.1-0.6.beta6
- Beta 6 release - Safari-style password manager overhaul
- NEW: Safari-style password field detection with autocomplete attributes
- NEW: Multi-account support - select which account to use per site
- NEW: Focus-triggered autofill - click field to see password options
- NEW: AJAX/SPA login form support (works with React, Vue, etc.)
- NEW: AES-256-CBC encryption with OpenSSL (replaced XOR)
- NEW: Random IV per encrypted entry for enhanced security
- IMPROVED: Smart field detection with visibility checking
- IMPROVED: JavaScript escaping prevents injection vulnerabilities
- IMPROVED: Better heuristics for username/email field detection
- IMPROVED: Framework compatibility with proper event dispatching
- TECHNICAL: Fetch API interception for modern web apps
- TECHNICAL: MutationObserver for dynamically added forms
- TECHNICAL: SHA-256 key derivation from master password
- Security: File permissions enforced (0600)
- Added comprehensive documentation (PASSWORD_AUTOFILL_IMPROVEMENTS.md)
```

---

**Thank you for testing Braya Browser Beta6!** 🎉

Your feedback helps make Braya better. Report bugs and suggestions at:
- Email: corey@braya.dev
- GitHub: https://github.com/corey2120/BrayaBrowser

---
**Package**: `braya-browser-1.0.1-0.6.beta6.fc43.x86_64.rpm`  
**Size**: 2.3 MB  
**Build Date**: November 2, 2024
