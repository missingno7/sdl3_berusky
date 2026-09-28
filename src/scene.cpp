/*
 * Berusky (C) AnakreoN
 * Martin Stransky <stransky@anakreon.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* Resolution independent scene - see scene.h */

#include <math.h>

#include "scene.h"
#include "utils.h"

// -------------------------------------------------------
//   Canvas
// -------------------------------------------------------

#define COMPACT_MIN   512

canvas::canvas(int width_, int height_)
: width(width_), height(height_), compact_limit(COMPACT_MIN), p_listener(NULL)
{
}

canvas::~canvas(void)
{
  release_all();
}

void canvas::release(DRAW_OP &op)
{
  if(op.type == OP_IMAGE)
    op.p_region->p_asset->unref();
}

void canvas::release_all(void)
{
  for(size_t i = 0; i < ops.size(); i++)
    release(ops[i]);
  ops.clear();
  compact_limit = COMPACT_MIN;
}

// Clips the operation to a rectangle (the source rectangle follows)
static bool op_clip(DRAW_OP &op, const SDL_Rect &clip)
{
  SDL_Rect r;
  if(!SDL_GetRectIntersection(&op.dst, &clip, &r))
    return(false);

  if(op.type == OP_IMAGE && (r.x != op.dst.x || r.y != op.dst.y || r.w != op.dst.w || r.h != op.dst.h)) {
    const float kx = op.src.w / op.dst.w;
    const float ky = op.src.h / op.dst.h;
    op.src.x += (r.x - op.dst.x) * kx;
    op.src.y += (r.y - op.dst.y) * ky;
    op.src.w = r.w * kx;
    op.src.h = r.h * ky;
  }
  op.dst = r;
  return(true);
}

void canvas::push(DRAW_OP &op)
{
  const SDL_Rect whole = { 0, 0, width, height };
  if(!op_clip(op, whole))
    return;

  // Painting over everything - nothing older can be seen
  if(op.opaque && op.dst.x == 0 && op.dst.y == 0 && op.dst.w == width && op.dst.h == height)
    release_all();

  if(op.type == OP_IMAGE)
    op.p_region->p_asset->ref();
  ops.push_back(op);

  if(p_listener)
    p_listener->canvas_op(op);

  if(ops.size() > compact_limit) {
    compact();
    compact_limit = SDL_max((size_t)COMPACT_MIN, ops.size() * 2);
  }
}

void canvas::fill(const SDL_Rect &dst, Uint32 color)
{
  DRAW_OP op;
  memset(&op, 0, sizeof(op));
  op.type = OP_FILL;
  op.alpha = 255;
  op.opaque = 1;
  op.color = color & 0xffffff;
  op.dst = dst;
  push(op);
}

void canvas::image(IMAGE_REGION *p_region, const SDL_FRect &src, const SDL_Rect &dst, Uint8 alpha)
{
  DRAW_OP op;
  memset(&op, 0, sizeof(op));
  op.type = OP_IMAGE;
  op.alpha = alpha;
  op.opaque = (alpha == 255 && p_region->is_opaque()) ? 1 : 0;
  op.dst = dst;
  op.p_region = p_region;
  op.src = src;
  push(op);
}

void canvas::draw_canvas(const canvas *p_src, const SDL_Rect &src, int tx, int ty)
{
  const SDL_Rect whole = { 0, 0, p_src->width, p_src->height };
  SDL_Rect clip;
  if(!SDL_GetRectIntersection(&src, &whole, &clip))
    return;

  const int dx = tx - src.x;
  const int dy = ty - src.y;

  // (a copy: the source may be this canvas)
  std::vector<DRAW_OP> copy;
  for(size_t i = 0; i < p_src->ops.size(); i++) {
    const SDL_Rect &r = p_src->ops[i].dst;
    if(r.x < clip.x + clip.w && clip.x < r.x + r.w && r.y < clip.y + clip.h && clip.y < r.y + r.h)
      copy.push_back(p_src->ops[i]);
  }

  for(size_t i = 0; i < copy.size(); i++) {
    DRAW_OP op = copy[i];
    if(!op_clip(op, clip))
      continue;
    op.dst.x += dx;
    op.dst.y += dy;
    push(op);
  }
}

