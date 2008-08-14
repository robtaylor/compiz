/*
 * Copyright © 2007 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Novell, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#ifndef _COMPIZ_CORE_H
#define _COMPIZ_CORE_H

#include <compplugin.h>

#define CORE_ABIVERSION 20080618

#include <stdio.h>
#include <sys/time.h>
#include <assert.h>

#include <X11/Xlib-xcb.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/sync.h>
#include <X11/Xregion.h>
#include <X11/XKBlib.h>

#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>

#include <GL/gl.h>
#include <GL/glx.h>


#define TIMEVALDIFF(tv1, tv2)						   \
    ((tv1)->tv_sec == (tv2)->tv_sec || (tv1)->tv_usec >= (tv2)->tv_usec) ? \
    ((((tv1)->tv_sec - (tv2)->tv_sec) * 1000000) +			   \
     ((tv1)->tv_usec - (tv2)->tv_usec)) / 1000 :			   \
    ((((tv1)->tv_sec - 1 - (tv2)->tv_sec) * 1000000) +			   \
     (1000000 + (tv1)->tv_usec - (tv2)->tv_usec)) / 1000


COMPIZ_BEGIN_DECLS

#if COMPOSITE_MAJOR > 0 || COMPOSITE_MINOR > 2
#define USE_COW
#endif

/*
 * WORDS_BIGENDIAN should be defined before including this file for
 * IMAGE_BYTE_ORDER and BITMAP_BIT_ORDER to be set correctly.
 */
#define LSBFirst 0
#define MSBFirst 1

#ifdef WORDS_BIGENDIAN
#  define IMAGE_BYTE_ORDER MSBFirst
#  define BITMAP_BIT_ORDER MSBFirst
#else
#  define IMAGE_BYTE_ORDER LSBFirst
#  define BITMAP_BIT_ORDER LSBFirst
#endif

class CompTexture;
class CompIcon;
typedef struct _CompWindowExtents CompWindowExtents;
typedef struct _CompProgram	  CompProgram;
typedef struct _CompFunction	  CompFunction;
typedef struct _CompFunctionData  CompFunctionData;
typedef struct _FragmentAttrib    FragmentAttrib;
class CompMatch;
class CompOutput;
typedef struct _CompWalker        CompWalker;

class CompDisplay;

/* virtual modifiers */

#define CompModAlt        0
#define CompModMeta       1
#define CompModSuper      2
#define CompModHyper      3
#define CompModModeSwitch 4
#define CompModNumLock    5
#define CompModScrollLock 6
#define CompModNum        7

#define CompAltMask        (1 << 16)
#define CompMetaMask       (1 << 17)
#define CompSuperMask      (1 << 18)
#define CompHyperMask      (1 << 19)
#define CompModeSwitchMask (1 << 20)
#define CompNumLockMask    (1 << 21)
#define CompScrollLockMask (1 << 22)

#define CompNoMask         (1 << 25)

#define OPAQUE 0xffff
#define COLOR  0xffff
#define BRIGHT 0xffff

#define RED_SATURATION_WEIGHT   0.30f
#define GREEN_SATURATION_WEIGHT 0.59f
#define BLUE_SATURATION_WEIGHT  0.11f

extern char       *programName;
extern char       **programArgv;
extern int        programArgc;
extern char       *backgroundImage;
extern REGION     emptyRegion;
extern REGION     infiniteRegion;
extern GLushort   defaultColor[4];
extern Window     currentRoot;
extern Bool       shutDown;
extern Bool       restartSignal;
extern CompWindow *lastFoundWindow;
extern CompWindow *lastDamagedWindow;
extern Bool       replaceCurrentWm;
extern Bool       indirectRendering;
extern Bool       strictBinding;
extern Bool       useCow;
extern Bool       noDetection;
extern Bool	  useDesktopHints;
extern Bool       onlyCurrentScreen;

extern int  defaultRefreshRate;
extern char *defaultTextureFilter;

extern int lastPointerX;
extern int lastPointerY;
extern int pointerX;
extern int pointerY;

extern CompCore     *core;
extern CompMetadata coreMetadata;

#define RESTRICT_VALUE(value, min, max)				     \
    (((value) < (min)) ? (min): ((value) > (max)) ? (max) : (value))

#define MOD(a,b) ((a) < 0 ? ((b) - ((-(a) - 1) % (b))) - 1 : (a) % (b))


/* privates.c */

#define WRAP(priv, real, func, wrapFunc) \
    (priv)->func = (real)->func;	 \
    (real)->func = (wrapFunc)

#define UNWRAP(priv, real, func) \
    (real)->func = (priv)->func


