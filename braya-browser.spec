Name:           braya-browser
Version:        1.0.9
Release:        1%{?dist}
Summary:        A modern, highly customizable web browser built with C++ and WebKit
License:        MIT
URL:            https://github.com/corey2120/BrayaBrowser
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  cmake >= 3.10
BuildRequires:  gtk4-devel
BuildRequires:  webkit2gtk4.1-devel
BuildRequires:  openssl-devel
BuildRequires:  libsodium-devel
BuildRequires:  pkgconfig

Requires:       gtk4
Requires:       webkit2gtk4.1
Requires:       openssl-libs
Requires:       libsodium

%description
Braya Browser is a cutting-edge web browser featuring:
- WebKit rendering engine for fast, modern web standards
- Vivaldi-level customization with 60+ appearance options
- Built-in ad-blocker with custom rules and filter lists
- Advanced password manager with generator and auto-fill
- Tab groups and organization
- Built-in bookmarks, history, and download management
- Password strength indicator and secure generation
- CSV import/export support for passwords
- Multiple theme support (Dark, Light, Industrial)
- Advanced privacy and security settings
- Native GTK4 interface for Linux

%prep
%setup -q

%build
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
cd build
make install DESTDIR=$RPM_BUILD_ROOT

# Create directories
mkdir -p $RPM_BUILD_ROOT%{_bindir}
mkdir -p $RPM_BUILD_ROOT%{_datadir}/applications
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/16x16/apps
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/32x32/apps
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/64x64/apps
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/128x128/apps
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/256x256/apps
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/512x512/apps
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/scalable/apps
mkdir -p $RPM_BUILD_ROOT%{_datadir}/%{name}/resources

# Binary is already installed by 'make install' above

# Install desktop file
cat > $RPM_BUILD_ROOT%{_datadir}/applications/%{name}.desktop << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=Braya Browser
Comment=Modern, customizable web browser
Exec=braya-browser %U
Icon=braya-browser
Categories=Network;WebBrowser;
MimeType=text/html;text/xml;application/xhtml+xml;x-scheme-handler/http;x-scheme-handler/https;
Keywords=browser;web;internet;
StartupNotify=true
EOF

# Install resources if they exist
if [ -d "../resources" ]; then
    cp -r ../resources/* $RPM_BUILD_ROOT%{_datadir}/%{name}/resources/
fi

# Install icons at multiple sizes
if [ -f "../resources/icons/braya-browser-16.png" ]; then
    install -m 644 ../resources/icons/braya-browser-16.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/16x16/apps/braya-browser.png
fi
if [ -f "../resources/icons/braya-browser-32.png" ]; then
    install -m 644 ../resources/icons/braya-browser-32.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/32x32/apps/braya-browser.png
fi
if [ -f "../resources/icons/braya-browser-48.png" ]; then
    install -m 644 ../resources/icons/braya-browser-48.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps/braya-browser.png
fi
if [ -f "../resources/icons/braya-browser-64.png" ]; then
    install -m 644 ../resources/icons/braya-browser-64.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/64x64/apps/braya-browser.png
fi
if [ -f "../resources/icons/braya-browser-128.png" ]; then
    install -m 644 ../resources/icons/braya-browser-128.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/128x128/apps/braya-browser.png
fi
if [ -f "../resources/icons/braya-browser-256.png" ]; then
    install -m 644 ../resources/icons/braya-browser-256.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/256x256/apps/braya-browser.png
fi
if [ -f "../resources/icons/braya-browser-512.png" ]; then
    install -m 644 ../resources/icons/braya-browser-512.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/512x512/apps/braya-browser.png
fi
if [ -f "../resources/icons/braya-browser.svg" ]; then
    install -m 644 ../resources/icons/braya-browser.svg $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/scalable/apps/braya-browser.svg
fi

%files
%license LICENSE
%doc README.md
%{_bindir}/braya-browser
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/braya-browser.*
%{_datadir}/icons/hicolor/*/apps/dev.braya.BrayaBrowser.png
%{_datadir}/%{name}/
/usr/lib/%{name}/web-extensions/libbraya-web-extension.so

%post
/usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
/usr/bin/update-desktop-database &>/dev/null || :

%postun
/usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
/usr/bin/update-desktop-database &>/dev/null || :

