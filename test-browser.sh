#!/bin/bash

echo "🐕 Testing Braya Browser"
echo "========================"
echo ""
echo "Starting browser with crash logging enabled..."
echo "Crash logs will be saved to: braya-crash.log"
echo ""

cd /home/cobrien/Projects/braya-browser-cpp
./build/braya-browser 2>&1 | tee braya-test-latest.log

if [ $? -ne 0 ]; then
    echo ""
    echo "❌ Browser crashed or exited abnormally!"
    echo "Check the logs:"
    echo "  - braya-test-latest.log"
    echo "  - braya-crash.log"
    
    if [ -f braya-crash.log ]; then
        echo ""
        echo "Last crash:"
        tail -10 braya-crash.log
    fi
else
    echo ""
    echo "✅ Browser exited normally"
fi