typedef union _CompPrivate {
    void	  *ptr;
    long	  val;
    unsigned long uval;
    void	  *(*fptr) (void);
} CompPrivate;


/* session.c */

typedef enum {
    CompSessionEventSaveYourself = 0,
    CompSessionEventSaveComplete,
    CompSessionEventDie,
    CompSessionEventShutdownCancelled
} CompSessionEvent;

typedef enum {
    CompSessionClientId = 0,
    CompSessionPrevClientId
} CompSessionClientIdType;



void
initSession (char *smPrevClientId);

void
closeSession (void);


char *
getSessionClientId (CompSessionClientIdType type);

/* option.c */

typedef enum {
    CompBindingTypeNone       = 0,
    CompBindingTypeKey        = 1 << 0,
    CompBindingTypeButton     = 1 << 1,
    CompBindingTypeEdgeButton = 1 << 2
} CompBindingTypeEnum;

typedef unsigned int CompBindingType;

typedef enum {
    CompActionStateInitKey     = 1 <<  0,
    CompActionStateTermKey     = 1 <<  1,
    CompActionStateInitButton  = 1 <<  2,
    CompActionStateTermButton  = 1 <<  3,
    CompActionStateInitBell    = 1 <<  4,
    CompActionStateInitEdge    = 1 <<  5,
    CompActionStateTermEdge    = 1 <<  6,
    CompActionStateInitEdgeDnd = 1 <<  7,
    CompActionStateTermEdgeDnd = 1 <<  8,
    CompActionStateCommit      = 1 <<  9,
    CompActionStateCancel      = 1 << 10,
    CompActionStateAutoGrab    = 1 << 11,
    CompActionStateNoEdgeDelay = 1 << 12
} CompActionStateEnum;

typedef unsigned int CompActionState;

typedef enum {
    CompLogLevelFatal = 0,
    CompLogLevelError,
    CompLogLevelWarn,
    CompLogLevelInfo,
    CompLogLevelDebug
} CompLogLevel;

typedef struct _CompKeyBinding {
    int		 keycode;
    unsigned int modifiers;
} CompKeyBinding;

typedef struct _CompButtonBinding {
    int		 button;
    unsigned int modifiers;
} CompButtonBinding;

typedef struct _CompAction CompAction;

typedef bool (*CompActionCallBackProc) (CompDisplay	*d,
					CompAction	*action,
					CompActionState state,
					CompOption	*option,
					int		nOption);

struct _CompAction {
    CompActionCallBackProc initiate;
    CompActionCallBackProc terminate;

    CompActionState state;

    CompBindingType   type;
    CompKeyBinding    key;
    CompButtonBinding button;

    Bool bell;

    unsigned int edgeMask;

    CompPrivate priv;
};



typedef struct {
    CompOptionType  type;
    CompOptionValue *value;
    int		    nValue;
} CompListValue;

union _CompOptionValue {
    Bool	   b;
    int		   i;
    float	   f;
    char	   *s;
    unsigned short c[4];
    CompAction     action;
    CompMatch      *match;
    CompListValue  list;
};

typedef struct _CompOptionIntRestriction {
    int min;
    int max;
} CompOptionIntRestriction;

typedef struct _CompOptionFloatRestriction {
    float min;
    float max;
    float precision;
} CompOptionFloatRestriction;

typedef union {
    CompOptionIntRestriction    i;
    CompOptionFloatRestriction  f;
} CompOptionRestriction;

struct _CompOption {
    char		  *name;
    CompOptionType	  type;
    CompOptionValue	  value;
    CompOptionRestriction rest;
};

typedef CompOption *(*DisplayOptionsProc) (CompDisplay *display, int *count);
typedef CompOption *(*ScreenOptionsProc) (CompScreen *screen, int *count);

Bool
getBoolOptionNamed (CompOption *option,
		    int	       nOption,
		    const char *name,
		    Bool       defaultValue);

int
getIntOptionNamed (CompOption *option,
		   int	      nOption,
		   const char *name,
		   int	      defaultValue);

float
getFloatOptionNamed (CompOption *option,
		     int	nOption,
		     const char *name,
		     float	defaultValue);

char *
getStringOptionNamed (CompOption *option,
		      int	 nOption,
		      const char *name,
		      char	 *defaultValue);

unsigned short *
getColorOptionNamed (CompOption	    *option,
		     int	    nOption,
		     const char     *name,
		     unsigned short *defaultValue);

CompMatch *
getMatchOptionNamed (CompOption	*option,
		     int	nOption,
		     const char *name,
		     CompMatch  *defaultValue);

