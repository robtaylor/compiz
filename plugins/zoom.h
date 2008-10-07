#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include <X11/cursorfont.h>

#include <core/core.h>
#include <core/privatehandler.h>

#include <composite/composite.h>
#include <opengl/opengl.h>

#define ZOOM_OPTION_INITIATE_BUTTON 0
#define ZOOM_OPTION_IN_BUTTON	    1
#define ZOOM_OPTION_OUT_BUTTON	    2
#define ZOOM_OPTION_PAN_BUTTON	    3
#define ZOOM_OPTION_SPEED	    4
#define ZOOM_OPTION_TIMESTEP	    5
#define ZOOM_OPTION_ZOOM_FACTOR     6
#define ZOOM_OPTION_FILTER_LINEAR   7
#define ZOOM_OPTION_NUM		    8

#define ZOOM_SCREEN(s)						        \
    ZoomScreen *zs = ZoomScreen::get (s)

struct ZoomBox {
    float x1;
    float y1;
    float x2;
    float y2;
};

class ZoomScreen :
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface,
    public PrivateHandler<ZoomScreen,CompScreen>
{
    public:
	
	ZoomScreen (CompScreen *screen);
	~ZoomScreen ();

	CompOption::Vector & getOptions ();
	bool setOption (const char *name, CompOption::Value &value);	

	void handleEvent (XEvent *);

	void preparePaint (int);
	void donePaint ();

	bool glPaintOutput (const GLScreenPaintAttrib &,
			    const GLMatrix &, const CompRegion &,
			    CompOutput *, unsigned int);

	void getCurrentZoom (int output, ZoomBox *pBox);
	void handleMotionEvent (int xRoot, int yRoot);
	void initiateForSelection (int output);

	void zoomInEvent ();
	void zoomOutEvent ();

	CompositeScreen *cScreen;
	GLScreen        *gScreen;
	
	CompOption::Vector opt;

	float pointerSensitivity;

	CompScreen::GrabHandle grabIndex;
	bool grab;

	int zoomed;

	bool adjust;

	CompScreen::GrabHandle panGrabIndex;
	Cursor panCursor;

	GLfloat velocity;
	GLfloat scale;

	ZoomBox current[16];
	ZoomBox last[16];

	int x1, y1, x2, y2;

	int zoomOutput;
};

class ZoomPluginVTable :
    public CompPlugin::VTableForScreen<ZoomScreen>
{
    public:

	const char * name () { return "zoom"; };

	CompMetadata * getMetadata ();

	bool init ();
	void fini ();

	PLUGIN_OPTION_HELPER (ZoomScreen);

};
