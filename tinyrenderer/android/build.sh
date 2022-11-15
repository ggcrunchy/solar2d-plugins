#!/bin/sh

# This option is used to exit the script as
# soon as a command returns a non-zero value.
set -o errexit

path=`dirname $0`

TARGET_NAME=tinyrenderer
CONFIG=Release
DEVICE_TYPE=all
BUILD_TYPE=clean

if [ $OS == Windows_NT ]
then
	ANDROID_NDK="D:/android-ndk-r21b"
	LIBS_SRC_DIR="$CORONA_ROOT/Corona/android/lib/gradle/Corona.aar"
	CMD="cmd //c "
else
    ANDROID_NDK="$HOME/Library/Android/sdk/ndk/24.0.8215888"
    LIBS_SRC_DIR="/Applications/Native/Corona/android/lib/gradle/Corona.aar"
	CMD=
fi
#
# Checks exit value for error
# 
if [ -z "$ANDROID_NDK" ]
then
	echo "ERROR: ANDROID_NDK environment variable must be defined"
	exit 0
fi

# Canonicalize paths
pushd $path > /dev/null
dir=`pwd`
path=$dir
popd > /dev/null

######################
# Build .so          #
######################

pushd $path/jni > /dev/null

if [ "Release" == "$CONFIG" ]
then
	echo "Building RELEASE"
	OPTIM_FLAGS="release"
else
	echo "Building DEBUG"
	OPTIM_FLAGS="debug"
fi

if [ "clean" == "$BUILD_TYPE" ]
then
	echo "== Clean build =="
	rm -rf $path/obj/ $path/libs/ $path/data.tgz
	FLAGS="-B"
else
	echo "== Incremental build =="
	FLAGS=""
fi

CFLAGS=

if [ "$OPTIM_FLAGS" = "debug" ]
then
	CFLAGS="${CFLAGS} -DRtt_DEBUG -g"
	FLAGS="$FLAGS NDK_DEBUG=1"
fi

# Copy .so files
LIBS_DST_DIR="$path/corona-libs"
mkdir -p "$LIBS_DST_DIR"

unzip -u "$LIBS_SRC_DIR" "jni/*/*.so" -d "$LIBS_DST_DIR"

if [ -z "$CFLAGS" ]
then
	echo "----------------------------------------------------------------------------"
	echo "$ANDROID_NDK/ndk-build $FLAGS V=1 APP_OPTIM=$OPTIM_FLAGS"
	echo "----------------------------------------------------------------------------"

	$CMD $ANDROID_NDK/ndk-build $FLAGS V=1 APP_OPTIM=$OPTIM_FLAGS
else
	echo "----------------------------------------------------------------------------"
	echo "$ANDROID_NDK/ndk-build $FLAGS V=1 MY_CFLAGS="$CFLAGS" APP_OPTIM=$OPTIM_FLAGS"
	echo "----------------------------------------------------------------------------"

	$CMD $ANDROID_NDK/ndk-build $FLAGS V=1 MY_CFLAGS="$CFLAGS" APP_OPTIM=$OPTIM_FLAGS
fi

find "$path/libs" \( -name liblua.so -or -name libcorona.so \)  -delete
echo "$path/libs"
rm -rf "$path/jniLibs"
mv "$path/libs" "$path/jniLibs"

popd > /dev/null

######################
# Post-compile Steps #
######################

echo Done.
echo $path/jniLibs/armeabi-v7a/libplugin.$TARGET_NAME.so

echo Packing binaries...
tar -czvf data.tgz -C $path jniLibs -C $path/jniLibs/armeabi-v7a libplugin.$TARGET_NAME.so
echo $path/data.tgz.
