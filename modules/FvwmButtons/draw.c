/*
 * Fvwmbuttons, copyright 1996, Jarl Totland
 *
 * This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1993, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.
 *
 */

/* This program is free software; you can redistribute it and/or modify
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
 */

/* ------------------------------- includes -------------------------------- */

#include "config.h"

#ifdef HAVE_SYS_BSDTYPES_H
#include <sys/bsdtypes.h> /* Saul */
#endif

#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef I18N_MB
#include <X11/Xlocale.h>
#endif
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>

#include "FvwmButtons.h"
#include "misc.h" /* ConstrainSize() */
#include "icons.h" /* ConfigureIconWindow() */
#include "button.h"
#include "draw.h"

/* ---------------- Functions that design and draw buttons ----------------- */

/**
*** RelieveButton()
*** Draws the relief pattern around a window.
**/
void RelieveButton(Window wn,int width,int x,int y,int w,int h,Pixel relief,
		   Pixel shadow,int rev)
{
  unsigned long gcm=0;
  XGCValues gcv;
  GC swapGC, reliefGC, shadowGC;

  if(!width)
    return;

  gcm=GCForeground;
  gcv.foreground=relief;
  XChangeGC(Dpy,NormalGC,gcm,&gcv);
  reliefGC=NormalGC;

  gcm=GCForeground;
  gcv.foreground=shadow;
  XChangeGC(Dpy,ShadowGC,gcm,&gcv);
  shadowGC=ShadowGC;

  if(width<0)
  {
    width=-width;
    swapGC=reliefGC;reliefGC=shadowGC;shadowGC=swapGC;
  }
  if(rev)
  {
    swapGC=reliefGC;reliefGC=shadowGC;shadowGC=swapGC;
  }

  RelieveRectangle(Dpy,wn,x,y,w-1,h-1,reliefGC,shadowGC,width);
}

/**
*** MakeButton()
*** To position subwindows in a button: icons and swallowed windows.
**/
void MakeButton(button_info *b)
{
  /* This is resposible for drawing the contents of a button, placing the
   * icon and/or swallowed item in the correct position inside potential
   * padding or frame.
   */
  int ih,iw,ix,iy;
  XFontStruct *font;

  if(!b)
  {
    fprintf(stderr,"%s: BUG: MakeButton called with NULL pointer\n",MyName);
    exit(2);
  }
  if(b->flags&b_Container)
  {
    fprintf(stderr,"%s: BUG: MakeButton called with container\n",MyName);
    exit(2);
  }

  if(!(b->flags&b_Icon) && (buttonSwallowCount(b)<3))
    return;

  /* Check if parent container has an icon as background */
  if (b->parent->c->flags&b_IconBack || b->parent->c->flags&b_IconParent)
    b->flags|=b_IconParent;

  font = buttonFont(b);

  GetInternalSize(b,&ix,&iy,&iw,&ih);

  /* At this point iw,ih,ix and iy should be correct. Now all we have to do is
     place title and iconwin in their proper positions */

  /* For now, use the old routine in icons.h for buttons with icons */
  if(b->flags&b_Icon)
    ConfigureIconWindow(b);

  /* For now, hardcoded window centered, title bottom centered, below window */
  else if(buttonSwallowCount(b)==3)
  {
    long supplied;
    if(!b->IconWin)
    {
      fprintf(stderr,"%s: BUG: Swallowed window has no IconWin\n",MyName);
      exit(2);
    }

    if(b->flags&b_Title && font && !(buttonJustify(b)&b_Horizontal))
      ih -= font->ascent+font->descent;

    b->icon_w=iw;
    b->icon_h=ih;

    if(iw>0 && ih>0)
    {
      if(!(buttonSwallow(b)&b_NoHints))
      {
	if(!XGetWMNormalHints(Dpy,b->IconWin,b->hints,&supplied))
	  b->hints->flags=0;
	ConstrainSize(b->hints,&b->icon_w,&b->icon_h);
      }
      if (b->flags & b_Right)
	ix += iw-b->icon_w;
      else if (!(b->flags & b_Left))
	ix += (iw-b->icon_w)/2;
      XMoveResizeWindow(Dpy,b->IconWin,ix,iy+(ih-b->icon_h)/2,
			b->icon_w,b->icon_h);
    }
    else
      XMoveWindow(Dpy,b->IconWin,2000,2000);
  }
}

