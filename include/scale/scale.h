/*
 * Copyright © 2007 Novell, Inc.
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

#ifndef _COMPIZ_SCALE_H
#define _COMPIZ_SCALE_H

#include <core/core.h>
#include <core/privatehandler.h>

#include <composite/composite.h>
#include <opengl/opengl.h>

#define COMPIZ_SCALE_ABI 0

class ScaleScreen;
class PrivateScaleScreen;
class ScaleWindow;
class PrivateScaleWindow;

class ScaleScreenInterface :
    public WrapableInterface<ScaleScreen, ScaleScreenInterface>
{
    public:
	virtual bool layoutSlotsAndAssignWindows ();

};

class ScaleScreen :
    public WrapableHandler<ScaleScreenInterface, 1>,
    public PrivateHandler<ScaleScreen, CompScreen, COMPIZ_SCALE_ABI>
{
    public:
	ScaleScreen (CompScreen *s);
	~ScaleScreen ();

	CompOption::Vector & getOptions ();
        bool setOption (const char *name, CompOption::Value &value);
	CompOption * getOption (const char *name);

	WRAPABLE_HND (0, ScaleScreenInterface, bool,
		      layoutSlotsAndAssignWindows)
	
	friend class ScaleWindow;
	friend class PrivateScaleScreen;
	friend class PrivateScaleWindow;

    private:
	PrivateScaleScreen *priv;
};

class ScaleWindowInterface :
    public WrapableInterface<ScaleWindow, ScaleWindowInterface>
{
    public:
	virtual void scalePaintDecoration (const GLWindowPaintAttrib &,
					   const GLMatrix &,
					   const CompRegion &, unsigned int);
	virtual bool setScaledPaintAttributes (GLWindowPaintAttrib &);
	virtual void scaleSelectWindow ();
};

class ScaleWindow :
    public WrapableHandler<ScaleWindowInterface, 3>,
    public PrivateHandler<ScaleWindow, CompWindow, COMPIZ_SCALE_ABI>
{
    public:
	ScaleWindow (CompWindow *w);
	~ScaleWindow ();

	WRAPABLE_HND (0, ScaleWindowInterface, void, scalePaintDecoration,
		      const GLWindowPaintAttrib &, const GLMatrix &,
		      const CompRegion &, unsigned int)
	WRAPABLE_HND (1, ScaleWindowInterface, bool, setScaledPaintAttributes,
		      GLWindowPaintAttrib &)
	WRAPABLE_HND (2, ScaleWindowInterface, void, scaleSelectWindow)
	
	friend class ScaleScreen;
	friend class PrivateScaleScreen;
	friend class PrivateScaleWindow;
	
    private:
	PrivateScaleWindow *priv;
};



#endif
