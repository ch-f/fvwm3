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

#include "config.h"

#include <stdio.h>

#include "fvwm.h"
#include "cursor.h"
#include "functions.h"
#include "libs/fvwmlib.h"
#include "bindings.h"
#include "misc.h"
#include "screen.h"
#ifdef HAVE_STROKE
#include "stroke.h"
#endif /* HAVE_STROKE */

#define MODS_UNUSED_DEFAULT LockMask
static int mods_unused = MODS_UNUSED_DEFAULT;


/****************************************************************************
 *
 *  Parses a mouse or key binding
 *
 ****************************************************************************/
Binding *ParseBinding(Display *dpy, Binding **pblist, char *tline,
		      BindingType type, int *nr_left_buttons,
		      int *nr_right_buttons, unsigned char *buttons_grabbed)
{
  char *action, context[20], modifiers[20], *ptr, *token;
  char key[20] = { '\0' };
  int button = 0;
#ifdef HAVE_STROKE
  char stroke[MAX_SEQUENCE+1] = {'\0'};
#endif /* HAVE_STROKE */
  int n1=0,n2=0,n3=0;
#ifdef HAVE_STROKE
  int n4=0;
#endif /* HAVE_STROKE */
  int i;
  KeySym keysym = NoSymbol;
  int contexts;
  int mods;

  /* tline points after the key word "Mouse" or "Key" */
  ptr = GetNextToken(tline, &token);
  if(token != NULL)
  {
    if (type == KEY_BINDING)
      n1 = sscanf(token,"%19s",key);
#ifdef HAVE_STROKE
    else if (type == STROKE_BINDING)
    {
      int num = 0;
      int j;

      n1 = 1;
      i = 0;
      if (token[0] == 'N' && token[1] != '\0')
	num = 1;
      j=i+num;
      while(n1 && token[j] != '\0' && i < MAX_SEQUENCE)
      {
	if (!isdigit(token[j]))
	    n1 = 0;
	if (num)
	{
	  /* Numeric pad to Telephone  */
	  if ('7' <= token[j] && token[j] <= '9')
	    token[j] -= 6;
	  else if ('1' <= token[j] && token[j] <= '3')
	    token[j] += 6;
	}
	stroke[i] = token[j];
	i++;
	j=i+num;
      }
      stroke[i] = '\0';
      if (strlen(token) > MAX_SEQUENCE+num)
      {
	fvwm_msg(WARN,"ParseBinding","To long stroke sequence in line %s"
		 "Only %i elements will be taken in account", 
		 tline, MAX_SEQUENCE);
      }
    }
#endif /* HAVE_STROKE */
    else
      n1 = sscanf(token,"%d",&button);
    free(token);
  }

#ifdef HAVE_STROKE
  if (type == STROKE_BINDING)
  {
    ptr = GetNextToken(ptr,&token);
    if(token != NULL)
    {
	  n4 = sscanf(token,"%d",&button);
	  free(token);
    }
  }
#endif /* HAVE_STROKE */

  ptr = GetNextToken(ptr,&token);
  if(token != NULL)
  {
    n2 = sscanf(token,"%19s",context);
    free(token);
  }

  action = GetNextToken(ptr,&token);
  if(token != NULL)
  {
    n3 = sscanf(token,"%19s",modifiers);
    free(token);
  }

  if(n1 != 1 || n2 != 1 || n3 != 1
#ifdef HAVE_STROKE
     || (type == STROKE_BINDING && n4 != 1)
#endif /* HAVE_STROKE */
    )
  {
    fvwm_msg(ERR,"ParseBinding","Syntax error in line %s", tline);
    return NULL;
  }

  if (ParseContext(context, &contexts))
    fvwm_msg(WARN,"ParseBinding","Illegal context in line %s", tline);
  if (ParseModifiers(modifiers, &mods))
    fvwm_msg(WARN,"ParseBinding","Illegal modifier in line %s", tline);

  if (type == KEY_BINDING)
    {
      /*
       * Don't let a 0 keycode go through, since that means AnyKey to the
       * XGrabKey call.
       */
      keysym = XStringToKeysym(key);
      if (keysym == NoSymbol)
	{
	  char c = 'X';
	  char d = 'X';

	  /* If the key name is in the form '<letter><digits>...' it's probably
	   * something like 'f10'. Convert the letter to upper case and try
	   * again. */
	  sscanf(key, "%c%c", &c, &d);
	  if (islower(c) && isdigit(d))
	    {
	      key[0] = toupper(key[0]);
	      keysym = XStringToKeysym(key);
	    }
	}
      if (keysym == NoSymbol || XKeysymToKeycode(dpy, keysym) == 0)
	return NULL;
    }

  /*
  ** strip leading whitespace from action if necessary
  */
  while (*action && (*action == ' ' || *action == '\t'))
    action++;

  /*
  ** is this an unbind request?
  */
  if (!action || action[0] == '-')
  {
#ifdef HAVE_STROKE
    if (type == KEY_BINDING)
      RemoveBinding(dpy, pblist, KEY_BINDING, 0, 0, keysym, mods, contexts);
    else if (type == STROKE_BINDING)
      RemoveBinding(dpy, pblist, STROKE_BINDING, stroke, button, 0, mods,
		    contexts);
    else
      RemoveBinding(dpy, pblist, MOUSE_BINDING, 0, button, 0, mods, contexts);
#else
    if (type == KEY_BINDING)
      RemoveBinding(dpy, pblist, KEY_BINDING, 0, keysym, mods, contexts);
    else
      RemoveBinding(dpy, pblist, MOUSE_BINDING, button, 0, mods, contexts);
#endif /* HAVE_STROKE */
    return NULL;
  }

  if (type == MOUSE_BINDING)
    {
      int j;

      if((contexts != C_ALL) && (contexts & C_LALL) &&
	 (nr_left_buttons != NULL))
	{
	  /* check for nr_left_buttons */
	  i=0;
	  j=(contexts &C_LALL)/C_L1;
	  while(j>0)
	    {
	      i++;
	      j=j>>1;
	    }
	  if(*nr_left_buttons < i)
	    *nr_left_buttons = i;
	}
      if((contexts != C_ALL) && (contexts & C_RALL) &&
	 (nr_right_buttons != NULL))
	{
	  /* check for nr_right_buttons */
	  i=0;
	  j=(contexts&C_RALL)/C_R1;
	  while(j>0)
	    {
	      i++;
	      j=j>>1;
	    }
	  if(*nr_right_buttons < i)
	    *nr_right_buttons = i;
	}
    }

  if((mods & AnyModifier)&&(mods&(~AnyModifier)))
  {
    fvwm_msg(WARN,"ParseBinding","Binding specified AnyModifier and other "
	     "modifers too. Excess modifiers will be ignored.");
    mods &= AnyModifier;
  }

  if((type == MOUSE_BINDING)&&(contexts & C_WINDOW)&&
     (((mods==0)||mods == AnyModifier)) && (buttons_grabbed != NULL))
  {
    *buttons_grabbed &= ~(1<<(button-1));
  }

#ifdef HAVE_STROKE
  return AddBinding(dpy, pblist, type, (void *)(stripcpy(stroke)), button, 
		    keysym, key, mods, contexts, (void *)(stripcpy(action)), 
		    NULL);
#else
  return AddBinding(dpy, pblist, type, button, keysym, key, mods, contexts,
		    (void *)(stripcpy(action)), NULL);
#endif /* HAVE_STROKE */
}

