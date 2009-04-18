/**
 * Graphics

 * Copyright (C) 2007, 2008, 2009  Sylvain Beucler

 * This file is part of GNU FreeDink

 * GNU FreeDink is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.

 * GNU FreeDink is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h> /* putenv */
#include <string.h>
#include <math.h>

#include "SDL.h"
#include "SDL_image.h"
#include "SDL_rotozoom.h"

#include "freedink_xpm.h"
#include "io_util.h"
#include "gfx.h"
#include "gfx_fonts.h"
#include "gfx_tiles.h"
#include "gfx_sprites.h"
#include "gfx_utils.h"
#include "gfx_fade.h"
#include "init.h"
#include "paths.h"
#include "log.h"


/* Is the screen depth more than 8bit? */
int truecolor = 0;

// // DELETEME
// LPDIRECTDRAW            lpDD = NULL;           // DirectDraw object
// //LPDIxRECTDRAWSURFACE     lpDDSOne;       // Offscreen surface 1

// LPDIRECTDRAWSURFACE     lpDDSPrimary = NULL;   // DirectDraw primary surface
// LPDIRECTDRAWSURFACE     lpDDSBack = NULL;      // DirectDraw back surface

// LPDIRECTDRAWSURFACE     lpDDSTwo = NULL;       // Offscreen surface 2
// LPDIRECTDRAWSURFACE     lpDDSTrick = NULL;       // Offscreen surface 2
// LPDIRECTDRAWSURFACE     lpDDSTrick2 = NULL;       // Offscreen surface 2


SDL_Surface *GFX_lpDDSBack = NULL; /* Backbuffer and link to physical
				      screen*/

/* GFX_lpDDSTwo: holds the base scene */
/* Rationale attempt :*/
/* lpDDSTwo contains the background, which is reused for each new
   frame. It is overwritten when switching to another screen. However,
   it can change during a screen: 1) animated tiles (water & fire) 2)
   when a sprite is written on the background (eg when an enemy dies)
   3) with various hacks such as fill_screen() (and maybe
   copy_bmp_to_screen()). */
/* Those changes may conflict with each other (eg: an animated tile
   overwrites half the carcass of a dead enemy). We might want to fix
   that. */
/* After the background is done, all the other operations are applied
   on lpDDSBack, the double buffer which is directly used by the
   physical screen. */
SDL_Surface *GFX_lpDDSTwo = NULL;

/* Beuc: apparently used for the scrolling screen transition and more
   generaly as temporary buffers. Only used by the game, not the
   editor. */
/* Used in freedink.cpp only + as a local/independent temporary buffer
   in show_bmp&copy_bmp&process_show_bmp&load_sprite* */
SDL_Surface *GFX_lpDDSTrick = NULL;
/* Used in freedink.cpp and update_frame.cpp */
SDL_Surface *GFX_lpDDSTrick2 = NULL;


/* Reference palette: this is the canonical Dink palette, loaded from
   TS01.bmp (for freedink) and esplash.bmp (for freedinkedit). The
   physical screen may be changed (e.g. show_bmp()), but this
   canonical palette will stay constant. */
/* PALETTEENTRY  real_pal[256]; */
SDL_Color GFX_real_pal[256];

/** Game-specific **/
/* Palette change: with SDL, SDL_SetColors (aka
   SDL_SetPalette(SDL_PHYSPAL)) apparently triggers a Flip, which
   displays weird colors on the screen for a brief but displeasing
   moment. Besides, SDL_Flip() does not refresh the hardware palette,
   so update the physical palette needs to be done manually - but only
   when the surface is already in its final form. The palette may need
   to be changed before the screen content is ready, so we'll make the
   engine know when he needs to refresh the physical palette: */
/* Tell flip_it* to install the new palette */
int trigger_palette_change = 0;
SDL_Color cur_screen_palette[256];

/* True color fade in [0,256]; 0 is completely dark, 256 is unaltered */
double truecolor_fade_brightness = 256;
/* Time elapsed since last fade computation; -1 is disabled */
Uint32 truecolor_fade_lasttick = -1;


static int cur_video_flags = 0;

