#include "privates.h"

GLushort defaultColor[4] = { 0xffff, 0xffff, 0xffff, 0xffff };

GLDisplay::GLDisplay (CompDisplay *d) :
    OpenGLPrivateHandler<GLDisplay, CompDisplay, COMPIZ_OPENGL_ABI> (d),
    priv (new PrivateGLDisplay (d, this))
{
    if (!glMetadata->initDisplayOptions (d, glDisplayOptionInfo,
					 GL_DISPLAY_OPTION_NUM, priv->opt))
    {
	setFailed ();
	return;
    }
}

GLDisplay::~GLDisplay ()
{
    delete priv;
}

PrivateGLDisplay::PrivateGLDisplay (CompDisplay *d,
				    GLDisplay   *gd) :
    display (d),
    gDisplay (gd),
    textureFilter (GL_LINEAR)
{
}

PrivateGLDisplay::~PrivateGLDisplay ()
{
}

GLenum
GLDisplay::textureFilter ()
{
    return priv->textureFilter;
}

void
PrivateGLDisplay::handleEvent (XEvent *event)
{
    CompScreen *s;
    CompWindow *w;

    display->handleEvent (event);

    switch (event->type) {
	case PropertyNotify:
	    if (event->xproperty.atom == display->atoms ().xBackground[0] ||
		event->xproperty.atom == display->atoms ().xBackground[1])
	    {
		s = display->findScreen (event->xproperty.window);
		if (s)
		    GLScreen::get (s)->updateBackground ();
	    }
	    else if (event->xproperty.atom == display->atoms ().winOpacity ||
		     event->xproperty.atom == display->atoms ().winBrightness ||
		     event->xproperty.atom == display->atoms ().winSaturation)
	    {
		w = display->findWindow (event->xproperty.window);
		if (w)
		    GLWindow::get (w)->updatePaintAttribs ();
	    }
	    break;
	break;
	default:
	    break;
    }
}

void
GLDisplay::clearTargetOutput (unsigned int mask)
{
    if (targetScreen)
	targetScreen->clearOutput (targetOutput, mask);
}