static void activate_binding(Binding *binding, BindingType type)
{
  FvwmWindow *t;

  if (fFvwmInStartup == True || binding == NULL)
    return;

  /* grab keys immediately */
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      if (binding->Context & (C_WINDOW|C_TITLE|C_RALL|C_LALL|C_SIDEBAR))
      {
	GrabWindowKey(dpy, t->frame, binding,
		      C_WINDOW|C_TITLE|C_RALL|C_LALL|C_SIDEBAR,
		      GetUnusedModifiers(), True);
      }
      if (binding->Context & C_ICON)
	{
	  if(t->icon_w != None)
	    GrabWindowKey(dpy, t->icon_w, binding, C_ICON,
			  GetUnusedModifiers(), True);
	  if(t->icon_pixmap_w != None)
	    GrabWindowKey(dpy, t->icon_pixmap_w, binding, C_ICON,
			  GetUnusedModifiers(), True);
	}
    }
}


void key_binding(F_CMD_ARGS)
{
  Binding *b;

  b = ParseBinding(dpy, &Scr.AllBindings, action, KEY_BINDING,
		   &Scr.nr_left_buttons, &Scr.nr_right_buttons,
		   &Scr.buttons2grab);
  activate_binding(b, KEY_BINDING);
}

void mouse_binding(F_CMD_ARGS)
{
  Binding *b;

  b = ParseBinding(dpy, &Scr.AllBindings, action, MOUSE_BINDING,
		   &Scr.nr_left_buttons, &Scr.nr_right_buttons,
		   &Scr.buttons2grab);
  activate_binding(b, MOUSE_BINDING);
}

#ifdef HAVE_STROKE
void stroke_binding(F_CMD_ARGS)
{
  Binding *b;

  b = ParseBinding(dpy, &Scr.AllBindings, action, STROKE_BINDING,
		   &Scr.nr_left_buttons, &Scr.nr_right_buttons,
		   &Scr.buttons2grab);
  activate_binding(b, STROKE_BINDING);
}
#endif /* HAVE_STROKE */

/* Removes all unused modifiers from in_modifiers */
unsigned int MaskUsedModifiers(unsigned int in_modifiers)
{
  return in_modifiers & ~mods_unused;
}

unsigned int GetUnusedModifiers(void)
{
  return mods_unused;
}

/***********************************************************************
 *
 *	SetModifierLocks : declares which X modifiers are actually locks
 *		and should be ignored when testing mouse/key binding modifiers.
 *	syntax :
 *		ModifierLocks [S][L][C][M][1][2][3][4][5]
 *
 *	S is shift
 *	L is caps lock
 *	C is control
 *	M is meta, the same as mod1
 *	1-5 are mod1-mod5
 *	The default is L25 (ie. mask out caps lock, num lock & scroll lock
 *	for XFree86 3.3.3.1
 *
 *	Benoit TRIQUET <benoit@adsl-216-100-248-201.dsl.pacbell.net> 2/21/99
 ***********************************************************************/
void ignore_modifiers(F_CMD_ARGS)
{
  char *token;

  mods_unused = 0;
  GetNextToken(action, &token);
  if (!token)
    return;

  if (StrEquals(token, "default"))
    {
      free(token);
      mods_unused = MODS_UNUSED_DEFAULT;
      return;
    }

  if (ParseModifiers(token, &mods_unused))
    fvwm_msg(ERR, "ignore_modifiers", "illegal modifier in line %s\n", action);

  free(token);
  return;
}