char *
keyBindingToString (CompDisplay    *d,
		    CompKeyBinding *key);

char *
buttonBindingToString (CompDisplay       *d,
		       CompButtonBinding *button);

char *
keyActionToString (CompDisplay *d,
		   CompAction  *action);

char *
buttonActionToString (CompDisplay *d,
		      CompAction  *action);

Bool
stringToKeyBinding (CompDisplay    *d,
		    const char     *binding,
		    CompKeyBinding *key);

Bool
stringToButtonBinding (CompDisplay	 *d,
		       const char	 *binding,
		       CompButtonBinding *button);

void
stringToKeyAction (CompDisplay *d,
		   const char  *binding,
		   CompAction  *action);

void
stringToButtonAction (CompDisplay *d,
		      const char  *binding,
		      CompAction  *action);

const char *
edgeToString (unsigned int edge);

unsigned int
stringToEdgeMask (const char *edge);

char *
edgeMaskToString (unsigned int edgeMask);

Bool
stringToColor (const char     *color,
	       unsigned short *rgba);

char *
colorToString (unsigned short *rgba);

const char *
optionTypeToString (CompOptionType type);

Bool
isActionOption (CompOption *option);


/* core.c */

/* display.c */



typedef void (*ForEachWindowProc) (CompWindow *window,
				   void	      *closure);




bool
setDisplayOption (CompObject      *object,
		  const char      *name,
		  CompOptionValue *value);

void
compLogMessage (CompDisplay  *d,
		const char   *componentName,
		CompLogLevel level,
		const char   *format,
		...);

const char *
logLevelToString (CompLogLevel level);

int
compCheckForError (Display *dpy);

void
warpPointer (CompScreen *screen,
	     int	 dx,
	     int	 dy);

Bool
setDisplayAction (CompDisplay     *display,
		  CompOption      *o,
		  CompOptionValue *value);


/* event.c */

typedef struct _CompDelayedEdgeSettings
{
    unsigned int edge;
    unsigned int state;

    CompOption   option[7];
    unsigned int nOption;
} CompDelayedEdgeSettings;


void
handleSyncAlarm (CompWindow *w);

Bool
eventMatches (CompDisplay *display,
	      XEvent      *event,
	      CompOption  *option);

Bool
eventTerminates (CompDisplay *display,
		 XEvent      *event,
		 CompOption  *option);

/* paint.c */

#define MULTIPLY_USHORT(us1, us2)		 \
    (((GLuint) (us1) * (GLuint) (us2)) / 0xffff)

/* camera distance from screen, 0.5 * tan (FOV) */
#define DEFAULT_Z_CAMERA 0.866025404f

#define DEG2RAD (M_PI / 180.0f)

typedef struct _CompTransform {
    float m[16];
} CompTransform;

typedef union _CompVector {
    float v[4];
    struct {
	float x;
	float y;
	float z;
	float w;
    };
} CompVector;

/* XXX: ScreenPaintAttrib will be removed */
typedef struct _ScreenPaintAttrib {
    GLfloat xRotate;
    GLfloat yRotate;
    GLfloat vRotate;
    GLfloat xTranslate;
    GLfloat yTranslate;
    GLfloat zTranslate;
    GLfloat zCamera;
} ScreenPaintAttrib;

/* XXX: scale and translate fields will be removed */
typedef struct _WindowPaintAttrib {
    GLushort opacity;
    GLushort brightness;
    GLushort saturation;
    GLfloat  xScale;
    GLfloat  yScale;
    GLfloat  xTranslate;
    GLfloat  yTranslate;
} WindowPaintAttrib;

extern ScreenPaintAttrib defaultScreenPaintAttrib;
extern WindowPaintAttrib defaultWindowPaintAttrib;



typedef void (*WalkerFiniProc) (CompScreen *screen,
				CompWalker *walker);

typedef CompWindow *(*WalkInitProc) (CompScreen *screen);
typedef CompWindow *(*WalkStepProc) (CompWindow *window);

struct _CompWalker {
    WalkerFiniProc fini;
    CompPrivate	   priv;

    WalkInitProc first;
    WalkInitProc last;
    WalkStepProc next;
    WalkStepProc prev;
};


/* XXX: prepareXCoords will be removed */
void
prepareXCoords (CompScreen *screen,
		CompOutput *output,
		float      z);

/* screen.c */


#define MAX_DEPTH 32

typedef struct _CompGroup {
    struct _CompGroup *next;
    unsigned int      refCnt;
    Window	      id;
} CompGroup;

typedef struct _CompStartupSequence {
    struct _CompStartupSequence *next;
    SnStartupSequence		*sequence;
    unsigned int		viewportX;
    unsigned int		viewportY;
} CompStartupSequence;