%changelog
* Fri Nov 15 2024 Corey O'Brien <corey@braya.dev> - 1.0.6-1
- Version 1.0.6 release - Password Manager Polish & UX Refinements
- NEW: 3-panel password manager layout (filters, list, detail view)
- NEW: Detail pane with full password information and Edit/Delete buttons
- NEW: Keyboard navigation in autofill dropdown (Up/Down/Enter/Esc)
- NEW: Multi-step login capture with toggle in settings
- NEW: Password mismatch warnings when saved password differs
- NEW: View button in autofill toasts to open password manager
- NEW: Timestamp display in save dialogs (relative time)
- NEW: Tab preview debouncing with 400ms delay and settings toggle
- FIXED: Password manager detail pane now shows in separate right panel
- FIXED: Long URLs wrap properly in detail pane (no window expansion)
- FIXED: Detail pane scrolling for overflow content (max width 450px)
- FIXED: Search functionality in password manager
- FIXED: Import/Export buttons work in Settings dialog
- FIXED: Iframe autofill positioning (absolute coordinates)
- IMPROVED: Fixed-width panels prevent layout expansion (list 400px, detail 320-450px)
- IMPROVED: Visual consistency with scrollable containers
- IMPROVED: Tab hover preview performance and responsiveness
- TECHNICAL: Child widget data access from GtkListBoxRow
- TECHNICAL: Scrolled window with max-content-width constraint
- TECHNICAL: Label wrapping with PANGO_WRAP_WORD_CHAR
- All password manager UX improvements from v1.0.6 roadmap completed

* Thu Nov 14 2024 Corey O'Brien <corey@braya.dev> - 1.0.5-1
- Version 1.0.5 release - UX Improvements & Password Manager Relocation
- NEW: Password manager relocated to Settings→Passwords tab (Ctrl+P shortcut)
- NEW: Links now open in new tabs by default (modern browser behavior)
- NEW: Middle-click and Ctrl+Click support for opening links in new tabs
- NEW: Removed password manager button from sidebar (cleaner UI)
- FIXED: Password save dialog now properly sized (400x150, non-resizable)
- FIXED: Long URLs in password dialog now ellipsized (no more huge dialogs)
- IMPROVED: Theme enum mapping to handle removed Light theme gracefully
- REMOVED: Light theme temporarily removed (will be redesigned in v1.0.6)
- TECHNICAL: WebKit decide-policy signal for link click interception
- TECHNICAL: New tab callback system for link handling
- TECHNICAL: Theme dropdown index remapping without breaking saved settings
- Users with Light theme saved will automatically fallback to Dark theme
- All password manager and link handling features fully tested

* Wed Nov 13 2024 Corey O'Brien <corey@braya.dev> - 1.0.3-1
- Version 1.0.3 release - Ad-Blocker & Password Manager Enhancements
- NEW: Built-in ad-blocker fully functional (Phase 4-5 complete)
- NEW: Ad-blocker settings dialog with security levels (OFF/MINIMAL/STANDARD/STRICT/CUSTOM)
- NEW: 8 blocking features (ads, trackers, malware, cryptominers, popups, autoplay, social, NSFW)
- NEW: Whitelist manager - add/remove trusted domains
- NEW: Filter list manager - enable/disable EasyList, EasyPrivacy, Malware Domains
- NEW: Custom blocking rules editor
- NEW: Ad-blocker statistics dashboard (total blocked, daily, breakdown by type)
- NEW: Import/Export ad-blocker settings
- NEW: Shield icon in toolbar showing blocked count
- NEW: Password generator - cryptographically secure (OpenSSL RAND_bytes)
- NEW: Password strength indicator - real-time scoring (0-100)
- NEW: Password search/filter - instant lookup in password manager
- NEW: Show/Hide password toggle in password dialogs
- FIXED: Window icon now displays properly in taskbar/window list
- FIXED: Application icon works for both dev and production app IDs
- IMPROVED: Password generator with customizable length and character sets
- IMPROVED: Smart strength scoring (length, variety, patterns, bonuses)
- IMPROVED: Color-coded strength labels (Very Weak → Strong)
- IMPROVED: Icon search paths for development builds
- IMPROVED: Ad-blocker UI with live updates and callbacks
- TECHNICAL: WebKit content blocking rules compilation
- TECHNICAL: GTK4 file dialogs for settings import/export
- TECHNICAL: Icon theme search path management
- TECHNICAL: Advanced statistics tracking (ads/trackers/malware breakdown)
- All ad-blocker and password manager features fully integrated and tested