// Bit ranges [x0, x1) of a coverage row
static bool bits_all(const Uint64 *p_row, int x0, int x1)
{
  while(x0 < x1) {
    int    b = x0 & 63;
    int    n = SDL_min(64 - b, x1 - x0);
    Uint64 mask = (n == 64) ? ~(Uint64)0 : ((((Uint64)1 << n) - 1) << b);
    if((p_row[x0 >> 6] & mask) != mask)
      return(false);
    x0 += n;
  }
  return(true);
}

static void bits_set(Uint64 *p_row, int x0, int x1)
{
  while(x0 < x1) {
    int    b = x0 & 63;
    int    n = SDL_min(64 - b, x1 - x0);
    Uint64 mask = (n == 64) ? ~(Uint64)0 : ((((Uint64)1 << n) - 1) << b);
    p_row[x0 >> 6] |= mask;
    x0 += n;
  }
}

// From the newest operation to the oldest one: an operation is dropped when
// everything it touches is already covered by newer opaque operations.
void canvas::compact(void)
{
  if(ops.empty())
    return;

  const size_t words = (width + 63) / 64;
  coverage.assign(words * height, 0);

  std::vector<char> keep(ops.size(), 1);
  for(size_t i = ops.size(); i-- > 0; ) {
    const SDL_Rect &r = ops[i].dst;

    bool hidden = true;
    for(int y = r.y; y < r.y + r.h; y++) {
      if(!bits_all(&coverage[y * words], r.x, r.x + r.w)) {
        hidden = false;
        break;
      }
    }

    if(hidden) {
      keep[i] = 0;
    } else if(ops[i].opaque) {
      for(int y = r.y; y < r.y + r.h; y++)
        bits_set(&coverage[y * words], r.x, r.x + r.w);
    }
  }

  size_t last = 0;
  for(size_t i = 0; i < ops.size(); i++) {
    if(keep[i])
      ops[last++] = ops[i];
    else
      release(ops[i]);
  }
  ops.resize(last);
}

void canvas::dump(FILE *f)
{
  compact();
  fprintf(f, "canvas %dx%d, %d operations\n", width, height, (int)ops.size());
  for(size_t i = 0; i < ops.size(); i++) {
    const DRAW_OP &op = ops[i];
    if(op.type == OP_FILL) {
      fprintf(f, "fill  %4d %4d %4d %4d #%06x\n", op.dst.x, op.dst.y, op.dst.w, op.dst.h, op.color);
    } else {
      const SDL_Rect &r = op.p_region->rect;
      fprintf(f, "image %4d %4d %4d %4d %s [%d %d %d %d] src %.2f %.2f %.2f %.2f%s",
              op.dst.x, op.dst.y, op.dst.w, op.dst.h, op.p_region->p_asset->name_get(),
              r.x, r.y, r.w, r.h, op.src.x, op.src.y, op.src.w, op.src.h,
              op.opaque ? " opaque" : "");
      if(op.alpha != 255)
        fprintf(f, " alpha %d", op.alpha);
      fprintf(f, "\n");
    }
  }
}

// -------------------------------------------------------
//   Scene renderer
// -------------------------------------------------------

#define TARGET_FORMAT   SDL_PIXELFORMAT_ARGB8888

scene_renderer::scene_renderer(void)
: p_renderer(NULL), p_target(NULL), target_w(0), target_h(0), scale(1.0f), scaler(SCALER_PIXELART)
{
  stats_reset();
}

scene_renderer::~scene_renderer(void)
{
  shutdown();
}

void scene_renderer::init(SDL_Renderer *p_renderer_, ASSET_SCALER scaler_)
{
  p_renderer = p_renderer_;
  scaler = scaler_;
}

void scene_renderer::shutdown(void)
{
  textures_drop();
  target_drop();
  p_renderer = NULL;
}

