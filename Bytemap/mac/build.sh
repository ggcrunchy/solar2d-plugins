#!/bin/bash

path=`dirname $0`

OUTPUT_DIR=$1
TARGET_NAME=Bytemap
OUTPUT_SUFFIX=dylib
CONFIG=Release

#
# Checks exit value for error
# 
checkError() {
    if [ $? -ne 0 ]
    then
        echo "Exiting due to errors (above)"
        exit -1
    fi
}

# 
# Canonicalize relative paths to absolute paths
# 
pushd $path > /dev/null
dir=`pwd`
path=$dir
popd > /dev/null

if [ -z "$OUTPUT_DIR" ]
then
    OUTPUT_DIR=.
fi

pushd $OUTPUT_DIR > /dev/null
dir=`pwd`
OUTPUT_DIR=$dir
popd > /dev/null

echo "OUTPUT_DIR: $OUTPUT_DIR"

xcodebuild -project "$path/Plugin.xcodeproj" -configuration $CONFIG clean
checkError

xcodebuild -project "$path/Plugin.xcodeproj" -configuration $CONFIG
checkError

PLUGINS_DIR="$HOME/Library/Application Support/Corona/Simulator/Plugins/plugin/"

cp "$path/build/Release/$TARGET_NAME.$OUTPUT_SUFFIX" "$OUTPUT_DIR"
cp "./$TARGET_NAME.$OUTPUT_SUFFIX" "$PLUGINS_DIR/$TARGET_NAME.dylib"

mkdir -p "$HOME/Solar2DPlugins/com.xibalbastudios/$TARGET_NAME/macos-sim"
tar -czf "$HOME/Solar2DPlugins/com.xibalbastudios/$TARGET_NAME/macos-sim/data.tgz" "./$TARGET_NAME.dylib"