typedef struct _CompFBConfig {
    GLXFBConfig fbConfig;
    int         yInverted;
    int         mipmap;
    int         textureFormat;
    int         textureTargets;
} CompFBConfig;

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

typedef struct _CompScreenEdge {
    Window	 id;
    unsigned int count;
} CompScreenEdge;


#define ACTIVE_WINDOW_HISTORY_SIZE 64
#define ACTIVE_WINDOW_HISTORY_NUM  32

typedef struct _CompActiveWindowHistory {
    Window id[ACTIVE_WINDOW_HISTORY_SIZE];
    int    x;
    int    y;
    int    activeNum;
} CompActiveWindowHistory;

bool
setScreenOption (CompObject      *object,
		 const char      *name,
		 CompOptionValue *value);



/* window.c */


typedef enum {
    CompStackingUpdateModeNone = 0,
    CompStackingUpdateModeNormal,
    CompStackingUpdateModeAboveFullscreen,
    CompStackingUpdateModeInitialMap,
    CompStackingUpdateModeInitialMapDeniedFocus
} CompStackingUpdateMode;

struct _CompWindowExtents {
    int left;
    int right;
    int top;
    int bottom;
};

typedef struct _CompStruts {
    XRectangle left;
    XRectangle right;
    XRectangle top;
    XRectangle bottom;
} CompStruts;


/* plugin.c */

#define HOME_PLUGINDIR ".compiz/plugins"

typedef CompPluginVTable *(*PluginGetInfoProc) (void);

typedef Bool (*LoadPluginProc) (CompPlugin *p,
				const char *path,
				const char *name);

typedef void (*UnloadPluginProc) (CompPlugin *p);

typedef char **(*ListPluginsProc) (const char *path,
				   int	      *n);

extern LoadPluginProc   loaderLoadPlugin;
extern UnloadPluginProc loaderUnloadPlugin;
extern ListPluginsProc  loaderListPlugins;

struct _CompPlugin {
    CompPlugin       *next;
    CompPrivate	     devPrivate;
    char	     *devType;
    CompPluginVTable *vTable;
};

bool
objectInitPlugins (CompObject *o);

void
objectFiniPlugins (CompObject *o);

CompPlugin *
findActivePlugin (const char *name);

CompPlugin *
loadPlugin (const char *plugin);

void
unloadPlugin (CompPlugin *p);

Bool
pushPlugin (CompPlugin *p);

CompPlugin *
popPlugin (void);

CompPlugin *
getPlugins (void);

char **
availablePlugins (int *n);

int
getPluginABI (const char *name);

Bool
checkPluginABI (const char *name,
		int	   abi);

Bool
getPluginDisplayIndex (CompDisplay *d,
		       const char  *name,
		       int	   *index);


/* fragment.c */

#define MAX_FRAGMENT_FUNCTIONS 16

struct _FragmentAttrib {
    GLushort opacity;
    GLushort brightness;
    GLushort saturation;
    int	     nTexture;
    int	     function[MAX_FRAGMENT_FUNCTIONS];
    int	     nFunction;
    int	     nParam;
};

CompFunctionData *
createFunctionData (void);

void
destroyFunctionData (CompFunctionData *data);

Bool
addTempHeaderOpToFunctionData (CompFunctionData *data,
			       const char	*name);

Bool
addParamHeaderOpToFunctionData (CompFunctionData *data,
				const char	 *name);

Bool
addAttribHeaderOpToFunctionData (CompFunctionData *data,
				 const char	  *name);

#define COMP_FETCH_TARGET_2D   0
#define COMP_FETCH_TARGET_RECT 1
#define COMP_FETCH_TARGET_NUM  2

Bool
addFetchOpToFunctionData (CompFunctionData *data,
			  const char	   *dst,
			  const char	   *offset,
			  int		   target);

Bool
addColorOpToFunctionData (CompFunctionData *data,
			  const char	   *dst,
			  const char	   *src);

Bool
addDataOpToFunctionData (CompFunctionData *data,
			 const char	  *str,
			 ...);

Bool
addBlendOpToFunctionData (CompFunctionData *data,
			  const char	   *str,
			  ...);

int
createFragmentFunction (CompScreen	 *s,
			const char	 *name,
			CompFunctionData *data);

void
destroyFragmentFunction (CompScreen *s,
			 int	    id);

int
getSaturateFragmentFunction (CompScreen  *s,
			     CompTexture *texture,
			     int	 param);

int
allocFragmentTextureUnits (FragmentAttrib *attrib,
			   int		  nTexture);

