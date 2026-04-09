# Copyright (C) 2012 Corona Labs Inc.
#

# TARGET_PLATFORM := android-8

LOCAL_PATH := $(call my-dir)

ifeq ($(OS),Windows_NT)
	CORONA_ROOT := C:\PROGRA~2\CORONA~1\Corona\Native
else
	CORONA_ROOT := /Applications/Native
endif

LUA_API_DIR := $(CORONA_ROOT)/Corona/shared/include/lua
LUA_API_CORONA := $(CORONA_ROOT)/Corona/shared/include/Corona

PLUGIN_DIR := ../..

SHARED_DIR := $(PLUGIN_DIR)/shared
SRC_DIR := $(SHARED_DIR)/src

ASSIMP_DIR := $(SHARED_DIR)/assimp

# -----------------------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := libassimp
LOCAL_SRC_FILES := ../assimp-libs/$(TARGET_ARCH_ABI)/libassimp.a
LOCAL_EXPORT_C_INCLUDES := $(ASSIMP_DIR)
include $(PREBUILT_STATIC_LIBRARY)

# The above was generated with \
ANDROID_ABI=arm64-v8a (or the others) \
ANDROID_NDK=/path/to/ndk/24.0.8215888 (or whatever) \
ANDROID_PLATFORM=android-21 (for arm64-v8a and x86_64, else android-16) \
cd assimp/build \
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=$ANDROID_ABI \
      -DANDROID_PLATFORM=$ANDROID_PLATFORM \
      -DASSIMP_ANDROID_JNIIOSYSTEM=OFF \
      -DASSIMP_NO_EXPORT=ON \
      -DASSIMP_BUILD_TESTS=OFF \
      -DBUILD_SHARED_LIBS=OFF \
      -DCMAKE_BUILD_TYPE=Release \
      ..  (cmake slashes in original, but not this one) \
make \
This was also relevant: https://github.com/assimp/assimp/issues/5408#issuecomment-1907690426

#include $(CLEAR_VARS)
#LOCAL_MODULE := libzlibstatic
#LOCAL_SRC_FILES := ../assimp-libs/$(TARGET_ARCH_ABI)/libzlibstatic.a
#LOCAL_EXPORT_C_INCLUDES := $(ASSIMP_DIR)
#include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := liblua
LOCAL_SRC_FILES := ../corona-libs/jni/$(TARGET_ARCH_ABI)/liblua.so
LOCAL_EXPORT_C_INCLUDES := $(LUA_API_DIR)
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libcorona
LOCAL_SRC_FILES := ../corona-libs/jni/$(TARGET_ARCH_ABI)/libcorona.so
LOCAL_EXPORT_C_INCLUDES := $(LUA_API_CORONA)
include $(PREBUILT_SHARED_LIBRARY)

# -----------------------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := libplugin.moonassimp

LOCAL_C_INCLUDES := \
	$(SRC_DIR) $(SHARED_DIR) $(ASSIMP_DIR)

LOCAL_SRC_FILES := $(SRC_DIR)/additional.c $(SRC_DIR)/animation.c $(SRC_DIR)/animmesh.c \
	$(SRC_DIR)/bitfields.c $(SRC_DIR)/bone.c $(SRC_DIR)/camera.c $(SRC_DIR)/compat-5.3.c \
	$(SRC_DIR)/enums.c $(SRC_DIR)/face.c $(SRC_DIR)/import.c $(SRC_DIR)/light.c \
	$(SRC_DIR)/main.c $(SRC_DIR)/material.c $(SRC_DIR)/mesh.c $(SRC_DIR)/meshanim.c \
	$(SRC_DIR)/node.c $(SRC_DIR)/nodeanim.c $(SRC_DIR)/scene.c $(SRC_DIR)/texture.c \
	$(SRC_DIR)/udata.c $(SRC_DIR)/utils.c $(SRC_DIR)/dummy.cpp
	

LOCAL_CFLAGS := \
	-DANDROID_NDK \
	-DNDEBUG \
	-D_REENTRANT \
	-DRtt_ANDROID_ENV

LOCAL_CPPFLAGS := -std=c++17

LOCAL_STATIC_LIBRARIES := \
	libassimp #libzlibstatic

LOCAL_LDLIBS := -llog -lz

LOCAL_SHARED_LIBRARIES := \
	liblua libcorona

ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS += -D_ARM_ASSEM_ -D_M_ARM
endif

ifeq ($(TARGET_ARCH),armeabi-v7a)
LOCAL_CFLAGS += -DHAVENEON=1
endif

LOCAL_CPP_FEATURES += exceptions rtti

LOCAL_LDFLAGS+="-Wl,-z,max-page-size=16384"

# Arm vs Thumb.
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true
include $(BUILD_SHARED_LIBRARY)
