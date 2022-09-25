#!/bin/bash
set -e


pushd `dirname $0` > /dev/null
path=`pwd`
plugin=`basename $path`
buildId=`ls ../../binaries/memoryBitmap/plugins | tail -n 1`


if [ -d android ]; then
	pushd android
	./build.sh
	cp libs/armeabi-v7a/libplugin.$plugin.so ../../../binaries/$plugin/plugins/$buildId/android/
	popd
fi

if [ -d ios ]; then
	pushd ios
	./build.sh
	cp build/Release-iphoneos/libplugin_$plugin.a ../../../binaries/$plugin/plugins/$buildId/iphone/
	cp build/Release-iphonesimulator/libplugin_$plugin.a ../../../binaries/$plugin/plugins/$buildId/iphone-sim/
	popd
fi

if [ -d tvos ]; then
	pushd tvos
	./build.sh
	rm -rf ../../../binaries/$plugin/plugins/$buildId/appletvos/*.framework
	cp -R build/Release-appletvos/Corona_plugin_$plugin.framework ../../../binaries/$plugin/plugins/$buildId/appletvos/
	rm -rf ../../../binaries/$plugin/plugins/$buildId/appletvsimulator/*.framework
	cp -R build/Release-appletvsimulator/Corona_plugin_$plugin.framework ../../../binaries/$plugin/plugins/$buildId/appletvsimulator/
	popd
fi

if [ -d mac ]; then
	pushd mac
	./build.sh
	cp plugin_$plugin.dylib ../../../binaries/$plugin/plugins/$buildId/mac-sim/
	popd
fi

if [ -d win32/Release ]; then
	pushd win32
	cp Release/plugin_$plugin.dll ../../../binaries/$plugin/plugins/$buildId/win32-sim/
	popd
fi


popd > /dev/null