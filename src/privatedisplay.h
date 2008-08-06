#ifndef _PRIVATEDISPLAY_H
#define _PRIVATEDISPLAY_H

#include <compiz-core.h>
#include <compdisplay.h>

class PrivateDisplay {

    public:
	PrivateDisplay (CompDisplay *display);
	~PrivateDisplay ();
	
	void
	updatePlugins ();

	bool
	triggerButtonPressBindings (CompOption  *option,
				    int		nOption,
				    XEvent	*event,
				    CompOption  *argument,
				    int		nArgument);

	bool
	triggerButtonReleaseBindings (CompOption  *option,
				      int	  nOption,
				      XEvent	  *event,
				      CompOption  *argument,
				      int	  nArgument);

	bool
	triggerKeyPressBindings (CompOption  *option,
				 int	     nOption,
				 XEvent	     *event,
				 CompOption  *argument,
				 int	     nArgument);

	bool
	triggerKeyReleaseBindings (CompOption  *option,
				   int	       nOption,
				   XEvent      *event,
				   CompOption  *argument,
				   int	       nArgument);

	bool
	triggerStateNotifyBindings (CompOption		*option,
				    int			nOption,
				    XkbStateNotifyEvent *event,
				    CompOption		*argument,
				    int			nArgument);

	bool
	triggerEdgeEnter (unsigned int    edge,
			  CompActionState state,
			  CompOption      *argument,
			  unsigned int    nArgument);

	void
	setAudibleBell (bool audible);

	void
	handlePingTimeout ();
	
	bool
	handleActionEvent (XEvent *event);

	void
	handleSelectionRequest (XEvent *event);

	void
	handleSelectionClear (XEvent *event);

	void
	initAtoms ();

    public:

	CompDisplay *display;

	xcb_connection_t *connection;

	Display    *dpy;
	CompScreen *screens;

	CompWatchFdHandle watchFdHandle;

	int compositeEvent, compositeError, compositeOpcode;
	int damageEvent, damageError;
	int syncEvent, syncError;
	int fixesEvent, fixesError, fixesVersion;

	bool randrExtension;
	int  randrEvent, randrError;

	bool shapeExtension;
	int  shapeEvent, shapeError;

	bool xkbExtension;
	int  xkbEvent, xkbError;

	bool xineramaExtension;
	int  xineramaEvent, xineramaError;

	std::vector<XineramaScreenInfo> screenInfo;

	SnDisplay *snDisplay;

	unsigned int      lastPing;
	CompTimeoutHandle pingHandle;

	GLenum textureFilter;

	Window activeWindow;

	Window below;
	char   displayString[256];

	XModifierKeymap *modMap;
	unsigned int    modMask[CompModNum];
	unsigned int    ignoredModMask;

	KeyCode escapeKeyCode;
	KeyCode returnKeyCode;

	CompOption opt[COMP_DISPLAY_OPTION_NUM];

	CompTimeoutHandle autoRaiseHandle;
	Window	      autoRaiseWindow;

	CompTimeoutHandle edgeDelayHandle;

	CompOptionValue plugin;
	bool	    dirtyPluginList;

	CompDisplay::Atoms atoms;
};

#endif
