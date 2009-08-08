/*
 * Copyright © 2006 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Novell, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#include <cmath>
#include <cairo-xlib-xrender.h>

#include <core/core.h>
#include <opengl/opengl.h>
#include <composite/composite.h>

#include "annotate_options.h"

static int annoLastPointerX = 0;
static int annoLastPointerY = 0;

class AnnoScreen :
    public PluginClassHandler <AnnoScreen, CompScreen>,
    public ScreenInterface,
    public GLScreenInterface,
    public AnnotateOptions
{
    public:
	AnnoScreen (CompScreen *screen);
	~AnnoScreen ();

	CompositeScreen *cScreen;
	GLScreen *gScreen;

	CompScreen::GrabHandle grabIndex;

	Pixmap pixmap;
	GLTexture::List texture;
	cairo_surface_t *surface;
	cairo_t		*cairo;
	bool		content;
	bool		eraseMode;

	void handleEvent (XEvent *);

	bool glPaintOutput (const GLScreenPaintAttrib &,
			    const GLMatrix &, const CompRegion &,
			    CompOutput *, unsigned int);

	void
	cairoClear (cairo_t    *cr);

	cairo_t *
	cairoContext ();

	void
	setSourceColor (cairo_t	   *cr,
			unsigned short *color);

	void
	drawCircle (double	       xc,
		    double	       yc,
		    double	       radius,
		    unsigned short *fillColor,
		    unsigned short *strokeColor,
		    double	       strokeWidth);

	void
	drawRectangle (double	  x,
		       double	  y,
		       double	  w,
		       double	  h,
		       unsigned short *fillColor,
		       unsigned short *strokeColor,
		       double	  strokeWidth);

	void
	drawLine (double	     x1,
		  double	     y1,
		  double	     x2,
		  double	     y2,
		  double	     width,
		  unsigned short *color);

	void
	drawText (double	     		     x,
	          double	     		     y,
	          const char	     		     *text,
	          const char	     		     *fontFamily,
	          double	     		     fontSize,
	          cairo_font_slant_t	     	     fontSlant,
	          cairo_font_weight_t	     	     fontWeight,
	          unsigned short 		     *fillColor,
	          unsigned short 		     *strokeColor,
	          double	     		     strokeWidth);

/* Actions */

	bool
	draw (CompAction         *action,
	      CompAction::State  state,
	      CompOption::Vector options);

	bool
	initiate (CompAction         *action,
		  CompAction::State  state,
		  CompOption::Vector options);

	bool
	terminate (CompAction         *action,
		   CompAction::State  state,
		   CompOption::Vector options);

	bool
	eraseInitiate (CompAction         *action,
		       CompAction::State  state,
		       CompOption::Vector options);

	bool
	clear (CompAction         *action,
	       CompAction::State  state,
	       CompOption::Vector options);

	void
	handleMotionEvent (int	  xRoot,
			   int	  yRoot);

};

#define ANNO_SCREEN(s)							       \
    AnnoScreen *as = get (s);

class AnnoPluginVTable :
    public CompPlugin::VTableForScreen <AnnoScreen>
{
    public:
	bool init ();
};
