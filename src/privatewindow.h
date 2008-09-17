#ifndef _PRIVATEWINDOW_H
#define _PRIVATEWINDOW_H

#include <compiz-core.h>
#include <core/window.h>
#include <core/point.h>
#include <core/timer.h>
#include "privatescreen.h"

#define WINDOW_INVISIBLE(w)				          \
    ((w)->attrib.map_state != IsViewable		       || \
     (w)->attrib.x + (w)->width  + (w)->output.right  <= 0     || \
     (w)->attrib.y + (w)->height + (w)->output.bottom <= 0     || \
     (w)->attrib.x - (w)->output.left >= (int) screen->size().width () || \
     (w)->attrib.y - (w)->output.top >= (int) screen->size().height () )


class PrivateWindow {

    public:
	PrivateWindow (CompWindow *window);
	~PrivateWindow ();

	void
	recalcNormalHints ();

	void
	updateFrameWindow ();

	void
	setWindowMatrix ();

	bool
	restack (Window aboveId);

	bool
	initializeSyncCounter ();

	bool
	isGroupTransient (Window clientLeader);


	static bool
	stackLayerCheck (CompWindow *w,
			 Window	    clientLeader,
			 CompWindow *below);

	static bool
	avoidStackingRelativeTo (CompWindow *w);

	static CompWindow *
	findSiblingBelow (CompWindow *w,
			  bool	     aboveFs);

	static CompWindow *
	findLowestSiblingBelow (CompWindow *w);

	static bool
	validSiblingBelow (CompWindow *w,
			   CompWindow *sibling);

	void
	saveGeometry (int mask);

	int
	restoreGeometry (XWindowChanges *xwc, int mask);

	void
	reconfigureXWindow (unsigned int   valueMask,
			    XWindowChanges *xwc);

	static bool
	stackTransients (CompWindow     *w,
			 CompWindow	*avoid,
			 XWindowChanges *xwc);

	static void
	stackAncestors (CompWindow     *w, XWindowChanges *xwc);

	static bool
	isAncestorTo (CompWindow *transient,
		      CompWindow *ancestor);

	Window
	getClientLeaderOfAncestor ();

	CompWindow *
	getModalTransient ();

	int
	addWindowSizeChanges (XWindowChanges *xwc,
			      CompWindow::Geometry old);

	int
	addWindowStackChanges (XWindowChanges *xwc,
			       CompWindow     *sibling);

	static CompWindow *
	findValidStackSiblingBelow (CompWindow *w,
				    CompWindow *sibling);

	void
	ensureWindowVisibility ();

	void
	reveal ();

	static void
	revealAncestors (CompWindow *w,
			 CompWindow *transient);

	static void
	minimizeTransients (CompWindow *w,
			    CompWindow *ancestor);

	static void
	unminimizeTransients (CompWindow *w,
			      CompWindow *ancestor);

	bool
	getUsageTimestamp (Time *timestamp);

	bool
	isWindowFocusAllowed (Time timestamp);

	static void
	handleDamageRect (CompWindow *w,
			  int	   x,
			  int	   y,
			  int	   width,
			  int	   height);

	bool reparent ();
	void unreparent ();

	bool handlePingTimeout (unsigned int lastPing);

	void handlePing (int lastPing);

	void applyStartupProperties (CompStartupSequence *s);

	void updateNormalHints ();

	void updateWmHints ();

	void updateClassHints ();

	void updateTransientHint ();

	void updateIconGeometry ();

	Window getClientLeader ();

	char * getStartupId ();

	void updateRegion ();

	bool updateStruts ();

	bool handleSyncAlarm ();

	void configure (XConfigureEvent *ce);

	void configureFrame (XConfigureEvent *ce);

	void circulate (XCirculateEvent *ce);

	unsigned int adjustConfigureRequestForGravity (XWindowChanges *xwc,
						       unsigned int   xwcm,
						       int            gravity);

	
	void updateSize ();

	bool getUserTime (Time *time);

	void setUserTime (Time time);

	bool allowWindowFocus (unsigned int noFocusMask,
			       Time         timestamp);

	void freeIcons ();

	void setDesktop (unsigned int desktop);

	void updateMwmHints ();

	void updateStartupId ();

	void processMap ();

	static unsigned int windowTypeFromString (const char *str);

	static int compareWindowActiveness (CompWindow *w1,
					    CompWindow *w2);

	void setOverrideRedirect (bool overrideRedirect);

    public:

	PrivateWindow *priv;

	CompWindow *window;

	int		      refcnt;
	Window	      id;
	Window	      frame;
	Window        wrapper;
	unsigned int      mapNum;
	unsigned int      activeNum;
	XWindowAttributes attrib;
	CompWindow::Geometry geometry;
	CompWindow::Geometry serverGeometry;
	Window	      transientFor;
	Window	      clientLeader;
	XSizeHints	      sizeHints;

	bool	      inputHint;
	bool	      alpha;
	int	      width;
	int	      height;
	Region        region;
	Region        frameRegion;

	unsigned int      wmType;
	unsigned int      type;
	unsigned int      state;
	unsigned int      actions;
	unsigned int      protocols;
	unsigned int      mwmDecor;
	unsigned int      mwmFunc;
	bool	      invisible;
	bool	      destroyed;

	bool	      managed;

	int		      destroyRefCnt;
	int		      unmapRefCnt;

	CompPoint initialViewport;

	Time initialTimestamp;
	Bool initialTimestampSet;

	bool placed;
	bool minimized;
	bool inShowDesktopMode;
	bool shaded;
	bool hidden;
	bool grabbed;

	unsigned int desktop;

	int pendingUnmaps;
	int pendingMaps;

	char *startupId;
	char *resName;
	char *resClass;

	CompGroup *group;

	unsigned int lastPong;
	bool	 alive;


	CompWindowExtents input;
	CompWindowExtents output;

	CompStruts *struts;

	std::vector<CompIcon *> icons;
	bool noIcons;

	XRectangle iconGeometry;
	bool       iconGeometrySet;

	XWindowChanges saveWc;
	int		   saveMask;

	XSyncCounter  syncCounter;
	XSyncValue	  syncValue;
	XSyncAlarm	  syncAlarm;
	unsigned long syncAlarmConnection;
	CompTimer   syncWaitTimer;

	bool     syncWait;
	CompWindow::Geometry syncGeometry;

	bool closeRequests;
	Time lastCloseRequestTime;




};

#endif
