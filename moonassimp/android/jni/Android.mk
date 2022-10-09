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

# -----------------------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := libassimp
LOCAL_SRC_FILES := ../assimp-libs/$(TARGET_ARCH_ABI)/libassimp.a
LOCAL_EXPORT_C_INCLUDES := $(ASSIMP_DIR)
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libzlibstatic
LOCAL_SRC_FILES := ../assimp-libs/$(TARGET_ARCH_ABI)/libzlibstatic.a
LOCAL_EXPORT_C_INCLUDES := $(ASSIMP_DIR)
include $(PREBUILT_STATIC_LIBRARY)

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
LOCAL_MODULE := libplugin.moonassimp

LOCAL_C_INCLUDES := \
	$(SRC_DIR) $(SHARED_DIR)

LOCAL_SRC_FILES := $(SRC_DIR)/additional.c $(SRC_DIR)/animation.c $(SRC_DIR)/animmesh.c \
	$(SRC_DIR)/bitfields.c $(SRC_DIR)/bone.c $(SRC_DIR)/camera.c $(SRC_DIR)/compat-5.3.c \
	$(SRC_DIR)/enums.c $(SRC_DIR)/face.c $(SRC_DIR)/import.c $(SRC_DIR)/light.c \
	$(SRC_DIR)/main.c $(SRC_DIR)/material.c $(SRC_DIR)/mesh.c $(SRC_DIR)/meshanim.c \
	$(SRC_DIR)/node.c $(SRC_DIR)/nodeanim.c $(SRC_DIR)/scene.c $(SRC_DIR)/texture.c \
	$(SRC_DIR)/udata.c $(SRC_DIR)/utils.c
	

LOCAL_CFLAGS := \
	-DANDROID_NDK \
	-DNDEBUG \
	-D_REENTRANT \
	-DRtt_ANDROID_ENV

LOCAL_STATIC_LIBRARIES := \
	libzlibstatic libassimp

LOCAL_LDLIBS := -llog

LOCAL_SHARED_LIBRARIES := \
	liblua libcorona

ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS += -D_ARM_ASSEM_ -D_M_ARM
endif

ifeq ($(TARGET_ARCH),armeabi-v7a)
LOCAL_CFLAGS += -DHAVENEON=1
endif

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
LOCAL_CPP_FEATURES += exceptions

# Arm vs Thumb.
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true
include $(BUILD_SHARED_LIBRARY)
