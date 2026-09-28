/*
 *        .þÛÛþ þ    þ þÛÛþ.     þ    þ þÛÛÛþ.  þÛÛÛþ .þÛÛþ. þ    þ
 *       .þ   Û Ûþ.  Û Û   þ.    Û    Û Û    þ  Û.    Û.   Û Ûþ.  Û
 *       Û    Û Û Û  Û Û    Û    Û   þ. Û.   Û  Û     Û    Û Û Û  Û
 *     .þþÛÛÛÛþ Û  Û Û þÛÛÛÛþþ.  þþÛÛ.  þþÛÛþ.  þÛ    Û    Û Û  Û Û
 *    .Û      Û Û  .þÛ Û      Û. Û   Û  Û    Û  Û.    þ.   Û Û  .þÛ
 *    þ.      þ þ    þ þ      .þ þ   .þ þ    .þ þÛÛÛþ .þÛÛþ. þ    þ
 *
 * Berusky (C) AnakreoN
 * Martin Stransky <stransky@anakreon.cz> 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
  2D Graphics library
*/
#include <SDL3/SDL.h>

#include <math.h>
#include <vector>
#include <string>

#include "berusky.h"

// -------------------------------------------------------
//   the surface class
// -------------------------------------------------------
char surface::graphics_dir[MAX_FILENAME] = "";

surface::surface(void)
: used(0), p_image(NULL), p_canvas(NULL)
{
}

surface::surface(tpos width, tpos height)
: used(0), p_image(NULL), p_canvas(NULL)
{
  create(width, height);
}

surface::surface(class surface *p_src, tpos sx, tpos sy, tpos width, tpos height)
: used(0), p_image(NULL), p_canvas(NULL)
{
  RECT src = {sx,sy,width,height};
  copy(p_src, &src);
}

surface::~surface(void)
{
  free();
}

bool surface::load(const char *p_file, float density, float zoom)
{
  free();
  p_image = image_asset::load(graphics_dir_get(), p_file, density, zoom);
  if(!p_image) {
    bprintf("Unable to load %s: %s", p_file, SDL_GetError());
    assert(p_image);
  }
  used = 0;
  return(p_image != NULL);
}

void surface::create(tpos width, tpos height)
{
  assert(!p_image && !p_canvas);
  p_canvas = new canvas(width, height);
  used = 0;
}

void surface::copy(class surface *p_src, RECT *p_src_rect)
{
  assert(p_src->is_loaded());
  free();

  RECT rect = p_src_rect ? *p_src_rect : p_src->rect_get();

  if(p_src->p_image) {
    SDL_Rect px = p_src->pixel_rect(rect);
    p_image = image_asset::copy(p_src->p_image, &px);
  } else {
    p_canvas = new canvas(rect.w, rect.h);
    p_canvas->draw_canvas(p_src->p_canvas, rect, 0, 0);
  }

  used = 0;
}

void surface::free(void)
{
  assert(used == 0);

  if(p_image) {
    p_image->unref();
    p_image = NULL;
  }
  if(p_canvas) {
    delete p_canvas;
    p_canvas = NULL;
  }
}

tpos surface::width_get(void)
{
  if(p_canvas)
    return(p_canvas->width_get());
  assert(p_image);
  return((tpos)lroundf(p_image->pixel_width() / p_image->px_per_unit()));
}

tpos surface::height_get(void)
{
  if(p_canvas)
    return(p_canvas->height_get());
  assert(p_image);
  return((tpos)lroundf(p_image->pixel_height() / p_image->px_per_unit()));
}

tpos surface::pixel_width_get(void)
{
  assert(p_image);
  return(p_image->pixel_width());
}

tpos surface::pixel_height_get(void)
{
  assert(p_image);
  return(p_image->pixel_height());
}

SDL_Rect surface::pixel_rect(const RECT &r)
{
  const float k = p_image ? p_image->px_per_unit() : 1.0f;
  SDL_Rect px = { (int)lroundf(r.x * k), (int)lroundf(r.y * k),
                  (int)lroundf(r.w * k), (int)lroundf(r.h * k) };
  return(px);
}

