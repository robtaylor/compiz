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

#include <core/core.h>
#include <core/output.h>

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

    CompRect::setGeometry (x1, x2, y1, y2);

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

