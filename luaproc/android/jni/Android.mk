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
CUCKOO_DIR := $(PLUGIN_DIR)/../libcuckoo

SRC_DIR := $(PLUGIN_DIR)/shared
CEU_DIR := $(PLUGIN_DIR)/../corona_enterprise_utils

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
LOCAL_MODULE := libplugin.luaproc

LOCAL_C_INCLUDES := \
	$(SRC_DIR) $(CUCKOO_DIR) $(CUCKOO_DIR)/.. $(CEU_DIR)

LOCAL_SRC_FILES := \
	$(SRC_DIR)/luaproc.c $(SRC_DIR)/lpsched.c \
	$(SRC_DIR)/extensions.cpp $(SRC_DIR)/pevents.cpp \
	$(CUCKOO_DIR)/city.cc

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

LOCAL_CPPFLAGS += -std=c++11 -DWFMO
LOCAL_CPP_FEATURES += exceptions

# Arm vs Thumb.
LOCAL_ARM_MODE := arm
include $(BUILD_SHARED_LIBRARY)
