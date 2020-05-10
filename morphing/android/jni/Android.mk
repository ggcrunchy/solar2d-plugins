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

# -----------------------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := libplugin.morphing
LOCAL_C_INCLUDES := \
	$(SRC_DIR)

LOCAL_SRC_FILES := \
	$(SRC_DIR)/plugin.morphing.cpp \
	$(SRC_DIR)/Point.cpp $(SRC_DIR)/CubicMVCs.cpp $(SRC_DIR)/PoissonMVCs.cpp \
	$(SRC_DIR)/State.cpp $(SRC_DIR)/CubicCoords.cpp $(SRC_DIR)/PoissonCoords.cpp \
	$(SRC_DIR)/MorphState.cpp $(SRC_DIR)/Poly.cpp $(SRC_DIR)/utils.cpp

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

LOCAL_CPPFLAGS += -std=c++11

# Arm vs Thumb.
LOCAL_ARM_MODE := arm
include $(BUILD_SHARED_LIBRARY)