bool scene_renderer::target_set(int width, int height, float scale_)
{
  if(p_target && width == target_w && height == target_h && scale_ == scale)
    return(false);

  scale = scale_;

  if(!p_target || width != target_w || height != target_h) {
    if(p_target) {
      if(SDL_GetRenderTarget(p_renderer) == p_target)
        SDL_SetRenderTarget(p_renderer, NULL);
      SDL_DestroyTexture(p_target);
    }
    p_target = SDL_CreateTexture(p_renderer, TARGET_FORMAT, SDL_TEXTUREACCESS_TARGET, width, height);
    if(!p_target) {
      bprintf("Unable to create the %dx%d render target: %s", width, height, SDL_GetError());
      target_w = target_h = 0;
      return(false);
    }
    SDL_SetTextureBlendMode(p_target, SDL_BLENDMODE_NONE);
    target_w = width;
    target_h = height;
  }
  return(true);
}

void scene_renderer::target_drop(void)
{
  if(p_target) {
    if(SDL_GetRenderTarget(p_renderer) == p_target)
      SDL_SetRenderTarget(p_renderer, NULL);
    SDL_DestroyTexture(p_target);
    p_target = NULL;
  }
  target_w = target_h = 0;
}

void scene_renderer::target_bind(void)
{
  if(SDL_GetRenderTarget(p_renderer) != p_target)
    SDL_SetRenderTarget(p_renderer, p_target);
}

void scene_renderer::scaler_set(ASSET_SCALER scaler_)
{
  if(scaler != scaler_) {
    scaler = scaler_;
    textures_drop();
  }
}

void scene_renderer::textures_drop(void)
{
  for(auto &it : textures)
    SDL_DestroyTexture(it.second.p_tex);
  textures.clear();
}

void scene_renderer::region_forget(IMAGE_REGION *p_region)
{
  for(auto it = textures.begin(); it != textures.end(); ) {
    if(it->first.p_region == p_region) {
      SDL_DestroyTexture(it->second.p_tex);
      it = textures.erase(it);
    } else {
      ++it;
    }
  }
}

SDL_Texture * scene_renderer::texture_get(IMAGE_REGION *p_region, int variant, int factor)
{
  const tex_key  key = { p_region, variant, factor };
  const unsigned version = p_region->p_asset->version_get();

  auto it = textures.find(key);
  if(it != textures.end()) {
    if(it->second.version == version)
      return(it->second.p_tex);
    SDL_DestroyTexture(it->second.p_tex);
    textures.erase(it);
  }

  SDL_Surface *p_pixels = p_region->p_asset->region_pixels(p_region, variant,
                                                           asset_scaler_effective(scaler), factor);
  if(!p_pixels)
    return(NULL);

  SDL_Texture *p_tex = SDL_CreateTexture(p_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC,
                                         p_pixels->w, p_pixels->h);
  if(p_tex) {
    SDL_UpdateTexture(p_tex, NULL, p_pixels->pixels, p_pixels->pitch);
    SDL_SetTextureScaleMode(p_tex, image_filter_sdl(asset_scaler_filter(scaler)));
    textures[key] = { p_tex, version };
  } else {
    bprintf("Unable to create a texture (%dx%d): %s", p_pixels->w, p_pixels->h, SDL_GetError());
  }
  SDL_DestroySurface(p_pixels);
  return(p_tex);
}

