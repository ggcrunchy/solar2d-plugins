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

SRC_DIR := $(PLUGIN_DIR)/shared
YOCTO_DIR := $(PLUGIN_DIR)/shared/yocto
BR_DIR := $(PLUGIN_DIR)/../ByteReader
SNU_DIR := $(PLUGIN_DIR)/../solar2d_native_utils
SNU_SRC := $(SNU_DIR)/utils

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
LOCAL_MODULE := libplugin.yoctoshapes

LOCAL_C_INCLUDES := \
	$(SRC_DIR) $(SRC_DIR)/fast_float/include $(YOCTO_DIR) $(BR_DIR) $(SNU_DIR)

LOCAL_SRC_FILES := \
	$(SRC_DIR)/plugin.yoctoshapes.cpp $(SRC_DIR)/fvshape_data.cpp $(SRC_DIR)/shape_data.cpp \
	$(SRC_DIR)/specializations.cpp $(SRC_DIR)/vector1f.cpp $(SRC_DIR)/vector2f.cpp \
	$(SRC_DIR)/vector3f.cpp $(SRC_DIR)/vector4f.cpp $(SRC_DIR)/vector1i.cpp \
	$(SRC_DIR)/vector2i.cpp $(SRC_DIR)/vector3i.cpp $(SRC_DIR)/vector4i.cpp \
	$(YOCTO_DIR)/yocto_shape.cpp \
	$(BR_DIR)/ByteReader.cpp \
	$(SNU_SRC)/Blob.cpp $(SNU_SRC)/Byte.cpp $(SNU_SRC)/LuaEx.cpp

LOCAL_CFLAGS := \
	-DANDROID_NDK \
	-DNDEBUG \
	-D_REENTRANT \
	-DRtt_ANDROID_ENV

LOCAL_LDLIBS := -llog

LOCAL_SHARED_LIBRARIES := \
	liblua libcorona

ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS += -D_ARM_ASSEM_ -D_M_ARM
endif

# LOCAL_CFLAGS += -mfloat-abi=softfp -mfpu=neon -march=armv7 -mthumb
LOCAL_CPPFLAGS += -std=c++17
LOCAL_CPP_FEATURES += exceptions

# Arm vs Thumb.
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true
include $(BUILD_SHARED_LIBRARY)
