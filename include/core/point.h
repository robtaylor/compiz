/*
 * Copyright © 2008 Dennis Kasprzyk
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
 */

#ifndef _COMPPOINT_H
#define _COMPPOINT_H

#include <vector>
#include <list>

class CompPoint {

    public:
	CompPoint ();
	CompPoint (int, int);

	int x () const;
	int y () const;

	void set (int, int);
	void setX (int);
	void setY (int);

	bool operator== (const CompPoint &) const;
	bool operator!= (const CompPoint &) const;

	typedef std::vector<CompPoint> vector;
	typedef std::vector<CompPoint *> ptrVector;
	typedef std::list<CompPoint> list;
	typedef std::list<CompPoint *> ptrList;

    private:
	int mX, mY;
};

inline int
CompPoint::x () const
{
    return mX;
}

inline int
CompPoint::y () const
{
    return mY;
}


#endif
