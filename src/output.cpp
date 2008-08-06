
#include <compiz-core.h>
#include <compoutput.h>

CompOutput::CompOutput ()
{
    mName = "";
    mId = ~0;
    mRegion.rects = &mRegion.extents;
    mRegion.numRects = 1;
    mRegion.extents.x1 = 0;
    mRegion.extents.x2 = 0;
    mRegion.extents.y1 = 0;
    mRegion.extents.y2 = 0;

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

unsigned int
CompOutput::x1 ()
{
    return mRegion.extents.x1;
}

unsigned int
CompOutput::y1 ()
{
    return mRegion.extents.y1;
}

unsigned int
CompOutput::x2 ()
{
    return mRegion.extents.x2;
}

unsigned int
CompOutput::y2 ()
{
    return mRegion.extents.y2;
}

unsigned int
CompOutput::width ()
{
    return mRegion.extents.x2 - mRegion.extents.x1;
}

unsigned int
CompOutput::height ()
{
    return mRegion.extents.y2 - mRegion.extents.y1;
}

Region
CompOutput::region ()
{
    return &mRegion;
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
CompOutput::setGeometry (unsigned int x1, unsigned int x2,
			 unsigned int y1, unsigned int y2)
{
    if (x1 < 0)
	mRegion.extents.x1 = 0;
    else
	mRegion.extents.x1 = x1;

    if (y1 < 0)
	mRegion.extents.y1 = 0;
    else
	mRegion.extents.y1 = y1;

    if ((int) x2 < mRegion.extents.x1)
	mRegion.extents.x2 = mRegion.extents.x1;
    else
	mRegion.extents.x2 = x2;

    if ((int) y2 < mRegion.extents.y1)
	mRegion.extents.y2 = mRegion.extents.y1;
    else
	mRegion.extents.y2 = y2;

    mWorkArea.x      = mRegion.extents.x1;
    mWorkArea.y      = mRegion.extents.y1;
    mWorkArea.width  = width ();
    mWorkArea.height = height ();
}

void
CompOutput::setId (CompString name, unsigned int id)
{
    mName = name;
    mId = id;
}

