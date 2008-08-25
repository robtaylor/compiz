#ifndef _COMPSCREEN_H
#define _COMPSCREEN_H

#include <compwindow.h>
#include <comptexture.h>
#include <compfragment.h>
#include <compmatrix.h>
#include <compoutput.h>
#include <compsession.h>

class CompScreen;
class PrivateScreen;
typedef std::list<CompWindow *> CompWindowList;

extern GLushort   defaultColor[4];

#if COMPOSITE_MAJOR > 0 || COMPOSITE_MINOR > 2
#define USE_COW
#endif

/* camera distance from screen, 0.5 * tan (FOV) */
#define DEFAULT_Z_CAMERA 0.866025404f

#define OPAQUE 0xffff
#define COLOR  0xffff
#define BRIGHT 0xffff

#define GET_CORE_SCREEN(object) (dynamic_cast<CompScreen *> (object))
#define CORE_SCREEN(object) CompScreen *s = GET_CORE_SCREEN (object)

#define PAINT_SCREEN_REGION_MASK		   (1 << 0)
#define PAINT_SCREEN_FULL_MASK			   (1 << 1)
#define PAINT_SCREEN_TRANSFORMED_MASK		   (1 << 2)
#define PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK (1 << 3)
#define PAINT_SCREEN_CLEAR_MASK			   (1 << 4)
#define PAINT_SCREEN_NO_OCCLUSION_DETECTION_MASK   (1 << 5)
#define PAINT_SCREEN_NO_BACKGROUND_MASK            (1 << 6)

#ifndef GLX_EXT_texture_from_pixmap
#define GLX_BIND_TO_TEXTURE_RGB_EXT        0x20D0
#define GLX_BIND_TO_TEXTURE_RGBA_EXT       0x20D1
#define GLX_BIND_TO_MIPMAP_TEXTURE_EXT     0x20D2
#define GLX_BIND_TO_TEXTURE_TARGETS_EXT    0x20D3
#define GLX_Y_INVERTED_EXT                 0x20D4
#define GLX_TEXTURE_FORMAT_EXT             0x20D5
#define GLX_TEXTURE_TARGET_EXT             0x20D6
#define GLX_MIPMAP_TEXTURE_EXT             0x20D7
#define GLX_TEXTURE_FORMAT_NONE_EXT        0x20D8
#define GLX_TEXTURE_FORMAT_RGB_EXT         0x20D9
#define GLX_TEXTURE_FORMAT_RGBA_EXT        0x20DA
#define GLX_TEXTURE_1D_BIT_EXT             0x00000001
#define GLX_TEXTURE_2D_BIT_EXT             0x00000002
#define GLX_TEXTURE_RECTANGLE_BIT_EXT      0x00000004
#define GLX_TEXTURE_1D_EXT                 0x20DB
#define GLX_TEXTURE_2D_EXT                 0x20DC
#define GLX_TEXTURE_RECTANGLE_EXT          0x20DD
#define GLX_FRONT_LEFT_EXT                 0x20DE
#endif

typedef void (*FuncPtr) (void);
typedef FuncPtr (*GLXGetProcAddressProc) (const GLubyte *procName);

typedef void    (*GLXBindTexImageProc)    (Display	 *display,
					   GLXDrawable	 drawable,
					   int		 buffer,
					   int		 *attribList);
typedef void    (*GLXReleaseTexImageProc) (Display	 *display,
					   GLXDrawable	 drawable,
					   int		 buffer);
typedef void    (*GLXQueryDrawableProc)   (Display	 *display,
					   GLXDrawable	 drawable,
					   int		 attribute,
					   unsigned int  *value);

typedef void (*GLXCopySubBufferProc) (Display     *display,
				      GLXDrawable drawable,
				      int	  x,
				      int	  y,
				      int	  width,
				      int	  height);

typedef int (*GLXGetVideoSyncProc)  (unsigned int *count);
typedef int (*GLXWaitVideoSyncProc) (int	  divisor,
				     int	  remainder,
				     unsigned int *count);

#ifndef GLX_VERSION_1_3
typedef struct __GLXFBConfigRec *GLXFBConfig;
#endif

typedef GLXFBConfig *(*GLXGetFBConfigsProc) (Display *display,
					     int     screen,
					     int     *nElements);
typedef int (*GLXGetFBConfigAttribProc) (Display     *display,
					 GLXFBConfig config,
					 int	     attribute,
					 int	     *value);
typedef GLXPixmap (*GLXCreatePixmapProc) (Display     *display,
					  GLXFBConfig config,
					  Pixmap      pixmap,
					  const int   *attribList);

