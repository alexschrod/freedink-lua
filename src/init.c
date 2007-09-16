/**
 * System initialization, common to FreeDink and FreeDinkEdit

 * Copyright (C) 2007  Sylvain Beucler

 * This file is part of GNU FreeDink

 * GNU FreeDink is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.

 * GNU FreeDink is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with program; see the file COPYING. If not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "binreloc.h"
/* Msg */
#include "dinkvar.h"
#include "gfx.h"
#include "input.h"
#include "io_util.h"
#include "init.h"

/* Create a mask in MSB for using r,g,b as the transparent color */
Uint8 *create_mask_msb(SDL_Surface *source, Uint8 r, Uint8 g, Uint8 b) {
  Uint32 transparent;
  Uint32 *pixels;
  SDL_Surface *surface;
  Uint8 *mask;

  /* Convert surface to 32bit to ease parsing the pixel data */
  surface = SDL_CreateRGBSurface(SDL_SWSURFACE, source->w, source->h, 32,
			      0xff000000, 0x00ff0000, 0x0000ff00, 0x00000000);
  if(surface == NULL) {
    fprintf(stderr, "Could not convert surface to 32bit: %s", SDL_GetError());
    return NULL;
  }

  SDL_BlitSurface(source, NULL, surface, NULL);

  transparent = SDL_MapRGB(surface->format, r, g, b);

  if (SDL_MUSTLOCK(surface))
    SDL_LockSurface(surface);
  pixels = (Uint32*) surface->pixels;
  if (SDL_MUSTLOCK(surface))
    SDL_UnlockSurface(surface);
    
  /* 8 bits per Uint8 */
  mask = malloc(ceil(surface->w / 8.0) * surface->h);

  { 
    int i, row, col;
    i = -1;
    for (row = 0; row < surface->h; row++)
      {
	for (col = 0; col < surface->w; col++)
	  {
	    /* Shift to the next mask bit */
	    if (col % 8 == 0)
	      {
		i++;
		mask[i] = 0;
	      }
	    else
	      {
		mask[i] <<= 1;
	      }
	    
	    /* Set the current mask bit */
	    if (pixels[row*surface->w + col] != transparent)
	      mask[i] |= 0x01;
	  }
      }
  }
  SDL_FreeSurface(surface);
  return mask;
}


/* The goal is to replace freedink and freedinkedit's doInit() by a
   common init procedure. This procedure will also initialize each
   subsystem as needed (eg InitSound) */
