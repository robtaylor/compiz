/*
 * Copyright © 2005 Novell, Inc.
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

#include <core/core.h>
#include <core/privatehandler.h>
#include <composite/composite.h>
#include <opengl/opengl.h>

#define FADE_OPTION_FADE_MODE		    0
#define FADE_OPTION_FADE_SPEED		    1
#define FADE_OPTION_FADE_TIME		    2
#define FADE_OPTION_WINDOW_MATCH	    3
#define FADE_OPTION_VISUAL_BELL		    4
#define FADE_OPTION_FULLSCREEN_VISUAL_BELL  5
#define FADE_OPTION_DIM_UNRESPONSIVE	    6
#define FADE_OPTION_UNRESPONSIVE_BRIGHTNESS 7
#define FADE_OPTION_UNRESPONSIVE_SATURATION 8
#define FADE_OPTION_NUM			    9

#define FADE_MODE_CONSTANTSPEED 0
#define FADE_MODE_CONSTANTTIME  1
#define FADE_MODE_MAX           FADE_MODE_CONSTANTTIME

class FadeScreen :
    public ScreenInterface,
    public CompositeScreenInterface,
    public PrivateHandler<FadeScreen, CompScreen>
{
    public:
	FadeScreen (CompScreen *s);

	CompOption::Vector & getOptions ();
	bool setOption (const char *, CompOption::Value &);

	void handleEvent (XEvent *);
	void preparePaint (int);

	int displayModals;
	int fadeTime;

	CompOption::Vector opt;

	CompositeScreen *cScreen;
};

class FadeWindow :
    public WindowInterface,
    public GLWindowInterface,
    public PrivateHandler<FadeWindow, CompWindow>
{
    public:
	FadeWindow (CompWindow *w);
	~FadeWindow ();

	void windowNotify (CompWindowNotify);
	void paintStep (unsigned int, int, int);

	bool glPaint (const GLWindowPaintAttrib&, const GLMatrix&,
		      const CompRegion&, unsigned int);

	void addDisplayModal ();
	void removeDisplayModal ();

	void dim (bool);

    private:
	FadeScreen      *fScreen;
	CompWindow      *window;
	CompositeWindow *cWindow;
	GLWindow        *gWindow;

	GLushort opacity;
	GLushort brightness;
	GLushort saturation;

	GLushort targetOpacity;
	GLushort targetBrightness;
	GLushort targetSaturation;

	bool dModal;

	int steps;
	int fadeTime;

	int opacityDiff;
	int brightnessDiff;
	int saturationDiff;
};

class FadePluginVTable :
    public CompPlugin::VTableForScreenAndWindow<FadeScreen, FadeWindow>
{
    public:
	bool init ();

	PLUGIN_OPTION_HELPER (FadeScreen);
};
