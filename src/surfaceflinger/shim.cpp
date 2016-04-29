// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "eglplatform_shim.h"

#include <cstdio>
#include <map>

#include <binder/ProcessState.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/Surface.h>
#include <ui/PixelFormat.h>

//#define WIDTH_OVERRIDE 1440
//#define HEIGHT_OVERRIDE 2560

#define LOG(...)
#ifndef LOG
#define LOG(...)                                                               \
  if (kTrace)                                                                  \
  fprintf(stderr, __VA_ARGS__)
#endif

static const bool kTrace = true;

static const int kSurfaceIdBase = 500000; // On top of most android windows

class ShimSurface {
public:
  ShimSurface(android::sp<android::SurfaceControl> surface_control,
              uint32_t width, uint32_t height)
      : surface_control_(surface_control), width_(width), height_(height) {}

  uint32_t getWidth() { return width_; }

  uint32_t getHeight() { return height_; }

  ANativeWindow *getNativeWindow() {
    return surface_control_->getSurface().get();
  }

private:
  android::sp<android::SurfaceControl> surface_control_;
  uint32_t width_;
  uint32_t height_;
};

class Shim {
public:
  Shim() {}
  virtual ~Shim() {}

  bool init();
  int createSurface(int32_t left, int32_t top, uint32_t width, uint32_t height);
  ShimSurface *getSurface(int id);

private:
  android::sp<android::SurfaceComposerClient> composer_client_{};
  std::map<int, ShimSurface *> surface_map_{};
  int surface_id_ = kSurfaceIdBase;
};

bool Shim::init() {
  composer_client_ = new android::SurfaceComposerClient;

  LOG("Inc ref on composerClient\n");

  // Required to pass initCheck
  composer_client_->incStrong(nullptr);

  android::status_t status;

  LOG("calling initCheck\n");

  status = composer_client_->initCheck();
  if (status != android::NO_ERROR) {
    fprintf(stderr, "failed composer init check %d\n", status);
    return false;
  }

  return true;
}

int Shim::createSurface(int32_t left, int32_t top, uint32_t width,
                        uint32_t height) {
  uint32_t flags = 0;

#ifdef WIDTH_OVERRIDE
  width = WIDTH_OVERRIDE;
#endif
#ifdef HEIGHT_OVERRIDE
  height = HEIGHT_OVERRIDE;
#endif

  auto surface_control =
      composer_client_->createSurface(android::String8("ozone"), width, height,
                                      android::PIXEL_FORMAT_BGRA_8888, flags);

  int id = ++surface_id_;

  android::SurfaceComposerClient::openGlobalTransaction();
  surface_control->setLayer(id);
  surface_control->setPosition(left, top);
  android::SurfaceComposerClient::closeGlobalTransaction();

  surface_map_.insert(std::pair<int, ShimSurface *>(
      id, new ShimSurface(surface_control, width, height)));

  return id;
}

ShimSurface *Shim::getSurface(int id) { return surface_map_.at(id); }

//////////////////////////////////////////////////////////////////////////////

static Shim *the_shim;

const char *ShimQueryString(int name) {
  LOG("ShimQueryString: %d\n", name);
  if (name == SHIM_EGL_LIBRARY)
    return "libEGL.so";
  if (name == SHIM_GLES_LIBRARY)
    return "libGLESv2.so";
  LOG("Unhandled string query\n");
  return nullptr;
}

bool ShimInitialize(void) {
  LOG("ShimInitialize\n");
  // TODO: necessary?
  android::ProcessState::self()->startThreadPool();

  if (!the_shim) {
    the_shim = new Shim();
    if (!the_shim->init()) {
      ShimTerminate();
      return false;
    }
  }
  return true;
}

bool ShimTerminate(void) {
  LOG("ShimTerminate\n");
  if (the_shim) {
    delete the_shim;
    the_shim = nullptr;
  }
  return true;
}

ShimEGLNativeDisplayType ShimGetNativeDisplay(void) {
  LOG("ShimGetNativeDisplay\n");
  return 0;
}

ShimNativeWindowId ShimCreateWindow(int32_t left, int32_t top, uint32_t width,
                                    uint32_t height) {
  LOG("ShimCreateWindow %dx%d size %dx%d", left, top, width, height);
  int id = the_shim->createSurface(left, top, width, height);
  LOG("Returning window_id %d\n", id);
  return id;
}

bool ShimQueryWindow(ShimNativeWindowId window_id, int attribute, int *value) {
  LOG("ShimQueryWindow %ld %d\n", window_id, attribute);
  auto surface = the_shim->getSurface(static_cast<int>(window_id));

  switch (attribute) {
  case SHIM_WINDOW_WIDTH:
    *value = surface->getWidth();
    break;
  case SHIM_WINDOW_HEIGHT:
    *value = surface->getHeight();
    break;
  default:
    fprintf(stderr, "Unhandled attribute");
    return false;
  }

  return true;
}

ShimEGLNativeWindowType ShimGetNativeWindow(ShimNativeWindowId window_id) {
  auto surface = the_shim->getSurface(window_id);
  LOG("ShimGetNativeWindow %ld (surface %p)\n", window_id, surface);
  ANativeWindow *window = surface ? surface->getNativeWindow() : nullptr;
  return reinterpret_cast<ShimEGLNativeWindowType>(window);
}

bool ShimDestroyWindow(ShimNativeWindowId window_id) {
  LOG("ShimDestroyWindow %ld\n", window_id);
  // TODO
  return true;
}

bool ShimReleaseNativeWindow(ShimEGLNativeWindowType native_window) {
  LOG("ShimReleaseNativeWindow 0x%lx\n", native_window);
  // TODO
  return true;
}
