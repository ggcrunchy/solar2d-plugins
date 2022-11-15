# albert: This is MIGHT be useless because this is a plugin, NOT an application.

#APP_MODULES      := bit 
#APP_PROJECT_PATH := $(call my-dir)/project
APP_PLATFORM := android-8
#APP_ABI := armeabi 
#APP_ABI := armeabi armeabi-v7a
APP_ABI := armeabi-v7a arm64-v8a x86 x86_64

# Box2D needs this
APP_STL := c++_static
