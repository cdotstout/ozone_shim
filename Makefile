# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

ANDROID_ROOT := $(HOME)/projects/android/android-6.0.1_r30
TOOLCHAIN_PREFIX := $(HOME)/projects/toolchains-android/bin/aarch64-linux-android-
CC := $(TOOLCHAIN_PREFIX)clang
LIBDIR := $(ANDROID_ROOT)/out/target/product/angler/system/lib64

LOCAL_SRCS := src/surfaceflinger/shim.cpp

LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -std=c++11 -Wno-inconsistent-missing-override

LOCAL_INCLUDES := \
	-Iinclude \
	-I$(ANDROID_ROOT)/frameworks/native/opengl/include \
	-I$(ANDROID_ROOT)/frameworks/native/include \
	-I$(ANDROID_ROOT)/system/core/include \
	-I$(ANDROID_ROOT)/hardware/libhardware/include

LOCAL_SHARED_LIBRARIES := \
	-lgui \
	-lbinder \
	-lutils \
	-llog \
	-lc++

LDFLAGS := -L$(LIBDIR) -Wl,-rpath=$(LIBDIR)

libeglplatform_shim.so: $(LOCAL_SRCS)
	$(CC) -shared -o libeglplatform_shim.so $(LOCAL_SRCS) $(LOCAL_CFLAGS) $(LOCAL_INCLUDES) $(LDFLAGS) $(LOCAL_SHARED_LIBRARIES)