typedef void (*GLActiveTextureProc) (GLenum texture);
typedef void (*GLClientActiveTextureProc) (GLenum texture);
typedef void (*GLMultiTexCoord2fProc) (GLenum, GLfloat, GLfloat);

typedef void (*GLGenProgramsProc) (GLsizei n,
				   GLuint  *programs);
typedef void (*GLDeleteProgramsProc) (GLsizei n,
				      GLuint  *programs);
typedef void (*GLBindProgramProc) (GLenum target,
				   GLuint program);
typedef void (*GLProgramStringProc) (GLenum	  target,
				     GLenum	  format,
				     GLsizei	  len,
				     const GLvoid *string);
typedef void (*GLProgramParameter4fProc) (GLenum  target,
					  GLuint  index,
					  GLfloat x,
					  GLfloat y,
					  GLfloat z,
					  GLfloat w);
typedef void (*GLGetProgramivProc) (GLenum target,
				    GLenum pname,
				    int    *params);

typedef void (*GLGenFramebuffersProc) (GLsizei n,
				       GLuint  *framebuffers);
typedef void (*GLDeleteFramebuffersProc) (GLsizei n,
					  GLuint  *framebuffers);
typedef void (*GLBindFramebufferProc) (GLenum target,
				       GLuint framebuffer);
typedef GLenum (*GLCheckFramebufferStatusProc) (GLenum target);
typedef void (*GLFramebufferTexture2DProc) (GLenum target,
					    GLenum attachment,
					    GLenum textarget,
					    GLuint texture,
					    GLint  level);
typedef void (*GLGenerateMipmapProc) (GLenum target);

struct CompScreenPaintAttrib {
    GLfloat xRotate;
    GLfloat yRotate;
    GLfloat vRotate;
    GLfloat xTranslate;
    GLfloat yTranslate;
    GLfloat zTranslate;
    GLfloat zCamera;
};

extern CompScreenPaintAttrib defaultScreenPaintAttrib;

struct CompGroup {
    unsigned int      refCnt;
    Window	      id;
};

struct CompStartupSequence {
    SnStartupSequence		*sequence;
    unsigned int		viewportX;
    unsigned int		viewportY;
};

#define MAX_DEPTH 32

struct CompFBConfig {
    GLXFBConfig fbConfig;
    int         yInverted;
    int         mipmap;
    int         textureFormat;
    int         textureTargets;
};

#define NOTHING_TRANS_FILTER 0
#define SCREEN_TRANS_FILTER  1
#define WINDOW_TRANS_FILTER  2

#define SCREEN_EDGE_LEFT	0
#define SCREEN_EDGE_RIGHT	1
#define SCREEN_EDGE_TOP		2
#define SCREEN_EDGE_BOTTOM	3
#define SCREEN_EDGE_TOPLEFT	4
#define SCREEN_EDGE_TOPRIGHT	5
#define SCREEN_EDGE_BOTTOMLEFT	6
#define SCREEN_EDGE_BOTTOMRIGHT 7
#define SCREEN_EDGE_NUM		8

struct CompScreenEdge {
    Window	 id;
    unsigned int count;
};


#define ACTIVE_WINDOW_HISTORY_SIZE 64
#define ACTIVE_WINDOW_HISTORY_NUM  32

struct CompActiveWindowHistory {
    Window id[ACTIVE_WINDOW_HISTORY_SIZE];
    int    x;
    int    y;
    int    activeNum;
};

class ScreenInterface : public WrapableInterface<CompScreen> {
    public:
	ScreenInterface ();

	WRAPABLE_DEF(void, preparePaint, int);
	WRAPABLE_DEF(void, donePaint);
	WRAPABLE_DEF(void, paint, CompOutput::ptrList &outputs, unsigned int);

	WRAPABLE_DEF(bool, paintOutput, const CompScreenPaintAttrib *,
		     const CompTransform *, Region, CompOutput *,
		     unsigned int);
	WRAPABLE_DEF(void, paintTransformedOutput,
		     const CompScreenPaintAttrib *,
		     const CompTransform *, Region, CompOutput *,
		     unsigned int);
	WRAPABLE_DEF(void, applyTransform, const CompScreenPaintAttrib *,
		     CompOutput *, CompTransform *);

	WRAPABLE_DEF(void, enableOutputClipping, const CompTransform *,
		     Region, CompOutput *);
	WRAPABLE_DEF(void, disableOutputClipping);

	WRAPABLE_DEF(void, enterShowDesktopMode);
	WRAPABLE_DEF(void, leaveShowDesktopMode, CompWindow *);