/**
 * Check if the graphics system is initialized, so we know if we can
 * use it to display error messages to the user
 */
static enum gfx_init_state init_state = GFX_NOT_INITIALIZED;
enum gfx_init_state gfx_get_init_state()
{
  return init_state;
}


void gfx_dumpflags(Uint32 flags)
{
  char buf[256]; // enough to display all flags

  sprintf(buf, "0x%8.8x", flags);

  strcat(buf, " ");
  int i = 0;
  for (; i < 32; i++)
    {
      unsigned int tflag = 1 << i;
      if ((flags & tflag) == tflag)
	strcat(buf, "1");
      else
	strcat(buf, "0");
    }

  if (flags & SDL_HWSURFACE)
    strcat(buf, " SDL_HWSURFACE");
  else
    strcat(buf, " SDL_SWSURFACE");
	
  if (flags & SDL_HWPALETTE)
    strcat(buf, " | SDL_HWPALETTE");
  
  if (flags & SDL_FULLSCREEN)
    strcat(buf, " | SDL_FULLSCREEN");
  
  if (flags & SDL_DOUBLEBUF)
    strcat(buf, " | SDL_DOUBLEBUF");
  
  if (flags & SDL_SRCCOLORKEY)
    strcat(buf, " | SDL_SRCCOLORKEY");
  
  if (flags & SDL_SRCALPHA)
    strcat(buf, " | SDL_SRCALPHA");
  
  if (flags & SDL_RLEACCEL)
    strcat(buf, " | SDL_RLEACCEL");
  
  if (flags & SDL_RLEACCELOK)
    strcat(buf, " | SDL_RLEACCELOK");

  log_info(buf);
}

/**
 * Graphics subsystem initalization
 */