* Thu Nov 07 2024 Corey O'Brien <corey@braya.dev> - 1.0.2-1
- Version 1.0.2 release - UI refinements
- IMPROVED: Bookmarks bar flows seamlessly with headerbar (Zen-style)
- IMPROVED: Cleaner sidebar without extra separators
- IMPROVED: Removed large add button from bookmarks bar
- FIXED: Tab favicon display issue
- IMPROVED: Better visual consistency with modern browser design

* Sun Nov 03 2024 Corey O'Brien <corey@braya.dev> - 1.0.1-0.12.beta12
- Beta 12 release - Icon and App ID fixes
- FIXED: Updated all icon sizes with new UpdatedBraya.png design
- FIXED: Changed application ID from "com.braya.browser" to "dev.braya.BrayaBrowser"
- IMPROVED: Application now shows correct icon in dock and menus
- IMPROVED: Application displays as "Braya Browser" instead of app ID
- All icon and identification issues resolved

* Sun Nov 03 2024 Corey O'Brien <corey@braya.dev> - 1.0.1-0.11.beta11
- Beta 11 release - Stability improvements and maintenance
- IMPROVED: Build system optimizations
- IMPROVED: Code cleanup and refinements
- TECHNICAL: Packaging improvements for RPM distribution
- TECHNICAL: Resource optimization and dependency updates
- Preparing for future tab UI enhancements

* Sun Nov 03 2024 Corey O'Brien <corey@braya.dev> - 1.0.1-0.10.beta10
- Beta 10 release - Professional Icon Design
- NEW: Completely redesigned professional application icon
- NEW: Modern gradient design with detailed browser window representation
- NEW: Icon includes browser tabs, address bar, and traffic lights
- NEW: Multiple icon sizes for all contexts (16x16 to 512x512)
- NEW: SVG scalable icon for perfect rendering at any size
- IMPROVED: Follows freedesktop.org icon standards
- IMPROVED: Better visibility in application menus and taskbars
- IMPROVED: Professional appearance matching modern browser standards
- All icon sizes properly installed to hicolor icon theme

* Sun Nov 03 2024 Corey O'Brien <corey@braya.dev> - 1.0.1-0.9.beta9
- Beta 9 release - Bookmarks System Complete Overhaul
- FIXED: Bookmarks bar now fully functional (was completely broken in beta8)
- FIXED: Click bookmarks to navigate to URLs
- FIXED: GTK viewport wrapping issues causing casting errors
- NEW: Right-click context menu on bookmarks (Edit/Delete/Copy)
- NEW: Edit bookmark dialog with Name/URL/Folder fields
- NEW: Delete confirmation dialog
- NEW: Copy URL to clipboard functionality
- NEW: Ctrl+Shift+B keyboard shortcut to toggle bar visibility
- NEW: Folder support for organizing bookmarks
- NEW: Auto-refresh bookmarks bar after any change
- IMPROVED: Chrome/Firefox-style bookmarks bar design
- IMPROVED: Compact, horizontal layout like modern browsers
- IMPROVED: Proper button sizing and hover effects
- All critical bookmark issues from beta8 resolved

* Sun Nov 03 2024 Corey O'Brien <corey@braya.dev> - 1.0.1-0.8.beta8
- Beta 8 release - Quick wins + Visual bookmarks overhaul
- NEW: Reader Mode - distraction-free reading (Alt+Shift+R)
- NEW: Screenshot tool - save visible page to PNG (Ctrl+Shift+S)
- NEW: Visual bookmarks bar with real favicons
- NEW: Speed dial / new tab page with thumbnail grid
- NEW: Favicon caching system
- NEW: Tab pinning backend (ready for UI)
- NEW: Tab muting backend (ready for UI)
- IMPROVED: Bookmark current page with Ctrl+D
- IMPROVED: Bookmarks bar shows site favicons
- IMPROVED: Beautiful modern bookmark styling
- UI: Reader mode extracts main content intelligently
- UI: Screenshot confirmation dialog with file path
- UI: Speed dial grid layout (4 columns, 12 sites)
- UI: Hover animations on bookmarks and speed dial
- UI: Add bookmark button in bookmarks bar
- TECHNICAL: Favicon storage in ~/.config/braya-browser/favicons
- TECHNICAL: GdkTexture snapshot API for screenshots
- TECHNICAL: JavaScript content extraction for reader mode
- Added 4 new keyboard shortcuts
- Added modern CSS with shadows and transitions

