#ifndef _COMPIZ_OPENGL_H
#define _COMPIZ_OPENGL_H

#include <GL/gl.h>
#include <GL/glx.h>

#include <opengl/matrix.h>
#include <opengl/texture.h>
#include <opengl/fragment.h>

#define COMPIZ_OPENGL_ABI 1

#define PLUGIN OpenGL
#include <compprivatehandler.h>

class PrivateGLDisplay;
class PrivateGLScreen;
class PrivateGLWindow;

class GLDisplay :
    public OpenGLPrivateHandler<GLDisplay, CompDisplay, COMPIZ_OPENGL_ABI>
{
    public:
	GLDisplay (CompDisplay *d);
	~GLDisplay ();

	CompOption::Vector & getOptions ();
        bool setOption (const char *name, CompOption::Value &value);
	
	GLenum textureFilter ();

	void clearTargetOutput (unsigned int mask);

    private:
	PrivateGLDisplay *priv;
};


extern GLushort   defaultColor[4];

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

struct GLScreenPaintAttrib {
    GLfloat xRotate;
    GLfloat yRotate;
    GLfloat vRotate;
    GLfloat xTranslate;
    GLfloat yTranslate;
    GLfloat zTranslate;
    GLfloat zCamera;
};

#define MAX_DEPTH 32

struct GLFBConfig {
    GLXFBConfig fbConfig;
    int         yInverted;
    int         mipmap;
    int         textureFormat;
    int         textureTargets;
};

#define NOTHING_TRANS_FILTER 0
#define SCREEN_TRANS_FILTER  1
#define WINDOW_TRANS_FILTER  2


extern GLScreenPaintAttrib defaultScreenPaintAttrib;

class GLScreen;

class GLScreenInterface :
    public WrapableInterface<GLScreen, GLScreenInterface>
{
    public:
	virtual bool glPaintOutput (const GLScreenPaintAttrib &,
				    const GLMatrix &, Region, CompOutput *,
				    unsigned int);
	virtual void glPaintTransformedOutput (const GLScreenPaintAttrib &,
					       const GLMatrix &, Region,
					       CompOutput *, unsigned int);
	virtual void glApplyTransform (const GLScreenPaintAttrib &,
				       CompOutput *, GLMatrix *);

	virtual void glEnableOutputClipping (const GLMatrix &, Region,
					     CompOutput *);
	virtual void glDisableOutputClipping ();

};


class GLScreen :
    public WrapableHandler<GLScreenInterface, 5>,
    public OpenGLPrivateHandler<GLScreen, CompScreen, COMPIZ_OPENGL_ABI>
{
    public:
	GLScreen (CompScreen *s);
	~GLScreen ();

	CompOption::Vector & getOptions ();
        bool setOption (const char *name, CompOption::Value &value);
	CompOption * getOption (const char *name);

	FuncPtr	getProcAddress (const char *name);

	void updateBackground ();

	int maxTextureSize ();
	bool textureNonPowerOfTwo ();
	bool textureCompression ();
	bool canDoSaturated ();
	bool canDoSlightlySaturated ();
	bool fragmentProgram ();
	bool framebufferObject ();

	GLTexture::Filter filter (int);

	GLFragment::Storage * fragmentStorage ();

	void setTexEnvMode (GLenum mode);
	void setLighting (bool lighting);
	bool lighting ();

	void makeCurrent ();

	void clearOutput (CompOutput *output, unsigned int mask);

	void setDefaultViewport ();
	
	GLFBConfig * glxPixmapFBConfig (unsigned int depth);

	WRAPABLE_HND (0, GLScreenInterface, bool, glPaintOutput,
		      const GLScreenPaintAttrib &, const GLMatrix &, Region,
		      CompOutput *, unsigned int);
	WRAPABLE_HND (1, GLScreenInterface, void, glPaintTransformedOutput,
		      const GLScreenPaintAttrib &,
		      const GLMatrix &, Region, CompOutput *,
		      unsigned int);
	WRAPABLE_HND (2, GLScreenInterface, void, glApplyTransform,
		      const GLScreenPaintAttrib &, CompOutput *, GLMatrix *);

	WRAPABLE_HND (3, GLScreenInterface, void, glEnableOutputClipping,
		      const GLMatrix &, Region, CompOutput *);
	WRAPABLE_HND (4, GLScreenInterface, void, glDisableOutputClipping);

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
	PrivateGLScreen *priv;
};

struct GLWindowPaintAttrib {
    GLushort opacity;
    GLushort brightness;
    GLushort saturation;
    GLfloat  xScale;
    GLfloat  yScale;
    GLfloat  xTranslate;
    GLfloat  yTranslate;
};

class GLWindow;

class GLWindowInterface :
    public WrapableInterface<GLWindow, GLWindowInterface>
{
    public:
	virtual bool glPaint (const GLWindowPaintAttrib &, const GLMatrix &,
			      Region, unsigned int);
	virtual bool glDraw (const GLMatrix &, GLFragment::Attrib &, Region,
			     unsigned int);
	virtual void glAddGeometry (GLTexture::Matrix *matrix, int, Region,
				    Region);
	virtual void glDrawTexture (GLTexture *texture, GLFragment::Attrib &,
				    unsigned int);
	virtual void glDrawGeometry ();
};

class GLWindow :
    public WrapableHandler<GLWindowInterface, 5>,
    public OpenGLPrivateHandler<GLWindow, CompWindow, COMPIZ_OPENGL_ABI>
{
    public:

	class Geometry {
	    public:
		Geometry ();
		~Geometry ();

		void reset ();
		
		bool moreVertices (int newSize);
		bool moreIndices (int newSize);

	    public:
		GLfloat  *vertices;
		int      vertexSize;
		int      vertexStride;
		GLushort *indices;
		int      indexSize;
		int      vCount;
		int      texUnits;
		int      texCoordSize;
		int      indexCount;
	};
	
	static GLWindowPaintAttrib defaultPaintAttrib;
    public:

	GLWindow (CompWindow *w);
	~GLWindow ();

	Region clip ();

	GLWindowPaintAttrib & paintAttrib ();

	bool bind ();
	void release ();

	void updatePaintAttribs ();

	Geometry & geometry ();

	WRAPABLE_HND (0, GLWindowInterface, bool, glPaint,
		      const GLWindowPaintAttrib &, const GLMatrix &, Region,
		      unsigned int);
	WRAPABLE_HND (1, GLWindowInterface, bool, glDraw, const GLMatrix &,
		      GLFragment::Attrib &, Region, unsigned int);
	WRAPABLE_HND (2, GLWindowInterface, void, glAddGeometry,
		      GLTexture::Matrix *matrix, int, Region, Region);
	WRAPABLE_HND (3, GLWindowInterface, void, glDrawTexture,
		      GLTexture *texture, GLFragment::Attrib &, unsigned int);
	WRAPABLE_HND (4, GLWindowInterface, void, glDrawGeometry);

    private:
	PrivateGLWindow *priv;
};

#endif
