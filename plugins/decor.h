#include <boost/shared_ptr.hpp>
#include <compiz-core.h>
#include <compprivatehandler.h>

#include <composite/composite.h>
#include <opengl/opengl.h>

#define DECOR_DISPLAY_OPTION_SHADOW_RADIUS   0
#define DECOR_DISPLAY_OPTION_SHADOW_OPACITY  1
#define DECOR_DISPLAY_OPTION_SHADOW_COLOR    2
#define DECOR_DISPLAY_OPTION_SHADOW_OFFSET_X 3
#define DECOR_DISPLAY_OPTION_SHADOW_OFFSET_Y 4
#define DECOR_DISPLAY_OPTION_COMMAND         5
#define DECOR_DISPLAY_OPTION_MIPMAP          6
#define DECOR_DISPLAY_OPTION_DECOR_MATCH     7
#define DECOR_DISPLAY_OPTION_SHADOW_MATCH    8
#define DECOR_DISPLAY_OPTION_NUM             9

class DecorPluginVTable : public CompPlugin::VTable
{
    public:

	const char * name () { return "decor"; };

	CompMetadata * getMetadata ();

	bool init ();
	void fini ();

	bool initObject (CompObject *object);
	void finiObject (CompObject *object);

	CompOption::Vector & getObjectOptions (CompObject *object);

	bool setObjectOption (CompObject        *object,
			      const char        *name,
			      CompOption::Value &value);
};

#define DECOR_DISPLAY(s) DecorDisplay *ds = DecorDisplay::get(d)
#define DECOR_SCREEN(s) DecorScreen *ds = DecorScreen::get(s)
#define DECOR_WINDOW(w) DecorWindow *dw = DecorWindow::get(w)

struct Vector {
    int	dx;
    int	dy;
    int	x0;
    int	y0;
};

#define DECOR_BARE   0
#define DECOR_NORMAL 1
#define DECOR_ACTIVE 2
#define DECOR_NUM    3

class DecorTexture : public GLTexture {

    public:
	DecorTexture (CompScreen *screen, Pixmap pixmap);
	~DecorTexture ();

    public:
	CompScreen *screen;
	bool       status;
	int        refCount;
        Pixmap     pixmap;
	Damage     damage;
};

class Decoration {

    public:
	static Decoration * create (CompScreen *screen, Window id,
				    Atom decorAtom);
	static void release (Decoration *);

    public:
	int                       refCount;
	DecorTexture              *texture;
	CompWindowExtents         output;
	CompWindowExtents         input;
	CompWindowExtents         maxInput;
	int                       minWidth;
	int                       minHeight;
	decor_quad_t              *quad;
	int                       nQuad;
	CompScreen                *screen;
};

struct ScaledQuad {
    GLTexture::Matrix matrix;
    BoxRec            box;
    float             sx;
    float             sy;
};

class WindowDecoration {
    public:
	static WindowDecoration * create (Decoration *);
	static void destroy (WindowDecoration *);

    public:
	Decoration *decor;
	ScaledQuad *quad;
	int	   nQuad;
};

class DecorCore :
    public CoreInterface,
    public PrivateHandler<DecorCore,CompCore>
{
    public:
	DecorCore (CompCore *c) :
	    PrivateHandler<DecorCore,CompCore> (c)
	{
	    CoreInterface::setHandler (c);
	}
	
	void objectAdd (CompObject *, CompObject *);
	void objectRemove (CompObject *, CompObject *);
};

class DecorDisplay :
    public DisplayInterface,
    public PrivateHandler<DecorDisplay,CompDisplay>
{
    public:
	DecorDisplay (CompDisplay *d);
	~DecorDisplay ();

	CompOption::Vector & getOptions ();
	bool setOption (const char *name, CompOption::Value &value);

	void handleEvent (XEvent *event);
	void matchPropertyChanged (CompWindow *);

	DecorTexture * getTexture (CompScreen *, Pixmap);
	void releaseTexture (DecorTexture *);

    public:
	CompDisplay      *display;
	CompositeDisplay *cDisplay;

	std::list<DecorTexture *> textures;

	Atom supportingDmCheckAtom;
	Atom winDecorAtom;
	Atom decorAtom[DECOR_NUM];
	Atom inputFrameAtom; 

	CompOption::Vector opt;
};

class DecorScreen :
    public PrivateHandler<DecorScreen,CompScreen>
{
    public:
	DecorScreen (CompScreen *s);
	~DecorScreen ();

	void checkForDm (bool);

    public:
	CompScreen   *screen;
	DecorDisplay *dDisplay;

	Window dmWin;

	Decoration *decor[DECOR_NUM];
};

class DecorWindow :
    public WindowInterface,
    public CompositeWindowInterface,
    public GLWindowInterface,
    public PrivateHandler<DecorWindow,CompWindow>
{
    public:
	DecorWindow (CompWindow *w);
	~DecorWindow ();

	void getOutputExtents (CompWindowExtents *);
	void resizeNotify (int, int, int, int);
	void moveNotify (int, int, bool);
	void stateChangeNotify (unsigned int);
	void updateFrameRegion (Region region);

	bool damageRect (bool, BoxPtr);

	bool glDraw (const GLMatrix &, GLFragment::Attrib &,
		     Region, unsigned int);

	void updateDecoration ();

	void setDecorationMatrices ();

	void updateDecorationScale ();

	void updateFrame ();

	bool checkSize (Decoration *decor);

	int shiftX ();
	int shiftY ();

	bool update (bool);

	bool resizeTimeout ();

    public:

	CompWindow      *window;
	GLWindow        *gWindow;
	CompositeWindow *cWindow;
	DecorScreen     *dScreen;
	CompDisplay     *display;
	DecorDisplay    *dDisplay;
	
	WindowDecoration *wd;
	Decoration	 *decor;

	Region frameRegion;

	Window inputFrame;

	CompCore::Timer resizeUpdate;
};

