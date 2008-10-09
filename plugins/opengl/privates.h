/*
 * Copyright © 2008 Dennis Kasprzyk
 * Copyright © 2007 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Dennis Kasprzyk not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Dennis Kasprzyk makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DENNIS KASPRZYK DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DENNIS KASPRZYK BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: Dennis Kasprzyk <onestone@compiz-fusion.org>
 *          David Reveman <davidr@novell.com>
 */

#ifndef _COMPOSITE_PRIVATES_H
#define _COMPOSITE_PRIVATES_H

#include <composite/composite.h>
#include <opengl/opengl.h>
#include <core/atoms.h>

#include "privatefragment.h"
#include "privatetexture.h"

#define GL_OPTION_TEXTURE_FILTER      0
#define GL_OPTION_LIGHTING            1
#define GL_OPTION_SYNC_TO_VBLANK      2
#define GL_OPTION_TEXTURE_COMPRESSION 3
#define GL_OPTION_NUM                 4

extern CompMetadata *glMetadata;

extern const CompMetadata::OptionInfo
    glOptionInfo[GL_OPTION_NUM];

extern CompOutput *targetOutput;


#define RED_SATURATION_WEIGHT   0.30f
#define GREEN_SATURATION_WEIGHT 0.59f
#define BLUE_SATURATION_WEIGHT  0.11f

class PrivateGLScreen :
    public ScreenInterface,
    public CompositeScreen::PaintHandler
{
    public:
	PrivateGLScreen (GLScreen *gs);
	~PrivateGLScreen ();

	void handleEvent (XEvent *event);

	void outputChangeNotify ();

	void paintOutputs (CompOutput::ptrList &outputs,
			   unsigned int        mask,
			   const CompRegion    &region);

	bool hasVSync ();

	void prepareDrawing ();

	void waitForVideoSync ();

	void paintBackground (const CompRegion &region,
			      bool             transformed);

	void paintOutputRegion (const GLMatrix   &transform,
			        const CompRegion &region,
			        CompOutput       *output,
			        unsigned int     mask);

	void updateScreenBackground ();

	void updateView ();

    public:

	GLScreen        *gScreen;
	CompositeScreen *cScreen;

	GLenum textureFilter;

	GLFBConfig      glxPixmapFBConfigs[MAX_DEPTH + 1];

	GLTexture::List backgroundTextures;
	bool            backgroundLoaded;

	GLTexture::Filter filter[3];

	CompPoint rasterPos;

	GLFragment::Storage fragmentStorage;

	GLfloat projection[16];

	bool clearBuffers;
	bool lighting;

	GL::GLXGetProcAddressProc getProcAddress;

	GLXContext ctx;

	CompRegion outputRegion;

	bool pendingCommands;

	XRectangle lastViewport;

	std::vector<GLTexture::BindPixmapProc> bindPixmap;
	bool hasCompositing;

	CompOption::Vector opt;
};

class PrivateGLWindow :
    public WindowInterface,
    public CompositeWindowInterface
{
    public:
	PrivateGLWindow (CompWindow *w, GLWindow *gw);
	~PrivateGLWindow ();

	void windowNotify (CompWindowNotify n);
	void resizeNotify (int dx, int dy, int dwidth, int dheight);
	void moveNotify (int dx, int dy, bool now);
	void updateFrameRegion (CompRegion &region);
	
	void setWindowMatrix ();
	void updateWindowRegions ();

	CompWindow      *window;
	GLWindow        *gWindow;
	CompositeWindow *cWindow;
	GLScreen        *gScreen;

	GLTexture::List       textures;
	GLTexture::MatrixList matrices;
	CompRegion::Vector    regions;
	bool                  updateReg;
	
	CompRegion    clip;
	
	bool	      bindFailed;
	bool	      overlayWindow;

	GLushort opacity;
	GLushort brightness;
	GLushort saturation;

	GLWindowPaintAttrib paint;
	GLWindowPaintAttrib lastPaint;

	unsigned int lastMask;

	GLWindow::Geometry geometry;
};


#endif
