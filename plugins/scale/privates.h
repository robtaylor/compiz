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

#ifndef _SCALE_PRIVATES_H
#define _SCALE_PRIVATES_H

#include <scale/scale.h>

#define SCALE_ICON_NONE   0
#define SCALE_ICON_EMBLEM 1
#define SCALE_ICON_BIG    2
#define SCALE_ICON_LAST   SCALE_ICON_BIG

#define SCALE_MOMODE_CURRENT 0
#define SCALE_MOMODE_ALL     1
#define SCALE_MOMODE_LAST    SCALE_MOMODE_ALL

#define SCALE_OPTION_INITIATE_EDGE          0
#define SCALE_OPTION_INITIATE_BUTTON        1
#define SCALE_OPTION_INITIATE_KEY           2
#define SCALE_OPTION_INITIATE_ALL_EDGE      3
#define SCALE_OPTION_INITIATE_ALL_BUTTON    4
#define SCALE_OPTION_INITIATE_ALL_KEY       5
#define SCALE_OPTION_INITIATE_GROUP_EDGE    6
#define SCALE_OPTION_INITIATE_GROUP_BUTTON  7
#define SCALE_OPTION_INITIATE_GROUP_KEY     8
#define SCALE_OPTION_INITIATE_OUTPUT_EDGE   9
#define SCALE_OPTION_INITIATE_OUTPUT_BUTTON 10
#define SCALE_OPTION_INITIATE_OUTPUT_KEY    11
#define SCALE_OPTION_SHOW_DESKTOP           12
#define SCALE_OPTION_SPACING                13
#define SCALE_OPTION_SPEED                  14
#define SCALE_OPTION_TIMESTEP               15
#define SCALE_OPTION_WINDOW_MATCH           16
#define SCALE_OPTION_DARKEN_BACK            17
#define SCALE_OPTION_OPACITY                18
#define SCALE_OPTION_ICON                   19
#define SCALE_OPTION_HOVER_TIME             20
#define SCALE_OPTION_MULTIOUTPUT_MODE       21
#define SCALE_OPTION_NUM                    22


class ScaleSlot {
    public:
	int   x1, y1, x2, y2;
	bool  filled;
	float scale;
};

class SlotArea {
    public:
	int      nWindows;
	CompRect workArea;

	typedef std::vector<SlotArea> vector;
};


enum ScaleType {
    ScaleTypeNormal = 0,
    ScaleTypeOutput,
    ScaleTypeGroup,
    ScaleTypeAll
};

class PrivateScaleScreen :
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface
{
    public:
	PrivateScaleScreen (CompScreen *);
	~PrivateScaleScreen ();

	void handleEvent (XEvent *event);

	void preparePaint (int);
	void donePaint ();

	bool glPaintOutput (const GLScreenPaintAttrib &,
			    const GLMatrix &, const CompRegion &,
			    CompOutput *, unsigned int);

	void activateEvent (bool activating);

	void layoutSlotsForArea (const CompRect&, int);
	void layoutSlots ();
	void findBestSlots ();
	bool fillInWindows ();
	bool layoutThumbs ();

	SlotArea::vector getSlotAreas ();

	ScaleWindow * checkForWindowAt (int x, int y);

	void sendDndStatusMessage (Window);

	static bool scaleTerminate (CompAction         *action,
				    CompAction::State  state,
				    CompOption::Vector &options);
	static bool scaleInitiate (CompAction         *action,
				   CompAction::State  state,
				   CompOption::Vector &options,
				   ScaleType          type);

	bool scaleInitiateCommon (CompAction         *action,
				  CompAction::State  state,
				  CompOption::Vector &options);

	bool ensureDndRedirectWindow ();

	bool selectWindowAt (int x, int y, bool moveInputFocus);

	void moveFocusWindow (int dx, int dy);

	void windowRemove (Window id);

	bool hoverTimeout ();

    public:

	CompositeScreen *cScreen;
	GLScreen        *gScreen;

	unsigned int lastActiveNum;
	Window       lastActiveWindow;

	Window       selectedWindow;
	Window       hoveredWindow;
	Window       previousActiveWindow;

	KeyCode	 leftKeyCode, rightKeyCode, upKeyCode, downKeyCode;

	bool grab;
	CompScreen::GrabHandle grabIndex;

	Window dndTarget;

	CompTimer hover;

	ScaleScreen::State state;
	int                moreAdjust;

	Cursor cursor;

	std::vector<ScaleSlot> slots;
	int                  nSlots;

	ScaleScreen::WindowList windows;

	GLushort opacity;

	ScaleType type;

	Window clientLeader;

	CompMatch match;
	CompMatch currentMatch;

	CompOption::Vector opt;
};

class PrivateScaleWindow :
    public CompositeWindowInterface,
    public GLWindowInterface
{
    public:
	PrivateScaleWindow (CompWindow *);
	~PrivateScaleWindow ();

	bool damageRect (bool, const CompRect &);

	bool glPaint (const GLWindowPaintAttrib &, const GLMatrix &,
		      const CompRegion &, unsigned int);

	bool isNeverScaleWin () const;
	bool isScaleWin () const;

	bool adjustScaleVelocity ();

	static bool compareWindowsDistance (ScaleWindow *, ScaleWindow *);
	
    public:

	CompWindow         *window;
	CompositeWindow    *cWindow;
	GLWindow           *gWindow;
	ScaleWindow        *sWindow;
	ScaleScreen        *sScreen;
	PrivateScaleScreen *spScreen;
	
	ScaleSlot *slot;

	int sid;
	int distance;

	GLfloat xVelocity, yVelocity, scaleVelocity;
	GLfloat scale;
	GLfloat tx, ty;
	float   delta;
	bool    adjust;

	float lastThumbOpacity;
};


#endif
