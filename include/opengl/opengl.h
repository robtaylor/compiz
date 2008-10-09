#ifndef _COMPIZ_OPENGL_H
#define _COMPIZ_OPENGL_H

#include <GL/gl.h>
#include <GL/glx.h>

#include <opengl/matrix.h>
#include <opengl/texture.h>
#include <opengl/fragment.h>

#define COMPIZ_OPENGL_ABI 1

#include <core/privatehandler.h>

/* camera distance from screen, 0.5 * tan (FOV) */
#define DEFAULT_Z_CAMERA 0.866025404f

class PrivateGLScreen;
class PrivateGLWindow;

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

namespace GL {

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

    extern GLXBindTexImageProc      bindTexImage;
    extern GLXReleaseTexImageProc   releaseTexImage;
    extern GLXQueryDrawableProc     queryDrawable;
    extern GLXCopySubBufferProc     copySubBuffer;
    extern GLXGetVideoSyncProc      getVideoSync;
    extern GLXWaitVideoSyncProc     waitVideoSync;
    extern GLXGetFBConfigsProc      getFBConfigs;
    extern GLXGetFBConfigAttribProc getFBConfigAttrib;
    extern GLXCreatePixmapProc      createPixmap;

    extern GLActiveTextureProc       activeTexture;
    extern GLClientActiveTextureProc clientActiveTexture;
    extern GLMultiTexCoord2fProc     multiTexCoord2f;

    extern GLGenProgramsProc        genPrograms;
    extern GLDeleteProgramsProc     deletePrograms;
    extern GLBindProgramProc        bindProgram;
    extern GLProgramStringProc      programString;
    extern GLProgramParameter4fProc programEnvParameter4f;
    extern GLProgramParameter4fProc programLocalParameter4f;
    extern GLGetProgramivProc       getProgramiv;

    extern GLGenFramebuffersProc        genFramebuffers;
    extern GLDeleteFramebuffersProc     deleteFramebuffers;
    extern GLBindFramebufferProc        bindFramebuffer;
    extern GLCheckFramebufferStatusProc checkFramebufferStatus;
    extern GLFramebufferTexture2DProc   framebufferTexture2D;
    extern GLGenerateMipmapProc         generateMipmap;

    extern bool  textureFromPixmap;
    extern bool  textureRectangle;
    extern bool  textureNonPowerOfTwo;
    extern bool  textureEnvCombine;
    extern bool  textureEnvCrossbar;
    extern bool  textureBorderClamp;
    extern bool  textureCompression;
    extern GLint maxTextureSize;
    extern bool  fbo;
    extern bool  fragmentProgram;
    extern GLint maxTextureUnits;

    extern bool canDoSaturated;
    extern bool canDoSlightlySaturated;
};

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
				    const GLMatrix &, const CompRegion &,
				    CompOutput *, unsigned int);
	virtual void glPaintTransformedOutput (const GLScreenPaintAttrib &,
					       const GLMatrix &,
					       const CompRegion &,
					       CompOutput *, unsigned int);
	virtual void glApplyTransform (const GLScreenPaintAttrib &,
				       CompOutput *, GLMatrix *);

	virtual void glEnableOutputClipping (const GLMatrix &,
					     const CompRegion &,
					     CompOutput *);
	virtual void glDisableOutputClipping ();

};


class GLScreen :
    public WrapableHandler<GLScreenInterface, 5>,
    public PrivateHandler<GLScreen, CompScreen, COMPIZ_OPENGL_ABI>
{
    public:
	GLScreen (CompScreen *s);
	~GLScreen ();

	CompOption::Vector & getOptions ();
        bool setOption (const char *name, CompOption::Value &value);
	CompOption * getOption (const char *name);

	GLenum textureFilter ();

	void clearTargetOutput (unsigned int mask);

	GL::FuncPtr getProcAddress (const char *name);

	void updateBackground ();

	GLTexture::Filter filter (int);
	void setFilter (int, GLTexture::Filter);

	GLFragment::Storage * fragmentStorage ();

	void setTexEnvMode (GLenum mode);
	void setLighting (bool lighting);
	bool lighting ();

	void clearOutput (CompOutput *output, unsigned int mask);

	void setDefaultViewport ();

	GLTexture::BindPixmapHandle registerBindPixmap (GLTexture::BindPixmapProc);
	void unregisterBindPixmap (GLTexture::BindPixmapHandle);
	
	GLFBConfig * glxPixmapFBConfig (unsigned int depth);

	WRAPABLE_HND (0, GLScreenInterface, bool, glPaintOutput,
		      const GLScreenPaintAttrib &, const GLMatrix &,
		      const CompRegion &, CompOutput *, unsigned int);
	WRAPABLE_HND (1, GLScreenInterface, void, glPaintTransformedOutput,
		      const GLScreenPaintAttrib &,
		      const GLMatrix &, const CompRegion &, CompOutput *,
		      unsigned int);
	WRAPABLE_HND (2, GLScreenInterface, void, glApplyTransform,
		      const GLScreenPaintAttrib &, CompOutput *, GLMatrix *);

	WRAPABLE_HND (3, GLScreenInterface, void, glEnableOutputClipping,
		      const GLMatrix &, const CompRegion &, CompOutput *);
	WRAPABLE_HND (4, GLScreenInterface, void, glDisableOutputClipping);

	friend class GLTexture;

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
			      const CompRegion &, unsigned int);
	virtual bool glDraw (const GLMatrix &, GLFragment::Attrib &,
			     const CompRegion &, unsigned int);
	virtual void glAddGeometry (const GLTexture::MatrixList &,
				    const CompRegion &,const CompRegion &);
	virtual void glDrawTexture (GLTexture *texture, GLFragment::Attrib &,
				    unsigned int);
	virtual void glDrawGeometry ();
};

class GLWindow :
    public WrapableHandler<GLWindowInterface, 5>,
    public PrivateHandler<GLWindow, CompWindow, COMPIZ_OPENGL_ABI>
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

	const CompRegion & clip () const;

	GLWindowPaintAttrib & paintAttrib ();
	GLWindowPaintAttrib & lastPaintAttrib ();

	bool bind ();
	void release ();

	const GLTexture::List &       textures () const;
	const GLTexture::MatrixList & matrices () const;

	void updatePaintAttribs ();

	Geometry & geometry ();

	WRAPABLE_HND (0, GLWindowInterface, bool, glPaint,
		      const GLWindowPaintAttrib &, const GLMatrix &,
		      const CompRegion &, unsigned int);
	WRAPABLE_HND (1, GLWindowInterface, bool, glDraw, const GLMatrix &,
		      GLFragment::Attrib &, const CompRegion &, unsigned int);
	WRAPABLE_HND (2, GLWindowInterface, void, glAddGeometry,
		      const GLTexture::MatrixList &, const CompRegion &,
		      const CompRegion &);
	WRAPABLE_HND (3, GLWindowInterface, void, glDrawTexture,
		      GLTexture *texture, GLFragment::Attrib &, unsigned int);
	WRAPABLE_HND (4, GLWindowInterface, void, glDrawGeometry);

	friend class GLScreen;
	friend class PrivateGLScreen;
	
    private:
	PrivateGLWindow *priv;
};

#endif
