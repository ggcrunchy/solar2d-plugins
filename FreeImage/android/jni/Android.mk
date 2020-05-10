# Copyright (C) 2012 Corona Labs Inc.
#

# TARGET_PLATFORM := android-8

LOCAL_PATH := $(call my-dir)

CORONA_ENTERPRISE := /Applications/CoronaEnterprise
CORONA_ROOT := $(CORONA_ENTERPRISE)/Corona
LUA_API_DIR := $(CORONA_ROOT)/shared/include/lua
LUA_API_CORONA := $(CORONA_ROOT)/shared/include/Corona

PLUGIN_DIR := ../..

SRC_DIR := $(PLUGIN_DIR)/shared

# -----------------------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := liblua
LOCAL_SRC_FILES := ../liblua.so
LOCAL_EXPORT_C_INCLUDES := $(LUA_API_DIR)
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libcorona
LOCAL_SRC_FILES := ../libcorona.so
LOCAL_EXPORT_C_INCLUDES := $(LUA_API_CORONA)
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libfreeimage
LOCAL_SRC_FILES := $(PLUGIN_DIR)/libs/android/libFreeImage.a
include $(PREBUILT_STATIC_LIBRARY)

# -----------------------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := libplugin.freeimage

LOCAL_C_INCLUDES := \
	$(SRC_DIR)

LOCAL_SRC_FILES := \
	$(SRC_DIR)/plugin.freeimage.cpp \
	$(SRC_DIR)/Enums.cpp $(SRC_DIR)/Flags.cpp $(SRC_DIR)/Image.cpp \
	$(SRC_DIR)/MemoryIO.cpp $(SRC_DIR)/MetadataFind.cpp \
	$(SRC_DIR)/MultiPage.cpp $(SRC_DIR)/Tag.cpp $(SRC_DIR)/Texture.cpp \
	$(SRC_DIR)/WinImage.cpp


LOCAL_CFLAGS := \
	-DANDROID_NDK \
	-DNDEBUG \
	-D_REENTRANT \
	-DRtt_ANDROID_ENV

LOCAL_LDLIBS := -llog

LOCAL_SHARED_LIBRARIES := \
	liblua libcorona

LOCAL_STATIC_LIBRARIES := libfreeimage

ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS+= -D_ARM_ASSEM_
endif

LOCAL_CPPFLAGS += -std=c++11
LOCAL_CPP_FEATURES += exceptions

# Arm vs Thumb.
LOCAL_ARM_MODE := arm
include $(BUILD_SHARED_LIBRARY)
