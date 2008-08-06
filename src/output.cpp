
#include <compiz-core.h>
#include <compoutput.h>

CompOutput::CompOutput ()
{
    mName = "";
    mId = ~0;

    mWorkArea.x      = 0;
    mWorkArea.y      = 0;
    mWorkArea.width  = 0;
    mWorkArea.height = 0;
}

CompString
CompOutput::name ()
{
    return mName;
}

unsigned int
CompOutput::id ()
{
    return mId;
}

XRectangle
CompOutput::workArea ()
{
    return mWorkArea;
}

void
CompOutput::setWorkArea (XRectangle workarea)
{
    if (workarea.x < (int) x1 ())
	mWorkArea.x = x1 ();
    else
	mWorkArea.x = workarea.x;

    if (workarea.y < (int) y1 ())
	mWorkArea.y = y1 ();
    else
	mWorkArea.y = workarea.y;

    if (workarea.x + workarea.width > (int) x2 ())
	mWorkArea.width = x2 () - mWorkArea.x;
    else
	mWorkArea.width = workarea.width;

    if (workarea.y + workarea.height > (int) y2 ())
	mWorkArea.height = y2 () - mWorkArea.y;
    else
	mWorkArea.height = workarea.height;

}
void
CompOutput::setGeometry (int x1, int x2, int y1, int y2)
{
    if (x1 < 0)
	x1 = 0;

    if (y1 < 0)
	y1 = 0;

    if (x2 < 0)
	x2 = 0;

    if (y2 < 0)
	y2 = 0;

    mWorkArea.x      = this->x1 ();
    mWorkArea.y      = this->y1 ();
    mWorkArea.width  = width ();
    mWorkArea.height = height ();
}

void
CompOutput::setId (CompString name, unsigned int id)
{
    mName = name;
    mId = id;
}