int gfx_init(enum gfx_windowed_state windowed, char* splash_path)
{
  /* Initialization in progress */
  init_state = GFX_INITIALIZING_VIDEO;

  /* Init graphics subsystem */
  if (SDL_WasInit(SDL_INIT_VIDEO) == 0 && SDL_InitSubSystem(SDL_INIT_VIDEO) == -1)
    {
      init_set_error_msg("Video initialization error: %s", SDL_GetError());
      return -1;
    }

  const SDL_VideoInfo* info = SDL_GetVideoInfo();

  if (info->wm_available)
    {
      SDL_Surface *icon = NULL;
      SDL_WM_SetCaption(PACKAGE_STRING, NULL);
      
      if ((icon = IMG_ReadXPMFromArray(freedink_xpm)) == NULL)
	{
	  log_error("Error loading icon: %s", IMG_GetError());
	}
      else
	{
	  SDL_WM_SetIcon(icon, NULL);
	  SDL_FreeSurface(icon);
	}
    }


  /* SDL_HWSURFACE gives direct 2D memory access if that's possible */
  /* SDL_DOUBLEBUF is supposed to enable hardware double-buffering
     and is a pre-requisite for SDL_Flip to use hardware, see
     http://www.libsdl.org/cgi/docwiki.cgi/FAQ_20Hardware_20Surfaces_20Flickering */
  int flags = SDL_HWSURFACE | SDL_DOUBLEBUF;

  if (windowed != GFX_WINDOWED)
    flags |= SDL_FULLSCREEN;


  int bits_per_pixel = 8;
  if (truecolor)
    {
      /* Recommended depth: */
      log_info("Recommended depth is %d", info->vfmt->BitsPerPixel);
      bits_per_pixel = info->vfmt->BitsPerPixel;
      
      if (bits_per_pixel < 15)
	{
	  /* Running truecolor mode in 8bit resolution? Let's emulate,
	     the user must know what he's doing. */
	  bits_per_pixel = 15;
	  log_info("Emulating truecolor mode within 8bit mode");
	}
    }
  else
    {
      /* SDL_HWPALETTE makes sure we can use all the colors we need
	 (override system palette reserved colors?) */
      flags |= SDL_HWPALETTE;
    }
  log_info("Requesting depth %d", bits_per_pixel);

  putenv("SDL_VIDEO_CENTERED=1");
  putenv("SDL_ASPECT_RATIO=4:3"); /* used by PSP to keep aspect ratio */
  if (GFX_lpDDSBack == NULL)
    {
      /* Hardware mode */
      log_info("Requesting video flags: "); gfx_dumpflags(flags);
      GFX_lpDDSBack = SDL_SetVideoMode(640, 480, bits_per_pixel, flags);
      if (GFX_lpDDSBack == NULL)
	log_warn("Unable to use hardware mode: %s", SDL_GetError());
    }
  if (GFX_lpDDSBack == NULL)
    {
      /* Software mode - in theory SDL automatically fallbacks to
	 software mode if hardware mode isn't available, but some
	 architectures need to do it explicitely, e.g. PSP's
	 640x480stretch mode that only work with
	 SDL_SWSURFACE|SDL_FULLSCREEN */
      flags &= ~SDL_HWSURFACE;
      flags &= ~SDL_DOUBLEBUF;
      flags |= SDL_SWSURFACE;
      log_info("Requesting video flags: "); gfx_dumpflags(flags);
      GFX_lpDDSBack = SDL_SetVideoMode(640, 480, bits_per_pixel, flags);
      if (GFX_lpDDSBack == NULL)
	log_error("Unable to use software fullscreen mode: %s", SDL_GetError());
    }
  if (GFX_lpDDSBack == NULL)
    {
      init_set_error_msg("Unable to set 640x480 video: %s\n", SDL_GetError());
      return -1;
    }
  log_info("Obtained video flags:   "); gfx_dumpflags(flags);
  cur_video_flags = flags;

  char buf[1024];
  if (SDL_VideoDriverName(buf, 1024) != NULL)
    log_info("INFO: Video driver is '%s'", buf);
  else
    log_info("INFO: Unable to determine video driver name");
  if (GFX_lpDDSBack->flags & SDL_HWSURFACE)
    log_info("INFO: Using hardware video mode.");
  else
    log_info("INFO: Not using a hardware video mode.");
  log_info("INFO: SDL depth is %d", bits_per_pixel);


  /* Hide mouse */
  SDL_ShowCursor(SDL_DISABLE);

  /* Disable Alt-Tab and any other window-manager shortcuts */
  /* SDL_WM_GrabInput(SDL_GRAB_ON); */


  /* Default palette (may be used by early init error messages */
  setup_palette(cur_screen_palette);
  setup_palette(GFX_real_pal);
  if (!truecolor)
    SDL_SetPalette(GFX_lpDDSBack, SDL_LOGPAL|SDL_PHYSPAL, cur_screen_palette, 0, 256);


  /* Create and set the reference palette */
  if (load_palette_from_bmp("Tiles/Ts01.bmp", GFX_real_pal) < 0)
    log_error("Failed to load default palette from Tiles/Ts01.bmp");

  /* Physical palette (the one we can change to make visual effects) */
  change_screen_palette(GFX_real_pal);

  /* Initialize graphic buffers */
  /* When a new image is loaded in DX, it's color-converted using the
     main palette (possibly altering the colors to match the palette);
     currently we emulate that by wrapping SDL_LoadBMP, converting
     image to the internal palette at load time - and we never change
     the buffer's palette again, so we're sure there isn't any
     conversion even if we change the screen palette: */
  if (!truecolor)
    SDL_SetPalette(GFX_lpDDSBack, SDL_LOGPAL, cur_screen_palette, 0, 256);
  GFX_lpDDSTwo    = SDL_DisplayFormat(GFX_lpDDSBack);
  GFX_lpDDSTrick  = SDL_DisplayFormat(GFX_lpDDSBack);
  GFX_lpDDSTrick2 = SDL_DisplayFormat(GFX_lpDDSBack);


  /* Display splash picture, as early as possible */
  {
    char* fullpath = paths_dmodfile(splash_path);
    if (!exist(fullpath))
      {
	free(fullpath);
	fullpath = paths_fallbackfile(splash_path);
      }
    SDL_Surface* splash = load_bmp(fullpath);
    free(fullpath);
    if (splash == NULL)
      {
	log_error("Cannot load base graphics %s", splash_path);
      }
    else
      {
	/* Copy splash to the background buffer so that D-Mod can
	   start an effect from it (e.g. Pilgrim Quest's burning
	   splash screen effect) */
	SDL_BlitSurface(splash, NULL, GFX_lpDDSTwo, NULL);
	SDL_FreeSurface(splash);
      }
    
    /* Copy splash screen (again) to the screen during loading time */
    SDL_BlitSurface(GFX_lpDDSTwo, NULL, GFX_lpDDSBack, NULL);

    flip_it();
  }


  /* Fonts system, default fonts */
  init_state = GFX_INITIALIZING_FONTS;
  if (gfx_fonts_init() < 0)
    return -1; /* error message set in gfx_fonts_init */

  /* Compute fade cache if necessary */
  gfx_fade_init();

  /* Mouse */
  /* Center mouse and reset relative positionning */
  SDL_WarpMouse(320, 240);
  SDL_PumpEvents();
  SDL_GetRelativeMouseState(NULL, NULL);


  /* SDL_MouseMotionEvent: If the cursor is hidden (SDL_ShowCursor(0))
     and the input is grabbed (SDL_WM_GrabInput(SDL_GRAB_ON)), then
     the mouse will give relative motion events even when the cursor
     reaches the edge of the screen. This is currently only
     implemented on Windows and Linux/Unix-alikes. */
  /* So it's not portable and it blocks Alt+Tab, so let's try
     something else - maybe enable it as a command line option. */
  /* SDL_WM_GrabInput(SDL_GRAB_ON); */

  /* make all pointers to NULL */
  memset(&GFX_tiles, 0, sizeof(GFX_tiles));
  memset(&k, 0, sizeof(k));
  memset(&GFX_k, 0, sizeof(GFX_k));
  memset(&seq, 0, sizeof(seq));

  /* Load the tiles from the BMPs */
  tiles_load_default();
  
  init_state = GFX_INITIALIZED;
  return 0;
}

