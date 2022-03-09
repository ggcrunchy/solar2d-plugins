# Copyright (C) 2012 Corona Labs Inc.
#

# TARGET_PLATFORM := android-8

LOCAL_PATH := $(call my-dir)

CORONA_ENTERPRISE := C:\PROGRA~2\CORONA~1\Corona\Native
CORONA_ROOT := $(CORONA_ENTERPRISE)/Corona
LUA_API_DIR := $(CORONA_ROOT)/shared/include/lua
LUA_API_CORONA := $(CORONA_ROOT)/shared/include/Corona

PLUGIN_DIR := ../..

SRC_DIR := $(PLUGIN_DIR)/shared
BR_DIR := $(PLUGIN_DIR)/../ByteReader

OGG_INC := $(PLUGIN_DIR)/../libogg/include
THEORA_INC := $(PLUGIN_DIR)/../libtheora/include
VORBIS_INC := $(PLUGIN_DIR)/../libvorbis/include

OGG_SRC := $(PLUGIN_DIR)/../libogg/src
THEORA_SRC := $(PLUGIN_DIR)/../libtheora/lib
VORBIS_SRC := $(PLUGIN_DIR)/../libvorbis/lib

THEORA_FLIST := $(wildcard $(LOCAL_PATH)/$(THEORA_SRC)/*.c)
VORBIS_FLIST := $(wildcard $(LOCAL_PATH)/$(VORBIS_SRC)/*.c)

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
LOCAL_MODULE := libplugin.theora

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/$(SRC_DIR) \
    $(LOCAL_PATH)/$(BR_DIR) \
    $(LOCAL_PATH)/$(OGG_INC) \
    $(LOCAL_PATH)/$(THEORA_INC) \
    $(LOCAL_PATH)/$(VORBIS_INC) \
    $(LOCAL_PATH)/$(VORBIS_SRC)

LOCAL_SRC_FILES := \
	$(SRC_DIR)/plugin.theora.cpp $(SRC_DIR)/theoraplay.c $(SRC_DIR)/video_encoder.cpp \
    $(BR_DIR)/ByteReader.cpp $(OGG_SRC)/bitwise.c $(OGG_SRC)/framing.c \
    $(VORBIS_FLIST:$(LOCAL_PATH)/%=%) $(THEORA_FLIST:$(LOCAL_PATH)/%=%)

LOCAL_CFLAGS := \
	-DANDROID_NDK \
	-DNDEBUG \
	-D_REENTRANT \
	-DRtt_ANDROID_ENV

LOCAL_LDLIBS := -llog

LOCAL_SHARED_LIBRARIES := \
	liblua libcorona

ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS+= -D_ARM_ASSEM_ -D_M_ARM
endif

LOCAL_CPPFLAGS += -std=c++11
LOCAL_CPP_FEATURES += exceptions

# Arm vs Thumb.
LOCAL_ARM_MODE := arm
include $(BUILD_SHARED_LIBRARY)
