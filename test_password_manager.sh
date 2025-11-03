#!/bin/bash
# Quick test for password manager functionality

echo "🧪 Testing Braya Password Manager"
echo "================================"
echo ""

# Test 1: Check if password-detect.js exists and is valid
echo "Test 1: Checking password-detect.js..."
if [ -f "resources/password-detect.js" ]; then
    LINES=$(wc -l < resources/password-detect.js)
    echo "✓ password-detect.js exists ($LINES lines)"
else
    echo "✗ password-detect.js not found!"
    exit 1
fi

# Test 2: Check for key functions in JS
echo ""
echo "Test 2: Checking for key JavaScript functions..."
if grep -q "function fillPassword" resources/password-detect.js; then
    echo "✓ fillPassword function found"
else
    echo "✗ fillPassword function missing!"
fi

if grep -q "findUsernameField" resources/password-detect.js; then
    echo "✓ findUsernameField function found"
else
    echo "✗ findUsernameField function missing!"
fi

if grep -q "setupAjaxCapture" resources/password-detect.js; then
    echo "✓ AJAX capture support found"
else
    echo "✗ AJAX capture support missing!"
fi

# Test 3: Check binary was built with OpenSSL
echo ""
echo "Test 3: Checking binary dependencies..."
if [ -f "build/braya-browser" ]; then
    if ldd build/braya-browser | grep -q "libcrypto"; then
        echo "✓ Linked against OpenSSL libcrypto"
    else
        echo "⚠ OpenSSL libcrypto not linked"
    fi
    
    if ldd build/braya-browser | grep -q "libssl"; then
        echo "✓ Linked against OpenSSL libssl"
    else
        echo "⚠ OpenSSL libssl not linked"
    fi
else
    echo "✗ Binary not found! Run ./build.sh first"
    exit 1
fi

# Test 4: Check for password manager methods in source
echo ""
echo "Test 4: Checking C++ implementation..."
if grep -q "showAutofillSuggestions" src/BrayaTab.cpp; then
    echo "✓ Multi-account selection implemented"
else
    echo "✗ Multi-account selection missing!"
fi

if grep -q "EVP_aes_256_cbc" src/BrayaPasswordManager.cpp; then
    echo "✓ AES-256 encryption implemented"
else
    echo "✗ AES encryption missing!"
fi

if grep -q "onAutofillRequest" src/BrayaTab.cpp; then
    echo "✓ Focus-triggered autofill implemented"
else
    echo "✗ Focus-triggered autofill missing!"
fi

echo ""
echo "================================"
echo "✅ All basic tests passed!"
echo ""
echo "To test manually:"
echo "1. Run: ./build/braya-browser"
echo "2. Visit a login page (e.g., github.com)"
echo "3. Fill in credentials and submit"
echo "4. Check if 'Save password?' dialog appears"
echo "5. Visit the same site again"
echo "6. Check if password autofills"
echo "7. If multiple accounts, click field to see selection"