/**
 * Failsafe graphics mode to display initialization error messages
 */
int gfx_init_failsafe()
{
  /* Init graphics subsystem */
  if (SDL_WasInit(SDL_INIT_VIDEO) == 0 && SDL_InitSubSystem(SDL_INIT_VIDEO) == -1)
    {
      log_fatal("Unable to init failsafe video: %s", SDL_GetError());
      return -1;
    }

  const SDL_VideoInfo* info = SDL_GetVideoInfo();
  if (info->wm_available)
    {
      SDL_WM_SetCaption(PACKAGE_STRING " - Initialization error", NULL);
      SDL_Surface *icon = IMG_ReadXPMFromArray(freedink_xpm);
      if (icon != NULL)
	{
	  SDL_WM_SetIcon(icon, NULL);
	  SDL_FreeSurface(icon);
	}
    }

  putenv("SDL_VIDEO_CENTERED=1");
  putenv("SDL_ASPECT_RATIO=4:3"); /* used by PSP to keep aspect ratio */
#ifdef _PSP
  //GFX_lpDDSBack = SDL_SetVideoMode(480, 272, 32, SDL_HWSURFACE | SDL_FULLSCREEN);
  GFX_lpDDSBack = SDL_SetVideoMode(640, 480, 32, SDL_SWSURFACE | SDL_FULLSCREEN);
#else
  GFX_lpDDSBack = SDL_SetVideoMode(640, 480, 0, SDL_DOUBLEBUF);
#endif
  if (GFX_lpDDSBack == NULL)
    {
      log_fatal("Unable to set failsafe video mode: %s", SDL_GetError());
      return -1;
    }

  setup_palette(cur_screen_palette);
  setup_palette(GFX_real_pal);
  SDL_SetPalette(GFX_lpDDSBack, SDL_LOGPAL|SDL_PHYSPAL, cur_screen_palette, 0, 256);

  return gfx_fonts_init_failsafe();
}

/**
 * Unload graphics subsystem
 */
