/*
 * Berusky (C) AnakreoN
 * Martin Stransky <stransky@anakreon.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* Image assets with native pixel density - see image_asset.h */

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <math.h>
#include <string.h>

#include "image_asset.h"
#include "utils.h"

// Source surfaces of the game: color keyed, no alpha (as SDL_DisplayFormat was)
#define SOURCE_FORMAT   SDL_PIXELFORMAT_XRGB8888
// HD variants with a real alpha channel, and all region textures
#define ALPHA_FORMAT    SDL_PIXELFORMAT_ARGB8888

const char           *image_asset::hd_dir = NULL;
IMAGE_REGION_RELEASE  image_asset::region_release = NULL;

static inline Uint64 region_key(const SDL_Rect &r)
{
  return(((Uint64)(Uint16)r.x) | ((Uint64)(Uint16)r.y << 16) |
         ((Uint64)(Uint16)r.w << 32) | ((Uint64)(Uint16)r.h << 48));
}

// -------------------------------------------------------
//   Loading
// -------------------------------------------------------

// Reads a PNG through the platform file layer (packaged assets on Android)
static SDL_Surface * image_file_load(const char *p_dir, const char *p_file, bool keep_alpha, bool *p_alpha)
{
  t_off  size = 0;
  void  *p_data = file_load(p_dir, p_file, &size, 0, FALSE);
  if(!p_data)
    return(NULL);

  SDL_Surface  *p_tmp = NULL;
  SDL_IOStream *p_io = SDL_IOFromConstMem(p_data, size);
  if(p_io)
    p_tmp = IMG_Load_IO(p_io, true);
  ::free(p_data);

  if(!p_tmp)
    return(NULL);

  bool alpha = keep_alpha && SDL_ISPIXELFORMAT_ALPHA(p_tmp->format);
  SDL_Surface *p_surf = SDL_ConvertSurface(p_tmp, alpha ? ALPHA_FORMAT : SOURCE_FORMAT);
  SDL_DestroySurface(p_tmp);
  if(p_alpha)
    *p_alpha = alpha;
  return(p_surf);
}

void image_asset::hd_dir_set(const char *p_dir)
{
  static char dir[MAX_FILENAME];
  if(p_dir && p_dir[0]) {
    strncpy(dir, p_dir, sizeof(dir)-1);
    dir[sizeof(dir)-1] = '\0';
    hd_dir = dir;
  } else {
    hd_dir = NULL;
  }
}

image_asset::image_asset(void)
: refs(1), variant_num(0), zoom(1.0f), alpha_mod(255), key_enabled(false), key_rgb(0), version(0)
{
  name[0] = '\0';
  memset(variants, 0, sizeof(variants));
}

image_asset::~image_asset(void)
{
  regions_drop();
  for(int i = 0; i < variant_num; i++)
    SDL_DestroySurface(variants[i].p_surf);
}

void image_asset::regions_drop(void)
{
  for(auto &it : regions) {
    if(region_release)
      region_release(it.second);
    delete it.second;
  }
  regions.clear();
}

void image_asset::unref(void)
{
  assert(refs > 0);
  if(--refs == 0)
    delete this;
}

