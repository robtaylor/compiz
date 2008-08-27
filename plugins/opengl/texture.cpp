/*
 * Copyright © 2005 Novell, Inc.
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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <compiz.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <compiz-core.h>
#include <opengl/texture.h>
#include <privatetexture.h>
#include "privates.h"

static GLTexture::Matrix _identity_matrix = {
    1.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 0.0f
};

GLTexture::GLTexture (CompScreen *screen) :
    priv (new PrivateTexture (this, screen))
{
}

GLTexture::~GLTexture ()
{
}

PrivateTexture::PrivateTexture (GLTexture *texture, CompScreen *screen) :
    screen (screen),
    gScreen (GLScreen::get (screen)),
    gDisplay (GLDisplay::get (screen->display ())),
    texture (texture),
    name (0),
    target (GL_TEXTURE_2D),
    pixmap  (None),
    filter  (GL_NEAREST),
    wrap    (GL_CLAMP_TO_EDGE),
    matrix  (_identity_matrix),
    damaged (true),
    mipmap  (false)
{
}

PrivateTexture::~PrivateTexture ()
{
    if (name)
    {
	gScreen->makeCurrent ();
	if (pixmap)
	{
	    glEnable (target);
	    if (!strictBinding)
	    {
		glBindTexture (target, name);

		(*gScreen->releaseTexImage) (screen->display ()->dpy (),
					     pixmap, GLX_FRONT_LEFT_EXT);
	    }

	    glBindTexture (target, 0);
	    glDisable (target);

	    glXDestroyGLXPixmap (screen->display ()->dpy (), pixmap);
	}
	glDeleteTextures (1, &name);
    }
}


bool
PrivateTexture::loadImageData (const char   *image,
			       unsigned int width,
			       unsigned int height,
			       GLenum       format,
			       GLenum       type)
{
    GLint internalFormat;

    gScreen->makeCurrent ();
    texture->releasePixmap ();

    if (gScreen->textureNonPowerOfTwo () ||
	(POWER_OF_TWO (width) && POWER_OF_TWO (height)))
    {
	target = GL_TEXTURE_2D;
	matrix.xx = 1.0f / width;
	matrix.yy = 1.0f / height;
	matrix.y0 = 0.0f;
	mipmap = true;
    }
    else
    {
	target = GL_TEXTURE_RECTANGLE_NV;
	matrix.xx = 1.0f;
	matrix.yy = 1.0f;
	matrix.y0 = 0.0f;
	mipmap = false;
    }

    this->width  = width;
    this->height = height;

    if (!name)
	glGenTextures (1, &name);

    glBindTexture (target, name);

    internalFormat =
	(gScreen->getOption ("texture_compression")->value ().b () &&
	 gScreen->textureCompression () ?
	 GL_COMPRESSED_RGBA_ARB : GL_RGBA);

    glTexImage2D (target, 0, internalFormat, width, height, 0,
		  format, type, image);

    filter = GL_NEAREST;

    glTexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri (target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    wrap = GL_CLAMP_TO_EDGE;

    glBindTexture (target, 0);

    return true;
}

bool
GLTexture::imageBufferToTexture (GLTexture  *texture,
				   const char   *image,
				   unsigned int width,
				   unsigned int height)
{
#if IMAGE_BYTE_ORDER == MSBFirst
    return texture->priv->loadImageData (image, width, height,
					 GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV);
#else
    return texture->priv->loadImageData (image, width, height,
					 GL_BGRA, GL_UNSIGNED_BYTE);
#endif
}

bool
GLTexture::imageDataToTexture (GLTexture  *texture,
				 const char   *image,
				 unsigned int width,
				 unsigned int height,
				 GLenum       format,
				 GLenum       type)
{
    return texture->priv->loadImageData (image, width, height, format, type);
}


bool
GLTexture::readImageToTexture (CompScreen   *screen,
				 GLTexture  *texture,
				 const char   *imageFileName,
				 unsigned int *returnWidth,
				 unsigned int *returnHeight)
{
    void *image;
    int  width, height;
    Bool status;

    if (screen->display ()->readImageFromFile (imageFileName, &width, &height,
					    &image))
	return false;

    status = GLTexture::imageBufferToTexture (texture, (char *)image,
						width, height);

    free (image);

    if (returnWidth)
	*returnWidth = width;
    if (returnHeight)
	*returnHeight = height;

    return status;
}

bool
GLTexture::bindPixmap (Pixmap pixmap,
		       int    width,
		       int    height,
		       int    depth)
{
    unsigned int target = 0;
    GLFBConfig   *config = priv->gScreen->glxPixmapFBConfig (depth);
    int          attribs[7], i = 0;

    if (!config->fbConfig)
    {
	compLogMessage (NULL, "core", CompLogLevelWarn,
			"No GLXFBConfig for depth %d",
			depth);

	return false;
    }

    attribs[i++] = GLX_TEXTURE_FORMAT_EXT;
    attribs[i++] = config->textureFormat;
    attribs[i++] = GLX_MIPMAP_TEXTURE_EXT;
    attribs[i++] = config->mipmap;

    /* If no texture target is specified in the fbconfig, or only the
       TEXTURE_2D target is specified and GL_texture_non_power_of_two
       is not supported, then allow the server to choose the texture target. */
    if (config->textureTargets & GLX_TEXTURE_2D_BIT_EXT &&
       (priv->gScreen->textureNonPowerOfTwo () ||
       (POWER_OF_TWO (width) && POWER_OF_TWO (height))))
	target = GLX_TEXTURE_2D_EXT;
    else if (config->textureTargets & GLX_TEXTURE_RECTANGLE_BIT_EXT)
	target = GLX_TEXTURE_RECTANGLE_EXT;

    /* Workaround for broken texture from pixmap implementations, 
       that don't advertise any texture target in the fbconfig. */
    if (!target)
    {
	if (!(config->textureTargets & GLX_TEXTURE_2D_BIT_EXT))
	    target = GLX_TEXTURE_RECTANGLE_EXT;
	else if (!(config->textureTargets & GLX_TEXTURE_RECTANGLE_BIT_EXT))
	    target = GLX_TEXTURE_2D_EXT;
    }

    if (target)
    {
	attribs[i++] = GLX_TEXTURE_TARGET_EXT;
	attribs[i++] = target;
    }

    attribs[i++] = None;

    priv->gScreen->makeCurrent ();
    priv->pixmap = (*priv->gScreen->createPixmap) (
	priv->screen->display ()->dpy (), config->fbConfig, pixmap, attribs);

    if (!priv->pixmap)
    {
	compLogMessage (NULL, "core", CompLogLevelWarn,
			"glXCreatePixmap failed");

	return false;
    }

    if (!target)
	(*priv->gScreen->queryDrawable) (priv->screen->display ()->dpy (),
					 priv->pixmap,
					 GLX_TEXTURE_TARGET_EXT, &target);

    switch (target) {
    case GLX_TEXTURE_2D_EXT:
	priv->target = GL_TEXTURE_2D;

	priv->matrix.xx = 1.0f / width;
	if (config->yInverted)
	{
	    priv->matrix.yy = 1.0f / height;
	    priv->matrix.y0 = 0.0f;
	}
	else
	{
	    priv->matrix.yy = -1.0f / height;
	    priv->matrix.y0 = 1.0f;
	}
	priv->mipmap = config->mipmap;
	break;
    case GLX_TEXTURE_RECTANGLE_EXT:
	priv->target = GL_TEXTURE_RECTANGLE_ARB;

	priv->matrix.xx = 1.0f;
	if (config->yInverted)
	{
	    priv->matrix.yy = 1.0f;
	    priv->matrix.y0 = 0;
	}
	else
	{
	    priv->matrix.yy = -1.0f;
	    priv->matrix.y0 = height;
	}
	priv->mipmap = false;
	break;
    default:
	compLogMessage (NULL, "core", CompLogLevelWarn,
			"pixmap 0x%x can't be bound to texture",
			(int) pixmap);

	glXDestroyGLXPixmap (priv->screen->display ()->dpy (), priv->pixmap);
	priv->pixmap = None;

	return false;
    }

    if (!priv->name)
	glGenTextures (1, &priv->name);

    glBindTexture (priv->target, priv->name);

    if (!strictBinding)
    {
	(*priv->gScreen->bindTexImage) (priv->screen->display ()->dpy (),
				        priv->pixmap,
				        GLX_FRONT_LEFT_EXT,
				        NULL);
    }

    priv->filter = GL_NEAREST;

    glTexParameteri (priv->target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (priv->target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri (priv->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (priv->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    priv->wrap = GL_CLAMP_TO_EDGE;

    glBindTexture (priv->target, 0);

    return true;
}

void
GLTexture::releasePixmap ()
{
    if (priv->pixmap)
    {
	priv->gScreen->makeCurrent ();
	glEnable (priv->target);
	if (!strictBinding)
	{
	    glBindTexture (priv->target, priv->name);

	    (*priv->gScreen->releaseTexImage) (priv->screen->display ()->dpy (),
					       priv->pixmap,
					       GLX_FRONT_LEFT_EXT);
	}

	glBindTexture (priv->target, 0);
	glDisable (priv->target);

	glXDestroyGLXPixmap (priv->screen->display ()->dpy (), priv->pixmap);

	priv->pixmap = None;
    }
}

void
GLTexture::enable (GLTexture::Filter filter)
{
    priv->gScreen->makeCurrent ();
    glEnable (priv->target);
    glBindTexture (priv->target, priv->name);

    if (strictBinding && priv->pixmap)
    {
	(*priv->gScreen->bindTexImage) (priv->screen->display ()->dpy (),
				        priv->pixmap,
				        GLX_FRONT_LEFT_EXT, NULL);
    }

    if (filter == Fast)
    {
	if (priv->filter != GL_NEAREST)
	{
	    glTexParameteri (priv->target,
			     GL_TEXTURE_MIN_FILTER,
			     GL_NEAREST);
	    glTexParameteri (priv->target,
			     GL_TEXTURE_MAG_FILTER,
			     GL_NEAREST);

	    priv->filter = GL_NEAREST;
	}
    }
    else if (priv->filter != priv->gDisplay->textureFilter ())
    {
	if (priv->gDisplay->textureFilter () ==
	    GL_LINEAR_MIPMAP_LINEAR)
	{
	    if (priv->gScreen->textureNonPowerOfTwo () &&
		priv->gScreen->framebufferObject () && priv->mipmap)
	    {
		glTexParameteri (priv->target,
				 GL_TEXTURE_MIN_FILTER,
				 GL_LINEAR_MIPMAP_LINEAR);

		if (priv->filter != GL_LINEAR)
		    glTexParameteri (priv->target,
				     GL_TEXTURE_MAG_FILTER,
				     GL_LINEAR);

		priv->filter = GL_LINEAR_MIPMAP_LINEAR;
	    }
	    else if (priv->filter != GL_LINEAR)
	    {
		glTexParameteri (priv->target,
				 GL_TEXTURE_MIN_FILTER,
				 GL_LINEAR);
		glTexParameteri (priv->target,
				 GL_TEXTURE_MAG_FILTER,
				 GL_LINEAR);

		priv->filter = GL_LINEAR;
	    }
	}
	else
	{
	    glTexParameteri (priv->target,
			     GL_TEXTURE_MIN_FILTER,
			     priv->gDisplay->textureFilter ());
	    glTexParameteri (priv->target,
			     GL_TEXTURE_MAG_FILTER,
			     priv->gDisplay->textureFilter ());

	    priv->filter = priv->gDisplay->textureFilter ();
	}
    }

    if (priv->filter == GL_LINEAR_MIPMAP_LINEAR)
    {
	if (priv->damaged)
	{
	    (*priv->gScreen->generateMipmap) (priv->target);
	    priv->damaged = false;
	}
    }
}

void
GLTexture::disable ()
{
    priv->gScreen->makeCurrent ();
    if (strictBinding && priv->pixmap)
    {
	glBindTexture (priv->target, priv->name);

	(*priv->gScreen->releaseTexImage) (priv->screen->display ()->dpy (),
					   priv->pixmap, GLX_FRONT_LEFT_EXT);
    }

    glBindTexture (priv->target, 0);
    glDisable (priv->target);
}

GLuint
GLTexture::name ()
{
    return priv->name;
}

GLenum
GLTexture::target ()
{
    return priv->target;
}

void
GLTexture::damage ()
{
    priv->damaged = true;
}

GLTexture::Matrix &
GLTexture::matrix ()
{
    return priv->matrix;
}
	
void
GLTexture::reset ()
{
    priv = boost::shared_ptr <PrivateTexture>
		(new PrivateTexture (this, priv->screen));
}

bool
GLTexture::hasPixmap ()
{
    return (priv->pixmap != None);
}
