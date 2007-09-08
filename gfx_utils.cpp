/**
 * Graphics utilities: palettes...

 * Copyright (C) 2003  Shawn Betts
 * Copyright (C) 2003, 2004, 2007  Sylvain Beucler

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
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "SDL.h"
#include "io_util.h"

/* Get a colors palette from the specified image */
int
load_palette_from_bmp (char *file, SDL_Color *palette)
{
  int i;
  SDL_Surface *bmp;
  char tmp_filename[PATH_MAX];

  bmp = SDL_LoadBMP(ciconvertbuf(file, tmp_filename));
  /* bmp = IMG_Load (file); */
  if (bmp == NULL)
    {
      fprintf(stderr, "load_palette_from_bmp: couldn't open %s", file);
      return 0;
    }

  for (i = 0; i < bmp->format->palette->ncolors; i++)
    {
      palette[i].r = bmp->format->palette->colors[i].r;
      palette[i].g = bmp->format->palette->colors[i].g;
      palette[i].b = bmp->format->palette->colors[i].b;
    }

  /* Reproduce DX/Woe limitation (see change_screen_palette) */
  palette[0].r = 0;
  palette[0].g = 0;
  palette[0].b = 0;
  palette[255].r = 255;
  palette[255].g = 255;
  palette[255].b = 255;

  SDL_FreeSurface (bmp);
  return 1;
}


/* Generate a default/fallback palette */
/* TODO: maybe use the default Woe palette? */
void
setup_palette(SDL_Color *palette)
{
  int ncolors, i;
  int r, g, b;

  /* Allocate 256 color palette */
  ncolors = 256;

  /* Set a 3,3,2 color cube */
  for (r = 0; r < 8; ++r)
    {
      for (g = 0; g < 8; ++g)
	{
	  for (b = 0; b < 4; ++b)
	    {
	      i = ((r << 5) | (g << 2) | b);
	      palette[i].r = r << 5;
	      palette[i].g = g << 5;
	      palette[i].b = b << 6;
	    }
	}
    }
}
