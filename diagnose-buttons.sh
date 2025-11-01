#!/bin/bash
# Diagnostic script to check button configuration

echo "🔍 Braya Browser - Button Diagnostic"
echo "======================================"
echo ""

# Check if binary exists
if [ ! -f "./build/braya-browser" ]; then
    echo "❌ Binary not found! Run ./build.sh first"
    exit 1
fi

# Check recent changes
echo "📋 Checking recent changes..."
echo ""

# Check BrayaWindow.cpp for button creation
echo "1️⃣ Button Creation Order:"
grep -A 3 "backBtn = gtk_button_new" src/BrayaWindow.cpp | head -4
echo ""

# Check for spacer
echo "2️⃣ Spacer After Home Button:"
grep -A 2 "Small spacer" src/BrayaWindow.cpp | head -3
echo ""

# Check CSS button sizing
echo "3️⃣ Button CSS Sizing:"
grep -A 5 "nav-btn {" resources/style.css | head -6
echo ""

# Check icon sizing
echo "4️⃣ Icon CSS Sizing:"
grep -A 3 "nav-btn image" resources/style.css | head -4
echo ""

# Check focus event
echo "5️⃣ URL Select-All Event:"
grep -A 3 "focus_controller" src/BrayaWindow.cpp | head -4
echo ""

echo "======================================"
echo "✅ All checks complete!"
echo ""
echo "If buttons still don't show, check:"
echo "  • GTK warnings in terminal output"
echo "  • Icon theme (using Papirus or Kora?)"
echo "  • Display server (Wayland/X11?)"
echo ""
echo "Run browser with: ./build/braya-browser"