/**
*** RedrawButton()
*** Writes out title, if any, and displays the bevel right, by calling
*** RelieveButton. If clean is nonzero, also clears background.
**/
void RedrawButton(button_info *b,int clean)
{
  int i,j,k,BH,BW;
  int f,x,y,px,py;
  int ix,iy,iw,ih;
  XFontStruct *font = buttonFont(b);
#ifdef I18N_MB
  /*
    there seems to be no I18N patch here
  XFontSet fontset = buttonFontSet(b);
  */
#endif
  XGCValues gcv;
  unsigned long gcm=0;
  int rev=0;

  BW = buttonWidth(b);
  BH = buttonHeight(b);
  buttonInfo(b,&x,&y,&px,&py,&f);
  GetInternalSize(b,&ix,&iy,&iw,&ih);

  /* This probably isn't the place for this, but it seems to work here and not
     elsewhere, so... */
  if((buttonSwallowCount(b)==3) && b->IconWin!=None)
    XSetWindowBorderWidth(Dpy,b->IconWin,0);

  /* ----------------------------------------------------------------------- */

  if(b->flags&b_Hangon || b==CurrentButton)  /* Hanging or held down by user */
    rev=1;
  if(b->flags&b_Action) /* If this is a Desk button that takes you to here.. */
  {
    int n=0;
    while(n<4 && (!b->action[n] || strncasecmp(b->action[n],"Desk",4)))
      n++;
    if(n<4)
    {
      k=sscanf(&b->action[n][4],"%d%d",&i,&j);
      if(k==2 && i==0 && j==new_desk)
	rev=1;
    }
  }

  RelieveButton(MyWindow,f,x,y,BW,BH,buttonHilite(b),buttonShadow(b),rev);

  /* ----------------------------------------------------------------------- */

  f=abs(f);

  if(clean && BW>2*f && BH>2*f)
  {
    gcm = GCForeground;
    gcv.foreground=buttonBack(b);
    XChangeGC(Dpy,NormalGC,gcm,&gcv);

    if(b->flags&b_Container)
    {
      int x1=x+f,y1=y+f;
      int w1=px,h1=py,w2=w1,h2=h1;
      int w=BW-2*f,h=BH-2*f;
      w2+=iw - b->c->num_columns*b->c->ButtonWidth;
      h2+=ih - b->c->num_rows*b->c->ButtonHeight;

      if(w1)
	XFillRectangle(Dpy,MyWindow,NormalGC,x1,y1,w1,h);
      if(w2)
	XFillRectangle(Dpy,MyWindow,NormalGC,x1+w-w2,y1,w2,h);
      if(h1)
	XFillRectangle(Dpy,MyWindow,NormalGC,x1,y1,w,h1);
      if(h2)
	XFillRectangle(Dpy,MyWindow,NormalGC,x1,y1+h-h2,w,h2);
    }
#if 0
    else if(!(b->flags&b_IconBack) && !(b->flags&b_IconParent) &&
	    !(b->flags&b_Swallow))
      XFillRectangle(Dpy,MyWindow,NormalGC,x+f,y+f,BW-2*f,BH-2*f);
#endif
  }

  /* ----------------------------------------------------------------------- */

  if(b->flags&b_Title)
  {
    if (font)
    {
      gcm = GCForeground | GCFont;
      gcv.foreground=buttonFore(b);
      gcv.font = font->fid;
      XChangeGC(Dpy,NormalGC,gcm,&gcv);
      DrawTitle(b,MyWindow,NormalGC);
    }
  }
}

/**
*** DrawTitle()
*** Writes out title.
**/
void DrawTitle(button_info *b,Window win,GC gc)
{
  int BH;
  int ix,iy,iw,ih;
  XFontStruct *font=buttonFont(b);
#ifdef I18N_MB
  XFontSet fontset = buttonFontSet(b);
#endif
  int justify=buttonJustify(b);
  int l,i,xpos;
  char *s;
  int just=justify&b_TitleHoriz; /* Left, center, right */

  BH = buttonHeight(b);

  GetInternalSize(b,&ix,&iy,&iw,&ih);

  /* ----------------------------------------------------------------------- */

  if(!(b->flags&b_Title) || !font)
    return;

  /* If a title is to be shown, truncate it until it fits */
  if(justify&b_Horizontal && !(b->flags & b_Right))
  {
    if(b->flags&b_Icon)
    {
      ix+=b->icon->width+buttonXPad(b);
      iw-=b->icon->width+buttonXPad(b);
    }
    else if (buttonSwallowCount(b)==3)
    {
      ix += b->icon_w+buttonXPad(b);
      iw -= b->icon_w+buttonXPad(b);
    }
  }

  s = b->title;
  l = strlen(s);
  i = XTextWidth(font,s,l);

  if(i>iw)
  {
    if(just==2)
    {
      while(i>iw && *s)
	i=XTextWidth(font,++s,--l);
    }
    else /* Left or center - cut off its tail */
    {
      while(i>iw && l>0)
	i=XTextWidth(font,s,--l);
    }
  }
  if(just==0 || ((justify&b_Horizontal) && (b->flags&b_Right))) /* Left */
    xpos=ix;
  else if(just==2) /* Right */
    xpos=max(ix,ix+iw-i);
  else /* Centered, I guess */
    xpos=ix+(iw-i)/2;

  if(*s && l>0 && BH>=font->descent+font->ascent) /* Clip it somehow? */
  {
    /* If there is more than the title, put it at the bottom */
    /* Unless stack flag is set, put it to the right of icon */
    if((b->flags&b_Icon || (buttonSwallowCount(b)==3)) &&
       !(justify&b_Horizontal))
    {
      XDrawString(Dpy, win, gc, xpos, iy+ih-font->descent, s, l);
      /* Shrink the space available for icon/window */
      ih-=font->descent+font->ascent;
    }
    /* Or else center vertically */
    else
    {
      XDrawString(
	Dpy, win, gc, xpos, iy+(ih+font->ascent-font->descent)/2, s, l);
    }
  }
}
