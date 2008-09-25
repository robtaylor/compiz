#ifndef _COMPOSITE_PRIVATES_H
#define _COMPOSITE_PRIVATES_H

#include <composite/composite.h>
#include <core/atoms.h>

#if COMPOSITE_MAJOR > 0 || COMPOSITE_MINOR > 2
#define USE_COW
extern bool       useCow;
#endif

#define COMPOSITE_OPTION_SLOW_ANIMATIONS_KEY 0
#define COMPOSITE_OPTION_DETECT_REFRESH_RATE 1
#define COMPOSITE_OPTION_REFRESH_RATE        2
#define COMPOSITE_OPTION_UNREDIRECT_FS       3
#define COMPOSITE_OPTION_FORCE_INDEPENDENT   4
#define COMPOSITE_OPTION_NUM                 5

extern CompMetadata *compositeMetadata;

extern const CompMetadata::OptionInfo
   compositeOptionInfo[COMPOSITE_OPTION_NUM];

extern CompWindow *lastDamagedWindow;

class PrivateCompositeScreen : ScreenInterface
{
    public:
	PrivateCompositeScreen (CompositeScreen *cs);
	~PrivateCompositeScreen ();

	void outputChangeNotify ();

	void handleEvent (XEvent *event);

	void makeOutputWindow ();

	bool init ();

	void handleExposeEvent (XExposeEvent *event);

    public:

	CompositeScreen *cScreen;

	int compositeEvent, compositeError, compositeOpcode;
	int damageEvent, damageError;
	int fixesEvent, fixesError, fixesVersion;

	bool shapeExtension;
	int  shapeEvent, shapeError;

	bool randrExtension;
	int  randrEvent, randrError;

	CompRegion    damage;
	unsigned long damageMask;

	CompRegion    tmpRegion;
	
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

	CompTimer paintTimer;

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
