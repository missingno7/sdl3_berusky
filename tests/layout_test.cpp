/*
 * Berusky (C) AnakreoN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * Unit test of the render layout (src/render_layout.h): how the logical
 * composition maps to outputs of any size, aspect ratio and pixel density,
 * and back (pointer / touch coordinates). Covers the HiDPI and phone cases
 * that can't be produced with a desktop window.
 *
 *   ctest --test-dir build   (or run berusky_layout_test directly)
 */

#include <stdio.h>
#include <math.h>

#include "render_layout.h"

// Provided by the game's main.cpp; the test has no config file
const char * config_file(bool)
{
  return("");
}

static int failures = 0;

#define CHECK(cond) \
  do { if(!(cond)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); failures++; } } while(0)

static bool near(float a, float b, float eps = 0.01f)
{
  return(fabsf(a - b) <= eps);
}

static RENDER_LAYOUT layout(int ww, int wh, int ow, int oh, int lw, int lh,
                            const RENDER_SETTINGS &s = RENDER_SETTINGS(), int max_texture = 0)
{
  RENDER_LAYOUT l;
  l.compute(ww, wh, ow, oh, lw, lh, s, max_texture);
  return(l);
}

static void rect_is(const SDL_Rect &r, int x, int y, int w, int h, int line)
{
  if(r.x != x || r.y != y || r.w != w || r.h != h) {
    printf("FAIL line %d: viewport %d,%d %dx%d, expected %d,%d %dx%d\n", line, r.x, r.y, r.w, r.h, x, y, w, h);
    failures++;
  }
}

