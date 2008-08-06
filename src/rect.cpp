#include <compiz-core.h>
#include <comprect.h>

CompRect::CompRect ()
{
    mRegion.rects = &mRegion.extents;
    mRegion.numRects = 1;
    mRegion.extents.x1 = 0;
    mRegion.extents.x2 = 0;
    mRegion.extents.y1 = 0;
    mRegion.extents.y2 = 0;
}

CompRect::CompRect (int x1, int x2, int y1, int y2)
{
    CompRect ();
    setGeometry (x1, x2, y1, y2);
}

int
CompRect::x ()
{
    return mRegion.extents.x1;
}

int
CompRect::y ()
{
    return mRegion.extents.y1;
}

int
CompRect::x1 ()
{
    return mRegion.extents.x1;
}

int
CompRect::y1 ()
{
    return mRegion.extents.y1;
}

int
CompRect::x2 ()
{
    return mRegion.extents.x2;
}

int
CompRect::y2 ()
{
    return mRegion.extents.y2;
}

unsigned int
CompRect::width ()
{
    return mRegion.extents.x2 - mRegion.extents.x1;
}

unsigned int
CompRect::height ()
{
    return mRegion.extents.y2 - mRegion.extents.y1;
}

Region
CompRect::region ()
{
    return &mRegion;
}

void
CompRect::setGeometry (int x1, int x2, int y1, int y2)
{
    mRegion.extents.x1 = x1;
    mRegion.extents.y1 = y1;

    if (x2 < x1)
	mRegion.extents.x2 = x1;
    else
	mRegion.extents.x2 = x2;

    if (y2 < y1)
	mRegion.extents.y2 = y1;
    else
	mRegion.extents.y2 = y2;
}

