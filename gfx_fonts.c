/**
 * Fonts

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

#include <stdlib.h>
#include <string.h>
#include "SDL_ttf.h"
#include "dinkvar.h"
#include "gfx.h"
#include "gfx_fonts.h"

/* HFONT */
/* #include <windows.h> */

/* Global, shared by other modules */
TTF_Font *FONTS_hfont_small = NULL;
/* The current font, when activated through FONTS_SetFont() */
static TTF_Font *cur_font = NULL;

/* HFONT hfont_small = NULL; */

static SDL_Color text_color;


void FONTS_initfonts(char* fontname) {
  /* TODO: lf.lfWeight = 600; */
  /* TODO: lf.lfHeight = 18; */
  /* TODO: Load from Woe's system font dir if not found in the current
     directory */

  if (FONTS_hfont_small != NULL) {
    TTF_CloseFont(FONTS_hfont_small);
    FONTS_hfont_small = NULL;
  }

  /* SDL_ttf makes the font bigger than Woe, let's try 17 */
  FONTS_hfont_small = TTF_OpenFont(fontname, 17);
  FONTS_SetFont(FONTS_hfont_small);

  if (FONTS_hfont_small == NULL) {
    printf("TTF_OpenFont: %s\n", TTF_GetError());
    exit(1);
  }
}

void FONTS_SetTextColor(Uint8 r, Uint8 g, Uint8 b) {
  text_color.r = r;
  text_color.g = g;
  text_color.b = b;
}

void FONTS_SetFont(TTF_Font *font)
{
  cur_font = font;
}

void
print_text (TTF_Font * font, char *str, int x, int y, SDL_Color /*&*/color,
	    /*bool*/int hcenter)
{
  int new_x, w, h;
  SDL_Surface *tmp;
  SDL_Rect dst;

  if (strlen (str) == 0)
    return;
  
  /* Msg (("printing \"%s\"", str)); */
  
  /* Transparent, low quality - closest to the original engine. */
  tmp = TTF_RenderText_Solid(font, str, color);

  /* Bigger, with a box background */
  // SDL_Color background = {0, 0, 0};
  // tmp = TTF_RenderText_Shaded(font, str, color, background);
  
  /* High quality, 32bit+alpha, but I can't the transparency
     OK. Directly applying it on lpDDSBack messed with the colors of
     the graphics behind the text, and trying to pre-convert it by
     blitting it on a transparent surface missed some of the text
     borders in the final result (i.e. multiple calls to print_text
     with a small postponement to make the borders). */
  // tmp = TTF_RenderText_Blended(font, str, color);
  // {
  //   // SDL_Surface *conv = SDL_DisplayFormat(tmp);
  //   SDL_Surface *conv = SDL_CreateRGBSurface(SDL_SWSURFACE, tmp->w, tmp->h, 8,
  // 			     			0, 0, 0, 0);
  //   SDL_SetPalette(conv, SDL_LOGPAL, GFX_real_pal, 0, 256);
  //   SDL_SetColorKey(conv, SDL_SRCCOLORKEY, 0);
  //   SDL_FillRect(conv, NULL, 0);
  //   SDL_BlitSurface(tmp, NULL, conv, NULL);
  //   SDL_FreeSurface(tmp);
  //   tmp = conv;
  // }

  if (tmp == NULL)
    {
      printf("Error rendering text: %s; font is %p\n", TTF_GetError(), font);
    }

  TTF_SizeText (font, str, &w, &h);
  new_x = x;
  if (hcenter)
    new_x -= w / 2;

  dst.x = new_x; dst.y = y;
  SDL_BlitSurface(tmp, NULL, GFX_lpDDSBack, &dst);

  SDL_FreeSurface (tmp);
}

int
font_len (TTF_Font * font, char *str, int len)
{
  int text_w;
  char *tmp;

  /* Get the specified string portion and terminate it by \0 */
  tmp = (char *) malloc ((len + 1) * sizeof (char));
  strncpy (tmp, str, len);
  tmp[len] = 0;

  TTF_SizeText (font, tmp, &text_w, NULL);
  free (tmp);

  return text_w;
}

