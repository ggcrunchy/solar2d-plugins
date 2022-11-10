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
MATH_LIBRARIES_DIR := $(PLUGIN_DIR)/../math_libraries
NE10_DIR := $(MATH_LIBRARIES_DIR)/projectNe10

SRC_DIR := $(PLUGIN_DIR)/shared
BR_DIR := $(PLUGIN_DIR)/../ByteReader
CEU_DIR := $(PLUGIN_DIR)/../solar2d_native_utils
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

NE10_MODULES := $(NE10_DIR)/modules

ne10_neon_source := \
    $(NE10_MODULES)/imgproc/NE10_boxfilter.neon.c \
    $(NE10_MODULES)/imgproc/NE10_resize.neon.c

ne10_source_files := \
    $(NE10_MODULES)/imgproc/NE10_boxfilter.c \
    $(NE10_MODULES)/imgproc/NE10_resize.c \
    $(NE10_MODULES)/imgproc/NE10_rotate.c \
    $(NE10_DIR)/common/NE10_mask_table.c \

# -----------------------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_MODULE := libplugin.impack

LOCAL_C_INCLUDES := \
	$(SRC_DIR) $(BR_DIR) $(CEU_DIR) $(NE10_DIR)/common $(NE10_DIR)/inc $(MATH_LIBRARIES_DIR)

LOCAL_SRC_FILES := \
	$(SRC_DIR)/plugin.impack.cpp \
	$(SRC_DIR)/image.cpp $(SRC_DIR)/image_resize.cpp $(SRC_DIR)/image_write.cpp $(SRC_DIR)/image_ops.cpp $(SRC_DIR)/image_utils.cpp \
	$(SRC_DIR)/spotc.c $(SRC_DIR)/spot.cpp $(SRC_DIR)/SpotInterface.cpp \
	$(SRC_DIR)/grayscale.cpp \
	$(SRC_DIR)/jo_file.cpp $(SRC_DIR)/jo_gif.cpp $(SRC_DIR)/jo_mpeg.cpp \
	$(SRC_DIR)/jo_jpeg.cpp \
	$(BR_DIR)/ByteReader.cpp \
	$(CEU_SRC)/Blob.cpp $(CEU_SRC)/Byte.cpp $(CEU_SRC)/LuaEx.cpp $(CEU_SRC)/Memory.cpp $(CEU_SRC)/Path.cpp \
	$(CEU_SRC)/SIMD.cpp $(CEU_SRC)/Thread.cpp \
	$(ne10_source_files)

IS_ARM=

ifeq ($(TARGET_ARCH),arm)
	IS_ARM = 1
endif

ifeq ($(TARGET_ARCH),arm64)
	IS_ARM = 1
endif

ifeq ($(IS_ARM), 1)
LOCAL_SRC_FILES += $(ne10_neon_source)
endif

ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS += -DENABLE_NE10_IMG_ROTATE_RGBA_NEON
LOCAL_SRC_FILES += $(NE10_MODULES)/imgproc/NE10_rotate.neon.s
endif

LOCAL_CFLAGS := \
	-DANDROID_NDK \
	-DNDEBUG \
	-D_REENTRANT \
	-DRtt_ANDROID_ENV

LOCAL_LDLIBS := -llog -lm -lGLESv2

LOCAL_CFLAGS += -fno-integrated-as # -fopenmp
#LOCAL_LDFLAGS += -fopenmp

LOCAL_SHARED_LIBRARIES := \
	liblua libcorona

ifeq ($(TARGET_ARCH), arm)
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