	WRAPABLE_DEF(void, outputChangeNotify);

	WRAPABLE_DEF(CompWindowList, getWindowPaintList);
};


class CompScreen : public WrapableHandler<ScreenInterface>, public CompObject {

    public:
	typedef void* grabHandle;

    public:
	CompScreen ();
	~CompScreen ();

	CompString objectName ();

	bool
	init (CompDisplay *, int);

	bool
	init (CompDisplay *, int, Window, Atom, Time);

	CompDisplay *
	display ();
	
	Window
	root ();

	int
	screenNum ();

	CompWindowList &
	windows ();

	CompOption *
	getOption (const char *name);

	unsigned int
	showingDesktopMask ();
	
	bool
	handlePaintTimeout ();
	
	bool
	setOption (const char       *name,
		   CompOption::Value &value);

	void
	setCurrentOutput (unsigned int outputNum);

	void
	configure (XConfigureEvent *ce);

	bool
	hasGrab ();

	void
	warpPointer (int dx, int dy);

	Time
	getCurrentTime ();

	Atom
	selectionAtom ();

	Window
	selectionWindow ();

	Time
	selectionTimestamp ();

	void
	updateWorkareaForScreen ();

	void
	setDefaultViewport ();

	void
	damageScreen ();

	FuncPtr
	getProcAddress (const char *name);

	void
	showOutputWindow ();

	void
	hideOutputWindow ();

	void
	updateOutputWindow ();

	void
	damageRegion (Region);

	void
	damagePending ();

	void
	forEachWindow (CompWindow::ForEach);

	void
	focusDefaultWindow ();

	CompWindow *
	findWindow (Window id);

	CompWindow *
	findTopLevelWindow (Window id);

	void
	insertWindow (CompWindow *w, Window aboveId);

	void
	unhookWindow (CompWindow *w);

	grabHandle
	pushGrab (Cursor cursor, const char *name);

	void
	updateGrab (grabHandle handle, Cursor cursor);

	void
	removeGrab (grabHandle handle, CompPoint *restorePointer);

	bool
	otherGrabExist (const char *, ...);

	bool
	addAction (CompAction *action);

	void
	removeAction (CompAction *action);

	void
	updatePassiveGrabs ();

	void
	updateWorkarea ();

	void
	updateClientList ();

	void
	toolkitAction (Atom	  toolkitAction,
		       Time       eventTime,
		       Window	  window,
		       long	  data0,
		       long	  data1,
		       long	  data2);

	void
	runCommand (CompString command);

	void
	moveViewport (int tx, int ty, bool sync);

	CompGroup *
	addGroup (Window id);

	void
	removeGroup (CompGroup *group);

	CompGroup *
	findGroup (Window id);

	void
	applyStartupProperties (CompWindow *window);

	void
	sendWindowActivationRequest (Window id);

	void
	setTexEnvMode (GLenum mode);

	void
	setLighting (bool lighting);

	void
	enableEdge (int edge);

	void
	disableEdge (int edge);

	Window
	getTopWindow ();

	void
	makeCurrent ();

	void
	finishDrawing ();

	int
	outputDeviceForPoint (int x, int y);

	void
	getCurrentOutputExtents (int *x1, int *y1, int *x2, int *y2);

	void
	setNumberOfDesktops (unsigned int nDesktop);

	void
	setCurrentDesktop (unsigned int desktop);

	void
	getWorkareaForOutput (int output, XRectangle *area);

	void
	clearOutput (CompOutput *output, unsigned int mask);

	void
	viewportForGeometry (CompWindow::Geometry gm,
			     int                  *viewportX,
			     int                  *viewportY);

	int
	outputDeviceForGeometry (CompWindow::Geometry gm);

	bool
	updateDefaultIcon ();

	void
	setCurrentActiveWindowHistory (int x, int y);

	void
	addToCurrentActiveWindowHistory (Window id);

	void
	setWindowPaintOffset (int x, int y);

	int
	getTimeToNextRedraw (struct timeval *tv);

	void
	waitForVideoSync ();

	int
	maxTextureSize ();

	unsigned int
	damageMask ();

	CompPoint vp ();

	CompSize vpSize ();

	CompSize size ();

	unsigned int &
	pendingDestroys ();

	unsigned int &
	mapNum ();

	int &
	desktopWindowCount ();

	int &
	overlayWindowCount ();

	CompOutput::vector &
	outputDevs ();

	XRectangle
	workArea ();

	unsigned int
	currentDesktop ();

	unsigned int
	nDesktop ();

