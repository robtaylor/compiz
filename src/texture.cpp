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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <compiz-core.h>
#include <comptexture.h>
#include <privatetexture.h>
#include "privatescreen.h"

static CompTexture::Matrix _identity_matrix = {
    1.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 0.0f
};

CompTexture::CompTexture (CompScreen *screen) :
    priv (new PrivateTexture (this, screen))
{
}

CompTexture::~CompTexture ()
{
}

PrivateTexture::PrivateTexture (CompTexture *texture, CompScreen *screen) :
    screen (screen),
    texture (texture)
{
    init ();
}

PrivateTexture::~PrivateTexture ()
{
    fini ();
}


void
PrivateTexture::init ()
{
    name    = 0;
    target  = GL_TEXTURE_2D;
    pixmap  = None;
    filter  = GL_NEAREST;
    wrap    = GL_CLAMP_TO_EDGE;
    matrix  = _identity_matrix;
    damaged = true;
    mipmap  = false;
}

void
PrivateTexture::fini ()
{
    if (name)
    {
	screen->makeCurrent ();
	texture->releasePixmap ();
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

    screen->makeCurrent ();
    texture->releasePixmap ();

    if (screen->textureNonPowerOfTwo () ||
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
	(screen->getOption ("texture_compression")->value.b &&
	 screen->textureCompression () ?
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
CompTexture::imageBufferToTexture (CompTexture  *texture,
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
CompTexture::imageDataToTexture (CompTexture  *texture,
				 const char   *image,
				 unsigned int width,
				 unsigned int height,
				 GLenum       format,
				 GLenum       type)
{
    return texture->priv->loadImageData (image, width, height, format, type);
}


Bool
readImageToTexture (CompScreen   *screen,
		    CompTexture  *texture,
		    const char	 *imageFileName,
		    unsigned int *returnWidth,
		    unsigned int *returnHeight)
{
    void *image;
    int  width, height;
    Bool status;

    if (screen->display ()->readImageFromFile (imageFileName, &width, &height,
					    &image))
	return false;

    status = CompTexture::imageBufferToTexture (texture, (char *)image,
						width, height);

    free (image);

    if (returnWidth)
	*returnWidth = width;
    if (returnHeight)
	*returnHeight = height;

    return status;
}

Bool
iconToTexture (CompScreen *screen,
	       CompIcon   *icon)
{
    return CompTexture::imageBufferToTexture (icon->texture,
					      (char *) (icon + 1),
					      icon->width,
					      icon->height);
}

bool
CompTexture::bindPixmap (Pixmap pixmap,
			 int    width,
			 int    height,
			 int    depth)
{
    unsigned int target = 0;
    CompFBConfig *config = priv->screen->glxPixmapFBConfig (depth);
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
       (priv->screen->textureNonPowerOfTwo () ||
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

    priv->screen->makeCurrent ();
    priv->pixmap = (*priv->screen->createPixmap) (
	priv->screen->display ()->dpy (), config->fbConfig, pixmap, attribs);

    if (!priv->pixmap)
    {
	compLogMessage (NULL, "core", CompLogLevelWarn,
			"glXCreatePixmap failed");

	return false;
    }

    if (!target)
	(*priv->screen->queryDrawable) (priv->screen->display ()->dpy (),
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
	(*priv->screen->bindTexImage) (priv->screen->display ()->dpy (),
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
CompTexture::releasePixmap ()
{
    if (priv->pixmap)
    {
	priv->screen->makeCurrent ();
	glEnable (priv->target);
	if (!strictBinding)
	{
	    glBindTexture (priv->target, priv->name);

	    (*priv->screen->releaseTexImage) (priv->screen->display ()->dpy (),
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
CompTexture::enable (CompTexture::Filter filter)
{
    priv->screen->makeCurrent ();
    glEnable (priv->target);
    glBindTexture (priv->target, priv->name);

    if (strictBinding && priv->pixmap)
    {
	(*priv->screen->bindTexImage) (priv->screen->display ()->dpy (),
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
    else if (priv->filter != priv->screen->display ()->textureFilter ())
    {
	if (priv->screen->display ()->textureFilter () ==
	    GL_LINEAR_MIPMAP_LINEAR)
	{
	    if (priv->screen->textureNonPowerOfTwo () &&
		priv->screen->framebufferObject () && priv->mipmap)
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
			     priv->screen->display ()->textureFilter ());
	    glTexParameteri (priv->target,
			     GL_TEXTURE_MAG_FILTER,
			     priv->screen->display ()->textureFilter ());

	    priv->filter = priv->screen->display ()->textureFilter ();
	}
    }

    if (priv->filter == GL_LINEAR_MIPMAP_LINEAR)
    {
	if (priv->damaged)
	{
	    (*priv->screen->generateMipmap) (priv->target);
	    priv->damaged = false;
	}
    }
}

void
CompTexture::disable ()
{
    priv->screen->makeCurrent ();
    if (strictBinding && priv->pixmap)
    {
	glBindTexture (priv->target, priv->name);

	(*priv->screen->releaseTexImage) (priv->screen->display ()->dpy (),
					  priv->pixmap, GLX_FRONT_LEFT_EXT);
    }

    glBindTexture (priv->target, 0);
    glDisable (priv->target);
}

GLuint
CompTexture::name ()
{
    return priv->name;
}

GLenum
CompTexture::target ()
{
    return priv->target;
}

void
CompTexture::damage ()
{
    priv->damaged = true;
}

CompTexture::Matrix &
CompTexture::matrix ()
{
    return priv->matrix;
}
	
void
CompTexture::reset ()
{
    priv->fini ();
    priv->init ();
}

bool
CompTexture::hasPixmap ()
{
    return (priv->pixmap != None);
}