void scene_renderer::draw(const DRAW_OP &op)
{
  if(!p_target)
    return;

  target_bind();

  // Snap the edges to whole pixels: neighbour cells share their edges
  // exactly, no gaps and no overlaps at fractional scales
  const float x0 = roundf(op.dst.x * scale);
  const float y0 = roundf(op.dst.y * scale);
  const float x1 = roundf((op.dst.x + op.dst.w) * scale);
  const float y1 = roundf((op.dst.y + op.dst.h) * scale);
  if(x1 <= x0 || y1 <= y0)
    return;

  const SDL_FRect dst = { x0, y0, x1 - x0, y1 - y0 };

  stats.draws++;

  if(op.type == OP_FILL) {
    SDL_SetRenderDrawBlendMode(p_renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(p_renderer, (op.color >> 16) & 0xff, (op.color >> 8) & 0xff, op.color & 0xff, 255);
    SDL_RenderFillRect(p_renderer, &dst);
    return;
  }

  image_asset *p_asset = op.p_region->p_asset;

  // The variant with the most useful resolution, then a CPU pre-scale if
  // the scaler has one and the variant still has to be magnified
  const int            variant = p_asset->variant_select(scale);
  const IMAGE_VARIANT *p_var = p_asset->variant_get(variant);
  const float          magnification = scale / (p_var->density / p_asset->zoom_get());
  int                  factor = 1;
  if(asset_scaler_is_cpu(scaler))
    factor = cpu_scaler_factor(asset_scaler_effective(scaler), magnification);

  SDL_Texture *p_tex = texture_get(op.p_region, variant, factor);
  if(!p_tex)
    return;

  // Source rectangle of the snapped destination (the mapping stays continuous
  // across neighbour operations), in texture pixels
  const float kx = op.src.w / op.dst.w;
  const float ky = op.src.h / op.dst.h;
  const float t = p_var->density / p_asset->density_get() * factor;

  float sx0 = (op.src.x + (x0 / scale - op.dst.x) * kx) * t;
  float sy0 = (op.src.y + (y0 / scale - op.dst.y) * ky) * t;
  float sx1 = (op.src.x + (x1 / scale - op.dst.x) * kx) * t;
  float sy1 = (op.src.y + (y1 / scale - op.dst.y) * ky) * t;

  float tw, th;
  SDL_GetTextureSize(p_tex, &tw, &th);
  sx0 = SDL_clamp(sx0, 0.0f, tw);
  sx1 = SDL_clamp(sx1, 0.0f, tw);
  sy0 = SDL_clamp(sy0, 0.0f, th);
  sy1 = SDL_clamp(sy1, 0.0f, th);
  if(sx1 <= sx0 || sy1 <= sy0)
    return;

  const SDL_FRect src = { sx0, sy0, sx1 - sx0, sy1 - sy0 };

  if(op.alpha != 255) {
    // premultiplied: the color is modulated with the alpha too
    SDL_SetTextureBlendMode(p_tex, SDL_BLENDMODE_BLEND_PREMULTIPLIED);
    SDL_SetTextureAlphaMod(p_tex, op.alpha);
    SDL_SetTextureColorMod(p_tex, op.alpha, op.alpha, op.alpha);
  } else {
    SDL_SetTextureBlendMode(p_tex, op.opaque ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND_PREMULTIPLIED);
    SDL_SetTextureAlphaMod(p_tex, 255);
    SDL_SetTextureColorMod(p_tex, 255, 255, 255);
  }

  SDL_RenderTexture(p_renderer, p_tex, &src, &dst);

  int d = (int)lroundf(p_var->density);
  stats.draws_by_density[SDL_clamp(d, 1, 5) - 1]++;
  if(factor > 1)
    stats.cpu_scaled++;
}

void scene_renderer::replay(const canvas *p_canvas)
{
  if(!p_target)
    return;

  target_bind();
  SDL_SetRenderDrawBlendMode(p_renderer, SDL_BLENDMODE_NONE);
  SDL_SetRenderDrawColor(p_renderer, 0, 0, 0, 255);
  SDL_RenderClear(p_renderer);

  Uint64 start = SDL_GetPerformanceCounter();
  if(p_canvas) {
    for(size_t i = 0; i < p_canvas->op_count(); i++)
      draw(p_canvas->op_get(i));
  }
  stats.replays++;
  stats.replay_ms = (SDL_GetPerformanceCounter() - start) * 1000.0f / SDL_GetPerformanceFrequency();
}

SDL_Surface * scene_renderer::target_read(void)
{
  if(!p_target)
    return(NULL);

  target_bind();
  SDL_Surface *p_read = SDL_RenderReadPixels(p_renderer, NULL);
  if(!p_read)
    return(NULL);

  SDL_Surface *p_out = SDL_ConvertSurface(p_read, SDL_PIXELFORMAT_XRGB8888);
  SDL_DestroySurface(p_read);
  return(p_out);
}
