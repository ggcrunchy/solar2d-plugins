#!/bin/bash

path=`dirname $0`

OUTPUT_DIR=$1
TARGET_NAME=tinyfiledialogs
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

PLUGINS_DIR=~/Solar2DPlugins/com.xibalbastudios/plugin.$TARGET_NAME/mac-sim

mkdir -p "$PLUGINS_DIR"

cp "$path/build/$CONFIG/$TARGET_NAME.$OUTPUT_SUFFIX" "$OUTPUT_DIR/plugin_$TARGET_NAME.$OUTPUT_SUFFIX"

tar -czf "data.tgz" "./plugin_$TARGET_NAME.$OUTPUT_SUFFIX"

cp "data.tgz" "$PLUGINS_DIR/data.tgz"