void gfx_quit()
{
  init_state = GFX_QUITTING;

  gfx_fade_quit();

  gfx_fonts_quit();

  tiles_unload_all();
  sprites_unload();
  
  if (GFX_lpDDSBack   != NULL) SDL_FreeSurface(GFX_lpDDSBack);
  if (GFX_lpDDSTwo    != NULL) SDL_FreeSurface(GFX_lpDDSTwo);
  if (GFX_lpDDSTrick  != NULL) SDL_FreeSurface(GFX_lpDDSTrick);
  if (GFX_lpDDSTrick2 != NULL) SDL_FreeSurface(GFX_lpDDSTrick2);

  GFX_lpDDSBack = NULL;
  GFX_lpDDSTwo = NULL;
  GFX_lpDDSTrick = NULL;
  GFX_lpDDSTrick2 = NULL;

  init_state = GFX_NOT_INITIALIZED;
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}


/* Schedule a change to the physical screen's palette for the next
   frame */
void change_screen_palette(SDL_Color* new_palette)
{
  /* Now this one is tricky: DX/Woe has a "feature" where palette
     indexes 0 and 255 are fixed to black and white,
     respectively. This is the opposite of the default Dink palette
     - which is why fill_screen(0) is black and not white as in the
     Dink palette. This also makes "Lyna's Story"'s palette change a
     bit ugly, because pure black and white colors are not reversed
     when you enter "negative" color mode. This does not affect
     other indexes. Technically this happens when you get a palette
     from GetEntries(), and when you CreatePalette() without
     specifying DDPCAPS_ALLOW256 (so respectively, in
     change_screen_palette() and load_palette_from_*()). But well,
     reproducing the bug is important for backward compatibility. */
  
  memcpy(cur_screen_palette, new_palette, sizeof(cur_screen_palette));
  
  cur_screen_palette[0].r = 0;
  cur_screen_palette[0].g = 0;
  cur_screen_palette[0].b = 0;
  cur_screen_palette[255].r = 255;
  cur_screen_palette[255].g = 255;
  cur_screen_palette[255].b = 255;
  
  /* Applying the logical palette to the physical screen may trigger
     a Flip, so don't do it right now */
  trigger_palette_change = 1;
}


/* LoadBMP wrapper. Load a new graphic from file, and apply the
   reference palette so that all subsequent blits are faster (color
   convertion is avoided) - although the initial loading time will be
   somewhat longer. */
static SDL_Surface* load_bmp_internal(char *filename, SDL_RWops *rw, int from_mem, int setpal) {
  SDL_Surface *image;

  if (from_mem == 1)
    {
      image = IMG_Load_RW(rw, 1);
    }
  else
    {
      ciconvert(filename);
      image = IMG_Load(filename);
    }

  if (image == NULL)
    {
      /* fprintf(stderr, "load_bmp_internal: %s\n", SDL_GetError()); */
      /* Maybe it's just because we're at the end of a sequence */
      return NULL;
    }

  if (!truecolor)
    {
      /* Make a copy of the surface using the screen format (in
	 particular: same color depth, which is needed when importing
	 24bit graphics in 8bit mode) */
      /* converted = SDL_ConvertSurface(image, image->format, image->flags); */
      SDL_Surface *converted = SDL_DisplayFormat(image);

      int palette_is_applied = 0;
      if (setpal == 1)
	{
	  SDL_Color palette[256];
	  if (!(load_palette_from_surface(image, palette) < 0))
	    {
	      change_screen_palette(palette);
	      /* Pretend that the image uses the current screen and
		 buffers palette, to avoid color conversion to the
		 reference palette (maintain palette indexes). We maintain
		 palette indexes so that they will match the physical
		 screen's palette, which we just change. */
	      /* Note: cur_screen_palette is not exactly the same as
		 palette, because DX reserves some indexes, and FreeDink
		 reimplement this limitation for compatibility. So we
		 still need a blit with color convertion to take reserved
		 indexes into account. Typically skipping this step will
		 reverse black and white (with Dink palette indexes: 255
		 and 0; with DX reserved indexes: 0 and 255). */
	      SDL_SetPalette(converted, SDL_LOGPAL, cur_screen_palette, 0, 256);
	      palette_is_applied = 1;
	    }
	}
      if (!palette_is_applied)
	{
	  /* Prepare a color conversion to the reference palette */
	  SDL_SetPalette(converted, SDL_LOGPAL, GFX_real_pal, 0, 256);
	}

      /* Blit the copy back to the original, with a potentially different
	 palette, which triggers color conversion to image's palette. */
      SDL_BlitSurface(image, NULL, converted, NULL);
  
      /* In the end, the image must use the reference palette: that way no
	 mistaken color conversion will occur during blits to other
	 surfaces/buffers. Blits should also be faster(?). */
      SDL_SetPalette(converted, SDL_LOGPAL, GFX_real_pal, 0, 256);


      SDL_FreeSurface(image);
      image = NULL;

      return converted;
    }
  else
    {
      /* In truecolor mode, converting a 8bit image to truecolor does
	 not bring noticeable performance increase or decrease, but
	 does increase memory usage by at least 10MB so let's use the
	 loaded image as-is. No need for palette conversion either. */
      return image;
    }

}