canvas * surface::canvas_get(void)
{
  if(!p_canvas) {
    assert(p_image);
    // The picture is the first thing on the canvas
    RECT whole = rect_get();
    p_canvas = new canvas(whole.w, whole.h);
    SDL_Rect px = pixel_rect(whole);
    SDL_FRect src = { 0, 0, (float)px.w, (float)px.h };
    p_canvas->image(p_image->region_get(px), src, whole, p_image->alpha_mod_get());
    p_image->unref();
    p_image = NULL;
  }
  return(p_canvas);
}

// Color-key set
void surface::ckey_set(trgbcomp r, trgbcomp g, trgbcomp b)
{
  if(p_image)
    p_image->colorkey_set(r, g, b);
}

void surface::alpha_mod_set(Uint8 alpha)
{
  if(p_image)
    p_image->alpha_mod_set(alpha);
}

void surface::fill(tcolor color)
{
  canvas *p_to = canvas_get();
  SDL_Rect r = { 0, 0, p_to->width_get(), p_to->height_get() };
  p_to->fill(r, color);
}

void surface::fill(tpos x, tpos y, tpos dx, tpos dy, tcolor color)
{
  SDL_Rect r = { x, y, dx, dy };
  canvas_get()->fill(r, color);
}

void surface::draw_part(const RECT &region, const RECT &src, class surface *p_dst, tpos tx, tpos ty)
{
  // What of src is really in the region (the sprite)
  SDL_Rect part;
  if(!SDL_GetRectIntersection(&src, &region, &part))
    return;
  tx += part.x - src.x;
  ty += part.y - src.y;

  canvas *p_to = p_dst->canvas_get();

  if(p_canvas) {
    p_to->draw_canvas(p_canvas, part, tx, ty);
    return;
  }

  assert(p_image);
  const float   k = p_image->px_per_unit();
  IMAGE_REGION *p_region = p_image->region_get(pixel_rect(region));
  SDL_FRect     s = { (part.x - region.x) * k, (part.y - region.y) * k, part.w * k, part.h * k };
  SDL_Rect      d = { tx, ty, part.w, part.h };
  p_to->image(p_region, s, d, p_image->alpha_mod_get());
}

// blit whole source surtace to destination surface
void surface::blit(class surface *p_dst, tpos tx, tpos ty)
{
  RECT whole = rect_get();
  draw_part(whole, whole, p_dst, tx, ty);
}

// blit part of this surface to a destination (target) surface
void surface::blit(tpos sx, tpos sy, tpos dx, tpos dy, class surface *p_dst, tpos tx, tpos ty)
{
  RECT whole = rect_get();
  RECT src = {sx,sy,dx,dy};
  draw_part(whole, src, p_dst, tx, ty);
}

typedef struct blend_data {

  RGB      color;
  BLEND_OP operation;

} BLEND_DATA;

static void blend_pixel(int *p_r, int *p_g, int *p_b, void *p_data, bool base)
{
  BLEND_DATA *p_blend = (BLEND_DATA *)p_data;
  RGB pixel(*p_r, *p_g, *p_b);

  switch(p_blend->operation) {
    case BLEND_SET:
      pixel = p_blend->color;
      break;
    case BLEND_ADD:
      pixel.r += p_blend->color.r;
      pixel.g += p_blend->color.g;
      pixel.b += p_blend->color.b;
      pixel.norm();
      break;
    case BLEND_SUB:
      {
        pixel.r -= p_blend->color.r;
        pixel.g -= p_blend->color.g;
        pixel.b -= p_blend->color.b;

        // The base image gets the same noise as before (rand() sequence);
        // HD variants use their own generator so they don't shift it
        static Uint64 variant_seed = 1;
        int rn = base ? (int)floor(((float)rand()/RAND_MAX)*10) : SDL_rand_r(&variant_seed, 10);
        pixel.r -= rn;
        pixel.g -= rn;
        pixel.b -= rn;
        pixel.norm();
      }
      break;
  }

  *p_r = pixel.r;
  *p_g = pixel.g;
  *p_b = pixel.b;
}

