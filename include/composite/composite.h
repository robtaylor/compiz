#ifndef _COMPIZ_COMPOSITE_H
#define _COMPIZ_COMPOSITE_H

#include <X11/extensions/Xcomposite.h>

#define COMPIZ_COMPOSITE_ABI 1

#define PLUGIN Composite
#include <compprivatehandler.h>
#include <compiz-core.h>

#define COMPOSITE_SCREEN_DAMAGE_PENDING_MASK (1 << 0)
#define COMPOSITE_SCREEN_DAMAGE_REGION_MASK  (1 << 1)
#define COMPOSITE_SCREEN_DAMAGE_ALL_MASK     (1 << 2)

class PrivateCompositeDisplay;
class PrivateCompositeScreen;
class PrivateCompositeWindow;
class CompositeScreen;
class CompositeWindow;

class CompositeDisplay :
    public CompositePrivateHandler<CompositeDisplay, CompDisplay,
				   COMPIZ_COMPOSITE_ABI>
{
    public:
	CompositeDisplay (CompDisplay *d);
	~CompositeDisplay ();

	CompOption::Vector & getOptions ();
        bool setOption (const char *name, CompOption::Value &value);

	int damageEvent ();

    private:
	PrivateCompositeDisplay *priv;
};

class CompositeScreenInterface : public WrapableInterface<CompositeScreen> {
    public:
	CompositeScreenInterface ();

	WRAPABLE_DEF(void, preparePaint, int);
	WRAPABLE_DEF(void, donePaint);
	WRAPABLE_DEF(void, paint, CompOutput::ptrList &outputs, unsigned int);

	WRAPABLE_DEF(CompWindowList, getWindowPaintList);
};


class CompositeScreen :
    public WrapableHandler<CompositeScreenInterface>,
    public CompositePrivateHandler<CompositeScreen, CompScreen,
				   COMPIZ_COMPOSITE_ABI>
{
    public:

	class PaintHandler {
	    public:
		virtual ~PaintHandler () {};

		virtual void paintOutputs (CompOutput::ptrList &outputs,
					   unsigned int        mask,
					   Region              region) = 0;

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
	
	
	void damageScreen ();
	void damageRegion (Region);
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

	WRAPABLE_HND(void, preparePaint, int);
	WRAPABLE_HND(void, donePaint);
	WRAPABLE_HND(void, paint, CompOutput::ptrList &outputs, unsigned int);

	WRAPABLE_HND(CompWindowList, getWindowPaintList);

	friend class PrivateCompositeDisplay;

    private:
	PrivateCompositeScreen *priv;

    public:
	static bool toggleSlowAnimations (CompDisplay        *d,
					  CompAction         *action,
					  CompAction::State  state,
					  CompOption::Vector &options);
};

class CompositeWindowInterface : public WrapableInterface<CompositeWindow> {
    public:
	CompositeWindowInterface ();

	WRAPABLE_DEF(bool, damageRect, bool, BoxPtr);
};

class CompositeWindow :
    public WrapableHandler<CompositeWindowInterface>,
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
	
	void damageTransformedRect (float  xScale,
				    float  yScale,
				    float  xTranslate,
				    float  yTranslate,
				    BoxPtr rect);

	void damageOutputExtents ();
	void addDamageRect (BoxPtr rect);
	void addDamage (bool force = false);

	bool damaged ();

	void processDamage (XDamageNotifyEvent *de);

	void updateOpacity ();
	void updateBrightness ();
	void updateSaturation ();

	unsigned short opacity ();
	unsigned short brightness ();
	unsigned short saturation ();

	WRAPABLE_HND(bool, damageRect, bool, BoxPtr);

	friend class PrivateCompositeWindow;
	friend class CompositeScreen;
	
    private:
	PrivateCompositeWindow *priv;
};

#endif