// name@Nx.png, N > base density, in the HD pack directory or next to the base
void image_asset::variants_find(const char *p_dir, const char *p_file)
{
  static const int densities[] = { 2, 3, 4, 6, 8 };

  char base[MAX_FILENAME];
  strncpy(base, p_file, sizeof(base)-1);
  base[sizeof(base)-1] = '\0';
  char *p_ext = strrchr(base, '.');
  const char *p_suffix = p_ext ? p_file + (p_ext - base) : "";
  if(p_ext)
    *p_ext = '\0';

  const float base_density = variants[0].density;
  const int   base_w = variants[0].p_surf->w, base_h = variants[0].p_surf->h;

  for(size_t i = 0; i < sizeof(densities)/sizeof(densities[0]) && variant_num < IMAGE_VARIANTS_MAX; i++) {
    const float density = (float)densities[i];
    if(density <= base_density)
      continue;

    char file[MAX_FILENAME];
    snprintf(file, sizeof(file), "%s@%dx%s", base, densities[i], p_suffix);

    const char *p_dirs[2] = { hd_dir, p_dir };
    for(int d = 0; d < 2; d++) {
      if(!p_dirs[d] || !file_exists(p_dirs[d], file))
        continue;

      bool alpha = false;
      SDL_Surface *p_surf = image_file_load(p_dirs[d], file, true, &alpha);
      if(!p_surf)
        continue;

      const float ratio = density / base_density;
      if(p_surf->w != (int)lroundf(base_w * ratio) || p_surf->h != (int)lroundf(base_h * ratio)) {
        bprintf("HD variant %s/%s: %dx%d, expected %dx%d - ignored", p_dirs[d], file,
                p_surf->w, p_surf->h, (int)lroundf(base_w * ratio), (int)lroundf(base_h * ratio));
        SDL_DestroySurface(p_surf);
        continue;
      }

      IMAGE_VARIANT *p_var = variants + variant_num++;
      p_var->p_surf = p_surf;
      p_var->density = density;
      p_var->alpha = alpha;
      bprintf("HD variant %s/%s (%gx)", p_dirs[d], file, density);
      break;
    }
  }
}

image_asset * image_asset::load(const char *p_dir, const char *p_file, float density, float zoom)
{
  SDL_Surface *p_surf = image_file_load(p_dir, p_file, false, NULL);
  if(!p_surf) {
    bprintf("Unable to load %s/%s: %s", p_dir ? p_dir : "", p_file, SDL_GetError());
    return(NULL);
  }

  image_asset *p_asset = new image_asset;
  strncpy(p_asset->name, p_file, IMAGE_NAME_MAX-1);
  p_asset->name[IMAGE_NAME_MAX-1] = '\0';
  p_asset->zoom = zoom > 0 ? zoom : 1.0f;
  p_asset->variants[0].p_surf = p_surf;
  p_asset->variants[0].density = density > 0 ? density : 1.0f;
  p_asset->variants[0].alpha = false;
  p_asset->variant_num = 1;

  p_asset->variants_find(p_dir, p_file);
  return(p_asset);
}

image_asset * image_asset::copy(image_asset *p_src, const SDL_Rect *p_rect)
{
  SDL_Rect whole = { 0, 0, p_src->pixel_width(), p_src->pixel_height() };
  SDL_Rect rect;
  if(!p_rect || !SDL_GetRectIntersection(p_rect, &whole, &rect))
    rect = whole;

  image_asset *p_asset = new image_asset;
  snprintf(p_asset->name, IMAGE_NAME_MAX, "%.40s[%d,%d,%d,%d]", p_src->name, rect.x, rect.y, rect.w, rect.h);
  p_asset->zoom = p_src->zoom;
  p_asset->alpha_mod = p_src->alpha_mod;
  p_asset->key_enabled = p_src->key_enabled;
  p_asset->key_rgb = p_src->key_rgb;

  for(int i = 0; i < p_src->variant_num; i++) {
    const IMAGE_VARIANT *p_var = p_src->variants + i;
    const float ratio = p_var->density / p_src->variants[0].density;
    SDL_Rect r = { (int)lroundf(rect.x * ratio), (int)lroundf(rect.y * ratio),
                   (int)lroundf(rect.w * ratio), (int)lroundf(rect.h * ratio) };

    SDL_Surface *p_surf = SDL_CreateSurface(r.w, r.h, p_var->p_surf->format);
    if(!p_surf) {
      berror("Unable to create surface! (%dx%d)", r.w, r.h);
    }
    // plain copy, no color key / blending
    SDL_SetSurfaceBlendMode(p_var->p_surf, SDL_BLENDMODE_NONE);
    SDL_Surface *p_from = p_var->p_surf;
    Uint32 key = 0;
    bool   keyed = SDL_GetSurfaceColorKey(p_from, &key);
    if(keyed)
      SDL_SetSurfaceColorKey(p_from, false, 0);
    SDL_BlitSurface(p_from, &r, p_surf, NULL);
    if(keyed) {
      SDL_SetSurfaceColorKey(p_from, true, key);
      SDL_SetSurfaceColorKey(p_surf, true, key);
    }

    IMAGE_VARIANT *p_dst = p_asset->variants + p_asset->variant_num++;
    *p_dst = *p_var;
    p_dst->p_surf = p_surf;
  }

  return(p_asset);
}

