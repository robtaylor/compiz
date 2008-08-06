#include <compwindow.h>


CompWindow::Geometry::Geometry () :
    mBorder (0)
{
}

CompWindow::Geometry::Geometry (int x, int y,
				unsigned int width,
				unsigned int height,
				unsigned int border) :
    mBorder (border)
{
    CompPoint::CompPoint (x,y);
    CompSize::CompSize (width, height);
}

unsigned int
CompWindow::Geometry::border ()
{
    return mBorder;
}

void
CompWindow::Geometry::setBorder (unsigned int border)
{
    mBorder = border;
}
