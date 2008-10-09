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

#ifndef _COMPSIZE_H
#define _COMPSIZE_H

#include <vector>
#include <list>

class CompSize {

    public:
	CompSize ();
	CompSize (unsigned int, unsigned int);

	unsigned int width () const;
	unsigned int height () const;

	void setWidth (unsigned int);
	void setHeight (unsigned int);

	typedef std::vector<CompSize> vector;
	typedef std::vector<CompSize *> ptrVector;
	typedef std::list<CompSize> list;
	typedef std::list<CompSize *> ptrList;

    private:
	unsigned int mWidth, mHeight;
};

inline unsigned int
CompSize::width () const
{
    return mWidth;
}

inline unsigned int
CompSize::height () const
{
    return mHeight;
}

#endif