int image_asset::variant_select(float render_scale) const
{
  // The smallest density that needs no magnification, or the biggest one
  for(int i = 0; i < variant_num; i++) {
    if(variants[i].density / zoom >= render_scale - 0.01f)
      return(i);
  }
  return(variant_num - 1);
}

void image_asset::colorkey_set(Uint8 r, Uint8 g, Uint8 b)
{
  Uint32 rgb = ((Uint32)r << 16) | ((Uint32)g << 8) | b;
  if(key_enabled && key_rgb == rgb)
    return;

  key_enabled = true;
  key_rgb = rgb;
  for(int i = 0; i < variant_num; i++) {
    if(!variants[i].alpha)
      SDL_SetSurfaceColorKey(variants[i].p_surf, true, SDL_MapSurfaceRGB(variants[i].p_surf, r, g, b));
  }

  // opaque flags and textures depend on the key (regions stay: recorded
  // draw operations may refer to them)
  for(auto &it : regions)
    it.second->opaque = -1;
  version++;
}

IMAGE_REGION * image_asset::region_get(const SDL_Rect &rect)
{
  Uint64 key = region_key(rect);
  auto it = regions.find(key);
  if(it != regions.end())
    return(it->second);

  IMAGE_REGION *p_region = new IMAGE_REGION;
  p_region->p_asset = this;
  p_region->rect = rect;
  p_region->opaque = -1;
  regions[key] = p_region;
  return(p_region);
}

// -------------------------------------------------------
//   Pixels
// -------------------------------------------------------

static inline Uint32 pixel_get(SDL_Surface *p_surf, int x, int y)
{
  return(*(Uint32 *)((Uint8 *)p_surf->pixels + y * p_surf->pitch + x * 4));
}

static inline void pixel_set(SDL_Surface *p_surf, int x, int y, Uint32 pixel)
{
  *(Uint32 *)((Uint8 *)p_surf->pixels + y * p_surf->pitch + x * 4) = pixel;
}

bool image_region::is_opaque(void)
{
  if(opaque >= 0)
    return(opaque != 0);

  Uint32 key;
  opaque = 1;
  if(p_asset->alpha_mod_get() != 255) {
    opaque = 0;
  } else if(p_asset->colorkey_get(&key)) {
    SDL_Surface *p_surf = p_asset->variant_get(0)->p_surf;
    SDL_Rect whole = { 0, 0, p_surf->w, p_surf->h }, r;
    if(SDL_GetRectIntersection(&rect, &whole, &r)) {
      SDL_LockSurface(p_surf);
      for(int y = r.y; y < r.y + r.h && opaque; y++) {
        for(int x = r.x; x < r.x + r.w; x++) {
          if((pixel_get(p_surf, x, y) & 0xffffff) == key) {
            opaque = 0;
            break;
          }
        }
      }
      SDL_UnlockSurface(p_surf);
    }
  }
  return(opaque != 0);
}

