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
LOCAL_MODULE := libplugin.generator

LOCAL_C_INCLUDES := \
	$(SHARED_DIR) $(SHARED_DIR)/include $(SHARED_DIR)/include/generator $(BR_DIR) $(SNU_DIR)

LOCAL_SRC_FILES := \
	$(SHARED_DIR)/plugin.generator.cpp $(SHARED_DIR)/generators.cpp \
	$(SHARED_DIR)/meshes.cpp $(SHARED_DIR)/paths.cpp $(SHARED_DIR)/shapes.cpp \
	$(SHARED_DIR)/specializations.cpp $(BR_DIR)/ByteReader.cpp \
	$(SNU_SRC)/LuaEx.cpp \
	$(SRC_DIR)/AnyMesh.cpp $(SRC_DIR)/AnyPath.cpp $(SRC_DIR)/AnyShape.cpp \
	$(SRC_DIR)/BoxMesh.cpp $(SRC_DIR)/CappedConeMesh.cpp $(SRC_DIR)/CappedCylinderMesh.cpp \
	$(SRC_DIR)/CappedTubeMesh.cpp $(SRC_DIR)/CapsuleMesh.cpp $(SRC_DIR)/CircleShape.cpp \
	$(SRC_DIR)/ConeMesh.cpp $(SRC_DIR)/ConvexPolygonMesh.cpp $(SRC_DIR)/CylinderMesh.cpp \
	$(SRC_DIR)/DiskMesh.cpp $(SRC_DIR)/DodecahedronMesh.cpp $(SRC_DIR)/EmptyMesh.cpp \
	$(SRC_DIR)/EmptyPath.cpp $(SRC_DIR)/EmptyShape.cpp $(SRC_DIR)/GridShape.cpp \
	$(SRC_DIR)/HelixPath.cpp $(SRC_DIR)/IcosahedronMesh.cpp $(SRC_DIR)/IcoSphereMesh.cpp \
	$(SRC_DIR)/KnotPath.cpp $(SRC_DIR)/LinePath.cpp $(SRC_DIR)/LineShape.cpp \
	$(SRC_DIR)/ParametricMesh.cpp $(SRC_DIR)/ParametricPath.cpp $(SRC_DIR)/ParametricShape.cpp \
	$(SRC_DIR)/PlaneMesh.cpp $(SRC_DIR)/RectangleShape.cpp $(SRC_DIR)/RoundedBoxMesh.cpp \
	$(SRC_DIR)/RoundedRectangleShape.cpp $(SRC_DIR)/SphereMesh.cpp \
	$(SRC_DIR)/SphericalConeMesh.cpp $(SRC_DIR)/SphericalTriangleMesh.cpp \
	$(SRC_DIR)/SpringMesh.cpp $(SRC_DIR)/TeapotMesh.cpp $(SRC_DIR)/TorusKnotMesh.cpp \
	$(SRC_DIR)/TorusMesh.cpp $(SRC_DIR)/TriangleMesh.cpp $(SRC_DIR)/TubeMesh.cpp

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
