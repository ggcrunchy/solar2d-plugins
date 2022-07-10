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
SNU_DIR := $(PLUGIN_DIR)/../solar2d_native_utils
SNU_SRC := $(SNU_DIR)/utils
MODEL_DIR := $(SRC_DIR)/model
MODULE_DIR := $(SRC_DIR)/module
UTILS_DIR := $(SRC_DIR)/noiseutils

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
LOCAL_MODULE := libplugin.libnoise

LOCAL_C_INCLUDES := \
	$(SRC_DIR) $(SNU_DIR)

LOCAL_SRC_FILES := \
	$(SRC_DIR)/plugin.libnoise.cpp $(SRC_DIR)/add_models.cpp $(SRC_DIR)/add_modulebase.cpp \
	$(SRC_DIR)/add_modules.cpp $(SRC_DIR)/add_properties.cpp $(SRC_DIR)/add_utils.cpp \
	$(SRC_DIR)/latlon.cpp $(SRC_DIR)/noisegen.cpp \
	$(SNU_SRC)/LuaEx.cpp \
	$(MODEL_DIR)/cylinder.cpp $(MODEL_DIR)/line.cpp $(MODEL_DIR)/plane.cpp \
	$(MODEL_DIR)/sphere.cpp \
	$(MODULE_DIR)/abs.cpp $(MODULE_DIR)/add.cpp $(MODULE_DIR)/billow.cpp \
	$(MODULE_DIR)/blend.cpp $(MODULE_DIR)/cache.cpp $(MODULE_DIR)/checkerboard.cpp \
	$(MODULE_DIR)/clamp.cpp $(MODULE_DIR)/const.cpp $(MODULE_DIR)/curve.cpp \
	$(MODULE_DIR)/cylinders.cpp $(MODULE_DIR)/displace.cpp $(MODULE_DIR)/exponent.cpp \
	$(MODULE_DIR)/invert.cpp $(MODULE_DIR)/max.cpp $(MODULE_DIR)/min.cpp \
	$(MODULE_DIR)/modulebase.cpp $(MODULE_DIR)/multiply.cpp $(MODULE_DIR)/perlin.cpp \
	$(MODULE_DIR)/power.cpp $(MODULE_DIR)/ridgedmulti.cpp $(MODULE_DIR)/rotatepoint.cpp \
	$(MODULE_DIR)/scalebias.cpp $(MODULE_DIR)/scalepoint.cpp $(MODULE_DIR)/select.cpp \
	$(MODULE_DIR)/spheres.cpp $(MODULE_DIR)/terrace.cpp $(MODULE_DIR)/translatepoint.cpp \
	$(MODULE_DIR)/turbulence.cpp $(MODULE_DIR)/voronoi.cpp \
	$(UTILS_DIR)/noiseutils.cpp

	

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
LOCAL_CPPFLAGS += -std=c++11
LOCAL_CPP_FEATURES += exceptions

# Arm vs Thumb.
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true
include $(BUILD_SHARED_LIBRARY)
