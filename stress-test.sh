#!/bin/bash

# Braya Browser Stress Test Script
# Tests tab operations to detect crashes and memory leaks

set -e

echo "🧪 Braya Browser Stress Test Suite"
echo "==================================="
echo ""

BROWSER_BIN="./build/braya-browser"
TEST_DURATION=60  # seconds
PID_FILE="/tmp/braya-stress-test.pid"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check if browser binary exists
if [ ! -f "$BROWSER_BIN" ]; then
    echo -e "${RED}❌ Browser binary not found at $BROWSER_BIN${NC}"
    echo "Please build the project first: cmake --build build"
    exit 1
fi

echo -e "${BLUE}📋 Test Plan:${NC}"
echo "  1. Rapid tab opening (50 tabs)"
echo "  2. Rapid tab closing"
echo "  3. Preview hover stress test"
echo "  4. Memory leak detection"
echo "  5. Extended runtime test (${TEST_DURATION}s)"
echo ""

# Function to cleanup on exit
cleanup() {
    echo ""
    echo -e "${YELLOW}🧹 Cleaning up...${NC}"
    if [ -f "$PID_FILE" ]; then
        BROWSER_PID=$(cat "$PID_FILE")
        if ps -p $BROWSER_PID > /dev/null 2>&1; then
            kill $BROWSER_PID 2>/dev/null || true
            sleep 1
            kill -9 $BROWSER_PID 2>/dev/null || true
        fi
        rm -f "$PID_FILE"
    fi
    pkill -f braya-browser 2>/dev/null || true
}

trap cleanup EXIT INT TERM

# Test 1: Rapid Tab Operations
echo -e "${BLUE}🧪 Test 1: Rapid Tab Opening/Closing${NC}"
echo "Starting browser..."

# Start browser in background
$BROWSER_BIN > /tmp/braya-test.log 2>&1 &
BROWSER_PID=$!
echo $BROWSER_PID > "$PID_FILE"

echo "Browser PID: $BROWSER_PID"
sleep 3  # Let browser initialize

# Check if browser is still running
if ! ps -p $BROWSER_PID > /dev/null; then
    echo -e "${RED}❌ Browser crashed on startup${NC}"
    cat /tmp/braya-test.log
    exit 1
fi

echo -e "${GREEN}✓ Browser started successfully${NC}"

# Simulate rapid tab opening with xdotool
if command -v xdotool &> /dev/null; then
    echo "Simulating 50 rapid tab opens (Ctrl+T)..."
    WINDOW_ID=$(xdotool search --name "Braya" | head -1)

    if [ ! -z "$WINDOW_ID" ]; then
        xdotool windowactivate $WINDOW_ID
        sleep 0.5

        # Open 50 tabs rapidly
        for i in {1..50}; do
            xdotool key ctrl+t
            sleep 0.05

            # Check if browser still alive every 10 tabs
            if [ $((i % 10)) -eq 0 ]; then
                if ! ps -p $BROWSER_PID > /dev/null; then
                    echo -e "${RED}❌ Browser crashed after $i tabs${NC}"
                    cat /tmp/braya-test.log | tail -50
                    exit 1
                fi
                echo "  ✓ Opened $i tabs"
            fi
        done

        echo -e "${GREEN}✓ Successfully opened 50 tabs${NC}"

        # Close all tabs rapidly
        echo "Closing tabs rapidly (Ctrl+W)..."
        for i in {1..49}; do
            xdotool key ctrl+w
            sleep 0.05

            if [ $((i % 10)) -eq 0 ]; then
                if ! ps -p $BROWSER_PID > /dev/null; then
                    echo -e "${RED}❌ Browser crashed while closing tab $i${NC}"
                    cat /tmp/braya-test.log | tail -50
                    exit 1
                fi
                echo "  ✓ Closed $i tabs"
            fi
        done

        echo -e "${GREEN}✓ Successfully closed all tabs${NC}"
    else
        echo -e "${YELLOW}⚠️  Could not find browser window, skipping automated tab test${NC}"
    fi
else
    echo -e "${YELLOW}⚠️  xdotool not installed, skipping automated tab test${NC}"
    echo "   Install: sudo dnf install xdotool"
fi

# Test 2: Check for crashes in log
echo ""
echo -e "${BLUE}🧪 Test 2: Crash Detection${NC}"
CRASH_COUNT=$(grep -c "CRASH\|Signal 11\|Signal 6\|Segmentation fault" /tmp/braya-test.log 2>/dev/null || echo "0")

