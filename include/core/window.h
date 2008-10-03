#ifndef _COMPWINDOW_H
#define _COMPWINDOW_H

#include <boost/function.hpp>

#include <X11/Xlib-xcb.h>
#include <X11/Xutil.h>
#include <X11/Xregion.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/sync.h>

#include <core/action.h>
#include <core/privates.h>
#include <core/size.h>
#include <core/point.h>
#include <core/region.h>

#include <core/wrapsystem.h>

#include <map>

class CompWindow;
class CompIcon;
class PrivateWindow;
struct CompStartupSequence;

#define GET_CORE_WINDOW(object) (dynamic_cast<CompWindow *> (object))
#define CORE_WINDOW(object) CompWindow *w = GET_CORE_WINDOW (object)

#define ROOTPARENT(x) (((x)->frame ()) ? (x)->frame () : (x)->id ())

#define CompWindowProtocolDeleteMask	  (1 << 0)
#define CompWindowProtocolTakeFocusMask	  (1 << 1)
#define CompWindowProtocolPingMask	  (1 << 2)
#define CompWindowProtocolSyncRequestMask (1 << 3)

#define CompWindowTypeDesktopMask      (1 << 0)
#define CompWindowTypeDockMask         (1 << 1)
#define CompWindowTypeToolbarMask      (1 << 2)
#define CompWindowTypeMenuMask         (1 << 3)
#define CompWindowTypeUtilMask         (1 << 4)
#define CompWindowTypeSplashMask       (1 << 5)
#define CompWindowTypeDialogMask       (1 << 6)
#define CompWindowTypeNormalMask       (1 << 7)
#define CompWindowTypeDropdownMenuMask (1 << 8)
#define CompWindowTypePopupMenuMask    (1 << 9)
#define CompWindowTypeTooltipMask      (1 << 10)
#define CompWindowTypeNotificationMask (1 << 11)
#define CompWindowTypeComboMask	       (1 << 12)
#define CompWindowTypeDndMask	       (1 << 13)
#define CompWindowTypeModalDialogMask  (1 << 14)
#define CompWindowTypeFullscreenMask   (1 << 15)
#define CompWindowTypeUnknownMask      (1 << 16)

#define NO_FOCUS_MASK (CompWindowTypeDesktopMask | \
		       CompWindowTypeDockMask    | \
		       CompWindowTypeSplashMask)

#define CompWindowStateModalMask	    (1 <<  0)
#define CompWindowStateStickyMask	    (1 <<  1)
#define CompWindowStateMaximizedVertMask    (1 <<  2)
#define CompWindowStateMaximizedHorzMask    (1 <<  3)
#define CompWindowStateShadedMask	    (1 <<  4)
#define CompWindowStateSkipTaskbarMask	    (1 <<  5)
#define CompWindowStateSkipPagerMask	    (1 <<  6)
#define CompWindowStateHiddenMask	    (1 <<  7)
#define CompWindowStateFullscreenMask	    (1 <<  8)
#define CompWindowStateAboveMask	    (1 <<  9)
#define CompWindowStateBelowMask	    (1 << 10)
#define CompWindowStateDemandsAttentionMask (1 << 11)
#define CompWindowStateDisplayModalMask	    (1 << 12)

#define MAXIMIZE_STATE (CompWindowStateMaximizedHorzMask | \
			CompWindowStateMaximizedVertMask)

#define CompWindowActionMoveMask	  (1 << 0)
#define CompWindowActionResizeMask	  (1 << 1)
#define CompWindowActionStickMask	  (1 << 2)
#define CompWindowActionMinimizeMask      (1 << 3)
#define CompWindowActionMaximizeHorzMask  (1 << 4)
#define CompWindowActionMaximizeVertMask  (1 << 5)
#define CompWindowActionFullscreenMask	  (1 << 6)
#define CompWindowActionCloseMask	  (1 << 7)
#define CompWindowActionShadeMask	  (1 << 8)
#define CompWindowActionChangeDesktopMask (1 << 9)
#define CompWindowActionAboveMask	  (1 << 10)
#define CompWindowActionBelowMask	  (1 << 11)

#define MwmFuncAll      (1L << 0)
#define MwmFuncResize   (1L << 1)
#define MwmFuncMove     (1L << 2)
#define MwmFuncIconify  (1L << 3)
#define MwmFuncMaximize (1L << 4)
#define MwmFuncClose    (1L << 5)

#define MwmDecorHandle   (1L << 2)
#define MwmDecorTitle    (1L << 3)
#define MwmDecorMenu     (1L << 4)
#define MwmDecorMinimize (1L << 5)
#define MwmDecorMaximize (1L << 6)

