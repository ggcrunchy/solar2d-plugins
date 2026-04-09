#!/bin/bash
# auto_generate_map.sh - Creates luajit_symbols.map from libluajit.a
# Usage: ./auto_generate_map.sh libluajit.a > luajit_symbols.map

LIB="$1"
if [ ! -f "$LIB" ]; then
    echo "Usage: $0 libluajit.a"
    exit 1
fi

nm "$LIB" | awk '
/^[0-9a-fA-F]* T _/ && /lua/ {
    sym = substr($3, 2)  # Strip leading _
    
    # Replace lua_ or luaL_ → lj_ or ljL_
    if (substr(sym, 1, 5) == "luaL_") {
        new_sym = "_ljL_" substr(sym, 6)
    } else if (substr(sym, 1, 4) == "lua_") {
        new_sym = "_lj_" substr(sym, 5)
    } else {
        next  # Skip non-standard lua symbols
    }
    
    printf "%s %s\n", $3, new_sym  # _luaX_xxx → _ljX_xxx
}' | sort -u