int main(void)
{
  // The classic window: 1:1
  {
    RENDER_LAYOUT l = layout(640, 480, 640, 480, 640, 480);
    rect_is(l.viewport, 0, 0, 640, 480, __LINE__);
    CHECK(near(l.render_scale, 1.0f));
    CHECK(l.target_w == 640 && l.target_h == 480);
  }

  // 16:9 full HD: pillarbox, the scene is rendered at 1440x1080 (not 640x480)
  {
    RENDER_LAYOUT l = layout(1920, 1080, 1920, 1080, 640, 480);
    rect_is(l.viewport, 240, 0, 1440, 1080, __LINE__);
    CHECK(near(l.render_scale, 2.25f));
    CHECK(l.target_w == 1440 && l.target_h == 1080);
    CHECK(near(l.output_aspect, 16.0f/9.0f));
  }

  // 2560x1440: 1920x1440 scene
  {
    RENDER_LAYOUT l = layout(2560, 1440, 2560, 1440, 640, 480);
    rect_is(l.viewport, 320, 0, 1920, 1440, __LINE__);
    CHECK(l.target_w == 1920 && l.target_h == 1440);
  }

  // Ultrawide 21:9 and 32:9
  {
    RENDER_LAYOUT l = layout(3440, 1440, 3440, 1440, 640, 480);
    rect_is(l.viewport, 760, 0, 1920, 1440, __LINE__);
    l = layout(5120, 1440, 5120, 1440, 640, 480);
    rect_is(l.viewport, 1600, 0, 1920, 1440, __LINE__);
  }

  // Portrait / tall: letterbox
  {
    RENDER_LAYOUT l = layout(1080, 1920, 1080, 1920, 640, 480);
    rect_is(l.viewport, 0, 555, 1080, 810, __LINE__);
    CHECK(near(l.render_scale, 1.6875f));
  }

  // 16:10
  {
    RENDER_LAYOUT l = layout(1920, 1200, 1920, 1200, 640, 480);
    rect_is(l.viewport, 160, 0, 1600, 1200, __LINE__);
    CHECK(near(l.render_scale, 2.5f));
  }

  // 4K
  {
    RENDER_LAYOUT l = layout(3840, 2160, 3840, 2160, 640, 480);
    rect_is(l.viewport, 480, 0, 2880, 2160, __LINE__);
    CHECK(near(l.render_scale, 4.5f));
  }

  // Small window: rendered below 1x
  {
    RENDER_LAYOUT l = layout(500, 400, 500, 400, 640, 480);
    rect_is(l.viewport, 0, 12, 500, 375, __LINE__);
    CHECK(near(l.render_scale, 0.78125f));
  }

  // HiDPI (macOS retina / Windows 200%): window coordinates are half the
  // output pixels, the scene is rendered at the pixel resolution
  {
    RENDER_LAYOUT l = layout(1280, 720, 2560, 1440, 640, 480);
    CHECK(near(l.pixel_density, 2.0f));
    rect_is(l.viewport, 320, 0, 1920, 1440, __LINE__);
    CHECK(near(l.render_scale, 3.0f));

    float lx, ly, wx, wy;
    l.window_to_logical(640, 360, &lx, &ly);            // window center
    CHECK(near(lx, 320.0f) && near(ly, 240.0f));
    l.window_to_logical(160, 0, &lx, &ly);              // top left of the picture
    CHECK(near(lx, 0.0f) && near(ly, 0.0f));
    l.window_to_logical(100, 100, &lx, &ly);            // in the left bar
    CHECK(lx < 0);
    l.logical_to_window(640, 480, &wx, &wy);            // bottom right
    CHECK(near(wx, 1120.0f) && near(wy, 720.0f));
  }

  // Android phone, landscape: 2400x1080 pixels, density 2.625
  {
    RENDER_LAYOUT l = layout(914, 411, 2400, 1080, 640, 480);
    rect_is(l.viewport, 480, 0, 1440, 1080, __LINE__);
    CHECK(near(l.render_scale, 2.25f));
    CHECK(near(l.pixel_density, 2400.0f / 914.0f));
    // A touch at the logical center lands at the window center
    float wx, wy, lx, ly;
    l.logical_to_window(320, 240, &wx, &wy);
    CHECK(near(wx, 457.0f, 0.5f) && near(wy, 205.5f, 0.5f));
    l.window_to_logical(wx, wy, &lx, &ly);
    CHECK(near(lx, 320.0f) && near(ly, 240.0f));
    // The free space on both sides (touch controls) is 480 pixels wide
    CHECK(l.viewport.x == 480 && l.output_w - (l.viewport.x + l.viewport.w) == 480);
  }

  // Integer presentation: whole multiples only
  {
    RENDER_SETTINGS s;
    s.presentation = PRESENT_INTEGER;
    RENDER_LAYOUT l = layout(1920, 1080, 1920, 1080, 640, 480, s);
    rect_is(l.viewport, 320, 60, 1280, 960, __LINE__);
    CHECK(near(l.render_scale, 2.0f));
    // smaller than 1x: fits anyway
    l = layout(500, 400, 500, 400, 640, 480, s);
    CHECK(l.viewport.w == 500);
  }

  // Integer render resolution: rendered at 2x, filtered to 2.25x
  {
    RENDER_SETTINGS s;
    s.render_resolution = RENDER_RES_INTEGER;
    RENDER_LAYOUT l = layout(1920, 1080, 1920, 1080, 640, 480, s);
    rect_is(l.viewport, 240, 0, 1440, 1080, __LINE__);
    CHECK(near(l.render_scale, 2.0f));
    CHECK(l.target_w == 1280 && l.target_h == 960);
  }

  // Fixed render scale 1: the classic 640x480 picture, upscaled
  {
    RENDER_SETTINGS s;
    s.render_resolution = RENDER_RES_FIXED;
    s.render_scale = 1.0f;
    RENDER_LAYOUT l = layout(1920, 1080, 1920, 1080, 640, 480, s);
    CHECK(l.target_w == 640 && l.target_h == 480);
    CHECK(l.viewport.w == 1440);
  }

  // The render target never exceeds the GPU's texture limit
  {
    RENDER_SETTINGS s;
    s.render_resolution = RENDER_RES_FIXED;
    s.render_scale = 16.0f;
    RENDER_LAYOUT l = layout(1920, 1080, 1920, 1080, 640, 480, s, 4096);
    CHECK(l.target_w <= 4096 && l.target_h <= 4096);
  }

  // The editor's composition (1280x900)
  {
    RENDER_LAYOUT l = layout(1920, 1200, 1920, 1200, 1280, 900);
    CHECK(near(l.render_scale, 1.3333f, 0.001f));
    CHECK(l.viewport.h == 1200 && l.viewport.w == 1707);
  }

  // Settings parsing
  {
    RENDER_SETTINGS s;
    CHECK(render_settings_set(&s, "asset_scaler", "scale2x") && s.asset_scaler == SCALER_SCALE2X);
    CHECK(render_settings_set(&s, "render_resolution", "2.5") && s.render_resolution == RENDER_RES_FIXED && near(s.render_scale, 2.5f));
    CHECK(render_settings_set(&s, "presentation", "integer") && s.presentation == PRESENT_INTEGER);
    CHECK(render_settings_set(&s, "presentation_filter", "linear") && s.presentation_filter == FILTER_LINEAR);
    CHECK(!render_settings_set(&s, "asset_scaler", "bogus"));
    CHECK(!render_settings_set(&s, "unknown", "1"));
    CHECK(asset_scaler_effective(SCALER_XBRZ) == SCALER_PIXELART);
    CHECK(asset_scaler_is_cpu(SCALER_LEGACY2X) && !asset_scaler_is_cpu(SCALER_PIXELART));
  }

  if(failures)
    printf("%d failure(s)\n", failures);
  else
    printf("render layout: all checks passed\n");
  return(failures ? 1 : 0);
}
