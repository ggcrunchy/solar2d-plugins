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
BR_DIR := $(PLUGIN_DIR)/../ByteReader
CEU_DIR := $(PLUGIN_DIR)/../corona_enterprise_utils
CEU_SRC := $(CEU_DIR)/utils

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

# -----------------------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := libplugin.object3d

LOCAL_C_INCLUDES := \
	$(SRC_DIR) $(BR_DIR) $(CEU_DIR)

LOCAL_SRC_FILES := \
	$(SRC_DIR)/plugin.object3d.cpp $(SRC_DIR)/geometry.cpp  \
	$(BR_DIR)/ByteReader.cpp \
	$(CEU_SRC)/Blob.cpp $(CEU_SRC)/Byte.cpp $(CEU_SRC)/LuaEx.cpp $(CEU_SRC)/Memory.cpp $(CEU_SRC)/Path.cpp \
	$(CEU_SRC)/SIMD.cpp $(CEU_SRC)/Thread.cpp

	

LOCAL_CFLAGS := \
	-DANDROID_NDK \
	-DNDEBUG \
	-D_REENTRANT \
	-DRtt_ANDROID_ENV

LOCAL_LDLIBS := -llog

LOCAL_CFLAGS += -fopenmp
LOCAL_LDFLAGS += -fopenmp

LOCAL_SHARED_LIBRARIES := \
	liblua libcorona

ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS += -D_ARM_ASSEM_ -D_M_ARM
endif

ifeq ($(TARGET_ARCH),armeabi-v7a)
LOCAL_CFLAGS += -DHAVENEON=1
endif

LOCAL_WHOLE_STATIC_LIBRARIES += cpufeatures
LOCAL_CFLAGS += -mfloat-abi=softfp -mfpu=neon -march=armv7 -mthumb
LOCAL_CPPFLAGS += -std=c++11

# Arm vs Thumb.
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true
include $(BUILD_SHARED_LIBRARY)

$(call import-module, android/cpufeatures)
