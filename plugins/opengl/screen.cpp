#include "privates.h"

#include <dlfcn.h>
#include <math.h>

GLScreen   *targetScreen = NULL;
CompOutput *targetOutput = NULL;

Window currentRoot = None;

GLScreen::GLScreen (CompScreen *s) :
    OpenGLPrivateHandler<GLScreen, CompScreen, COMPIZ_OPENGL_ABI> (s),
    priv (new PrivateGLScreen (s, this))
{
    CompDisplay          *d = s->display ();
    Display		 *dpy = d->dpy ();
    XVisualInfo		 templ;
    XVisualInfo		 *visinfo;
    GLXFBConfig		 *fbConfigs;
    int			 defaultDepth, nvisinfo, nElements, value, i;
    const char		 *glxExtensions, *glExtensions;
    GLfloat		 globalAmbient[]  = { 0.1f, 0.1f,  0.1f, 0.1f };
    GLfloat		 ambientLight[]   = { 0.0f, 0.0f,  0.0f, 0.0f };
    GLfloat		 diffuseLight[]   = { 0.9f, 0.9f,  0.9f, 0.9f };
    GLfloat		 light0Position[] = { -0.5f, 0.5f, -9.0f, 1.0f };
    XWindowAttributes    attr;

    priv->tmpRegion = XCreateRegion ();
    if (!priv->tmpRegion)
    {
	setFailed ();
	return;
    }
    priv->outputRegion = XCreateRegion ();
    if (!priv->outputRegion)
    {
	setFailed ();
	return;
    }

    if (!glMetadata->initScreenOptions (s, glScreenOptionInfo,
					GL_SCREEN_OPTION_NUM, priv->opt))
    {
	setFailed ();
	return;
    }

    if (!XGetWindowAttributes (dpy, s->root (), &attr))
    {
	setFailed ();
	return;
    }

    templ.visualid = XVisualIDFromVisual (attr.visual);

    visinfo = XGetVisualInfo (dpy, VisualIDMask, &templ, &nvisinfo);
    if (!nvisinfo)
    {
	compLogMessage (d, "opengl", CompLogLevelFatal,
			"Couldn't get visual info for default visual");
	setFailed ();
	return;
    }

    defaultDepth = visinfo->depth;

    glXGetConfig (dpy, visinfo, GLX_USE_GL, &value);
    if (!value)
    {
	compLogMessage (d, "opengl", CompLogLevelFatal,
			"Root visual is not a GL visual");
	XFree (visinfo);
	setFailed ();
	return;
    }

    glXGetConfig (dpy, visinfo, GLX_DOUBLEBUFFER, &value);
    if (!value)
    {
	compLogMessage (d, "opengl", CompLogLevelFatal,
			"Root visual is not a double buffered GL visual");
	XFree (visinfo);
	setFailed ();
	return;
    }

    priv->ctx = glXCreateContext (dpy, visinfo, NULL, !indirectRendering);
    if (!priv->ctx)
    {
	compLogMessage (d, "opengl", CompLogLevelFatal,
			"glXCreateContext failed");
	XFree (visinfo);

	setFailed ();
	return;
    }

    glxExtensions = glXQueryExtensionsString (dpy, s->screenNum ());
    if (!strstr (glxExtensions, "GLX_EXT_texture_from_pixmap"))
    {
	compLogMessage (d, "opengl", CompLogLevelFatal,
			"GLX_EXT_texture_from_pixmap is missing");
	XFree (visinfo);

	setFailed ();
	return;
    }

    XFree (visinfo);

    if (!strstr (glxExtensions, "GLX_SGIX_fbconfig"))
    {
	compLogMessage (d, "opengl", CompLogLevelFatal,
			"GLX_SGIX_fbconfig is missing");
	setFailed ();
	return;
    }

    priv->getProcAddress = (GLXGetProcAddressProc)
	getProcAddress ("glXGetProcAddressARB");
    bindTexImage = (GLXBindTexImageProc)
	getProcAddress ("glXBindTexImageEXT");
    releaseTexImage = (GLXReleaseTexImageProc)
	getProcAddress ("glXReleaseTexImageEXT");
    queryDrawable = (GLXQueryDrawableProc)
	getProcAddress ("glXQueryDrawable");
    getFBConfigs = (GLXGetFBConfigsProc)
	getProcAddress ("glXGetFBConfigs");
    getFBConfigAttrib = (GLXGetFBConfigAttribProc)
	getProcAddress ("glXGetFBConfigAttrib");
    createPixmap = (GLXCreatePixmapProc)
	getProcAddress ("glXCreatePixmap");

    if (!bindTexImage)
    {
	compLogMessage (d, "opengl", CompLogLevelFatal,
			"glXBindTexImageEXT is missing");
	setFailed ();
	return;
    }

    if (!releaseTexImage)
    {
	compLogMessage (d, "opengl", CompLogLevelFatal,
			"glXReleaseTexImageEXT is missing");
	setFailed ();
	return;
    }

    if (!queryDrawable     ||
	!getFBConfigs      ||
	!getFBConfigAttrib ||
	!createPixmap)
    {
	compLogMessage (d, "opengl", CompLogLevelFatal,
			"fbconfig functions missing");
	setFailed ();
	return;
    }

    copySubBuffer = NULL;
    if (strstr (glxExtensions, "GLX_MESA_copy_sub_buffer"))
	copySubBuffer = (GLXCopySubBufferProc)
	    getProcAddress ("glXCopySubBufferMESA");

    getVideoSync = NULL;
    waitVideoSync = NULL;
    if (strstr (glxExtensions, "GLX_SGI_video_sync"))
    {
	getVideoSync = (GLXGetVideoSyncProc)
	    getProcAddress ("glXGetVideoSyncSGI");

	waitVideoSync = (GLXWaitVideoSyncProc)
	    getProcAddress ("glXWaitVideoSyncSGI");
    }

    glXMakeCurrent (dpy, CompositeScreen::get (s)->output (),
		    priv->ctx);
    currentRoot = s->root ();

    glExtensions = (const char *) glGetString (GL_EXTENSIONS);
    if (!glExtensions)
    {
	compLogMessage (d, "opengl", CompLogLevelFatal,
			"No valid GL extensions string found.");
	setFailed ();
	return;
    }

    priv->textureNonPowerOfTwo = false;
    if (strstr (glExtensions, "GL_ARB_texture_non_power_of_two"))
	priv->textureNonPowerOfTwo = true;

    glGetIntegerv (GL_MAX_TEXTURE_SIZE, &priv->maxTextureSize);

    priv->textureRectangle = false;
    if (strstr (glExtensions, "GL_NV_texture_rectangle")  ||
	strstr (glExtensions, "GL_EXT_texture_rectangle") ||
	strstr (glExtensions, "GL_ARB_texture_rectangle"))
    {
	priv->textureRectangle = true;

	if (!priv->textureNonPowerOfTwo)
	{
	    GLint maxTextureSize;

	    glGetIntegerv (GL_MAX_RECTANGLE_TEXTURE_SIZE_NV, &maxTextureSize);
	    if (maxTextureSize > priv->maxTextureSize)
		priv->maxTextureSize = maxTextureSize;
	}
    }

    if (!(priv->textureRectangle || priv->textureNonPowerOfTwo))
    {
	compLogMessage (d, "opengl", CompLogLevelFatal,
			"Support for non power of two textures missing");
	setFailed ();
	return;
    }

    priv->textureEnvCombine = priv->textureEnvCrossbar = false;
    if (strstr (glExtensions, "GL_ARB_texture_env_combine"))
    {
	priv->textureEnvCombine = true;

	/* XXX: GL_NV_texture_env_combine4 need special code but it seams to
	   be working anyway for now... */
	if (strstr (glExtensions, "GL_ARB_texture_env_crossbar") ||
	    strstr (glExtensions, "GL_NV_texture_env_combine4"))
	    priv->textureEnvCrossbar = true;
    }

    priv->textureBorderClamp = false;
    if (strstr (glExtensions, "GL_ARB_texture_border_clamp") ||
	strstr (glExtensions, "GL_SGIS_texture_border_clamp"))
	priv->textureBorderClamp = true;

    priv->maxTextureUnits = 1;
    if (strstr (glExtensions, "GL_ARB_multitexture"))
    {
	activeTexture = (GLActiveTextureProc)
	    getProcAddress ("glActiveTexture");
	clientActiveTexture = (GLClientActiveTextureProc)
	    getProcAddress ("glClientActiveTexture");
	multiTexCoord2f = (GLMultiTexCoord2fProc)
	    getProcAddress ("glMultiTexCoord2f");

	if (activeTexture && clientActiveTexture && multiTexCoord2f)
	    glGetIntegerv (GL_MAX_TEXTURE_UNITS_ARB, &priv->maxTextureUnits);
    }

    priv->fragmentProgram = false;
    if (strstr (glExtensions, "GL_ARB_fragment_program"))
    {
	genPrograms = (GLGenProgramsProc)
	    getProcAddress ("glGenProgramsARB");
	deletePrograms = (GLDeleteProgramsProc)
	    getProcAddress ("glDeleteProgramsARB");
	bindProgram = (GLBindProgramProc)
	    getProcAddress ("glBindProgramARB");
	programString = (GLProgramStringProc)
	    getProcAddress ("glProgramStringARB");
	programEnvParameter4f = (GLProgramParameter4fProc)
	    getProcAddress ("glProgramEnvParameter4fARB");
	programLocalParameter4f = (GLProgramParameter4fProc)
	    getProcAddress ("glProgramLocalParameter4fARB");
	getProgramiv = (GLGetProgramivProc)
	    getProcAddress ("glGetProgramivARB");

	if (genPrograms             &&
	    deletePrograms          &&
	    bindProgram             &&
	    programString           &&
	    programEnvParameter4f   &&
	    programLocalParameter4f &&
	    getProgramiv)
	    priv->fragmentProgram = true;
    }

    priv->fbo = false;
    if (strstr (glExtensions, "GL_EXT_framebuffer_object"))
    {
	genFramebuffers = (GLGenFramebuffersProc)
	    getProcAddress ("glGenFramebuffersEXT");
	deleteFramebuffers = (GLDeleteFramebuffersProc)
	    getProcAddress ("glDeleteFramebuffersEXT");
	bindFramebuffer = (GLBindFramebufferProc)
	    getProcAddress ("glBindFramebufferEXT");
	checkFramebufferStatus = (GLCheckFramebufferStatusProc)
	    getProcAddress ("glCheckFramebufferStatusEXT");
	framebufferTexture2D = (GLFramebufferTexture2DProc)
	    getProcAddress ("glFramebufferTexture2DEXT");
	generateMipmap = (GLGenerateMipmapProc)
	    getProcAddress ("glGenerateMipmapEXT");

	if (genFramebuffers        &&
	    deleteFramebuffers     &&
	    bindFramebuffer        &&
	    checkFramebufferStatus &&
	    framebufferTexture2D   &&
	    generateMipmap)
	    priv->fbo = true;
    }

    priv->textureCompression = false;
    if (strstr (glExtensions, "GL_ARB_texture_compression"))
	priv->textureCompression = true;

    fbConfigs = (*getFBConfigs) (dpy, s->screenNum (), &nElements);

    for (i = 0; i <= MAX_DEPTH; i++)
    {
	int j, db, stencil, depth, alpha, mipmap, rgba;

	priv->glxPixmapFBConfigs[i].fbConfig       = NULL;
	priv->glxPixmapFBConfigs[i].mipmap         = 0;
	priv->glxPixmapFBConfigs[i].yInverted      = 0;
	priv->glxPixmapFBConfigs[i].textureFormat  = 0;
	priv->glxPixmapFBConfigs[i].textureTargets = 0;

	db      = MAXSHORT;
	stencil = MAXSHORT;
	depth   = MAXSHORT;
	mipmap  = 0;
	rgba    = 0;

	for (j = 0; j < nElements; j++)
	{
	    XVisualInfo *vi;
	    int		visualDepth;

	    vi = glXGetVisualFromFBConfig (dpy, fbConfigs[j]);
	    if (vi == NULL)
		continue;

	    visualDepth = vi->depth;

	    XFree (vi);

	    if (visualDepth != i)
		continue;

	    (*getFBConfigAttrib) (dpy,
				  fbConfigs[j],
				  GLX_ALPHA_SIZE,
				  &alpha);
	    (*getFBConfigAttrib) (dpy,
				  fbConfigs[j],
				  GLX_BUFFER_SIZE,
				  &value);
	    if (value != i && (value - alpha) != i)
		continue;

	    value = 0;
	    if (i == 32)
	    {
		(*getFBConfigAttrib) (dpy,
				      fbConfigs[j],
				      GLX_BIND_TO_TEXTURE_RGBA_EXT,
				      &value);

		if (value)
		{
		    rgba = 1;

		    priv->glxPixmapFBConfigs[i].textureFormat =
			GLX_TEXTURE_FORMAT_RGBA_EXT;
		}
	    }

	    if (!value)
	    {
		if (rgba)
		    continue;

		(*getFBConfigAttrib) (dpy,
				      fbConfigs[j],
				      GLX_BIND_TO_TEXTURE_RGB_EXT,
				      &value);
		if (!value)
		    continue;

		priv->glxPixmapFBConfigs[i].textureFormat =
		    GLX_TEXTURE_FORMAT_RGB_EXT;
	    }

	    (*getFBConfigAttrib) (dpy,
				  fbConfigs[j],
				  GLX_DOUBLEBUFFER,
				  &value);
	    if (value > db)
		continue;

	    db = value;

	    (*getFBConfigAttrib) (dpy,
				  fbConfigs[j],
				  GLX_STENCIL_SIZE,
				  &value);
	    if (value > stencil)
		continue;

	    stencil = value;

	    (*getFBConfigAttrib) (dpy,
				  fbConfigs[j],
				  GLX_DEPTH_SIZE,
				  &value);
	    if (value > depth)
		continue;

	    depth = value;

	    if (priv->fbo)
	    {
		(*getFBConfigAttrib) (dpy,
				      fbConfigs[j],
				      GLX_BIND_TO_MIPMAP_TEXTURE_EXT,
				      &value);
		if (value < mipmap)
		    continue;

		mipmap = value;
	    }

	    (*getFBConfigAttrib) (dpy,
				  fbConfigs[j],
				  GLX_Y_INVERTED_EXT,
				  &value);

	    priv->glxPixmapFBConfigs[i].yInverted = value;

	    (*getFBConfigAttrib) (dpy,
				  fbConfigs[j],
				  GLX_BIND_TO_TEXTURE_TARGETS_EXT,
				  &value);

	    priv->glxPixmapFBConfigs[i].textureTargets = value;

	    priv->glxPixmapFBConfigs[i].fbConfig = fbConfigs[j];
	    priv->glxPixmapFBConfigs[i].mipmap   = mipmap;
	}
    }

    if (nElements)
	XFree (fbConfigs);

    if (!priv->glxPixmapFBConfigs[defaultDepth].fbConfig)
    {
	compLogMessage (d, "opengl", CompLogLevelFatal,
			"No GLXFBConfig for default depth, "
			"this isn't going to work.");
	setFailed ();
	return;
    }

    glClearColor (0.0, 0.0, 0.0, 1.0);
    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glEnable (GL_CULL_FACE);
    glDisable (GL_BLEND);
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glColor4usv (defaultColor);
    glEnableClientState (GL_VERTEX_ARRAY);
    glEnableClientState (GL_TEXTURE_COORD_ARRAY);

    priv->canDoSaturated = priv->canDoSlightlySaturated = false;
    if (priv->textureEnvCombine && priv->maxTextureUnits >= 2)
    {
	priv->canDoSaturated = true;
	if (priv->textureEnvCrossbar && priv->maxTextureUnits >= 4)
	    priv->canDoSlightlySaturated = true;
    }

    priv->updateView ();

    glLightModelfv (GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    glEnable (GL_LIGHT0);
    glLightfv (GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv (GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glLightfv (GL_LIGHT0, GL_POSITION, light0Position);

    glColorMaterial (GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    glNormal3f (0.0f, 0.0f, -1.0f);

    priv->lighting = false;


    priv->filter[NOTHING_TRANS_FILTER] = GLTexture::Fast;
    priv->filter[SCREEN_TRANS_FILTER]  = GLTexture::Good;
    priv->filter[WINDOW_TRANS_FILTER]  = GLTexture::Good;


    if (!CompositeScreen::get (s)->registerPaintHandler (priv))
	setFailed ();

}

GLScreen::~GLScreen ()
{
    CompositeScreen::get (priv->screen)->unregisterPaintHandler ();
    glXDestroyContext (priv->screen->display ()->dpy (), priv->ctx);
    delete priv;
}

PrivateGLScreen::PrivateGLScreen (CompScreen *s,
				  GLScreen   *gs) :
    screen (s),
    gScreen (gs),
    cScreen (CompositeScreen::get (s)),
    textureRectangle (false),
    textureNonPowerOfTwo (false),
    textureEnvCombine (false),
    textureEnvCrossbar (false),
    textureBorderClamp (false),
    textureCompression (false),
    maxTextureSize (0),
    fbo (false),
    fragmentProgram (false),
    maxTextureUnits (1),
    backgroundTexture (screen),
    backgroundLoaded (false),
    canDoSaturated (false),
    canDoSlightlySaturated (false),
    rasterPos (0, 0),
    fragmentStorage (),
    clearBuffers (true),
    lighting (false),
    getProcAddress (0),
    tmpRegion (NULL),
    outputRegion (NULL)
{
}

PrivateGLScreen::~PrivateGLScreen ()
{
    if (tmpRegion)
	XDestroyRegion (tmpRegion);
    if (outputRegion)
	XDestroyRegion (outputRegion);
}

static void
frustum (GLfloat *m,
	 GLfloat left,
	 GLfloat right,
	 GLfloat bottom,
	 GLfloat top,
	 GLfloat nearval,
	 GLfloat farval)
{
    GLfloat x, y, a, b, c, d;

    x = (2.0 * nearval) / (right - left);
    y = (2.0 * nearval) / (top - bottom);
    a = (right + left) / (right - left);
    b = (top + bottom) / (top - bottom);
    c = -(farval + nearval) / ( farval - nearval);
    d = -(2.0 * farval * nearval) / (farval - nearval);

#define M(row,col)  m[col*4+row]
    M(0,0) = x;     M(0,1) = 0.0f;  M(0,2) = a;      M(0,3) = 0.0f;
    M(1,0) = 0.0f;  M(1,1) = y;     M(1,2) = b;      M(1,3) = 0.0f;
    M(2,0) = 0.0f;  M(2,1) = 0.0f;  M(2,2) = c;      M(2,3) = d;
    M(3,0) = 0.0f;  M(3,1) = 0.0f;  M(3,2) = -1.0f;  M(3,3) = 0.0f;
#undef M

}

static void
perspective (GLfloat *m,
	     GLfloat fovy,
	     GLfloat aspect,
	     GLfloat zNear,
	     GLfloat zFar)
{
    GLfloat xmin, xmax, ymin, ymax;

    ymax = zNear * tan (fovy * M_PI / 360.0);
    ymin = -ymax;
    xmin = ymin * aspect;
    xmax = ymax * aspect;

    frustum (m, xmin, xmax, ymin, ymax, zNear, zFar);
}

void
PrivateGLScreen::updateView ()
{
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();
    glDepthRange (0, 1);
    glViewport (-1, -1, 2, 2);
    glRasterPos2f (0, 0);

    rasterPos = CompPoint (0, 0);

    perspective (projection, 60.0f, 1.0f, 0.1f, 100.0f);

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glMultMatrixf (projection);
    glMatrixMode (GL_MODELVIEW);

    Region region = XCreateRegion ();
    if (region)
    {
	XSubtractRegion (screen->region (), &emptyRegion, region);
	/* remove all output regions from visible screen region */
	foreach (CompOutput &o, screen->outputDevs ())
	    XSubtractRegion (region, o.region (), region);

	/* we should clear color buffers before swapping if we have visible
           regions without output */
	clearBuffers = REGION_NOT_EMPTY (region);

	XDestroyRegion (region);
    }
    gScreen->setDefaultViewport ();
}

void
PrivateGLScreen::outputChangeNotify ()
{
    screen->outputChangeNotify ();

    updateView ();
}

FuncPtr
GLScreen::getProcAddress (const char *name)
{
    static void *dlhand = NULL;
    FuncPtr     funcPtr = NULL;

    if (priv->getProcAddress)
	funcPtr = priv->getProcAddress ((GLubyte *) name);

    if (!funcPtr)
    {
	if (!dlhand)
	    dlhand = dlopen (NULL, RTLD_LAZY);

	if (dlhand)
	{
	    dlerror ();
	    funcPtr = (FuncPtr) dlsym (dlhand, name);
	    if (dlerror () != NULL)
		funcPtr = NULL;
	}
    }

    return funcPtr;
}

void
PrivateGLScreen::updateScreenBackground (GLTexture *texture)
{
    Display	  *dpy = screen->display ()->dpy ();
    Atom	  pixmapAtom, actualType;
    int		  actualFormat, i, status;
    unsigned int  width = 1, height = 1, depth = 0;
    unsigned long nItems;
    unsigned long bytesAfter;
    unsigned char *prop;
    Pixmap	  pixmap = 0;

    pixmapAtom = XInternAtom (dpy, "PIXMAP", FALSE);

    for (i = 0; pixmap == 0 && i < 2; i++)
    {
	status = XGetWindowProperty (dpy, screen->root (),
				     screen->display ()->atoms ().xBackground[i],
				     0, 4, FALSE, AnyPropertyType,
				     &actualType, &actualFormat, &nItems,
				     &bytesAfter, &prop);

	if (status == Success && nItems && prop)
	{
	    if (actualType   == pixmapAtom &&
		actualFormat == 32         &&
		nItems	     == 1)
	    {
		Pixmap p;

		memcpy (&p, prop, 4);

		if (p)
		{
		    unsigned int ui;
		    int		 i;
		    Window	 w;

		    if (XGetGeometry (dpy, p, &w, &i, &i,
				      &width, &height, &ui, &depth))
		    {
			if ((int) depth == screen->attrib ().depth)
			    pixmap = p;
		    }
		}
	    }

	    XFree (prop);
	}
    }

    if (pixmap)
    {
/* FIXME:
	if (pixmap == texture->pixmap)
	    return;
*/
	texture->reset ();

	if (!texture->bindPixmap (pixmap, width, height, depth))
	{
	    compLogMessage (NULL, "core", CompLogLevelWarn,
			    "Couldn't bind background pixmap 0x%x to "
			    "texture", (int) pixmap);
	}
    }
    else
    {
	texture->reset ();
    }

    if (!texture->name () && backgroundImage)
	GLTexture::readImageToTexture (screen, texture, backgroundImage,
				       &width, &height);

    if (texture->target () == GL_TEXTURE_2D)
    {
	glBindTexture (texture->target (), texture->name ());
	glTexParameteri (texture->target (), GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (texture->target (), GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture (texture->target (), 0);
    }
}

void
GLScreen::setTexEnvMode (GLenum mode)
{
    if (priv->lighting)
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    else
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
}

void
GLScreen::setLighting (bool lighting)
{
    if (priv->lighting != lighting)
    {
	if (!priv->opt[GL_SCREEN_OPTION_LIGHTING].value ().b ())
	    lighting = false;

	if (lighting)
	{
	    glEnable (GL_COLOR_MATERIAL);
	    glEnable (GL_LIGHTING);
	}
	else
	{
	    glDisable (GL_COLOR_MATERIAL);
	    glDisable (GL_LIGHTING);
	}

	priv->lighting = lighting;

	setTexEnvMode (GL_REPLACE);
    }
}

void
GLScreen::makeCurrent ()
{
    if (currentRoot != priv->screen->root ())
    {
	glXMakeCurrent (priv->screen->display ()->dpy (),
			CompositeScreen::get (priv->screen)->output (),
			priv->ctx);
	currentRoot = priv->screen->root ();
    }

    priv->pendingCommands = true;
}

bool
GLScreenInterface::glPaintOutput (const GLScreenPaintAttrib &sAttrib,
			          const GLMatrix            &transform,
			          Region                    region,
			          CompOutput                *output,
			          unsigned int              mask)
    WRAPABLE_DEF (glPaintOutput, sAttrib, transform, region, output, mask)

void
GLScreenInterface::glPaintTransformedOutput (const GLScreenPaintAttrib &sAttrib,
					     const GLMatrix            &transform,
					     Region                    region,
					     CompOutput                *output,
					     unsigned int              mask)
    WRAPABLE_DEF (glPaintTransformedOutput, sAttrib, transform, region,
		  output, mask)

void
GLScreenInterface::glApplyTransform (const GLScreenPaintAttrib &sAttrib,
				     CompOutput                *output,
				     GLMatrix                  *transform)
    WRAPABLE_DEF (glApplyTransform, sAttrib, output, transform)

void
GLScreenInterface::glEnableOutputClipping (const GLMatrix &transform,
				           Region         region,
				           CompOutput     *output)
    WRAPABLE_DEF (glEnableOutputClipping, transform, region, output)

void
GLScreenInterface::glDisableOutputClipping ()
    WRAPABLE_DEF (glDisableOutputClipping)

int
GLScreen::maxTextureSize ()
{
    return priv->maxTextureSize;
}

void
GLScreen::updateBackground ()
{
    priv->backgroundTexture.reset ();

    if (priv->backgroundLoaded)
    {
	priv->backgroundLoaded = false;
	CompositeScreen::get (priv->screen)->damageScreen ();
    }
}

bool
GLScreen::textureNonPowerOfTwo ()
{
    return priv->textureNonPowerOfTwo;
}

bool
GLScreen::textureCompression ()
{
    return priv->textureCompression;
}

bool
GLScreen::canDoSaturated ()
{
    return priv->canDoSaturated;
}

bool
GLScreen::canDoSlightlySaturated ()
{
    return priv->canDoSlightlySaturated;
}

bool
GLScreen::lighting ()
{
    return priv->lighting;
}

GLTexture::Filter
GLScreen::filter (int filter)
{
    return priv->filter[filter];
}

GLFragment::Storage *
GLScreen::fragmentStorage ()
{
    return &priv->fragmentStorage;
}

bool
GLScreen::fragmentProgram ()
{
    return priv->fragmentProgram;
}

GLFBConfig*
GLScreen::glxPixmapFBConfig (unsigned int depth)
{
    return &priv->glxPixmapFBConfigs[depth];
}

bool
GLScreen::framebufferObject ()
{
    return priv->fbo;
}

void
GLScreen::clearOutput (CompOutput   *output,
		       unsigned int mask)
{
    BoxPtr pBox = &output->region ()->extents;

    if (pBox->x1 != 0	     ||
	pBox->y1 != 0	     ||
	pBox->x2 != (int) priv->screen->size ().width () ||
	pBox->y2 != (int) priv->screen->size ().height ())
    {
	glPushAttrib (GL_SCISSOR_BIT);

	glEnable (GL_SCISSOR_TEST);
	glScissor (pBox->x1,
		   priv->screen->size ().height () - pBox->y2,
		   pBox->x2 - pBox->x1,
		   pBox->y2 - pBox->y1);
	glClear (mask);

	glPopAttrib ();
    }
    else
    {
	glClear (mask);
    }
}

void
GLScreen::setDefaultViewport ()
{
    priv->lastViewport.x      = priv->screen->outputDevs ()[0].x1 ();
    priv->lastViewport.y      = priv->screen->size ().height () -
				priv->screen->outputDevs ()[0].y2 ();
    priv->lastViewport.width  = priv->screen->outputDevs ()[0].width ();
    priv->lastViewport.height = priv->screen->outputDevs ()[0].height ();

    glViewport (priv->lastViewport.x,
		priv->lastViewport.y,
		priv->lastViewport.width,
		priv->lastViewport.height);
}

void
PrivateGLScreen::waitForVideoSync ()
{
    unsigned int sync;

    if (!opt[GL_SCREEN_OPTION_SYNC_TO_VBLANK].value ().b ())
	return;

    if (gScreen->getVideoSync)
    {
	glFlush ();

	(*gScreen->getVideoSync) (&sync);
	(*gScreen->waitVideoSync) (2, (sync + 1) % 2, &sync);
    }
}

CompOption *
GLScreen::getOption (const char *name)
{
    CompOption *o = CompOption::findOption (priv->opt, name);
    return o;
}

void
PrivateGLScreen::paintOutputs (CompOutput::ptrList &outputs,
			       unsigned int        mask,
			       Region              region)
{
    XRectangle r;

    if (clearBuffers)
    {
	if (mask & COMPOSITE_SCREEN_DAMAGE_ALL_MASK)
	    glClear (GL_COLOR_BUFFER_BIT);
    }

    XSubtractRegion (region, &emptyRegion, tmpRegion);

    foreach (CompOutput *output, outputs)
    {
	targetScreen = gScreen;
	targetOutput = output;

	r.x	 = output->x1 ();
	r.y	 = screen->size ().height () - output->y2 ();
	r.width  = output->width ();
	r.height = output->height ();

	if (lastViewport.x      != r.x     ||
 	    lastViewport.y      != r.y     ||
	    lastViewport.width  != r.width ||
	    lastViewport.height != r.height)
	{
	    glViewport (r.x, r.y, r.width, r.height);
	    lastViewport = r;
	}

	if (mask & COMPOSITE_SCREEN_DAMAGE_ALL_MASK)
	{
	    GLMatrix identity;

	    gScreen->glPaintOutput (defaultScreenPaintAttrib,
				    identity,
				    output->region (), output,
				    PAINT_SCREEN_REGION_MASK |
				    PAINT_SCREEN_FULL_MASK);
	}
	else if (mask & COMPOSITE_SCREEN_DAMAGE_REGION_MASK)
	{
	    GLMatrix identity;

	    XIntersectRegion (tmpRegion, output->region (), outputRegion);

	    if (!gScreen->glPaintOutput (defaultScreenPaintAttrib,
					 identity,
					 outputRegion, output,
					 PAINT_SCREEN_REGION_MASK))
	    {
		identity.reset ();

		gScreen->glPaintOutput (defaultScreenPaintAttrib,
					identity,
					output->region (), output,
					PAINT_SCREEN_FULL_MASK);

		XUnionRegion (tmpRegion, output->region (), tmpRegion);

	    }
	}
    }
	
    targetScreen = NULL;
    targetOutput = &screen->outputDevs ()[0];

    waitForVideoSync ();

    if (mask & COMPOSITE_SCREEN_DAMAGE_ALL_MASK)
    {
	glXSwapBuffers (screen->display ()->dpy (), cScreen->output ());
    }
    else
    {
	BoxPtr pBox;
	int    nBox, y;

	pBox = tmpRegion->rects;
	nBox = tmpRegion->numRects;

	if (gScreen->copySubBuffer)
	{
	    while (nBox--)
	    {
		y = screen->size ().height () - pBox->y2;

		(*gScreen->copySubBuffer) (screen->display ()->dpy (),
					   cScreen->output (),
					   pBox->x1, y,
					   pBox->x2 - pBox->x1,
					   pBox->y2 - pBox->y1);

		pBox++;
	    }
	}
	else
	{
	    glEnable (GL_SCISSOR_TEST);
	    glDrawBuffer (GL_FRONT);

	    while (nBox--)
	    {
		y = screen->size ().height () - pBox->y2;

		glBitmap (0, 0, 0, 0,
			  pBox->x1 - rasterPos.x (),
			  y - rasterPos.y (),
			  NULL);

		rasterPos = CompPoint (pBox->x1, y);

		glScissor (pBox->x1, y,
			   pBox->x2 - pBox->x1,
			   pBox->y2 - pBox->y1);

		glCopyPixels (pBox->x1, y,
			      pBox->x2 - pBox->x1,
			      pBox->y2 - pBox->y1,
			      GL_COLOR);

		pBox++;
	    }

	    glDrawBuffer (GL_BACK);
	    glDisable (GL_SCISSOR_TEST);
	    glFlush ();
	}
    }
}

bool
PrivateGLScreen::hasVSync ()
{
   return (gScreen->getVideoSync &&
	   opt[GL_SCREEN_OPTION_SYNC_TO_VBLANK].value ().b ());
}

void
PrivateGLScreen::prepareDrawing ()
{
    gScreen->makeCurrent ();
    if (pendingCommands)
    {
	glFinish ();
	pendingCommands = false;
    }
    targetScreen = gScreen;
}
