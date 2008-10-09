#ifndef _COMPIZ_COMPOSITE_H
#define _COMPIZ_COMPOSITE_H

#include <X11/extensions/Xcomposite.h>

#define COMPIZ_COMPOSITE_ABI 1

#include <core/privatehandler.h>
#include <core/timer.h>
#include <core/core.h>

#define COMPOSITE_SCREEN_DAMAGE_PENDING_MASK (1 << 0)
#define COMPOSITE_SCREEN_DAMAGE_REGION_MASK  (1 << 1)
#define COMPOSITE_SCREEN_DAMAGE_ALL_MASK     (1 << 2)

#define OPAQUE 0xffff
#define COLOR  0xffff
#define BRIGHT 0xffff

#define PAINT_SCREEN_REGION_MASK		   (1 << 0)
#define PAINT_SCREEN_FULL_MASK			   (1 << 1)
#define PAINT_SCREEN_TRANSFORMED_MASK		   (1 << 2)
#define PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK (1 << 3)
#define PAINT_SCREEN_CLEAR_MASK			   (1 << 4)
#define PAINT_SCREEN_NO_OCCLUSION_DETECTION_MASK   (1 << 5)
#define PAINT_SCREEN_NO_BACKGROUND_MASK            (1 << 6)



class PrivateCompositeScreen;
class PrivateCompositeWindow;
class CompositeScreen;
class CompositeWindow;

class CompositeScreenInterface :
    public WrapableInterface<CompositeScreen, CompositeScreenInterface>
{
    public:

	virtual void preparePaint (int);
	virtual void donePaint ();
	virtual void paint (CompOutput::ptrList &outputs, unsigned int);

	virtual CompWindowList getWindowPaintList ();
};


class CompositeScreen :
    public WrapableHandler<CompositeScreenInterface, 4>,
    public PrivateHandler<CompositeScreen, CompScreen, COMPIZ_COMPOSITE_ABI>
{
    public:

	class PaintHandler {
	    public:
		virtual ~PaintHandler () {};

		virtual void paintOutputs (CompOutput::ptrList &outputs,
					   unsigned int        mask,
					   const CompRegion    &region) = 0;

		virtual bool hasVSync () { return false; };

		virtual void prepareDrawing () {};
	};

    public:
	CompositeScreen (CompScreen *s);
	~CompositeScreen ();

	CompOption::Vector & getOptions ();
        bool setOption (const char *name, CompOption::Value &value);
	CompOption * getOption (const char *name);

	bool registerPaintHandler (PaintHandler *pHnd);
        void unregisterPaintHandler ();

	bool compositingActive ();

	int damageEvent ();
	
	void damageScreen ();
	void damageRegion (const CompRegion &);
	void damagePending ();
	unsigned int damageMask ();

	void showOutputWindow ();
	void hideOutputWindow ();
	void updateOutputWindow ();

	Window overlay ();
	Window output ();

	int & overlayWindowCount ();

	void setWindowPaintOffset (int x, int y);
	CompPoint windowPaintOffset ();


	void detectRefreshRate ();
	int getTimeToNextRedraw (struct timeval *tv);

	int redrawTime ();
	
	bool handlePaintTimeout ();

	WRAPABLE_HND (0, CompositeScreenInterface, void, preparePaint, int);
	WRAPABLE_HND (1, CompositeScreenInterface, void, donePaint);
	WRAPABLE_HND (2, CompositeScreenInterface, void, paint,
		      CompOutput::ptrList &outputs, unsigned int);

	WRAPABLE_HND (3, CompositeScreenInterface, CompWindowList,
		      getWindowPaintList);

	friend class PrivateCompositeDisplay;

    private:
	PrivateCompositeScreen *priv;

    public:
	static bool toggleSlowAnimations (CompAction         *action,
					  CompAction::State  state,
					  CompOption::Vector &options);
};

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

class CompositeWindowInterface :
    public WrapableInterface<CompositeWindow, CompositeWindowInterface>
{
    public:
	virtual bool damageRect (bool, const CompRect &);
};

class CompositeWindow :
    public WrapableHandler<CompositeWindowInterface, 1>,
    public PrivateHandler<CompositeWindow, CompWindow, COMPIZ_COMPOSITE_ABI>
{
    public:

	CompositeWindow (CompWindow *w);
	~CompositeWindow ();

	bool bind ();
	void release ();
	Pixmap pixmap ();

	void redirect ();
	void unredirect ();
	bool redirected ();
	bool overlayWindow ();
	
	void damageTransformedRect (float          xScale,
				    float          yScale,
				    float          xTranslate,
				    float          yTranslate,
				    const CompRect &rect);

	void damageOutputExtents ();
	void addDamageRect (const CompRect &);
	void addDamage (bool force = false);

	bool damaged ();

	void processDamage (XDamageNotifyEvent *de);

	void updateOpacity ();
	void updateBrightness ();
	void updateSaturation ();

	unsigned short opacity ();
	unsigned short brightness ();
	unsigned short saturation ();

	WRAPABLE_HND (0, CompositeWindowInterface, bool, damageRect,
		      bool, const CompRect &);

	friend class PrivateCompositeWindow;
	friend class CompositeScreen;
	
    private:
	PrivateCompositeWindow *priv;
};

#endif
