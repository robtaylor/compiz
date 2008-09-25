#ifndef _COMPIZ_COMPOSITE_H
#define _COMPIZ_COMPOSITE_H

#include <X11/extensions/Xcomposite.h>

#define COMPIZ_COMPOSITE_ABI 1

#define PLUGIN Composite
#include <core/privatehandler.h>
#include <core/timer.h>
#include <core/core.h>

#define COMPOSITE_SCREEN_DAMAGE_PENDING_MASK (1 << 0)
#define COMPOSITE_SCREEN_DAMAGE_REGION_MASK  (1 << 1)
#define COMPOSITE_SCREEN_DAMAGE_ALL_MASK     (1 << 2)

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
    public CompositePrivateHandler<CompositeScreen, CompScreen,
				   COMPIZ_COMPOSITE_ABI>
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

class CompositeWindowInterface :
    public WrapableInterface<CompositeWindow, CompositeWindowInterface>
{
    public:
	virtual bool damageRect (bool, const CompRect &);
};

class CompositeWindow :
    public WrapableHandler<CompositeWindowInterface, 1>,
    public CompositePrivateHandler<CompositeWindow, CompWindow,
				   COMPIZ_COMPOSITE_ABI>
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