/* LoadBMP wrapper, from file */
SDL_Surface* load_bmp(char *filename)
{
  return load_bmp_internal(filename, NULL, 0, 0);
}

/* LoadBMP wrapper, from FILE pointer */
SDL_Surface* load_bmp_from_fp(FILE* in)
{
  if (in == NULL)
    return NULL;
  SDL_RWops *rw = SDL_RWFromFP(in, /*autoclose=*/1);
  return load_bmp_internal(NULL, rw, 1, 0);
}

/* LoadBMP wrapper, from memory */
SDL_Surface* load_bmp_from_mem(SDL_RWops *rw)
{
  return load_bmp_internal(NULL, rw, 1, 0);
}

/* LoadBMP wrapper + use as current palette */
SDL_Surface* load_bmp_setpal(FILE* in)
{
  if (in == NULL)
    return NULL;
  SDL_RWops *rw = SDL_RWFromFP(in, /*autoclose=*/1);
  return load_bmp_internal(NULL, rw, 1, 1);
}

/**
 * Temporary disable src's transparency and blit it to dst
 */
int gfx_blit_nocolorkey(SDL_Surface *src, SDL_Rect *src_rect,
			 SDL_Surface *dst, SDL_Rect *dst_rect)
{
  int retval = -1;

  Uint32 colorkey_flags, colorkey, alpha_flags, alpha;
  colorkey_flags = src->flags & (SDL_SRCCOLORKEY|SDL_RLEACCEL);
  colorkey = src->format->colorkey;
  alpha_flags = src->flags & (SDL_SRCALPHA|SDL_RLEACCEL);
  alpha = src->format->alpha;
  SDL_SetColorKey(src, 0, -1);
  SDL_SetAlpha(src, 0, -1);
  
  retval = SDL_BlitSurface(src, src_rect, dst, dst_rect);
  
  SDL_SetColorKey(src, colorkey_flags, colorkey);
  SDL_SetAlpha(src, alpha_flags, alpha);

  return retval;
}

/**
 * Blit and resize so that 'src' fits in 'dst_rect'
 */
int gfx_blit_stretch(SDL_Surface *src_surf, SDL_Rect *src_rect,
		     SDL_Surface *dst_surf, SDL_Rect *dst_rect)
{
  int retval = -1;

  SDL_Rect src_rect_if_null;
  if (src_rect == NULL)
    {
      src_rect = &src_rect_if_null;
      src_rect->x = 0;
      src_rect->y = 0;
      src_rect->w = src_surf->w;
      src_rect->h = src_surf->h;
    }

  double sx = 1.0 * dst_rect->w / src_rect->w;
  double sy = 1.0 * dst_rect->h / src_rect->h;
  /* In principle, double's are precise up to 15 decimal digits */
  if (fabs(sx-1) > 1e-10 || fabs(sy-1) > 1e-10)
    {
      SDL_Surface *scaled = zoomSurface(src_surf, sx, sy, SMOOTHING_OFF);

      /* Keep the same transparency / alpha parameters (SDL_gfx bug,
	 report submitted to the author: SDL_gfx adds transparency to
	 non-transparent surfaces) */
      int colorkey_flag = src_surf->flags & SDL_SRCCOLORKEY;
      Uint8 r, g, b, a;
      SDL_GetRGBA(src_surf->format->colorkey, src_surf->format, &r, &g, &b, &a);

      SDL_SetColorKey(scaled, colorkey_flag,
		      SDL_MapRGBA(scaled->format, r, g, b, a));
      /* Don't mess with alpha transparency, though: */
      /* int alpha_flag = src->flags & SDL_SRCALPHA; */
      /* int alpha = src->format->alpha; */
      /* SDL_SetAlpha(scaled, alpha_flag, alpha); */
      
      src_rect->x = (int) round(src_rect->x * sx);
      src_rect->y = (int) round(src_rect->y * sy);
      src_rect->w = (int) round(src_rect->w * sx);
      src_rect->h = (int) round(src_rect->h * sy);
      retval = SDL_BlitSurface(scaled, src_rect, dst_surf, dst_rect);
      SDL_FreeSurface(scaled);
    }
  else
    {
      /* No scaling */
      retval = SDL_BlitSurface(src_surf, src_rect, dst_surf, dst_rect);
    }
  return retval;
}


