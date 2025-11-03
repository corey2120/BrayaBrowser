Name:           braya-browser
Version:        1.0.1
Release:        0.7.beta7%{?dist}
Summary:        A modern, highly customizable web browser built with C++ and WebKit
License:        MIT
URL:            https://github.com/corey2120/BrayaBrowser
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  cmake >= 3.10
BuildRequires:  gtk4-devel
BuildRequires:  webkit2gtk4.1-devel
BuildRequires:  openssl-devel
BuildRequires:  pkgconfig

Requires:       gtk4
Requires:       webkit2gtk4.1
Requires:       openssl-libs

%description
Braya Browser is a cutting-edge web browser featuring:
- WebKit rendering engine for fast, modern web standards
- Vivaldi-level customization with 60+ appearance options
- Tab groups and organization
- Built-in bookmarks, history, and download management
- Advanced password manager with auto-fill/auto-save
- Bitwarden integration and CSV import/export
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
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/256x256/apps
mkdir -p $RPM_BUILD_ROOT%{_datadir}/%{name}/resources

# Install binary
install -m 755 braya-browser $RPM_BUILD_ROOT%{_bindir}/braya-browser

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

# Install icon
if [ -f "../resources/icons/braya-browser.png" ]; then
    install -m 644 ../resources/icons/braya-browser.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/256x256/apps/braya-browser.png
fi

%files
%license LICENSE
%doc README.md
%{_bindir}/braya-browser
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/256x256/apps/braya-browser.png
%{_datadir}/%{name}/

%post
/usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
/usr/bin/update-desktop-database &>/dev/null || :

%postun
/usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
/usr/bin/update-desktop-database &>/dev/null || :

%changelog
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
