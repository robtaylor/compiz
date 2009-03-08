/*
 * Copyright © 2008 Danny Baumann
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Danny Baumann not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Danny Baumann makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DANNY BAUMANN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DENNIS KASPRZYK BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Danny Baumann <dannybaumann@web.de>
 */

#include <core/core.h>
#include <core/privatehandler.h>

#include <composite/composite.h>
#include <opengl/opengl.h>

#define OBS_OPTION_OPACITY_INCREASE_KEY	       0
#define OBS_OPTION_OPACITY_INCREASE_BUTTON     1
#define OBS_OPTION_OPACITY_DECREASE_KEY	       2
#define OBS_OPTION_OPACITY_DECREASE_BUTTON     3
#define OBS_OPTION_SATURATION_INCREASE_KEY     4
#define OBS_OPTION_SATURATION_INCREASE_BUTTON  5
#define OBS_OPTION_SATURATION_DECREASE_KEY     6
#define OBS_OPTION_SATURATION_DECREASE_BUTTON  7
#define OBS_OPTION_BRIGHTNESS_INCREASE_KEY     8
#define OBS_OPTION_BRIGHTNESS_INCREASE_BUTTON  9
#define OBS_OPTION_BRIGHTNESS_DECREASE_KEY     10
#define OBS_OPTION_BRIGHTNESS_DECREASE_BUTTON  11
#define OBS_OPTION_OPACITY_STEP                12
#define OBS_OPTION_SATURATION_STEP             13
#define OBS_OPTION_BRIGHTNESS_STEP             14
#define OBS_OPTION_OPACITY_MATCHES             15
#define OBS_OPTION_OPACITY_VALUES              16
#define OBS_OPTION_SATURATION_MATCHES          17
#define OBS_OPTION_SATURATION_VALUES           18
#define OBS_OPTION_BRIGHTNESS_MATCHES          19
#define OBS_OPTION_BRIGHTNESS_VALUES           20
#define OBS_OPTION_NUM                         21

#define MODIFIER_OPACITY    0
#define MODIFIER_SATURATION 1
#define MODIFIER_BRIGHTNESS 2
#define MODIFIER_COUNT      3

class ObsScreen :
    public ScreenInterface,
    public PrivateHandler<ObsScreen, CompScreen>
{
    public:
	ObsScreen (CompScreen *);

	void matchPropertyChanged (CompWindow *);
	void matchExpHandlerChanged ();

	CompOption::Vector & getOptions ();
	bool setOption (const char *, CompOption::Value &);

	CompOption *stepOptions[MODIFIER_COUNT];
	CompOption *matchOptions[MODIFIER_COUNT];
	CompOption *valueOptions[MODIFIER_COUNT];

    private:
	CompOption::Vector opt;
};

class ObsWindow :
    public GLWindowInterface,
    public PrivateHandler<ObsWindow, CompWindow>
{
    public:
	ObsWindow (CompWindow *);

	bool glPaint (const GLWindowPaintAttrib &, const GLMatrix &,
		      const CompRegion &, unsigned int);
	bool glDraw (const GLMatrix &, GLFragment::Attrib &,
		     const CompRegion &, unsigned int);

	void changePaintModifier (unsigned int, int);
	void updatePaintModifier (unsigned int);
	void modifierChanged (unsigned int);

    private:
	CompWindow      *window;
	CompositeWindow *cWindow;
	GLWindow        *gWindow;
	ObsScreen       *oScreen;

	int customFactor[MODIFIER_COUNT];
	int matchFactor[MODIFIER_COUNT];
};

class ObsPluginVTable :
    public CompPlugin::VTableForScreenAndWindow<ObsScreen, ObsWindow>
{
    public:
	bool init ();

	PLUGIN_OPTION_HELPER (ObsScreen);
};
