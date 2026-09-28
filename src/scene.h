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
 * Resolution independent scene.
 *
 * The game draws incrementally: it paints a menu or a level once and then only
 * repaints what changed, relying on the old picture staying where it was (the
 * software framebuffer of SDL 1.2). That model is kept, but in logical units:
 *
 *   canvas       something the game draws into (the screen, the level
 *                background with static items). It records every drawing
 *                operation in logical units - a retained display list.
 *                Operations hidden by later opaque ones are dropped, so the
 *                list stays as long as what's visible, not as the history.
 *
 *   draw_canvas  drawing one canvas into another copies (and clips) the
 *                operations, so every list refers to images only and can be
 *                replayed at any time.
 *
 *   scene_renderer
 *                rasterizes operations of the screen canvas into a render
 *                target texture at the current render scale (GPU compositing
 *                through SDL_Renderer). New operations are drawn as they come;
 *                when the render scale, the asset scaler or the device changes
 *                the whole list is replayed - the scene is rendered again at
 *                the new resolution, never stretched from an old frame.
 */

#ifndef __SCENE_H__
#define __SCENE_H__

#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <unordered_map>

#include "image_asset.h"
#include "render_layout.h"

typedef enum {

  OP_FILL = 0,
  OP_IMAGE

} DRAW_OP_TYPE;

typedef struct draw_op {

  Uint8         type;
  Uint8         alpha;          // alpha modulation, 255 = none
  Uint8         opaque;         // covers dst completely
  Uint32        color;          // OP_FILL: 0xRRGGBB
  SDL_Rect      dst;            // logical units, canvas coordinates
  IMAGE_REGION *p_region;       // OP_IMAGE
  SDL_FRect     src;            // OP_IMAGE: base pixels, relative to the region

} DRAW_OP;

class canvas_listener {

public:

  virtual ~canvas_listener(void) {}
  virtual void canvas_op(const DRAW_OP &op) = 0;

};

class canvas {

  int                    width;
  int                    height;
  std::vector<DRAW_OP>   ops;
  size_t                 compact_limit;
  canvas_listener       *p_listener;
  std::vector<Uint64>    coverage;          // compaction scratch, 1 bit per unit

private:

  void push(DRAW_OP &op);
  void release(DRAW_OP &op);
  void release_all(void);

public:

  canvas(int width_, int height_);
  ~canvas(void);

  int width_get(void) const
  {
    return(width);
  }
  int height_get(void) const
  {
    return(height);
  }

  void listener_set(canvas_listener *p_listener_)
  {
    p_listener = p_listener_;
  }

  void fill(const SDL_Rect &dst, Uint32 color);
  void image(IMAGE_REGION *p_region, const SDL_FRect &src, const SDL_Rect &dst, Uint8 alpha);
  // Draws a part of another canvas (it may be this one) at tx, ty
  void draw_canvas(const canvas *p_src, const SDL_Rect &src, int tx, int ty);

  // Drops operations hidden by later opaque ones
  void compact(void);

  size_t op_count(void) const
  {
    return(ops.size());
  }
  const DRAW_OP & op_get(size_t i) const
  {
    return(ops[i]);
  }

  // Text form of the display list (logical units) - layout regression tests
  void dump(FILE *f);

};

typedef struct scene_stats {

  int draws;                    // operations rasterized since the last reset
  int replays;
  int draws_by_density[5];      // variant used: 1x, 2x, 3x, 4x, more
  int cpu_scaled;               // draws of CPU pre-scaled textures
  int textures;                 // cached textures
  float replay_ms;              // duration of the last full replay

} SCENE_STATS;

class scene_renderer {

  struct tex_key {

    IMAGE_REGION *p_region;
    int           variant;
    int           factor;

    bool operator==(const tex_key &k) const
    {
      return(p_region == k.p_region && variant == k.variant && factor == k.factor);
    }
  };

  struct tex_key_hash {

    size_t operator()(const tex_key &k) const
    {
      return(std::hash<const void *>()(k.p_region) ^ ((size_t)k.variant << 3) ^ ((size_t)k.factor << 7));
    }
  };

  struct tex_entry {

    SDL_Texture *p_tex;
    unsigned     version;
  };

  SDL_Renderer   *p_renderer;
  SDL_Texture    *p_target;
  int             target_w, target_h;
  float           scale;
  ASSET_SCALER    scaler;

  std::unordered_map<tex_key, tex_entry, tex_key_hash> textures;

  SCENE_STATS     stats;

private:

  SDL_Texture * texture_get(IMAGE_REGION *p_region, int variant, int factor);
  void          target_bind(void);

public:

  scene_renderer(void);
  ~scene_renderer(void);

  void init(SDL_Renderer *p_renderer_, ASSET_SCALER scaler_);
  void shutdown(void);

  // Render target of the given size for the given scale.
  // Returns true when it was (re)created - the content must be replayed.
  bool target_set(int width, int height, float scale_);
  void target_drop(void);
  SDL_Texture * target_get(void)
  {
    return(p_target);
  }
  float scale_get(void) const
  {
    return(scale);
  }

  void         scaler_set(ASSET_SCALER scaler_);
  ASSET_SCALER scaler_get(void) const
  {
    return(scaler);
  }

  // All textures are gone (device reset) or stale
  void textures_drop(void);
  void region_forget(IMAGE_REGION *p_region);

  void draw(const DRAW_OP &op);
  void replay(const canvas *p_canvas);

  // The render target's pixels (XRGB8888), the caller frees it
  SDL_Surface * target_read(void);

  SCENE_STATS & stats_get(void)
  {
    stats.textures = (int)textures.size();
    return(stats);
  }
  void stats_reset(void)
  {
    memset(&stats, 0, sizeof(stats));
  }

};

#endif // __SCENE_H__