void surface::blend(tpos sx, tpos sy, tpos dx, tpos dy, tcolor color, BLEND_OP operation)
{
  assert(p_image);
  assert(sx+dx <= pixel_width_get());
  assert(sy+dy <= pixel_height_get());

  BLEND_DATA data;
  data.color = RGB((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
  data.operation = operation;

  SDL_Rect r = { sx, sy, dx, dy };
  p_image->pixels_modify(r, blend_pixel, &data);
}

// -------------------------------------------------------
//   the sprite class
// -------------------------------------------------------

RGB  sprite::key;

void sprite::rect_check(void)
{
  if((flag&SDL_SPRITE_RECT) && p_surf) {
    assert(rec.x+rec.w <= p_surf->width_get());
    assert(rec.y+rec.h <= p_surf->height_get());
  }
}

void sprite::load(SURFACE *p_surf_, tflag flag_, RECT *p_rect)
{
  flag = flag_;
  p_surf = p_surf_;

  if(p_rect) {
    rec = *p_rect;
    flag |= SDL_SPRITE_RECT;
  } else {
    if(p_surf)
      rec = p_surf->rect_get();
  }

  if(p_surf)
    p_surf->inc_ref();

  rect_check();
}

void sprite::free(void)
{
  if(p_surf)
    p_surf->dec_ref();

  flag = 0;
  p_surf = NULL;
}

sprite::sprite(void)
:flag(0), p_surf(NULL)
{
}

sprite::sprite(SURFACE *p_surf_, tflag flag_, RECT *p_rect)
{
  load(p_surf_, flag_,p_rect);
}

sprite::sprite(class sprite &src)
{
  flag = src.flag;
  rec = src.rec;
  p_surf = src.p_surf;
  if(p_surf)
    p_surf->inc_ref();
  rect_check();
}

sprite::~sprite(void)
{
  flag = 0;

  if(p_surf)
    p_surf->dec_ref();
}

void sprite::fill(tcolor color)
{
  assert(p_surf);
  if(flag&SDL_SPRITE_RECT)
    p_surf->fill(rec.x,rec.y,rec.w,rec.h,color);
  else
    p_surf->fill(color);
}

void sprite::fill(tpos x, tpos y, tpos dx, tpos dy, tcolor color)
{
  assert(p_surf);
  if(flag&SDL_SPRITE_RECT) {
    p_surf->fill(rec.x + x, rec.y + y,
                 dx > rec.w ? rec.w : dx,
                 dy > rec.h ? rec.h : dy,
                 color);
  } else {
    p_surf->fill(x,y,dx,dy,color);
  }
}

// blit whole source sprite to destination sprite
void sprite::blit(class sprite *p_dst, tpos tx, tpos ty)
{
  assert(p_surf && p_dst->p_surf);

  if(p_dst->flag&SDL_SPRITE_RECT) {
    tx += p_dst->rec.x;
    ty += p_dst->rec.y;
  }

  p_surf->draw_part(rec, rec, p_dst->p_surf, tx, ty);
}

// blit part of source sprite (surface coordinates) to destination sprite
void sprite::blit(tpos sx, tpos sy, tpos dx, tpos dy, class sprite *p_dst, tpos tx, tpos ty)
{
  assert(p_surf && p_dst->p_surf);

  if(p_dst->flag&SDL_SPRITE_RECT) {
    tx += p_dst->rec.x;
    ty += p_dst->rec.y;
  }

  RECT src = {sx, sy, dx, dy};
  p_surf->draw_part(rec, src, p_dst->p_surf, tx, ty);
}

/*
*/
void sprite::rect_clamp(RECT *p_rect)
{
  tpos width = p_rect->w;
  tpos height = p_rect->h;

  if(p_rect->x < 0) {
    width += p_rect->x;
    p_rect->x = 0;
    if(width < 0)
      width = 0;
  }
  if(p_rect->y < 0) {
    height += p_rect->y;
    p_rect->y = 0;
    if(height < 0)
      height = 0;
  }
  if(p_rect->x+width > rec.w) {
    assert(p_rect->x < rec.w && rec.x == 0);
    width = rec.w-p_rect->x;
  }
  if(p_rect->y+height > rec.h) {
    assert(p_rect->y < rec.h && rec.y == 0);
    height = rec.h-p_rect->y;
  }
  p_rect->w = width;
  p_rect->h = height;

  assert(width > 0 && height > 0);
  assert(p_rect->x+p_rect->w >= 0 && p_rect->x+p_rect->w <= rec.w);
  assert(p_rect->y+p_rect->h >= 0 && p_rect->y+p_rect->h <= rec.h);
}


// -------------------------------------------------------
//   the sprite store class
// -------------------------------------------------------

sprite_store::sprite_store(surf_handle surf_num, spr_handle spr_num)
{
  p_surfaces = (SURFACE *)mmalloc(sizeof(p_surfaces[0])*surf_num);
  surface_num = surf_num;
  surface_last = 0;

  p_sprites = (SPRITE *)mmalloc(sizeof(p_sprites[0])*spr_num);
  sprite_num = spr_num;
  sprite_last = 0;

  cell_zoom = 1.0f;
}

sprite_store::~sprite_store(void)
{
  ffree(p_surfaces);
  ffree(p_sprites);
}

void sprite_store::sprite_flag_set(tflag flag, spr_handle first, spr_handle num)
{
  spr_handle max = first+num;

  assert(max < sprite_num);

  for(; first < max; first++)
    p_sprites[first].flag_set(flag);
}

void sprite_store::sprite_flag_clear(tflag flag, spr_handle first, spr_handle num)
{
  spr_handle max = first+num;

  assert(max < sprite_num);

  for(; first < max; first++)
    p_sprites[first].flag_clear(flag);
}

/*
  Sprite sheet (.spr) - rectangles are in pixels of the PNG:

    s x y w h [scale]     one sprite
    f dx dy dw dh count   'count' more sprites, each shifted from the previous
    m density N           native pixel density of the sheet (pixels per unit)
    m cell 0|1            level cell art (follows the cell zoom) or UI art

  Without 'm' lines the legacy double-size field decides (see
  docs/RENDERER.md): scale 0 = genuine 2x artwork (density 2), scale 1 or no
  field = 1x artwork. Sheets with the field are cell art.
*/
spr_handle sprite_store::sprite_insert(const char *p_file, spr_handle first, spr_handle *p_last)
{
  char filename[200];
  char line[200];

  strncpy(filename, p_file, 200);
  change_tail(filename, BITMAP_FORMAT);

  // Read the whole sheet first: the metadata decides how the image is loaded
  std::vector<std::string> lines;
  FHANDLE f = file_open(surface::graphics_dir_get(),p_file, "r");
  while (file_gets(line, 200, f))
    lines.push_back(line);
  file_close(f);

  float density = 0;
  int   cell = -1;
  int   legacy = -1;
  for(size_t l = 0; l < lines.size(); l++) {
    const char *p_line = lines[l].c_str();
    if(p_line[0] == 'm') {
      char key[50];
      float value;
      if(sscanf(p_line, "m %49s %f", key, &value) == 2) {
        if(!strcmp(key, "density") && value > 0)
          density = value;
        else if(!strcmp(key, "cell"))
          cell = value != 0;
      }
    }
    else if(p_line[0] == 's' && legacy < 0) {
      int x, y, w, h, scale;
      if(sscanf(p_line, "s %d %d %d %d %d", &x, &y, &w, &h, &scale) == 5)
        legacy = scale;
      else
        legacy = 2;       // no field
    }
  }
  if(density <= 0)
    density = (legacy == 0) ? 2.0f : 1.0f;
  if(cell < 0)
    cell = (legacy == 0 || legacy == 1);

  const float zoom = cell ? cell_zoom : 1.0f;
  // sheet pixels -> logical units
  const float k = zoom / density;

  spr_handle i = first;
  SURFACE   *p_surf = NULL;
  RECT       rec = {0,0,0,0};

  for(size_t l = 0; l < lines.size(); l++) {
    const char *p_line = lines[l].c_str();
    int x, y, w, h, count = 0;

    if (p_line[0] == ';') {
      continue;
    }
    else if (p_line[0] == 's') {
      if(sscanf(p_line, "s %d %d %d %d", &x, &y, &w, &h) != 4)
        continue;
      rec.x = x; rec.y = y; rec.w = w; rec.h = h;
      count = 1;
    }
    else if (p_line[0] == 'f') {
      int dx, dy, dw, dh;
      if(sscanf(p_line, "f %d %d %d %d %d", &dx, &dy, &dw, &dh, &count) != 5)
        continue;
      // the first shifted sprite
      rec.x += dx; rec.y += dy; rec.w += dw; rec.h += dh;
      x = dx; y = dy; w = dw; h = dh;
    }
    else {
      continue;
    }

    if(!p_surf) {
      surf_handle s_handle = surface_insert(filename, density, zoom);
      p_surf = surface_get(s_handle);
      if(!p_surf->is_loaded()) {
        berror("Can't load surface %s",filename);
      }
      // The color key of the whole sheet
      SPRITE tmp(p_surf);
      tmp.color_key_apply();
    }

    for(int j = 0; j < count; j++) {
      if(j > 0) {
        rec.x += x; rec.y += y; rec.w += w; rec.h += h;
      }
      RECT logical = { (int)lroundf(rec.x * k), (int)lroundf(rec.y * k),
                       (int)lroundf(rec.w * k), (int)lroundf(rec.h * k) };
      if(logical.x != rec.x * k || logical.y != rec.y * k || logical.w != rec.w * k || logical.h != rec.h * k) {
        bprintf("%s: sprite %d,%d %dx%d is not on the %gx pixel grid", p_file, rec.x, rec.y, rec.w, rec.h, density);
      }
      SPRITE tmp(p_surf,SDL_SPRITE_RECT,&logical);
      sprite_insert(&tmp,1, i);
      i++;
    }
  }

  if(p_last)
    *p_last = i;

  sprite_flag_set(SDL_SPRITE_USED, first, i - first);

  return (i - first);
}

spr_handle sprite_store::sprite_insert(surf_handle sf_handle, tflag flag_, RECT *p_rect, spr_handle first)
{
  spr_handle shandle;
  if(first == INSERT_APPEND) {
    shandle = sprite_last++;
  } else {
    shandle = first;
    if(shandle > sprite_last)
      sprite_last = shandle;
  }
  assert(shandle < sprite_num);
  flag_ &= ~SDL_SPRITE_SEPARATE_SURFACE;
  SPRITE tmp(p_surfaces+sf_handle, flag_, p_rect);
  memcpy(p_sprites+shandle,&tmp, sizeof(tmp));
  sprite_flag_set(SDL_SPRITE_USED, shandle);
  return shandle;
}

spr_handle sprite_store::sprite_insert(SURFACE *p_surf, tflag flag_, RECT *p_rect, spr_handle first)
{
  spr_handle shandle;
  if(first == INSERT_APPEND) {
    shandle = sprite_last++;
  } else {
    shandle = first;
    if(shandle > sprite_last)
      sprite_last = shandle;
  }
  assert(shandle < sprite_num);
  flag_ |= SDL_SPRITE_SEPARATE_SURFACE;
  SPRITE tmp(p_surf, flag_, p_rect);
  memcpy(p_sprites+shandle,&tmp, sizeof(tmp));
  sprite_flag_set(SDL_SPRITE_USED, shandle);
  return shandle;
}

spr_handle sprite_store::sprite_insert(SPRITE *p_spr, int num, spr_handle first)
{
  spr_handle start = (first == INSERT_APPEND) ? sprite_last : first;
  assert(start+num < sprite_num);
  if(start+num > sprite_last)
    sprite_last = start+num;
  memcpy(p_sprites+start,p_spr,sizeof(p_spr[0])*num);
  sprite_flag_set(SDL_SPRITE_USED, start, num);
  return(start);
}

spr_handle sprite_store::sprite_copy(spr_handle dst_handle, spr_handle src_handle, bool copy_surface)
{
  assert(dst_handle < sprite_num && src_handle < sprite_num);
  SPRITE *p_src = p_sprites + src_handle;

  if(copy_surface && p_src->surf_get()) {
    RECT *p_src_rec = p_src->rect_get();
    RECT r = {0, 0, p_src_rec->w, p_src_rec->h};
    sprite_insert(surface_copy(p_src->surf_get(), p_src_rec),
                  p_src->flag_get(), &r, dst_handle);
  } else {
    p_sprites[dst_handle] = *p_src;
    // -- add ref-counnt?
  }

  return(dst_handle);
}

void sprite_store::sprite_delete(spr_handle handle, int num)
{
  // TODO - removing sprites/surfaces
  sprite_flag_clear(SDL_SPRITE_USED, handle, num);
}


// -------------------------------------------------------
//   the graph 2d store class
// -------------------------------------------------------
void graph_2d::screen_create(int flag, int width, int height, int bpp, int fullscreen)
{
  // (flag, bpp) are relics of SDL_SetVideoMode()
  if(!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
    berror("Unable to init SDL video: %s", SDL_GetError());
  }

  graphics_fullscreen = fullscreen;

  screen_resize(width, height);
}

// Create the screen canvas (logical size of the composition) and connect it
// with the window. If we have any screen, release it first.
bool graph_2d::screen_regenerate(void)
{
  video.scene_set(NULL);
  screen_destroy();

  bprintf("Init screen %dx%d (logical units)...\n", graphics_width, graphics_height);

  p_screen_surface = new SURFACE(graphics_width, graphics_height);
  p_screen = new SPRITE(p_screen_surface, SDL_SPRITE_SEPARATE_SURFACE, NULL);

  video.create(graphics_width, graphics_height, graphics_fullscreen, render_settings);
  video.scene_set(p_screen_surface->canvas_peek());

  // Show the (black) window right away
  video.present();

  redraw_reset();

  return(TRUE);
}

void graph_2d::screen_destroy(void)
{
  if(p_screen) {
    delete p_screen;
    p_screen = NULL;
  }

  if(p_screen_surface) {
    delete p_screen_surface;
    p_screen_surface = NULL;
  }
}

void graph_2d::screen_resize(tpos width, tpos height)
{
  graphics_width = width;
  graphics_height = height;

  screen_regenerate();
}

// Present the window again (it was exposed / resized / restored)
void graph_2d::present_if_needed(void)
{
  video.present_if_needed();
}

void graph_2d::check(void)
{
  int i;

  bprintf("Screen sprite %p, surface %p", p_screen, p_screen->surf_get());

  bprintf("Stored surfaces:");
  SURFACE *p_srlist = store.get_surface_interface();

  for(i = 0; i < store.get_surface_num(); i++) {
    SURFACE *p_srf = p_srlist+i;
    bprintf("%d Surface %p, image %s, used %d", i, p_srf,
            p_srf->image_get() ? p_srf->image_get()->name_get() : "(canvas)", p_srf->used_get());
  }

  bprintf("Stored sprites:");
  SPRITE *p_splist = store.sprite_interface_get();
  SPRITE *p_sp;

  for(i = 0; i < store.sprite_num_get(); i++) {
    p_sp = p_splist+i;
    if(p_sp->surf_get()) {
      RECT *p_rec = p_sp->rect_get();
      bprintf("%d Sprite %p, Surface %p, flag %d, rec = {%d, %d, %d, %d}",
      i, p_sp, p_sp->surf_get(), p_sp->flag_get(), p_rec->x, p_rec->y,
      p_rec->w, p_rec->h);
    }
  }
  bprintf("--- ALL ---");
}

void graph_2d::fullscreen_toggle(void)
{
  if(!video.fullscreen_set(!graphics_fullscreen)) {
    bprintf("Fullscreen switch failed!");
    return;
  }
  graphics_fullscreen = !graphics_fullscreen;
}

// -------------------------------------------------------
//   the font table interface
// -------------------------------------------------------

bool font_lookup_table::load(char *p_file)
{  
  FHANDLE f = file_open(surface::graphics_dir_get(),p_file,"r");
  
  memset(position,0,sizeof(position));
  
  int pos = 0;
  char line[10];
  while(file_gets(line,10,f)) {
    position[toupper(line[0])] = position[tolower(line[0])] = pos;
    pos++;
  }

  file_close(f);
  return(TRUE);
}

// -------------------------------------------------------
//   the font_info interface
// -------------------------------------------------------
GRAPH_2D * font_info::p_grf = NULL;

font_info::font_info(void)
{
  font_sprite_first = font_sprite_num = 0;
}

font_info::~font_info(void)
{
  p_grf->sprite_delete(font_sprite_first, font_sprite_num);
  font_sprite_first = font_sprite_num = 0;
}

bool font_info::load(int font_index, int first, int num)
{
  assert(!font_sprite_num && font_index < FONT_NUM);

  char  tmp[2000];
  sprintf(tmp, "font%d.spr", font_index);
  
  int i = p_grf->sprite_insert(tmp, first);
  if(i != num)
    return(FALSE);
  
  font_sprite_first = first;
  font_sprite_num = num;
  
  return(ftable.load(change_tail(tmp,".tab")));
}

void font_info::free(void)
{ 
  p_grf->sprite_delete(font_sprite_first, font_sprite_num);
}
// -------------------------------------------------------
//   the font_info interface
// -------------------------------------------------------
font::font(GRAPH_2D *p_graph)
{
  memset(this,0,sizeof(*this));

  font_info::p_grf = p_graph;
  align = MENU_LEFT;
}

font::~font(void)
{  
}

void font::print(char *p_string, RECT *p_res, int lines)
{
  if(!p_string)
    return;

  char *p_start = strdup(p_string);
  char *p_tmp = p_start;
  char *p_nl;

  FONT_INFO *p_font = finfo+font_selected;

  if(p_res) {
    p_res->x = ax;
    p_res->y = ay;
    p_res->w = p_res->h = 0;
  }

  while(*p_tmp) {
    p_nl = strchr(p_tmp, '\n');
    if(p_nl)
      *p_nl = '\0';
    
    tpos width_string = p_font->width_get(p_tmp);
    tpos height_string = p_font->height_get(p_tmp);
    tpos width_screen = p_font->screen_width_get();
    tpos px = ax+offset_x,
         py = ay+offset_y;
    
    switch(align) {
      case MENU_LEFT:
        break;
      case MENU_CENTER:
        px = (width_screen - width_string) / 2;
        break;
      case MENU_RIGHT:
        px -= width_string;
        break;
    }

    assert(px >= 0);
  
    if(p_res) {
      p_res->x = px;
      //bprintf("before print - [%d %d] -> [%d %d]",px, py, width_string, height_string);
      rect_adjust(p_res, px, py, width_string, height_string);
      //bprintf("after print - [%d %d] -> [%d %d]",p_res->x,p_res->y,p_res->w,p_res->h);
    }
  
    while(*p_tmp) {
      px += p_font->print(*p_tmp,px,py,!try_run);
      p_tmp++;
    }
 
    if(p_nl) {
      new_line();
      p_tmp = p_nl+1;
    }
  
    if(lines > 0) {
      lines--;
      if(!lines)
        break;
    }
  }

  ::free(p_start);
}

void font::print(RECT *p_res, tpos x, tpos y, const char *p_text,...)
{
  char      text[2000];
  va_list   arguments;  

  if(!p_text)
    return;

  va_start(arguments,p_text);
  vsnprintf(text,2000,p_text,arguments);
  va_end(arguments);
  
  start_set(x, y);
  print(text,p_res);
}

void font::print(RECT *p_res, const char *p_text,...)
{
  char      text[2000];
  va_list   arguments;  

  if(!p_text)
    return;

  va_start(arguments,p_text);
  vsnprintf(text,2000,p_text,arguments);
  va_end(arguments);
  
  print(text,p_res);
}

void font::print(RECT *p_res, tpos x, tpos y, int lines, const char *p_text,...)
{
  char      text[2000];
  va_list   arguments;  

  if(!p_text)
    return;
  
  va_start(arguments,p_text);
  vsnprintf(text,2000,p_text,arguments);
  va_end(arguments);
  
  start_set(x, y);
  print(text,p_res,lines);
}

void rect_adjust(RECT *p_r, tpos x, tpos y, tpos w, tpos h)
{
  assert(p_r);

  if(x < p_r->x) {
    p_r->w += p_r->x - x;
    p_r->x = x;
  }
  if(y < p_r->y) {
    p_r->h += p_r->y - y;
    p_r->y = y;
  }
  if(p_r->x + p_r->w < x + w) {
    p_r->w = x + w - p_r->x;
  }
  if(p_r->y + p_r->h < y + h) {
    p_r->h = y + h - p_r->y;
  }
}

void rect_adjust(RECT *p_r, RECT *p_s)
{
  rect_adjust(p_r, p_s->x, p_s->y, p_s->w, p_s->h);
}

// -------------------------------------------------------
//   Global interfaces
// -------------------------------------------------------

GRAPH_2D *p_grf  = NULL;
FONT     *p_font = NULL;

void graphics_start(tpos dx, tpos dy, int depth, bool fullscreen)
{
  if(!p_grf) {
    p_grf = new GRAPH_2D(dx, dy, depth, fullscreen, render_settings_load(INI_FILE));
  } else {
    p_grf->screen_resize(dx, dy);
  }

  if(!p_font) {
    p_font = new FONT(p_grf);
  }
}

void graphics_stop(void)
{
  if(p_font)
    delete p_font;
  if(p_grf)
    delete p_grf;
}
