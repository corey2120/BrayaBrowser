#!/bin/bash
cd /home/cobrien/Projects/braya-browser-cpp
echo "🐕 Testing Braya Browser - Button Fix"
echo "======================================"
echo ""
echo "Changes made:"
echo "✓ Fixed back/forward button creation order"
echo "✓ Added 6px spacer between home button and URL bar"
echo "✓ Fixed URL select-all to use focus event"
echo "✓ Increased button sizes slightly (30x30px)"
echo "✓ Made icons 18px (up from 16px)"
echo "✓ Added spacing (2px) between left box buttons"
echo "✓ Set buttons to not steal focus (can_focus=FALSE)"
echo ""
echo "Starting browser..."
./build/braya-browser
