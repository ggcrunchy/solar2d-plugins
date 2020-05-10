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

SRC_DIR := $(PLUGIN_DIR)/shared
BR_DIR := $(PLUGIN_DIR)/../ByteReader
CEU_DIR := $(PLUGIN_DIR)/../solar2d_native_utils
MATHLIBS_DIR := $(PLUGIN_DIR)/../math_libraries
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
LOCAL_MODULE := libplugin.eigen

LOCAL_C_INCLUDES := \
	$(SRC_DIR) $(BR_DIR) $(CEU_DIR) $(MATHLIBS_DIR)/eigen/eigen $(MATHLIBS_DIR)

LOCAL_SRC_FILES := \
	$(SRC_DIR)/plugin.eigencore.cpp \
	$(BR_DIR)/ByteReader.cpp \
	$(CEU_SRC)/Blob.cpp $(CEU_SRC)/Byte.cpp $(CEU_SRC)/LuaEx.cpp $(CEU_SRC)/Memory.cpp $(CEU_SRC)/Path.cpp \
	$(CEU_SRC)/SIMD.cpp $(CEU_SRC)/Thread.cpp	
LOCAL_CFLAGS := \
	-DANDROID_NDK \
	-DNDEBUG \
	-D_REENTRANT \
	-DRtt_ANDROID_ENV

LOCAL_LDLIBS := -llog -lm -lGLESv2

LOCAL_CFLAGS += -fopenmp -fpermissive
LOCAL_LDFLAGS += -fopenmp

LOCAL_SHARED_LIBRARIES := \
	liblua libcorona

ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS+= -D_ARM_ASSEM_
endif

#for adding cpufeatures
LOCAL_WHOLE_STATIC_LIBRARIES += cpufeatures
ifeq ($(IS_ARM), 1)
    LOCAL_CFLAGS += -DHAVE_NEON=1
else
    LOCAL_CFLAGS += -DTARGET_OS_SIMULATOR
endif

LOCAL_WHOLE_STATIC_LIBRARIES += cpufeatures

# LOCAL_CFLAGS += -mfloat-abi=softfp -mfpu=neon -march=armv7 -mthumb
LOCAL_CPPFLAGS += -std=c++11
LOCAL_CPP_FEATURES += exceptions

# Arm vs Thumb.
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true
include $(BUILD_SHARED_LIBRARY)

$(call import-module, android/cpufeatures)