int init(void)
{
  /* BinReloc */
  BrInitError error;
  if (br_init (&error) == 0 && error != BR_INIT_ERROR_DISABLED)
    {
      printf ("Warning: BinReloc failed to initialize (error code %d)\n", error);
      printf ("Will fallback to hardcoded default path.\n");
    }
  
  /* SDL */
  /* Init timer subsystem */
  if (SDL_Init(SDL_INIT_TIMER) == -1)
    {
      Msg("SDL_Init: %s\n", SDL_GetError());
      return 0;
    }

  /* TODO: move to gfx.cpp */
  /* Init graphics subsystem */
  if (SDL_InitSubSystem(SDL_INIT_VIDEO) == -1)
    {
      Msg("SDL_Init: %s\n", SDL_GetError());
      return 0;
    }

  {
    char *icon_file;
    SDL_WM_SetCaption(PACKAGE_STRING, NULL);

    if ((icon_file = find_data_file("dink.bmp")) != NULL)
      {
	SDL_Surface *icon;
	if ((icon = SDL_LoadBMP(icon_file)) == NULL)
	  fprintf(stderr, "Error loading %s: %s\n", icon_file, SDL_GetError());
	else
	  {
	    /* Sets the color key to black (ahem): */
	    /* SDL_SetColorKey(icon, SDL_SRCCOLORKEY, SDL_MapRGB(icon->format, 255, 255, 0)); */
	    /* Trying with a manual mask instead: */
	    Uint8 *mask;
	    mask = create_mask_msb(icon, 255, 255, 0); /* Yellow */
	    if (mask != NULL)
	      {
		SDL_WM_SetIcon(icon, mask);
		free(mask);
	      }
	    SDL_FreeSurface(icon);
	  }
	free(icon_file);
      }
  }

  /* TODO: is that portable? */
  putenv("SDL_VIDEO_CENTERED=1");

  /* SDL_HWSURFACE is supposed to give direct memory access */
  /* SDL_HWPALETTE makes sure we can use all the colors we need
     (override system palette reserved colors?) */
  /* SDL_DOUBLEBUF is supposed to enable hardware double-buffering
     and is a pre-requisite for SDL_Flip to use hardware, see
     http://www.libsdl.org/cgi/docwiki.cgi/FAQ_20Hardware_20Surfaces_20Flickering */
  if (windowed)
    GFX_lpDDSPrimary = SDL_SetVideoMode(640, 480, 8, SDL_HWSURFACE | SDL_HWPALETTE | SDL_DOUBLEBUF);
  else
    GFX_lpDDSPrimary = SDL_SetVideoMode(640, 480, 8, SDL_HWSURFACE | SDL_HWPALETTE | SDL_DOUBLEBUF | SDL_FULLSCREEN);
  if (GFX_lpDDSPrimary == NULL)
    {
      fprintf(stderr, "Unable to set 640x480 video: %s\n", SDL_GetError());
      exit(1);
    }
  if (GFX_lpDDSPrimary->flags & SDL_HWSURFACE)
    printf("INFO: Using hardware video mode.\n");
  else
    printf("INFO: Not using a hardware video mode.\n");

  // GFX
  GFX_lpDDSBack = SDL_CreateRGBSurface(SDL_SWSURFACE, 640, 480, 8,
  				       0, 0, 0, 0);

  // lpDDSTwo/Trick/Trick2 are initialized by loading SPLASH.BMP in
  // doInit()

  /* Hide mouse */
  SDL_ShowCursor(SDL_DISABLE);

  /* Disable Alt-Tab and any other window-manager shortcuts */
  /* SDL_WM_GrabInput(SDL_GRAB_ON); */


  // FONTS
  /* TODO: create a separate initialization procedure */
  TTF_Init();


  /* Mouse */
  /* Center mouse and reset relative positionning */
  SDL_WarpMouse(320, 240);
  SDL_PumpEvents();
  SDL_GetRelativeMouseState(NULL, NULL);


  /* We'll handle those events manually */
  SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
  SDL_EventState(SDL_VIDEORESIZE, SDL_IGNORE);
  SDL_EventState(SDL_VIDEOEXPOSE, SDL_IGNORE);
  SDL_EventState(SDL_USEREVENT, SDL_IGNORE);
  SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
  SDL_EventState(SDL_KEYDOWN, SDL_IGNORE);
  SDL_EventState(SDL_KEYUP, SDL_IGNORE);
  SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
  SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);
  /* We still process through a SDL_PollEvent() loop: */
  /* - SDL_QUIT: quit on window close and Ctrl+C */
  /* - SDL_MOUSEBUTTONDOWN: don't miss quick clicks */
  /* - Joystick: apparently we need to keep them, otherwise joystick
       doesn't work at all */


  /* SDL_MouseMotionEvent: If the cursor is hidden (SDL_ShowCursor(0))
     and the input is grabbed (SDL_WM_GrabInput(SDL_GRAB_ON)), then
     the mouse will give relative motion events even when the cursor
     reaches the edge of the screen. This is currently only
     implemented on Windows and Linux/Unix-alikes. */
  /* So it's not portable and it blocks Alt+Tab, so let's try
     something else - maybe enable it as a command line option. */
  /* SDL_WM_GrabInput(SDL_GRAB_ON); */


  /* Joystick */
  input_init();


  /* Maybe use SDL_QuitSubSystem instead */
  atexit(SDL_Quit);


  /* Engine */
  /* Clean the game state structure - done by C++ but not
     automatically done by C, and this causes errors. TODO: fix the
     errors properly instead of using this dirty trick. */
  memset(&play, 0, sizeof(play));

  memset(&hmap, 0, sizeof(hmap));
  memset(&pam, 0, sizeof(pam));

  return 1;
}