void image_asset::pixels_modify(const SDL_Rect &rect, PIXEL_FUNC p_func, void *p_data)
{
  for(int i = 0; i < variant_num; i++) {
    IMAGE_VARIANT *p_var = variants + i;
    SDL_Surface   *p_surf = p_var->p_surf;
    const float    ratio = p_var->density / variants[0].density;

    SDL_Rect r = { (int)lroundf(rect.x * ratio), (int)lroundf(rect.y * ratio),
                   (int)lroundf(rect.w * ratio), (int)lroundf(rect.h * ratio) };
    SDL_Rect whole = { 0, 0, p_surf->w, p_surf->h };
    if(!SDL_GetRectIntersection(&r, &whole, &r))
      continue;

    SDL_LockSurface(p_surf);
    for(int y = r.y; y < r.y + r.h; y++) {
      for(int x = r.x; x < r.x + r.w; x++) {
        Uint32 pixel = pixel_get(p_surf, x, y);
        Uint32 a = pixel >> 24;
        if(p_var->alpha ? (a == 0) : (key_enabled && (pixel & 0xffffff) == key_rgb))
          continue;

        int red = (pixel >> 16) & 0xff, green = (pixel >> 8) & 0xff, blue = pixel & 0xff;
        p_func(&red, &green, &blue, p_data, i == 0);
        pixel_set(p_surf, x, y, (p_var->alpha ? (a << 24) : 0) |
                                ((Uint32)red << 16) | ((Uint32)green << 8) | (Uint32)blue);
      }
    }
    SDL_UnlockSurface(p_surf);
  }

  for(auto &it : regions)
    it.second->opaque = -1;
  version++;
}

SDL_Surface * image_asset::region_pixels(const IMAGE_REGION *p_region, int variant,
                                         ASSET_SCALER scaler, int factor)
{
  const IMAGE_VARIANT *p_var = variants + variant;
  const float ratio = p_var->density / variants[0].density;
  SDL_Rect r = { (int)lroundf(p_region->rect.x * ratio), (int)lroundf(p_region->rect.y * ratio),
                 (int)lroundf(p_region->rect.w * ratio), (int)lroundf(p_region->rect.h * ratio) };
  SDL_Rect whole = { 0, 0, p_var->p_surf->w, p_var->p_surf->h };
  if(!SDL_GetRectIntersection(&r, &whole, &r))
    return(NULL);

  SDL_Surface *p_out = SDL_CreateSurface(r.w, r.h, ALPHA_FORMAT);
  if(!p_out)
    return(NULL);

  SDL_LockSurface(p_var->p_surf);
  for(int y = 0; y < r.h; y++) {
    for(int x = 0; x < r.w; x++) {
      Uint32 pixel = pixel_get(p_var->p_surf, r.x + x, r.y + y);
      if(p_var->alpha) {
        // premultiply
        Uint32 a = pixel >> 24;
        Uint32 red = ((pixel >> 16) & 0xff) * a / 255;
        Uint32 green = ((pixel >> 8) & 0xff) * a / 255;
        Uint32 blue = (pixel & 0xff) * a / 255;
        pixel = (a << 24) | (red << 16) | (green << 8) | blue;
      } else if(key_enabled && (pixel & 0xffffff) == key_rgb) {
        pixel = 0;
      } else {
        pixel |= 0xff000000;
      }
      pixel_set(p_out, x, y, pixel);
    }
  }
  SDL_UnlockSurface(p_var->p_surf);

  if(factor > 1) {
    SDL_Surface *p_scaled = cpu_scaler_run(scaler, p_out, factor);
    if(p_scaled) {
      SDL_DestroySurface(p_out);
      p_out = p_scaled;
    }
  }
  return(p_out);
}

// -------------------------------------------------------
//   CPU scalers
//
//   They run once per (sprite, factor) and the result is cached as a texture
//   by the scene renderer. xBRZ plugs in here: a function that scales a
//   premultiplied ARGB surface by 2..6.
// -------------------------------------------------------

typedef SDL_Surface * (*CPU_SCALER_FUNC)(SDL_Surface *p_src, int factor);

static inline Uint32 px_clamped(SDL_Surface *p_surf, int x, int y)
{
  if(x < 0) x = 0;
  if(y < 0) y = 0;
  if(x >= p_surf->w) x = p_surf->w - 1;
  if(y >= p_surf->h) y = p_surf->h - 1;
  return(pixel_get(p_surf, x, y));
}

