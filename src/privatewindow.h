#ifndef _PRIVATEWINDOW_H
#define _PRIVATEWINDOW_H

#include <compiz-core.h>
#include <compwindow.h>
#include <comppoint.h>

#define WINDOW_INVISIBLE(w)				          \
    ((w)->attrib.map_state != IsViewable		       || \
     (!(w)->damaged)					       || \
     (w)->attrib.x + (w)->width  + (w)->output.right  <= 0     || \
     (w)->attrib.y + (w)->height + (w)->output.bottom <= 0     || \
     (w)->attrib.x - (w)->output.left >= (w)->screen->size().width () || \
     (w)->attrib.y - (w)->output.top >= (w)->screen->size().height () )


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
			 void       *closure);

	bool
	constrainNewWindowSize (int        width,
				int        height,
				int        *newWidth,
				int        *newHeight);

	static void
	minimizeTransients (CompWindow *w,
			    void       *closure);

	static void
	unminimizeTransients (CompWindow *w,
			      void       *closure);

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


    public:

	CompWindow *window;
	CompScreen *screen;

	int		      refcnt;
	Window	      id;
	Window	      frame;
	unsigned int      mapNum;
	unsigned int      activeNum;
	XWindowAttributes attrib;
	CompWindow::Geometry      serverGeometry;
	Window	      transientFor;
	Window	      clientLeader;
	XSizeHints	      sizeHints;
	Pixmap	      pixmap;
	CompTexture       *texture;
	CompTextureMatrix matrix;
	Damage	      damage;
	bool	      inputHint;
	bool	      alpha;
	GLint	      width;
	GLint	      height;
	Region	      region;
	Region	      clip;
	unsigned int      wmType;
	unsigned int      type;
	unsigned int      state;
	unsigned int      actions;
	unsigned int      protocols;
	unsigned int      mwmDecor;
	unsigned int      mwmFunc;
	bool	      invisible;
	bool	      destroyed;
	bool	      damaged;
	bool	      redirected;
	bool	      managed;
	bool	      bindFailed;
	bool	      overlayWindow;
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

	GLushort opacity;
	GLushort brightness;
	GLushort saturation;

	WindowPaintAttrib paint;
	WindowPaintAttrib lastPaint;

	unsigned int lastMask;

	CompWindowExtents input;
	CompWindowExtents output;

	CompStruts *struts;

	CompIcon **icon;
	int	     nIcon;

	XRectangle iconGeometry;
	bool       iconGeometrySet;

	XWindowChanges saveWc;
	int		   saveMask;

	XSyncCounter  syncCounter;
	XSyncValue	  syncValue;
	XSyncAlarm	  syncAlarm;
	unsigned long syncAlarmConnection;
	unsigned int  syncWaitHandle;

	bool     syncWait;
	CompWindow::Geometry syncGeometry;

	bool closeRequests;
	Time lastCloseRequestTime;

	XRectangle *damageRects;
	int	       sizeDamage;
	int	       nDamage;

	GLfloat  *vertices;
	int      vertexSize;
	int      vertexStride;
	GLushort *indices;
	int      indexSize;
	int      vCount;
	int      texUnits;
	int      texCoordSize;
	int      indexCount;
};

#endif
