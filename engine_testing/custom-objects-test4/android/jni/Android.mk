# Copyright (C) 2012 Corona Labs Inc.
#

# TARGET_PLATFORM := android-8

LOCAL_PATH := $(call my-dir)

ifeq ($(OS),Windows_NT)
	CORONA_ROOT := C:\PROGRA~2\CORONA~1\Corona\Native
else
	CORONA := /Applications/Corona-3649
	CORONA_ROOT := $(CORONA)/Native
endif

LUA_API_DIR := $(CORONA_ROOT)/Corona/shared/include/lua
LUA_API_CORONA := $(CORONA_ROOT)/Corona/shared/include/Corona

PLUGIN_DIR := ../..

SRC_DIR := $(PLUGIN_DIR)/shared

# -----------------------------------------------------------------------------

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
LOCAL_MODULE := libplugin.customobjects4

LOCAL_C_INCLUDES := \
	$(SRC_DIR)

LOCAL_SRC_FILES := \
	$(SRC_DIR)/plugin.customobjects.cpp $(SRC_DIR)/utils.cpp $(SRC_DIR)/DepthClear.cpp $(SRC_DIR)/DepthEnvironment.cpp $(SRC_DIR)/DepthLib.cpp $(SRC_DIR)/DepthState.cpp
	

LOCAL_CFLAGS := \
	-DANDROID_NDK \
	-DNDEBUG \
	-D_REENTRANT \
	-DRtt_ANDROID_ENV

LOCAL_LDLIBS := -llog -lGLESv2

LOCAL_SHARED_LIBRARIES := \
	liblua libcorona

ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS += -D_ARM_ASSEM_ -D_M_ARM
endif

ifeq ($(TARGET_ARCH),armeabi-v7a)
LOCAL_CFLAGS += -DHAVENEON=1
endif

# LOCAL_CFLAGS += -mfloat-abi=softfp -mfpu=neon -march=armv7 -mthumb
LOCAL_CPPFLAGS += -std=c++11
LOCAL_CPP_FEATURES += exceptions

# Arm vs Thumb.
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true
include $(BUILD_SHARED_LIBRARY)