#define MwmDecorAll      (1L << 0)
#define MwmDecorBorder   (1L << 1)
#define MwmDecorHandle   (1L << 2)
#define MwmDecorTitle    (1L << 3)
#define MwmDecorMenu     (1L << 4)
#define MwmDecorMinimize (1L << 5)
#define MwmDecorMaximize (1L << 6)

#define WmMoveResizeSizeTopLeft      0
#define WmMoveResizeSizeTop          1
#define WmMoveResizeSizeTopRight     2
#define WmMoveResizeSizeRight        3
#define WmMoveResizeSizeBottomRight  4
#define WmMoveResizeSizeBottom       5
#define WmMoveResizeSizeBottomLeft   6
#define WmMoveResizeSizeLeft         7
#define WmMoveResizeMove             8
#define WmMoveResizeSizeKeyboard     9
#define WmMoveResizeMoveKeyboard    10
#define WmMoveResizeCancel          11

/*
  window paint flags

  bit 1-16 are used for read-only flags and they provide
  information that describe the screen rendering pass
  currently in process.

  bit 17-32 are writable flags and they provide information
  that is used to optimize rendering.
*/

/*
  this flag is present when window is being painted
  on a transformed screen.
*/
#define PAINT_WINDOW_ON_TRANSFORMED_SCREEN_MASK (1 << 0)

/*
  this flag is present when window is being tested
  for occlusion of other windows.
*/
#define PAINT_WINDOW_OCCLUSION_DETECTION_MASK   (1 << 1)

/*
  this flag indicates that the window ist painted with
  an offset
*/
#define PAINT_WINDOW_WITH_OFFSET_MASK           (1 << 2)

/*
  flag indicate that window is translucent.
*/
#define PAINT_WINDOW_TRANSLUCENT_MASK           (1 << 16)

/*
  flag indicate that window is transformed.
*/
#define PAINT_WINDOW_TRANSFORMED_MASK           (1 << 17)

/*
  flag indicate that core PaintWindow function should
  not draw this window.
*/
#define PAINT_WINDOW_NO_CORE_INSTANCE_MASK	(1 << 18)

/*
  flag indicate that blending is required.
*/
#define PAINT_WINDOW_BLEND_MASK			(1 << 19)

#define CompWindowGrabKeyMask    (1 << 0)
#define CompWindowGrabButtonMask (1 << 1)
#define CompWindowGrabMoveMask   (1 << 2)
#define CompWindowGrabResizeMask (1 << 3)



enum CompStackingUpdateMode {
    CompStackingUpdateModeNone = 0,
    CompStackingUpdateModeNormal,
    CompStackingUpdateModeAboveFullscreen,
    CompStackingUpdateModeInitialMap,
    CompStackingUpdateModeInitialMapDeniedFocus
};

enum CompWindowNotify {
   CompWindowNotifyMap,
   CompWindowNotifyUnmap,
   CompWindowNotifyRestack,
   CompWindowNotifyHide,
   CompWindowNotifyShow,
   CompWindowNotifyAliveChanged,
   CompWindowNotifySyncAlarm,
   CompWindowNotifyReparent,
   CompWindowNotifyUnreparent,
   CompWindowNotifyFrameUpdate
};

struct CompWindowExtents {
    int left;
    int right;
    int top;
    int bottom;
};

struct CompStruts {
    XRectangle left;
    XRectangle right;
    XRectangle top;
    XRectangle bottom;
};

class WindowInterface : public WrapableInterface<CompWindow, WindowInterface> {
    public:
	virtual void getOutputExtents (CompWindowExtents *output);

	virtual void getAllowedActions (unsigned int *setActions,
					unsigned int *clearActions);

	virtual bool focus ();
	virtual void activate ();
	virtual bool place (int x, int y, int *newX, int *newY);

	virtual void validateResizeRequest (unsigned int   *mask,
					    XWindowChanges *xwc);

	virtual void resizeNotify (int dx, int dy, int dwidth, int dheight);
	virtual void moveNotify (int dx, int dy, bool immediate);
	virtual void windowNotify (CompWindowNotify n);
	
	virtual void grabNotify (int x, int y,
				 unsigned int state, unsigned int mask);
	virtual void ungrabNotify ();

	virtual void stateChangeNotify (unsigned int lastState);

	virtual void updateFrameRegion (CompRegion &region);
};

