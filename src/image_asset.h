/*
 * Berusky (C) AnakreoN
 * Martin Stransky <stransky@anakreon.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * Image assets with their own native pixel density.
 *
 * An image_asset is the source (CPU) representation of a loaded picture:
 *
 *   base variant   the PNG of the game data, e.g. klasik1.png (20x20 per cell,
 *                  density 1) or wall_wood1.png (40x40 per cell, density 2)
 *   HD variants    optional higher resolution copies of the same picture,
 *                  name@Nx.png (N = density), from the graphics directory or
 *                  an HD pack directory. Same layout, N/base times the pixels.
 *
 *   density        source pixels per logical unit. A level cell is 20 units,
 *                  so a 20x20 sprite has density 1 and a 40x40 one density 2:
 *                  both are ONE cell. Nothing is rescaled when it's loaded.
 *   cell zoom      logical units per native unit. 1 in the game; the editor
 *                  lays its level out in 40 unit cells, so cell art is
 *                  registered with zoom 2 there (UI art never is).
 *
 * Sprites refer to rectangles of an asset (IMAGE_REGION). A region is drawn as
 * one piece: its GPU texture is made from the best variant for the current
 * render scale (the lowest density that's not below it, or the highest one),
 * optionally pre-scaled by a CPU scaler, and cached by the scene renderer.
 * Filtering therefore never bleeds pixels of neighbour sprites of a sheet.
 *
 * Game code keeps using the old color key semantics: the source surfaces are
 * color keyed XRGB8888 (HD variants may have a real alpha channel); regions
 * are converted to premultiplied ARGB when a texture is made.
 */

#ifndef __IMAGE_ASSET_H__
#define __IMAGE_ASSET_H__

#include <SDL3/SDL.h>
#include <unordered_map>

#include "render_layout.h"

#define IMAGE_VARIANTS_MAX  6
#define IMAGE_NAME_MAX      64

class image_asset;

typedef struct image_region {

  image_asset *p_asset;
  SDL_Rect     rect;              // base variant pixels
  int          opaque;            // no transparent pixel (-1 = not checked yet)

  bool is_opaque(void);

} IMAGE_REGION;

typedef struct image_variant {

  SDL_Surface *p_surf;
  float        density;           // source pixels per logical unit (cell zoom 1)
  bool         alpha;             // real alpha channel instead of the color key

} IMAGE_VARIANT;

// A region is going away - texture caches have to drop it
typedef void (*IMAGE_REGION_RELEASE)(IMAGE_REGION *p_region);

class image_asset {

  int              refs;
  char             name[IMAGE_NAME_MAX];

  IMAGE_VARIANT    variants[IMAGE_VARIANTS_MAX];    // sorted by density
  int              variant_num;

  float            zoom;          // cell zoom (logical units per native unit)
  Uint8            alpha_mod;     // 255 = none
  bool             key_enabled;
  Uint32           key_rgb;       // 0xRRGGBB
  unsigned         version;       // changed pixels -> new textures

  std::unordered_map<Uint64, IMAGE_REGION *> regions;

  static const char          *hd_dir;
  static IMAGE_REGION_RELEASE region_release;

private:

  image_asset(void);
  ~image_asset(void);

  void variants_find(const char *p_dir, const char *p_file);
  void regions_drop(void);

public:

  // HD pack directory searched for name@Nx.png before the graphics directory
  static void hd_dir_set(const char *p_dir);
  static void region_release_set(IMAGE_REGION_RELEASE callback)
  {
    region_release = callback;
  }

  // Loads p_dir/p_file (and its HD variants) with the given native density
  static image_asset * load(const char *p_dir, const char *p_file, float density, float zoom);
  // A copy of a part (base pixels, NULL = whole) of another asset, all variants
  static image_asset * copy(image_asset *p_src, const SDL_Rect *p_rect);

  void ref(void)
  {
    refs++;
  }
  void unref(void);

  const char * name_get(void) const
  {
    return(name);
  }

  // Base variant
  int pixel_width(void) const
  {
    return(variants[0].p_surf->w);
  }
  int pixel_height(void) const
  {
    return(variants[0].p_surf->h);
  }

  float density_get(void) const
  {
    return(variants[0].density);
  }
  float zoom_get(void) const
  {
    return(zoom);
  }
  // Base pixels per logical unit
  float px_per_unit(void) const
  {
    return(variants[0].density / zoom);
  }

  int variant_num_get(void) const
  {
    return(variant_num);
  }
  const IMAGE_VARIANT * variant_get(int i) const
  {
    return(variants + i);
  }
  // The variant to draw at the given render scale (render pixels per unit)
  int variant_select(float render_scale) const;

  unsigned version_get(void) const
  {
    return(version);
  }

  void colorkey_set(Uint8 r, Uint8 g, Uint8 b);
  bool colorkey_get(Uint32 *p_rgb) const
  {
    if(p_rgb)
      *p_rgb = key_rgb;
    return(key_enabled);
  }

  void alpha_mod_set(Uint8 alpha)
  {
    alpha_mod = alpha;
  }
  Uint8 alpha_mod_get(void) const
  {
    return(alpha_mod);
  }

  // The region for a rectangle of base pixels (made on the first use)
  IMAGE_REGION * region_get(const SDL_Rect &rect);

  // CPU pixel operation of the game (graphics_generate): the rectangle is in
  // base pixels and it's applied to every variant. Transparent (color key)
  // pixels are left alone. p_pixel(r, g, b, data) changes one pixel.
  typedef void (*PIXEL_FUNC)(int *p_r, int *p_g, int *p_b, void *p_data, bool base);
  void pixels_modify(const SDL_Rect &rect, PIXEL_FUNC p_func, void *p_data);

  // Premultiplied ARGB8888 pixels of a region taken from a variant and
  // enlarged by a CPU scaler (factor 1 = as is). The caller frees it.
  SDL_Surface * region_pixels(const IMAGE_REGION *p_region, int variant,
                              ASSET_SCALER scaler, int factor);
};

// CPU asset scalers (see asset_scaler_is_cpu()). They work on premultiplied
// ARGB8888 surfaces with a binary alpha channel.
int           cpu_scaler_factor(ASSET_SCALER scaler, float magnification);
SDL_Surface * cpu_scaler_run(ASSET_SCALER scaler, SDL_Surface *p_src, int factor);

#endif // __IMAGE_ASSET_H__
