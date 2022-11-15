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

SRC_DIR := $(PLUGIN_DIR)/shared/src

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
LOCAL_MODULE := libplugin.moonnuklear

LOCAL_C_INCLUDES := \
	$(SRC_DIR) $(SRC_DIR)/ByteReader $(SRC_DIR)/nuklear

LOCAL_SRC_FILES := $(SRC_DIR)/atlas.c $(SRC_DIR)/buffer.c $(SRC_DIR)/canvas \
	$(SRC_DIR)/compat-5.3.c $(SRC_DIR)/context.c $(SRC_DIR)/cursor.c $(SRC_DIR)/edit.c \
	$(SRC_DIR)/enums.c $(SRC_DIR)/flags.c $(SRC_DIR)/font.c $(SRC_DIR)/image.c \
	$(SRC_DIR)/input.c $(SRC_DIR)/layout.c $(SRC_DIR)/main.c $(SRC_DIR)/nuklear.c \
	$(SRC_DIR)/objects.c $(SRC_DIR)/panel.c $(SRC_DIR)/structs.c $(SRC_DIR)/style.c \
	$(SRC_DIR)/tracing.c $(SRC_DIR)/udata.c $(SRC_DIR)/utils.c $(SRC_DIR)/versions.c \
	$(SRC_DIR)/widgets.c $(SRC_DIR)/window.c

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

ifeq ($(TARGET_ARCH),armeabi-v7a)
LOCAL_CFLAGS += -DHAVENEON=1
endif

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
LOCAL_CPP_FEATURES += exceptions

# Arm vs Thumb.
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true
include $(BUILD_SHARED_LIBRARY)