	CompActiveWindowHistory *
	currentHistory ();

	CompScreenEdge &
	screenEdge (int);

	Window
	overlay ();

	unsigned int &
	activeNum ();

	void
	handleExposeEvent (XExposeEvent *event);

	void
	detectRefreshRate ();

	void
	updateBackground ();

	bool
	textureNonPowerOfTwo ();

	bool
	textureCompression ();

	bool
	canDoSaturated ();

	bool
	canDoSlightlySaturated ();

	bool
	lighting ();

	CompTexture::Filter
	filter (int);

	CompFragment::Storage *
	fragmentStorage ();

	bool
	fragmentProgram ();

	bool
	framebufferObject ();
	
	CompFBConfig * glxPixmapFBConfig (unsigned int depth);

	static int allocPrivateIndex ();
	static void freePrivateIndex (int index);

	WRAPABLE_HND(void, preparePaint, int);
	WRAPABLE_HND(void, donePaint);
	WRAPABLE_HND(void, paint, CompOutput::ptrList &outputs, unsigned int);

	WRAPABLE_HND(bool, paintOutput, const CompScreenPaintAttrib *,
		     const CompTransform *, Region, CompOutput *,
		     unsigned int);
	WRAPABLE_HND(void, paintTransformedOutput,
		     const CompScreenPaintAttrib *,
		     const CompTransform *, Region, CompOutput *,
		     unsigned int);
	WRAPABLE_HND(void, applyTransform, const CompScreenPaintAttrib *,
		     CompOutput *, CompTransform *);

	WRAPABLE_HND(void, enableOutputClipping, const CompTransform *,
		     Region, CompOutput *);
	WRAPABLE_HND(void, disableOutputClipping);

	WRAPABLE_HND(void, enterShowDesktopMode);
	WRAPABLE_HND(void, leaveShowDesktopMode, CompWindow *);

	WRAPABLE_HND(void, outputChangeNotify);

	WRAPABLE_HND(CompWindowList, getWindowPaintList);

	GLXBindTexImageProc      bindTexImage;
	GLXReleaseTexImageProc   releaseTexImage;
	GLXQueryDrawableProc     queryDrawable;
	GLXCopySubBufferProc     copySubBuffer;
	GLXGetVideoSyncProc      getVideoSync;
	GLXWaitVideoSyncProc     waitVideoSync;
	GLXGetFBConfigsProc      getFBConfigs;
	GLXGetFBConfigAttribProc getFBConfigAttrib;
	GLXCreatePixmapProc      createPixmap;

	GLActiveTextureProc       activeTexture;
	GLClientActiveTextureProc clientActiveTexture;
	GLMultiTexCoord2fProc     multiTexCoord2f;

	GLGenProgramsProc	     genPrograms;
	GLDeleteProgramsProc     deletePrograms;
	GLBindProgramProc	     bindProgram;
	GLProgramStringProc	     programString;
	GLProgramParameter4fProc programEnvParameter4f;
	GLProgramParameter4fProc programLocalParameter4f;
	GLGetProgramivProc       getProgramiv;

	GLGenFramebuffersProc        genFramebuffers;
	GLDeleteFramebuffersProc     deleteFramebuffers;
	GLBindFramebufferProc        bindFramebuffer;
	GLCheckFramebufferStatusProc checkFramebufferStatus;
	GLFramebufferTexture2DProc   framebufferTexture2D;
	GLGenerateMipmapProc         generateMipmap;

    private:
	PrivateScreen *priv;

    public :
	static bool
	mainMenu (CompDisplay        *d,
		  CompAction         *action,
		  CompAction::State  state,
		  CompOption::Vector &options);

	static bool
	runDialog (CompDisplay        *d,
		   CompAction         *action,
		   CompAction::State  state,
		   CompOption::Vector &options);

	static bool
	showDesktop (CompDisplay        *d,
		     CompAction         *action,
		     CompAction::State  state,
		     CompOption::Vector &options);

	static bool
	toggleSlowAnimations (CompDisplay        *d,
			      CompAction         *action,
			      CompAction::State  state,
			      CompOption::Vector &options);

	static bool
	windowMenu (CompDisplay        *d,
		    CompAction         *action,
		    CompAction::State  state,
		    CompOption::Vector &options);

	static CompOption::Vector &
	getScreenOptions (CompObject *object);

	static bool setScreenOption (CompObject        *object,
				     const char        *name,
				     CompOption::Value &value);

	static void
	compScreenSnEvent (SnMonitorEvent *event,
			   void           *userData);
};

#endif
