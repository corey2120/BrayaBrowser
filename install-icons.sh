#!/bin/bash
cd build
sudo make install
sudo gtk-update-icon-cache /usr/share/icons/hicolor/ -f
echo "✅ Icons installed and cache updated!"
echo "You may need to restart your desktop environment or run: killall plasmashell; plasmashell &"
