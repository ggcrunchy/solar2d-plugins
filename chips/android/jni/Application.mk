# albert: This is MIGHT be useless because this is a plugin, NOT an application.

#APP_MODULES      := bit 
#APP_PROJECT_PATH := $(call my-dir)/project
APP_PLATFORM := android-8
#APP_ABI := armeabi 
#APP_ABI := armeabi armeabi-v7a
APP_ABI := armeabi-v7a

# Box2D needs this
APP_STL := gnustl_static

# Avoid the shared object
APP_LIBCRYSTAX := static
