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
BR_DIR := $(PLUGIN_DIR)/../ByteReader
SNU_DIR := $(PLUGIN_DIR)/../solar2d_native_utils
SNU_SRC := $(SNU_DIR)/utils

AS_DIR := $(SRC_DIR)/src/audiosource
BE_DIR := $(SRC_DIR)/src/backend
CORE_DIR := $(SRC_DIR)/src/core
F_DIR := $(SRC_DIR)/src/filter

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
LOCAL_MODULE := libplugin.soloud

LOCAL_C_INCLUDES := \
	$(SRC_DIR) $(SRC_DIR)/include $(BR_DIR) $(SNU_DIR)

LOCAL_SRC_FILES := \
	$(SRC_DIR)/plugin.soloud.cpp  \
	$(BR_DIR)/ByteReader.cpp \
	$(SNU_SRC)/LuaEx.cpp $(SNU_SRC)/Path.cpp \
	$(SRC_DIR)/marshal.c $(SRC_DIR)/add_audiosources.cpp $(SRC_DIR)/add_core.cpp \
	$(SRC_DIR)/add_filters.cpp $(SRC_DIR)/add_floatbuffer.cpp \
	$(SRC_DIR)/custom_audiosource.cpp \
	$(SRC_DIR)/custom_audiosource_instance.cpp \
	$(SRC_DIR)/custom_filter.cpp \
	$(SRC_DIR)/custom_filter_instance.cpp \
	$(SRC_DIR)/custom_object_data.cpp $(SRC_DIR)/custom_objects.cpp \
	$(AS_DIR)/ay/chipplayer.cpp $(AS_DIR)/ay/sndbuffer.cpp \
	$(AS_DIR)/ay/sndchip.cpp $(AS_DIR)/ay/sndrender.cpp $(AS_DIR)/ay/soloud_ay.cpp \
	$(AS_DIR)/monotone/soloud_monotone.cpp $(AS_DIR)/noise/soloud_noise.cpp \
	$(AS_DIR)/openmpt/soloud_openmpt.cpp $(AS_DIR)/openmpt/soloud_openmpt_dll.c \
	$(AS_DIR)/sfxr/soloud_sfxr.cpp $(AS_DIR)/speech/darray.cpp \
	$(AS_DIR)/speech/klatt.cpp $(AS_DIR)/speech/resonator.cpp \
	$(AS_DIR)/speech/soloud_speech.cpp $(AS_DIR)/speech/tts.cpp \
	$(AS_DIR)/tedsid/sid.cpp $(AS_DIR)/tedsid/soloud_tedsid.cpp \
	$(AS_DIR)/tedsid/ted.cpp $(AS_DIR)/vic/soloud_vic.cpp \
	$(AS_DIR)/vizsn/soloud_vizsn.cpp $(AS_DIR)/wav/dr_impl.cpp \
	$(AS_DIR)/wav/soloud_wav.cpp $(AS_DIR)/wav/soloud_wavstream.cpp \
	$(AS_DIR)/wav/stb_vorbis.c \
	$(BE_DIR)/miniaudio/soloud_miniaudio.cpp $(BE_DIR)/null/soloud_null.cpp \
	$(BE_DIR)/nosound/soloud_nosound.cpp $(BE_DIR)/opensles/soloud_opensles.cpp \
	$(CORE_DIR)/soloud.cpp $(CORE_DIR)/soloud_audiosource.cpp \
	$(CORE_DIR)/soloud_bus.cpp $(CORE_DIR)/soloud_core_3d.cpp \
	$(CORE_DIR)/soloud_core_basicops.cpp $(CORE_DIR)/soloud_core_faderops.cpp \
	$(CORE_DIR)/soloud_core_filterops.cpp $(CORE_DIR)/soloud_core_getters.cpp \
	$(CORE_DIR)/soloud_core_setters.cpp $(CORE_DIR)/soloud_core_voicegroup.cpp \
	$(CORE_DIR)/soloud_core_voiceops.cpp $(CORE_DIR)/soloud_fader.cpp \
	$(CORE_DIR)/soloud_fft.cpp $(CORE_DIR)/soloud_fft_lut.cpp \
	$(CORE_DIR)/soloud_file.cpp $(CORE_DIR)/soloud_filter.cpp \
	$(CORE_DIR)/soloud_misc.cpp $(CORE_DIR)/soloud_queue.cpp \
	$(CORE_DIR)/soloud_thread.cpp \
	$(F_DIR)/soloud_bassboostfilter.cpp $(F_DIR)/soloud_biquadresonantfilter.cpp \
	$(F_DIR)/soloud_dcremovalfilter.cpp $(F_DIR)/soloud_duckfilter.cpp \
	$(F_DIR)/soloud_echofilter.cpp $(F_DIR)/soloud_eqfilter.cpp \
	$(F_DIR)/soloud_fftfilter.cpp $(F_DIR)/soloud_flangerfilter.cpp \
	$(F_DIR)/soloud_freeverbfilter.cpp $(F_DIR)/soloud_lofifilter.cpp \
	$(F_DIR)/soloud_robotizefilter.cpp $(F_DIR)/soloud_waveshaperfilter.cpp
	

LOCAL_CFLAGS := \
	-DANDROID_NDK \
	-DNDEBUG \
	-D_REENTRANT \
	-DRtt_ANDROID_ENV \
	-DWITH_MINIAUDIO -DWITH_NOSOUND -DWITH_NULL -DWITH_OPENSLES

LOCAL_LDLIBS := -llog -lOpenSLES

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
