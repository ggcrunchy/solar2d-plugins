#!/bin/bash
# rename_luajit.sh - Creates libluajit_lj.a with lj_ prefixed symbols
# Usage: ./rename_luajit.sh libluajit.a luajit_symbols.map

LIB="$1"
MAP="$2"
OUTLIB="${LIB%.a}_lj.a"

if [ ! -f "$LIB" ] || [ ! -f "$MAP" ]; then
    echo "Usage: $0 libluajit.a luajit_symbols.map"
    exit 1
fi

# Extract objects
echo "Extracting objects from $LIB..."
cd "$(dirname "$LIB")"
ar x "$LIB"

# Rename symbols in each .o file using llvm-objcopy (macOS native)
echo "Renaming symbols..."
for obj in *.o; do
    [ -f "$obj" ] || continue
    llvm-objcopy --redefine-syms="$MAP" "$obj" "${obj%.o}_lj.o"
done

# Clean up originals
rm -f *.o

# Rebuild new static library
echo "Rebuilding $OUTLIB..."
ar rcs "$OUTLIB" *_lj.o
ranlib "$OUTLIB"

# Cleanup renamed objects
rm -f *_lj.o

echo "Created $OUTLIB with renamed symbols"
echo "Verify: nm $OUTLIB | grep _lj"
