#!/bin/bash

# Braya Browser Memory Leak Detection with Valgrind
# Comprehensive memory analysis tool

set -e

echo "🔍 Braya Browser Memory Leak Detection"
echo "======================================"
echo ""

BROWSER_BIN="./build/braya-browser"
VALGRIND_LOG="valgrind-report.txt"
SUPPRESSIONS_FILE="valgrind-suppressions.supp"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Check dependencies
if ! command -v valgrind &> /dev/null; then
    echo -e "${RED}❌ valgrind not found${NC}"
    echo "Install with: sudo dnf install valgrind"
    exit 1
fi

if [ ! -f "$BROWSER_BIN" ]; then
    echo -e "${RED}❌ Browser binary not found${NC}"
    echo "Build with: cmake --build build"
    exit 1
fi

# Create suppressions file for known GTK/WebKit issues
cat > "$SUPPRESSIONS_FILE" << 'EOF'
{
   GTK_g_type_register_static
   Memcheck:Leak
   ...
   fun:g_type_register_static
}
{
   GTK_g_object_new
   Memcheck:Leak
   ...
   fun:g_object_new*
}
{
   WebKit_JavaScriptCore
   Memcheck:Leak
   ...
   obj:*/libjavascriptcoregtk*
}
{
   GLib_g_quark_init
   Memcheck:Leak
   ...
   fun:g_quark_init
}
EOF

echo -e "${BLUE}📋 Configuration:${NC}"
echo "  Binary: $BROWSER_BIN"
echo "  Log file: $VALGRIND_LOG"
echo "  Suppressions: $SUPPRESSIONS_FILE"
echo ""

echo -e "${YELLOW}⚠️  This test will run SLOW (10-20x slower than normal)${NC}"
echo "   Be patient - it's checking every memory operation"
echo ""
echo "Press Ctrl+C within 5 seconds to cancel..."
sleep 5

echo ""
echo -e "${BLUE}🚀 Starting Valgrind Analysis...${NC}"
echo "   This will take a few minutes. Use the browser normally, then close it."
echo ""

# Run valgrind with comprehensive checks
G_SLICE=always-malloc \
G_DEBUG=gc-friendly \
valgrind \
    --tool=memcheck \
    --leak-check=full \
    --show-leak-kinds=all \
    --track-origins=yes \
    --verbose \
    --log-file="$VALGRIND_LOG" \
    --suppressions="$SUPPRESSIONS_FILE" \
    --num-callers=20 \
    --track-fds=yes \
    "$BROWSER_BIN" 2>&1 | tee valgrind-console.log

echo ""
echo -e "${GREEN}✓ Valgrind analysis complete${NC}"
echo ""

# Parse results
echo -e "${BLUE}📊 Analysis Results:${NC}"
echo "=================================="

# Count leaks
DEFINITELY_LOST=$(grep "definitely lost:" "$VALGRIND_LOG" | tail -1 | awk '{print $4}' | sed 's/,//g')
INDIRECTLY_LOST=$(grep "indirectly lost:" "$VALGRIND_LOG" | tail -1 | awk '{print $4}' | sed 's/,//g')
POSSIBLY_LOST=$(grep "possibly lost:" "$VALGRIND_LOG" | tail -1 | awk '{print $4}' | sed 's/,//g')
STILL_REACHABLE=$(grep "still reachable:" "$VALGRIND_LOG" | tail -1 | awk '{print $4}' | sed 's/,//g')

echo ""
echo "Memory Leak Summary:"
echo "  Definitely lost:  ${DEFINITELY_LOST:-0} bytes"
echo "  Indirectly lost:  ${INDIRECTLY_LOST:-0} bytes"
echo "  Possibly lost:    ${POSSIBLY_LOST:-0} bytes"
echo "  Still reachable:  ${STILL_REACHABLE:-0} bytes"
echo ""

# Check for critical issues
CRITICAL_LEAKS=0

if [ ! -z "$DEFINITELY_LOST" ] && [ "$DEFINITELY_LOST" -gt 10000 ]; then
    echo -e "${RED}❌ CRITICAL: Definitely lost > 10KB${NC}"
    CRITICAL_LEAKS=1
fi

if [ ! -z "$INDIRECTLY_LOST" ] && [ "$INDIRECTLY_LOST" -gt 50000 ]; then
    echo -e "${YELLOW}⚠️  WARNING: Indirectly lost > 50KB${NC}"
fi

# Count invalid reads/writes
INVALID_OPS=$(grep -c "Invalid read\|Invalid write" "$VALGRIND_LOG" || echo "0")
if [ "$INVALID_OPS" -gt 0 ]; then
    echo -e "${RED}❌ CRITICAL: $INVALID_OPS invalid memory operations${NC}"
    CRITICAL_LEAKS=1
fi

# Count use-after-free
USE_AFTER_FREE=$(grep -c "use after free" "$VALGRIND_LOG" || echo "0")
if [ "$USE_AFTER_FREE" -gt 0 ]; then
    echo -e "${RED}❌ CRITICAL: $USE_AFTER_FREE use-after-free errors${NC}"
    CRITICAL_LEAKS=1
fi

echo ""
echo "File Descriptor Leaks:"
FD_LEAKS=$(grep "Open file descriptor" "$VALGRIND_LOG" | wc -l || echo "0")
if [ "$FD_LEAKS" -gt 5 ]; then
    echo -e "${YELLOW}⚠️  WARNING: $FD_LEAKS file descriptors left open${NC}"
else
    echo -e "${GREEN}✓ File descriptors OK ($FD_LEAKS)${NC}"
fi

echo ""
echo "=================================="

if [ $CRITICAL_LEAKS -eq 0 ]; then
    echo -e "${GREEN}✅ No critical memory issues found!${NC}"
    echo ""
    echo "Note: Some 'still reachable' leaks are normal for GTK/WebKit"
    echo "These are typically cached resources that would be freed on exit"
else
    echo -e "${RED}❌ Critical memory issues detected${NC}"
    echo ""
    echo "Review detailed report: $VALGRIND_LOG"
    echo ""
    echo "Most common leak locations:"
    grep -A 5 "definitely lost" "$VALGRIND_LOG" | grep "at 0x" | sort | uniq -c | sort -rn | head -10
    exit 1
fi

echo ""
echo -e "${BLUE}📄 Full report saved to: $VALGRIND_LOG${NC}"
echo ""
echo "To view detailed leaks:"
echo "  grep -A 20 'definitely lost' $VALGRIND_LOG | less"
echo ""
echo "To view our code's leaks (excluding GTK/WebKit):"
echo "  grep -B 5 -A 10 'BrayaWindow\|BrayaTab' $VALGRIND_LOG | less"
echo ""

exit 0