// EPX / AdvMAME2x
static SDL_Surface * scale2x(SDL_Surface *p_src)
{
  SDL_Surface *p_dst = SDL_CreateSurface(p_src->w*2, p_src->h*2, ALPHA_FORMAT);
  if(!p_dst)
    return(NULL);

  for(int y = 0; y < p_src->h; y++) {
    for(int x = 0; x < p_src->w; x++) {
      Uint32 P = pixel_get(p_src, x, y);
      Uint32 A = px_clamped(p_src, x, y-1);
      Uint32 B = px_clamped(p_src, x+1, y);
      Uint32 C = px_clamped(p_src, x-1, y);
      Uint32 D = px_clamped(p_src, x, y+1);

      pixel_set(p_dst, 2*x,   2*y,   (C == A && C != D && A != B) ? A : P);
      pixel_set(p_dst, 2*x+1, 2*y,   (A == B && A != C && B != D) ? B : P);
      pixel_set(p_dst, 2*x,   2*y+1, (D == C && D != B && C != A) ? C : P);
      pixel_set(p_dst, 2*x+1, 2*y+1, (B == D && B != A && D != C) ? D : P);
    }
  }
  return(p_dst);
}

// AdvMAME3x / Scale3x
static SDL_Surface * scale3x(SDL_Surface *p_src)
{
  SDL_Surface *p_dst = SDL_CreateSurface(p_src->w*3, p_src->h*3, ALPHA_FORMAT);
  if(!p_dst)
    return(NULL);

  for(int y = 0; y < p_src->h; y++) {
    for(int x = 0; x < p_src->w; x++) {
      Uint32 A = px_clamped(p_src, x-1, y-1), B = px_clamped(p_src, x, y-1), C = px_clamped(p_src, x+1, y-1);
      Uint32 D = px_clamped(p_src, x-1, y),   E = pixel_get(p_src, x, y),     F = px_clamped(p_src, x+1, y);
      Uint32 G = px_clamped(p_src, x-1, y+1), H = px_clamped(p_src, x, y+1), I = px_clamped(p_src, x+1, y+1);

      Uint32 out[9];
      if(B != H && D != F) {
        out[0] = D == B ? D : E;
        out[1] = (D == B && E != C) || (B == F && E != A) ? B : E;
        out[2] = B == F ? F : E;
        out[3] = (D == B && E != G) || (D == H && E != A) ? D : E;
        out[4] = E;
        out[5] = (B == F && E != I) || (H == F && E != C) ? F : E;
        out[6] = D == H ? D : E;
        out[7] = (D == H && E != I) || (H == F && E != G) ? H : E;
        out[8] = H == F ? F : E;
      } else {
        for(int i = 0; i < 9; i++)
          out[i] = E;
      }
      for(int i = 0; i < 9; i++)
        pixel_set(p_dst, 3*x + i%3, 3*y + i/3, out[i]);
    }
  }
  return(p_dst);
}

static SDL_Surface * scaler_scale2x(SDL_Surface *p_src, int factor)
{
  switch(factor) {
    case 2:
      return(scale2x(p_src));
    case 3:
      return(scale3x(p_src));
    case 4:
      {
        SDL_Surface *p_tmp = scale2x(p_src);
        if(!p_tmp)
          return(NULL);
        SDL_Surface *p_dst = scale2x(p_tmp);
        SDL_DestroySurface(p_tmp);
        return(p_dst);
      }
    default:
      return(NULL);
  }
}

