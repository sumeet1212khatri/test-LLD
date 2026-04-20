#!/bin/bash
# ─── HFT Orderbook Build & Deploy Script ────────────────────────────────────
set -e

PROJ_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJ_DIR/build"

echo "════════════════════════════════════════"
echo " HFT Order Book Engine — Build System"
echo "════════════════════════════════════════"

# ── Prerequisites ──────────────────────────────────────────────────────────
check_tool() {
    if ! command -v "$1" &>/dev/null; then
        echo "[ERROR] $1 not found. Install with: sudo apt install $2"
        exit 1
    fi
}
check_tool cmake cmake
check_tool g++ g++

GCC_VER=$(g++ --version | head -1 | grep -oP '\d+\.\d+' | head -1)
echo "[INFO]  Compiler : g++ $GCC_VER"
echo "[INFO]  Cores    : $(nproc)"

# ── Build ──────────────────────────────────────────────────────────────────
echo ""
echo "[BUILD] Configuring..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake "$PROJ_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=20 2>&1 | tail -3

echo "[BUILD] Compiling with $(nproc) cores..."
make -j$(nproc) 2>&1

echo ""
echo "[BUILD] ✓ Build successful"
echo "  Binaries: $BUILD_DIR/benchmark"
echo "             $BUILD_DIR/run_tests"

# ── Tests ─────────────────────────────────────────────────────────────────
echo ""
echo "════════════════════════════════════════"
echo " Running Test Suite"
echo "════════════════════════════════════════"
"$BUILD_DIR/run_tests"
echo ""

# ── Quick benchmark ────────────────────────────────────────────────────────
if [[ "${1:-}" == "--quick" ]]; then
    echo "════════════════════════════════════════"
    echo " Quick Benchmark (1M orders)"
    echo "════════════════════════════════════════"
    "$BUILD_DIR/benchmark" --orders 1000000
fi

if [[ "${1:-}" == "--full" ]]; then
    echo "════════════════════════════════════════"
    echo " Full Benchmark (20M orders)"
    echo "════════════════════════════════════════"
    "$BUILD_DIR/benchmark" --orders 20000000
fi

if [[ "${1:-}" == "--json" ]]; then
    "$BUILD_DIR/benchmark" --orders 1000000 --json --quiet
fi

echo ""
echo "════════════════════════════════════════"
echo " Deployment"
echo "════════════════════════════════════════"
echo "[INFO]  Frontend : $PROJ_DIR/frontend/index.html"
echo "[INFO]  README   : $PROJ_DIR/README.md"
echo ""
echo "To deploy to Hugging Face Spaces:"
echo "  1. Create a new Space (SDK: Static)"
echo "  2. Upload frontend/index.html as index.html"
echo "  3. Upload README.md"
echo "  4. Push C++ sources to Space repo for reference"
echo ""
echo "✓ Done"
