Name:           braya-browser
Version:        1.0.0
Release:        1%{?dist}
Summary:        A modern, highly customizable web browser built with C++ and WebKit
License:        MIT
URL:            https://github.com/corey2120/BrayaBrowser
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  cmake >= 3.10
BuildRequires:  gtk4-devel
BuildRequires:  webkit2gtk4.1-devel
BuildRequires:  pkgconfig

Requires:       gtk4
Requires:       webkit2gtk4.1

%description
Braya Browser is a cutting-edge web browser featuring:
- WebKit rendering engine for fast, modern web standards
- Vivaldi-level customization with 60+ appearance options
- Tab groups and organization
- Built-in bookmarks, history, and download management
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
mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/scalable/apps
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
    install -m 644 ../resources/icons/braya-browser.png $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/scalable/apps/braya-browser.png
fi

%files
%license LICENSE
%doc README.md
%{_bindir}/braya-browser
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/scalable/apps/braya-browser.png
%{_datadir}/%{name}/

%post
/usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
/usr/bin/update-desktop-database &>/dev/null || :

%postun
/usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
/usr/bin/update-desktop-database &>/dev/null || :

%changelog
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