int
allocFragmentParameters (FragmentAttrib *attrib,
			 int		nParam);

void
addFragmentFunction (FragmentAttrib *attrib,
		     int	    function);

void
initFragmentAttrib (FragmentAttrib	    *attrib,
		    const WindowPaintAttrib *paint);

bool
enableFragmentAttrib (CompScreen     *s,
		      FragmentAttrib *attrib,
		      Bool	     *blending);

void
disableFragmentAttrib (CompScreen     *s,
		       FragmentAttrib *attrib);


/* matrix.c */

void
matrixMultiply (CompTransform       *product,
		const CompTransform *transformA,
		const CompTransform *transformB);

void
matrixMultiplyVector (CompVector          *product,
		      const CompVector    *vector,
		      const CompTransform *transform);

void
matrixVectorDiv (CompVector *v);

void
matrixRotate (CompTransform *transform,
	      float	    angle,
	      float	    x,
	      float	    y,
	      float	    z);

void
matrixScale (CompTransform *transform,
	     float	   x,
	     float	   y,
	     float	   z);

void
matrixTranslate (CompTransform *transform,
		 float	       x,
		 float	       y,
		 float	       z);

void
matrixGetIdentity (CompTransform *m);

/* match.c */


/* metadata.c */

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY (x)
#define MINTOSTRING(x) "<min>" TOSTRING (x) "</min>"
#define MAXTOSTRING(x) "<max>" TOSTRING (x) "</max>"
#define RESTOSTRING(min, max) MINTOSTRING (min) MAXTOSTRING (max)

typedef struct _CompMetadataOptionInfo {
    const char		   *name;
    const char		   *type;
    const char		   *data;
    CompActionCallBackProc initiate;
    CompActionCallBackProc terminate;
} CompMetadataOptionInfo;

struct _CompMetadata {
    char   *path;
    xmlDoc **doc;
    int    nDoc;
};

Bool
compInitPluginMetadataFromInfo (CompMetadata		     *metadata,
				const char		     *plugin,
				const CompMetadataOptionInfo *displayOptionInfo,
				int			     nDisplayOptionInfo,
				const CompMetadataOptionInfo *screenOptionInfo,
				int			     nScreenOptionInfo);

Bool
compInitScreenOptionFromMetadata (CompScreen   *screen,
				  CompMetadata *metadata,
				  CompOption   *option,
				  const char   *name);

void
compFiniScreenOption (CompScreen *screen,
		      CompOption *option);

Bool
compInitScreenOptionsFromMetadata (CompScreen			*screen,
				   CompMetadata			*metadata,
				   const CompMetadataOptionInfo *info,
				   CompOption			*option,
				   int				n);

void
compFiniScreenOptions (CompScreen *screen,
		       CompOption *option,
		       int	  n);

Bool
compSetScreenOption (CompScreen      *screen,
		     CompOption      *option,
		     CompOptionValue *value);

Bool
compInitDisplayOptionFromMetadata (CompDisplay  *display,
				   CompMetadata *metadata,
				   CompOption   *option,
				   const char   *name);

void
compFiniDisplayOption (CompDisplay *display,
		       CompOption  *option);

Bool
compInitDisplayOptionsFromMetadata (CompDisplay			 *display,
				    CompMetadata		 *metadata,
				    const CompMetadataOptionInfo *info,
				    CompOption			 *option,
				    int				 n);

void
compFiniDisplayOptions (CompDisplay *display,
			CompOption  *option,
			int	    n);

Bool
compSetDisplayOption (CompDisplay     *display,
		      CompOption      *option,
		      CompOptionValue *value);

char *
compGetShortPluginDescription (CompMetadata *metadata);

char *
compGetLongPluginDescription (CompMetadata *metadata);

char *
compGetShortScreenOptionDescription (CompMetadata *metadata,
				     CompOption   *option);

char *
compGetLongScreenOptionDescription (CompMetadata *metadata,
				    CompOption   *option);

char *
compGetShortDisplayOptionDescription (CompMetadata *metadata,
				      CompOption   *option);

char *
compGetLongDisplayOptionDescription (CompMetadata *metadata,
				     CompOption   *option);

int
compReadXmlChunkFromMetadataOptionInfo (const CompMetadataOptionInfo *info,
					int			     *offset,
					char			     *buffer,
					int			     length);



COMPIZ_END_DECLS
	
#include <string>
#include <vector>
#include <list>

typedef std::string CompString;

#include <comprect.h>
#include <compoutput.h>
#include <compobject.h>
#include <compcore.h>
#include <compdisplay.h>
#include <compscreen.h>
#include <compwindow.h>
		 
#endif
