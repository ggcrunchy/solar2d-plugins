# Copyright (C) 2012 Corona Labs Inc.
#

# TARGET_PLATFORM := android-8

LOCAL_PATH := $(call my-dir)

ifeq ($(OS),Windows_NT)
	CORONA_ROOT := C:\PROGRA~2\CORONA~1\Corona\Native
else
	CORONA := /Applications/Corona
	CORONA_ROOT := $(CORONA)/Native
endif

LUA_API_DIR := $(CORONA_ROOT)/Corona/shared/include/lua
LUA_API_CORONA := $(CORONA_ROOT)/Corona/shared/include/Corona

PLUGIN_DIR := ../..
MATH_LIBRARIES_DIR := $(PLUGIN_DIR)/../math_libraries

SRC_DIR := $(PLUGIN_DIR)/shared
BR_DIR := $(PLUGIN_DIR)/../ByteReader
CEU_DIR := $(PLUGIN_DIR)/../corona_enterprise_utils
CEU_SRC := $(CEU_DIR)/utils

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
LOCAL_MODULE := libplugin.serialize

LOCAL_C_INCLUDES := \
	$(SRC_DIR) $(BR_DIR) $(CEU_DIR) $(MATH_LIBRARIES_DIR)

LOCAL_SRC_FILES := \
	$(SRC_DIR)/plugin.serialize.cpp $(SRC_DIR)/stdafx.cpp \
	$(SRC_DIR)/lpack.c $(SRC_DIR)/marshal.c $(SRC_DIR)/struct.c \
	$(BR_DIR)/ByteReader.cpp \
	$(CEU_SRC)/Blob.cpp $(CEU_SRC)/Byte.cpp $(CEU_SRC)/LuaEx.cpp $(CEU_SRC)/Memory.cpp $(CEU_SRC)/SIMD.cpp

LOCAL_CFLAGS := \
	-DANDROID_NDK \
	-DNDEBUG \
	-D_REENTRANT \
	-DRtt_ANDROID_ENV

LOCAL_LDLIBS := -llog

LOCAL_SHARED_LIBRARIES := \
	liblua libcorona

ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS+= -D_ARM_ASSEM_
endif

#for adding cpufeatures
LOCAL_WHOLE_STATIC_LIBRARIES += cpufeatures
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_CFLAGS += -DHAVE_NEON=1
endif

LOCAL_WHOLE_STATIC_LIBRARIES += cpufeatures

LOCAL_CPPFLAGS += -std=c++11
LOCAL_CPP_FEATURES += exceptions

# Arm vs Thumb.
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true
include $(BUILD_SHARED_LIBRARY)

$(call import-module, android/cpufeatures)