LOCAL_PATH := $(call my-dir)

MY_ANDROID_SRC_ROOT := $(HOME)/src/android-source

MY_SRCS := mocktime.cc
MY_CFLAGS := -g -ggdb -O0 -Wall -Werror -DANDROID

include $(CLEAR_VARS)
LOCAL_MODULE := libmocktime
LOCAL_CPP_EXTENSION := .cc
LOCAL_C_INCLUDES := $(ANDROID_INCLUDES)
LOCAL_SRC_FILES := $(addprefix ../, $(MY_SRCS))
LOCAL_CFLAGS := $(MY_CFLAGS) 
include $(BUILD_SHARED_LIBRARY)