// The 2x enlarger of the original double-size mode (surface::scale of
// Berusky 1.7), including its averaging of the bottom-right pixel.
static SDL_Surface * scaler_legacy2x(SDL_Surface *p_src, int factor)
{
  if(factor != 2)
    return(NULL);

  SDL_Surface *p_dst = SDL_CreateSurface(p_src->w*2, p_src->h*2, ALPHA_FORMAT);
  if(!p_dst)
    return(NULL);

  for(int y = 0; y < p_src->h; y++) {
    for(int x = 0; x < p_src->w; x++) {
      Uint32 p0 = pixel_get(p_src, x, y);
      if(!(p0 >> 24)) {
        pixel_set(p_dst, 2*x, 2*y, 0);
        pixel_set(p_dst, 2*x+1, 2*y, 0);
        pixel_set(p_dst, 2*x, 2*y+1, 0);
        pixel_set(p_dst, 2*x+1, 2*y+1, 0);
        continue;
      }

      Uint32 c[4] = { p0, 0, 0, 0 };
      int    hits[4] = { 1, 0, 0, 0 };
      if(x+1 < p_src->w && (c[1] = pixel_get(p_src, x+1, y)) >> 24)
        hits[1] = 1;
      if(y+1 < p_src->h && (c[2] = pixel_get(p_src, x, y+1)) >> 24)
        hits[2] = 1;
      if(x+1 < p_src->w && y+1 < p_src->h && (c[3] = pixel_get(p_src, x+1, y+1)) >> 24)
        hits[3] = 1;

      int r[4], g[4], b[4];
      for(int i = 0; i < 4; i++) {
        r[i] = hits[i] ? (c[i] >> 16) & 0xff : 0;
        g[i] = hits[i] ? (c[i] >> 8) & 0xff : 0;
        b[i] = hits[i] ? c[i] & 0xff : 0;
      }

      #define RGB_PACK(R,G,B) (0xff000000 | ((Uint32)(R) << 16) | ((Uint32)(G) << 8) | (Uint32)(B))
      pixel_set(p_dst, 2*x, 2*y, p0);
      pixel_set(p_dst, 2*x+1, 2*y, hits[1] ? RGB_PACK((r[0]+r[1])/2, (g[0]+g[1])/2, (b[0]+b[1])/2) : p0);
      pixel_set(p_dst, 2*x, 2*y+1, hits[2] ? RGB_PACK((r[0]+r[2])/2, (g[0]+g[2])/2, (b[0]+b[2])/2) : p0);

      // interpolate(color, 4, hits) of 1.7: it sums color[i-1] for every hit i
      int sr = r[0], sg = g[0], sb = b[0], n = 1;
      for(int i = 1; i < 4; i++) {
        if(hits[i]) {
          sr += r[i-1]; sg += g[i-1]; sb += b[i-1];
          n++;
        }
      }
      pixel_set(p_dst, 2*x+1, 2*y+1, RGB_PACK(sr/n, sg/n, sb/n));
      #undef RGB_PACK
    }
  }
  return(p_dst);
}

static const struct {
  ASSET_SCALER     scaler;
  CPU_SCALER_FUNC  func;
  int              factor_max;
} cpu_scalers[] = {
  { SCALER_SCALE2X,  scaler_scale2x,  4 },
  { SCALER_LEGACY2X, scaler_legacy2x, 2 },
  // { SCALER_XBRZ, scaler_xbrz, 6 },
};

int cpu_scaler_factor(ASSET_SCALER scaler, float magnification)
{
  for(size_t i = 0; i < sizeof(cpu_scalers)/sizeof(cpu_scalers[0]); i++) {
    if(cpu_scalers[i].scaler == scaler) {
      if(magnification <= 1.05f)
        return(1);
      int factor = (int)ceilf(magnification - 0.05f);
      if(factor < 2)
        factor = 2;
      if(factor > cpu_scalers[i].factor_max)
        factor = cpu_scalers[i].factor_max;
      return(factor);
    }
  }
  return(1);
}

SDL_Surface * cpu_scaler_run(ASSET_SCALER scaler, SDL_Surface *p_src, int factor)
{
  for(size_t i = 0; i < sizeof(cpu_scalers)/sizeof(cpu_scalers[0]); i++) {
    if(cpu_scalers[i].scaler == scaler) {
      SDL_LockSurface(p_src);
      SDL_Surface *p_dst = cpu_scalers[i].func(p_src, factor);
      SDL_UnlockSurface(p_src);
      return(p_dst);
    }
  }
  return(NULL);
}