/**
 * Refresh the physical screen, and apply a new palette or fade effect
 * if needed
 */
void flip_it(void)
{
  /* We work directly on either lpDDSBack (no lpDDSPrimary as in
     the original game): the double buffer (Back) is directly
     managed by SDL; SDL_Flip is used to refresh the physical
     screen. */
  if (!truecolor && trigger_palette_change)
    {
      // Apply the logical palette to the physical screen. This
      // may trigger a Flip (so don't do that until Back is
      // ready), but not necessarily (so do a Flip anyway).
      SDL_SetPalette(GFX_lpDDSBack, SDL_PHYSPAL,
		     cur_screen_palette, 0, 256);
    }
  trigger_palette_change = 0;

  if (truecolor_fade_brightness < 256)
    {
      gfx_fade_apply(truecolor_fade_brightness);
    }

  SDL_Flip(GFX_lpDDSBack);
}

/* Like SDL_WM_ToggleFullScreen(), except that it works under more
   platforms */
void gfx_toggle_fullscreen(void)
{
  if ((cur_video_flags & SDL_FULLSCREEN) == SDL_FULLSCREEN)
    cur_video_flags &= ~SDL_FULLSCREEN;
  else
    cur_video_flags |= SDL_FULLSCREEN;
  GFX_lpDDSBack = SDL_SetVideoMode(640, 480, GFX_lpDDSBack->format->BitsPerPixel, cur_video_flags);

  /* Palette was lost in the process */
  if (!truecolor)
    SDL_SetPalette(GFX_lpDDSBack, SDL_LOGPAL, GFX_real_pal, 0, 256);
  trigger_palette_change = 1;
}

/**
 * Print GFX memory usage
 */
void gfx_log_meminfo()
{
  int total = 0;

  {
    int sum = 0;
    sum = GFX_lpDDSBack->h * GFX_lpDDSBack->pitch;
    log_debug("GFX screen = %8d\n", sum);
    total += sum;
  }
  
  {
    int sum = 0;
    SDL_Surface* s = NULL;
    s = GFX_lpDDSTwo;
    sum += s->h * s->pitch;
    s = GFX_lpDDSTrick;
    sum += s->h * s->pitch;
    s = GFX_lpDDSTrick2;
    sum += s->h * s->pitch;
    log_debug("GFX buf    = %8d\n", sum);
    total += sum;
  }
  
  {
    int sum = 0;
    int i = 0;
    SDL_Surface* s = NULL;
    for (; i < MAX_SPRITES; i++)
      {
	s = GFX_k[i].k;
	if (s != NULL)
	  sum += s->h * s->pitch;
	// Note: this does not take SDL_RLEACCEL into account
      }
    log_debug("GFX bmp    = %8d\n", sum);
    total += sum;
  }

  {
    int sum = 0;
    int i = 0;
    SDL_Surface* s = NULL;
    for (; i < NB_TILE_SCREENS+1; i++)
      {
	s = GFX_tiles[i];
	if (s != NULL)
	  sum += s->h * s->pitch;
      }
    log_debug("GFX tiles  = %8d\n", sum);
    total += sum;
  }

  log_debug("GFX total  = %8d (+ ~150kB fonts)\n", total);
}