int
process_text_for_wrapping (TTF_Font * font, char *str, rect * box)
{
  int i, start, line;

  start = 0;
  i = 0;
  line = 0;
  while (str[i])
    {
      int len;

      /* Skip forward to the end of the word */
      do
	{
	  i++;
	}
      while (str[i] && str[i] != ' ');

      /* If the length of the text from start to i is bigger than the */
      /* box, then draw the text between start and i. */
      len = font_len (font, &str[start], i - start + 1);
      if (len > box->right - box->left || str[i] == 0)
	{
	  if (str[i])
	    str[i] = '\n';
	  /* continue on a new line */
	  line++;
	  start = i + 1;
	}

    }

  return line;
}

int
print_text_wrap (char *str, rect * box,
		 /*bool*/int hcenter, /*bool*/int vcenter)
{
  int x, y, lines, line;
  char *tmp, *token;
  //  SDL_Color color = {0, 0, 0};
  SDL_Color color = text_color;
  TTF_Font *font;
  /* TODO: check whether all those calls to SelectObject(hdc, font)
     are needed - or if they could be done in initfonts directly */
  font = cur_font;
  tmp = (char*) malloc(strlen(str) + 1);
  strcpy(tmp, str);
  /*   tmp = strdup (str); */
  lines = process_text_for_wrapping (font, tmp, box);

  token = strtok (tmp, "\n");

  x = box->left;
  if (hcenter)
    x += (box->right - box->left) / 2;
  y = box->top;
  if (vcenter)
    y += ((box->bottom - box->top) - lines * TTF_FontLineSkip (font)) / 2;
  line = 0;
  while (token)
    {
      print_text (font, token, x, y + line, color, hcenter);
      line += TTF_FontLineSkip (font);
      token = strtok (NULL, "\n");
    }
  free(tmp);
  return line;
}




/* Say, SaySmall: only used by freedinkedit.cpp */
/* SaySmall: print text in a 40x40 small square; without font
   border */
void SaySmall(char thing[500], int px, int py, int r,int g,int b)
{
  rect rcRect;
/*   HDC hdc; */
/*   if (lpDDSBack->GetDC(&hdc) == DD_OK) */
/*     {       */
/*       SetBkMode(hdc, TRANSPARENT);  */
      rect_set(&rcRect,px,py,px+40,py+40);
/*       SetTextColor(hdc,RGB(r,g,b)); */
/*       DrawText(hdc,thing,lstrlen(thing),&rcRect,DT_WORDBREAK); */
      // FONTS
      FONTS_SetTextColor(r, g, b);
      print_text_wrap(thing, &rcRect, 0, 0);
      
/*       lpDDSBack->ReleaseDC(hdc); */
/*     }    */
}
/* Say: print text until it reaches the border of the screen, with a
   font border */
void Say(char thing[500], int px, int py)
{
  rect rcRect;
/*   HDC hdc; */
  
/*   if (lpDDSBack->GetDC(&hdc) == DD_OK) */
/*     {       */
/*       SetBkMode(hdc, TRANSPARENT);  */
      rect_set(&rcRect,px,py,620,480);
/*       SelectObject (hdc, hfont_small); */

/*       SetTextColor(hdc,RGB(8,14,21)); */
/*       DrawText(hdc,thing,lstrlen(thing),&rcRect,DT_WORDBREAK); */
      // FONTS
      FONTS_SetTextColor(8, 14, 21);
      print_text_wrap(thing, &rcRect, 0, 0);

      rect_offset(&rcRect,-2,-2);
/*       DrawText(hdc,thing,lstrlen(thing),&rcRect,DT_WORDBREAK); */
      // FONTS
      print_text_wrap(thing, &rcRect, 0, 0);

      rect_offset(&rcRect,1,1);
/*       SetTextColor(hdc,RGB(255,255,0)); */
/*       DrawText(hdc,thing,lstrlen(thing),&rcRect,DT_WORDBREAK); */
      // FONTS
      FONTS_SetTextColor(255, 255, 0);
      print_text_wrap(thing, &rcRect, 0, 0);
      
/*       lpDDSBack->ReleaseDC(hdc); */
/*     }    */
}

void kill_fonts(void)
{
/*   if (hfont_small) */
/*     { */
/*       DeleteObject(hfont_small); */
/*       hfont_small = NULL; */
/*     } */
  
  if (FONTS_hfont_small)
    {
      TTF_CloseFont(FONTS_hfont_small);
      FONTS_hfont_small = NULL;
    }
}