* Sun Nov 03 2024 Corey O'Brien <corey@braya.dev> - 1.0.1-0.7.beta7
- Beta 7 release - Chrome import fix + Safari UX improvements
- FIXED: Chrome password import now works correctly (4-column CSV format)
- FIXED: Auto-detects Chrome vs Braya CSV format automatically
- NEW: Visual key icon (🔑) appears in fields with saved passwords
- NEW: Password check handler shows indicators on page load
- IMPROVED: Removed auto-fill on page load (Safari-style behavior)
- IMPROVED: Passwords only fill when user clicks field (more secure)
- IMPROVED: Visual feedback with hover effects on password fields
- IMPROVED: Non-intrusive password indicators
- UX: Fields stay empty until user interaction (matches Safari)
- UX: Key icon provides visual confirmation passwords are available
- TECHNICAL: Added checkPasswords message handler
- TECHNICAL: CSS injection for field styling
- Backward compatible with Braya 3-column CSV export

* Sat Nov 02 2024 Corey O'Brien <corey@braya.dev> - 1.0.1-0.6.beta6
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

* Sat Nov 02 2024 Corey O'Brien <corey@braya.dev> - 1.0.1-0.5.beta5
- Beta 5 release - Password manager fixes
- FIXED: Password manager now opens correctly (GTK markup escaping issue)
- FIXED: Auto-fill now works with www/non-www domain variations
- FIXED: URL matching normalized to ignore www prefix and ports
- IMPROVED: Passwords from Chrome import now auto-fill properly

* Sat Nov 02 2024 Corey O'Brien <corey@braya.dev> - 1.0.1-0.4.beta4
- Beta 4 release - Critical crash fix
- FIXED: Browser crash on startup caused by invalid timestamp parsing
- FIXED: Password manager now handles corrupted/invalid timestamp data gracefully
- IMPROVED: Added proper error handling for stol() conversion failures
- Browser now shows warnings instead of crashing when encountering bad data

* Sat Nov 02 2024 Corey O'Brien <corey@braya.dev> - 1.0.1-0.3.beta3
- Beta 3 release - CSV import/export dialogs
- FIXED: CSV import now shows success/error dialogs
- FIXED: CSV export now shows success/error dialogs
- IMPROVED: User gets feedback on all import/export operations
- All password import methods now fully working with proper UI feedback

* Sat Nov 02 2024 Corey O'Brien <corey@braya.dev> - 1.0.1-0.2.beta2
- Beta 2 release - Bitwarden fixes
- FIXED: Bitwarden import/sync now works properly
- FIXED: Better error messages with installation instructions
- FIXED: Added jq dependency check
- FIXED: Improved session handling for Bitwarden CLI
- FIXED: Creates cache directory automatically
- FIXED: Checks for login status before import
- NEW: User-friendly success/error dialogs
- NEW: Validates exported data before import
- IMPROVED: More robust Bitwarden integration

* Sat Nov 02 2024 Corey O'Brien <corey@braya.dev> - 1.0.1-0.1.beta1
- Beta release for v1.0.1
- NEW: Advanced password manager with encryption
- NEW: Password auto-save on form submission
- NEW: Password auto-fill on page load
- NEW: Manual add/edit/delete password dialogs
- NEW: CSV import/export for password migration
- NEW: Bitwarden CLI integration
- NEW: Bitwarden sync functionality
- IMPROVED: Password storage security
- IMPROVED: User content script injection
- BETA: Testing password manager features

* Fri Nov 01 2024 Corey O'Brien <corey@braya.dev> - 1.0.0-1
- Initial v1.0.0 release
- Full WebKit integration
- Tab groups with color coding
- Vivaldi-level customization (60+ options)
- History, bookmarks, and find-in-page
- Three built-in themes
- Advanced settings panel
- Download manager UI
- Professional navigation controls