class CompWindow :
    public WrapableHandler<WindowInterface, 13>,
    public CompPrivateStorage
{

    public:

	class Geometry : public CompPoint, public CompSize {

	    public:
		Geometry ();
		Geometry (int, int, unsigned int, unsigned int, unsigned int);

		unsigned int border ();

		void set (int, int, unsigned int, unsigned int, unsigned int);

		void setBorder (unsigned int);

	    private:
		unsigned int mBorder;
	};



	typedef boost::function<void (CompWindow *)> ForEach;

	typedef std::map<Window, CompWindow *> Map;
	
    public:
	CompWindow *next;
	CompWindow *prev;

    public:

	CompWindow (Window     id,
	            Window     aboveId);
	~CompWindow ();

	Window id ();
	Window frame ();

	const CompRegion & region () const;

	const CompRegion & frameRegion () const;

	void updateFrameRegion ();
	void setWindowFrameExtents (CompWindowExtents *input);

	unsigned int & wmType ();
	
	unsigned int type ();
	
	unsigned int & state ();
	
	unsigned int actions ();
	
	unsigned int & protocols ();

	void close (Time serverTime);


	bool inShowDesktopMode ();

	void setShowDesktopMode (bool);

	bool managed ();

	bool grabbed ();

	unsigned int activeNum ();
	
	int mapNum ();

	CompStruts * struts ();

	int & saveMask ();

	XWindowChanges & saveWc ();

	void moveToViewportPosition (int x, int y, bool sync);

	char * startupId ();

	unsigned int desktop ();

	Window clientLeader ();

	void
	changeState (unsigned int newState);

	void recalcActions ();

	void recalcType ();

	void updateWindowOutputExtents ();

	void destroy ();

	void sendConfigureNotify ();

	void sendSyncRequest ();

	XSyncAlarm syncAlarm ();

	void map ();

	void unmap ();

	bool resize (XWindowAttributes);
	
	bool resize (Geometry);

	bool resize (int x, int y, unsigned int width, unsigned int height,
		     unsigned int border = 0);

	void move (int dx, int dy, bool immediate = true);

	void syncPosition ();

	void moveInputFocusTo ();

	void moveInputFocusToOtherWindow ();

	void configureXWindow (unsigned int valueMask,
			       XWindowChanges *xwc);

	void moveResize (XWindowChanges *xwc,
			 unsigned int   xwcm,
			 int            gravity);

	void raise ();

	void lower ();

	void restackAbove (CompWindow *sibling);

	void restackBelow (CompWindow *sibling);

	void updateAttributes (CompStackingUpdateMode stackingMode);

	void hide ();

	void show ();

	void minimize ();

	void unminimize ();

	void maximize (unsigned int state = 0);

	CompPoint defaultViewport ();

	CompIcon * getIcon (int width, int height);

	int outputDevice ();

	bool onCurrentDesktop ();

	bool onAllViewports ();

	CompPoint getMovementForOffset (CompPoint offset);

	Window transientFor ();

	int pendingUnmaps ();

	bool minimized ();

	bool placed ();

	bool shaded ();

	CompSize size ();

	Geometry & geometry ();

	Geometry & serverGeometry ();

	CompWindowExtents input ();

	CompWindowExtents output ();

	XSizeHints sizeHints ();

	bool destroyed ();

	bool invisible ();

	bool syncWait ();

	bool alpha ();

	bool alive ();

	bool overrideRedirect ();

	bool isViewable ();

	int windowClass ();

	unsigned int depth ();

	unsigned int mwmDecor ();
	unsigned int mwmFunc ();

	bool constrainNewWindowSize (int width,
				     int height,
				     int *newWidth,
				     int *newHeight);
	
	static unsigned int constrainWindowState (unsigned int state,
						  unsigned int actions);

	static int allocPrivateIndex ();
	static void freePrivateIndex (int index);

	WRAPABLE_HND (0, WindowInterface, void, getOutputExtents,
		      CompWindowExtents *);
	WRAPABLE_HND (1, WindowInterface, void, getAllowedActions,
		      unsigned int *, unsigned int *);

	WRAPABLE_HND (2, WindowInterface, bool, focus);
	WRAPABLE_HND (3, WindowInterface, void, activate);
	WRAPABLE_HND (4, WindowInterface, bool, place, int, int, int*, int*);
	WRAPABLE_HND (5, WindowInterface, void, validateResizeRequest,
		      unsigned int *, XWindowChanges *);

	WRAPABLE_HND (6, WindowInterface, void, resizeNotify,
		      int, int, int, int);
	WRAPABLE_HND (7, WindowInterface, void, moveNotify, int, int, bool);
	WRAPABLE_HND (8, WindowInterface, void, windowNotify, CompWindowNotify);
	WRAPABLE_HND (9, WindowInterface, void, grabNotify, int, int,
		      unsigned int, unsigned int);
	WRAPABLE_HND (10, WindowInterface, void, ungrabNotify);
	WRAPABLE_HND (11, WindowInterface, void, stateChangeNotify,
		      unsigned int);

	WRAPABLE_HND (12, WindowInterface, void, updateFrameRegion,
		      CompRegion &);

	friend class PrivateWindow;
	friend class CompScreen;
	friend class PrivateScreen;
	
    private:
	PrivateWindow *priv;


};

#endif
