#ifndef _COMPOSITE_PRIVATES_H
#define _COMPOSITE_PRIVATES_H

#include <composite/composite.h>

#if COMPOSITE_MAJOR > 0 || COMPOSITE_MINOR > 2
#define USE_COW
extern bool       useCow;
#endif

#define COMPOSITE_DISPLAY_OPTION_SLOW_ANIMATIONS_KEY 0
#define COMPOSITE_DISPLAY_OPTION_NUM                 1

#define COMPOSITE_SCREEN_OPTION_DETECT_REFRESH_RATE 0
#define COMPOSITE_SCREEN_OPTION_REFRESH_RATE        1
#define COMPOSITE_SCREEN_OPTION_UNREDIRECT_FS       2
#define COMPOSITE_SCREEN_OPTION_FORCE_INDEPENDENT   3
#define COMPOSITE_SCREEN_OPTION_NUM                 4

extern CompMetadata *compositeMetadata;
extern const CompMetadata::OptionInfo
    compositeDisplayOptionInfo[COMPOSITE_DISPLAY_OPTION_NUM];

extern const CompMetadata::OptionInfo
   compositeScreenOptionInfo[COMPOSITE_SCREEN_OPTION_NUM];

extern CompWindow *lastDamagedWindow;

class PrivateCompositeDisplay : public DisplayInterface
{
    public:
	PrivateCompositeDisplay (CompDisplay *d, CompositeDisplay *cd);
	~PrivateCompositeDisplay ();

	void handleEvent (XEvent *event);

    public:
	CompDisplay      *display;
	CompositeDisplay *cDisplay;

	int compositeEvent, compositeError, compositeOpcode;
	int damageEvent, damageError;
	int fixesEvent, fixesError, fixesVersion;

	bool shapeExtension;
	int  shapeEvent, shapeError;

	bool randrExtension;
	int  randrEvent, randrError;

	CompOption::Vector opt;
};

class PrivateCompositeScreen : ScreenInterface
{
    public:
	PrivateCompositeScreen (CompScreen *s, CompositeScreen *cs);
	~PrivateCompositeScreen ();

	void outputChangeNotify ();

	void makeOutputWindow ();

	bool init ();

	void handleExposeEvent (XExposeEvent *event);

    public:
	CompScreen      *screen;
	CompositeScreen *cScreen;

	Region	      damage;
	unsigned long damageMask;
	
	Window	      overlay;
	Window	      output;

	std::list <CompRect> exposeRects;

	CompPoint windowPaintOffset;

	int overlayWindowCount;

	struct timeval lastRedraw;
	int            nextRedraw;
	int            redrawTime;
	int            optimalRedrawTime;
	int            frameStatus;
	int            timeMult;
	bool           idle;
	int            timeLeft;

	bool slowAnimations;

	CompCore::Timer paintTimer;

	Region tmpRegion;

	bool active;
	CompositeScreen::PaintHandler *pHnd;

	CompOption::Vector opt;
};

class PrivateCompositeWindow : WindowInterface
{
    public:
	PrivateCompositeWindow (CompWindow *w, CompositeWindow *cw);
	~PrivateCompositeWindow ();

	void windowNotify (CompWindowNotify n);
	void resizeNotify (int dx, int dy, int dwidth, int dheight);
	void moveNotify (int dx, int dy, bool now);

	static void handleDamageRect (CompositeWindow *w,
				      int             x,
				      int             y,
				      int             width,
				      int             height);

    public:

	CompWindow      *window;
	CompositeWindow *cWindow;
	CompScreen      *screen;
	CompositeScreen *cScreen;

	Pixmap	      pixmap;

	Damage	      damage;

	bool	      damaged;
	bool	      redirected;
	bool          overlayWindow;
	bool          bindFailed;

	unsigned short opacity;
	unsigned short brightness;
	unsigned short saturation;
	
	XRectangle *damageRects;
	int        sizeDamage;
	int        nDamage;


};

#endif