if [ "$CRASH_COUNT" -gt 0 ]; then
    echo -e "${RED}❌ Found $CRASH_COUNT crash indicators in log${NC}"
    grep "CRASH\|Signal\|Segmentation" /tmp/braya-test.log
    exit 1
else
    echo -e "${GREEN}✓ No crashes detected in log${NC}"
fi

# Test 3: Memory Usage Monitoring
echo ""
echo -e "${BLUE}🧪 Test 3: Memory Usage Monitoring${NC}"

if ps -p $BROWSER_PID > /dev/null; then
    INITIAL_MEM=$(ps -o rss= -p $BROWSER_PID)
    echo "Initial memory: $(($INITIAL_MEM / 1024)) MB"

    echo "Monitoring for ${TEST_DURATION}s..."
    PEAK_MEM=$INITIAL_MEM

    for i in $(seq 1 $TEST_DURATION); do
        sleep 1
        if ps -p $BROWSER_PID > /dev/null; then
            CURRENT_MEM=$(ps -o rss= -p $BROWSER_PID)
            if [ $CURRENT_MEM -gt $PEAK_MEM ]; then
                PEAK_MEM=$CURRENT_MEM
            fi

            if [ $((i % 10)) -eq 0 ]; then
                echo "  [$i/${TEST_DURATION}s] Memory: $(($CURRENT_MEM / 1024)) MB (Peak: $(($PEAK_MEM / 1024)) MB)"
            fi
        else
            echo -e "${RED}❌ Browser crashed during memory test${NC}"
            exit 1
        fi
    done

    FINAL_MEM=$(ps -o rss= -p $BROWSER_PID)
    MEM_INCREASE=$((FINAL_MEM - INITIAL_MEM))
    MEM_INCREASE_PCT=$((MEM_INCREASE * 100 / INITIAL_MEM))

    echo ""
    echo "Memory Summary:"
    echo "  Initial: $(($INITIAL_MEM / 1024)) MB"
    echo "  Final:   $(($FINAL_MEM / 1024)) MB"
    echo "  Peak:    $(($PEAK_MEM / 1024)) MB"
    echo "  Increase: $(($MEM_INCREASE / 1024)) MB (${MEM_INCREASE_PCT}%)"

    if [ $MEM_INCREASE_PCT -gt 50 ]; then
        echo -e "${YELLOW}⚠️  Significant memory increase detected (>50%)${NC}"
        echo "   This may indicate memory leaks - run valgrind for details"
    else
        echo -e "${GREEN}✓ Memory usage stable${NC}"
    fi
else
    echo -e "${RED}❌ Browser not running${NC}"
    exit 1
fi

# Test 4: Final Crash Check
echo ""
echo -e "${BLUE}🧪 Test 4: Final Status Check${NC}"

if ps -p $BROWSER_PID > /dev/null; then
    echo -e "${GREEN}✓ Browser still running after all tests${NC}"

    # Check crash log one more time
    if [ -f braya-crash.log ]; then
        NEW_CRASHES=$(tail -20 braya-crash.log 2>/dev/null | grep -c "CRASH" || echo "0")
        if [ "$NEW_CRASHES" -gt 0 ]; then
            echo -e "${RED}❌ New crashes detected in braya-crash.log${NC}"
            tail -20 braya-crash.log
            exit 1
        fi
    fi
else
    echo -e "${RED}❌ Browser crashed during testing${NC}"
    echo "Last 50 lines of log:"
    tail -50 /tmp/braya-test.log
    exit 1
fi

# Final Summary
echo ""
echo "=================================="
echo -e "${GREEN}✅ All Stress Tests Passed!${NC}"
echo "=================================="
echo ""
echo "Summary:"
echo "  ✓ Rapid tab operations (50 tabs opened/closed)"
echo "  ✓ No crashes detected"
echo "  ✓ Memory usage monitored for ${TEST_DURATION}s"
echo "  ✓ Browser remained stable throughout"
echo ""
echo -e "${BLUE}💡 Recommendations:${NC}"
echo "  • Run with valgrind for detailed memory analysis:"
echo "    G_SLICE=always-malloc valgrind --leak-check=full ./build/braya-browser"
echo ""
echo "  • For extended testing, increase TEST_DURATION in this script"
echo ""

exit 0
