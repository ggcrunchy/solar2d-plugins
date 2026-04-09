
#!/bin/bash
# fat_luajit_rename.sh - FIXED version

rename_lib() {
    local arch_lib="$1"
    echo "Processing $arch_lib..."
    cd tmp
    ar x "$arch_lib"
    
    # Rename ALL objects FIRST
    for obj in *.o; do
        [ -f "$obj" ] || continue
        /opt/homebrew/Cellar/llvm/21.1.8/bin/llvm-objcopy --redefine-syms="../$MAP" "$obj" "${obj%.o}_lj.o"
    done
    
    # Build new archive from renamed objects only
    /opt/homebrew/Cellar/llvm/21.1.8/bin/llvm-ar rcs "${arch_lib%.a}_lj.a" *_lj.o
    /opt/homebrew/Cellar/llvm/21.1.8/bin/llvm-ranlib "${arch_lib%.a}_lj.a"
    
    # NOW delete originals (new files safe)
    rm -f *.o
    
    cd ..
}

# Main execution
LIB="$1"
MAP="$2"
OUTLIB="${LIB%.a}_lj.a"

mkdir -p tmp
cp "$LIB" tmp

# Split architectures
lipo tmp/"$LIB" -thin x86_64 -output tmp/luajit_x86_64.a
lipo tmp/"$LIB" -thin arm64  -output tmp/luajit_arm64.a

rename_lib luajit_x86_64.a
rename_lib luajit_arm64.a

# Create fat binary
lipo tmp/luajit_x86_64_lj.a tmp/luajit_arm64_lj.a -create -output "$OUTLIB"

# Final cleanup
rm -rf tmp/luajit_* 

echo "Created $OUTLIB"
