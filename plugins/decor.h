#include <boost/shared_ptr.hpp>
#include <core/core.h>
#include <core/privatehandler.h>

#include <composite/composite.h>
#include <opengl/opengl.h>
#include <core/atoms.h>

#define DECOR_OPTION_SHADOW_RADIUS   0
#define DECOR_OPTION_SHADOW_OPACITY  1
#define DECOR_OPTION_SHADOW_COLOR    2
#define DECOR_OPTION_SHADOW_OFFSET_X 3
#define DECOR_OPTION_SHADOW_OFFSET_Y 4
#define DECOR_OPTION_COMMAND         5
#define DECOR_OPTION_MIPMAP          6
#define DECOR_OPTION_DECOR_MATCH     7
#define DECOR_OPTION_SHADOW_MATCH    8
#define DECOR_OPTION_NUM             9

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

class DecorTexture {

    public:
	DecorTexture (Pixmap pixmap);
	~DecorTexture ();

    public:
	bool            status;
	int             refCount;
        Pixmap          pixmap;
	Damage          damage;
	GLTexture::List textures;
};

class Decoration {

    public:
	static Decoration * create (Window id, Atom decorAtom);
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
	int                       type;
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

class DecorWindow;

class DecorScreen :
    public ScreenInterface,
    public PrivateHandler<DecorScreen,CompScreen>
{
    public:
	DecorScreen (CompScreen *s);
	~DecorScreen ();

	CompOption::Vector & getOptions ();
	bool setOption (const char *name, CompOption::Value &value);

	void handleEvent (XEvent *event);
	void matchPropertyChanged (CompWindow *);

	DecorTexture * getTexture (Pixmap);
	void releaseTexture (DecorTexture *);

	
	void checkForDm (bool);

    public:

	CompositeScreen *cScreen;

	std::list<DecorTexture *> textures;

	Atom supportingDmCheckAtom;
	Atom winDecorAtom;
	Atom decorAtom[DECOR_NUM];
	Atom inputFrameAtom;
	Atom outputFrameAtom;
	Atom decorTypeAtom;
	Atom decorTypePixmapAtom;
	Atom decorTypeWindowAtom;

	Window dmWin;
	int    dmSupports;

	Decoration *decor[DECOR_NUM];
	Decoration windowDefault;

	bool cmActive;

	std::map<Window, DecorWindow *> frames;

	CompOption::Vector opt;
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
	void updateFrameRegion (CompRegion &region);

	bool damageRect (bool, const CompRect &);

	bool glDraw (const GLMatrix &, GLFragment::Attrib &,
		     const CompRegion &, unsigned int);

	void updateDecoration ();

	void setDecorationMatrices ();

	void updateDecorationScale ();

	void updateFrame ();
	void updateInputFrame ();
	void updateOutputFrame ();

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
	
	WindowDecoration *wd;
	Decoration	 *decor;

	CompRegion frameRegion;

	Window inputFrame;
	Window outputFrame;
	Damage frameDamage;

	int    oldX;
	int    oldY;
	int    oldWidth;
	int    oldHeight;

	CompTimer resizeUpdate;
	CompTimer moveUpdate;
};

class DecorPluginVTable :
    public CompPlugin::VTableForScreenAndWindow<DecorScreen, DecorWindow>
{
    public:

	const char * name () { return "decor"; };

	CompMetadata * getMetadata ();

	bool init ();
	void fini ();

	PLUGIN_OPTION_HELPER (DecorScreen);
};

