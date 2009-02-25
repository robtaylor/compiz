/*
 * Copyright (C) 2001 Havoc Pennington
 * Copyright (C) 2002, 2003 Red Hat, Inc.
 * Copyright (C) 2003 Rob Adams
 * Copyright (C) 2005 Novell, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <core/core.h>
#include <core/atoms.h>
#include <core/privatehandler.h>

#define PLACE_MODE_CASCADE  0
#define PLACE_MODE_CENTERED 1
#define PLACE_MODE_SMART    2
#define PLACE_MODE_MAXIMIZE 3
#define PLACE_MODE_RANDOM   4
#define PLACE_MODE_LAST     PLACE_MODE_RANDOM

#define PLACE_MOMODE_CURRENT    0
#define PLACE_MOMODE_POINTER    1
#define PLACE_MOMODE_ACTIVEWIN  2
#define PLACE_MOMODE_FULLSCREEN 3
#define PLACE_MOMODE_LAST       PLACE_MOMODE_FULLSCREEN

#define PLACE_OPTION_WORKAROUND         0
#define PLACE_OPTION_MODE               1
#define PLACE_OPTION_MULTIOUTPUT_MODE   2
#define PLACE_OPTION_FORCE_PLACEMENT    3
#define PLACE_OPTION_POSITION_MATCHES   4
#define PLACE_OPTION_POSITION_X_VALUES  5
#define PLACE_OPTION_POSITION_Y_VALUES  6
#define PLACE_OPTION_POSITION_CONSTRAIN 7
#define PLACE_OPTION_VIEWPORT_MATCHES   8
#define PLACE_OPTION_VIEWPORT_X_VALUES  9
#define PLACE_OPTION_VIEWPORT_Y_VALUES  10
#define PLACE_OPTION_NUM                11

#define PLACE_SCREEN(s) PlaceScreen *ps = PlaceScreen::get (s)

class PlaceScreen :
    public PrivateHandler<PlaceScreen, CompScreen>,
    public ScreenInterface
{
    public:
	PlaceScreen (CompScreen *screen);
	~PlaceScreen ();

	void handleEvent (XEvent *event);
	void handleScreenSizeChange (int width, int height);

	CompOption::Vector & getOptions ();
	bool setOption (const char *name, CompOption::Value &value);

	CompOption::Vector opt;
};

class PlaceWindow :
    public PrivateHandler<PlaceWindow, CompWindow>,
    public WindowInterface
{
    public:
	PlaceWindow (CompWindow *w);
	~PlaceWindow ();

	bool place (CompPoint &pos);
	void validateResizeRequest (unsigned int   &mask,
				    XWindowChanges *xwc,
				    unsigned int   source);

    private:
	typedef enum {
	    NoPlacement = 0,
	    PlaceOnly,
	    ConstrainOnly,
	    PlaceAndConstrain,
	    PlaceOverParent,
	    PlaceCenteredOnScreen
	} PlacementStrategy;

	void doPlacement (CompPoint &pos);
	bool windowIsPlaceRelevant (CompWindow *w);
	PlacementStrategy getStrategy ();
	const CompOutput & getPlacementOutput (PlacementStrategy strategy,
					       CompPoint pos);
	void sendMaximizationRequest ();
	void constrainToWorkarea (XRectangle &workArea, CompPoint &pos);

	void placeCascade (XRectangle &workArea, CompPoint &pos);
	void placeCentered (XRectangle &workArea, CompPoint &pos);
	void placeRandom (XRectangle &workArea, CompPoint &pos);
	void placeSmart (XRectangle &workArea, CompPoint &pos);

	bool cascadeFindFirstFit (CompWindowList &windows, XRectangle &workArea,
				  CompPoint &pos);
	void cascadeFindNext (CompWindowList &windows, XRectangle &workArea,
			      CompPoint &pos);

	bool matchPosition (CompPoint &pos, bool& keepInWorkarea);
	bool matchViewport (CompPoint &pos);

	bool matchXYValue (CompOption::Value::Vector &matches,
			   CompOption::Value::Vector &xValues,
			   CompOption::Value::Vector &yValues,
			   CompPoint &pos,
			   CompOption::Value::Vector *constrainValues = NULL,
			   bool *keepInWorkarea = NULL);

	CompWindow *window;
};

class PlacePluginVTable :
    public CompPlugin::VTableForScreenAndWindow<PlaceScreen, PlaceWindow>
{
    public:
	bool init ();

	PLUGIN_OPTION_HELPER (PlaceScreen);
};

typedef enum {
    NoPlacement = 0,
    PlaceOnly,
    ConstrainOnly,
    PlaceAndConstrain,
    PlaceOverParent,
    PlaceCenteredOnScreen
} PlacementStrategy;

