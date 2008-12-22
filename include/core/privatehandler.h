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

#ifndef _COMPPRIVATEHANDLER_H
#define _COMPPRIVATEHANDLER_H

#include <typeinfo>
#include <boost/preprocessor/cat.hpp>

#include <compiz.h>
#include <core/screen.h>
#include <core/privates.h>

extern unsigned int privateHandlerIndex;

class CompPrivateIndex {
    public:
	CompPrivateIndex () : index(-1), refCount (0),
			      initiated (false), failed (false),
			      privFailed (false), privIndex (0) {}
			
	int          index;
	int          refCount;
	bool         initiated;
	bool         failed;
	bool         privFailed;
	unsigned int privIndex;
};

template<class Tp, class Tb, int ABI = 0>
class PrivateHandler {
    public:
	PrivateHandler (Tb *);
	~PrivateHandler ();

	void setFailed () { mFailed = true; };
	bool loadFailed () { return mFailed; };
	
	Tb * get () { return mBase; };
	static Tp * get (Tb *);

    private:
	static CompString keyName ()
	{
	    return compPrintf ("%s_index_%lu", typeid (Tp).name (), ABI);
	}

    private:
	bool mFailed;
	Tb   *mBase;

	static CompPrivateIndex mIndex;
};

template<class Tp, class Tb, int ABI>
CompPrivateIndex PrivateHandler<Tp,Tb,ABI>::mIndex;

template<class Tp, class Tb, int ABI>
PrivateHandler<Tp,Tb,ABI>::PrivateHandler (Tb *base) :
    mFailed (false),
    mBase (base)
{
    if (mIndex.privFailed)
    {
	mFailed = true;
    }
    else
    {
	if (!mIndex.initiated)
	{
	    mIndex.index = Tb::allocPrivateIndex ();
	    if (mIndex.index >= 0)
	    {
		mIndex.initiated = true;
		mIndex.failed    = false;
		mIndex.privIndex = privateHandlerIndex;

		CompPrivate p;
		p.val = mIndex.index;

		if (!screen->hasValue (keyName ()))
		{
		    screen->storeValue (keyName (), p);
		    privateHandlerIndex++;
		}
		else
		{
		    compLogMessage ("core", CompLogLevelFatal,
			"Private index value \"%s\" already stored in screen.",
			keyName ().c_str ());
		}
	    }
	    else
	    {
		mIndex.failed = true;
		mIndex.initiated = false;
		mIndex.privFailed = true;
		mIndex.privIndex = privateHandlerIndex;
		mFailed = true;
	    }
	}

	if (!mIndex.failed)
	{
	    mIndex.refCount++;
	    mBase->privates[mIndex.index].ptr = static_cast<Tp *> (this);
	}
    }
}

template<class Tp, class Tb, int ABI>
PrivateHandler<Tp,Tb,ABI>::~PrivateHandler ()
{
    if (!mIndex.privFailed)
    {
	mIndex.refCount--;

	if (mIndex.refCount == 0)
	{
	    Tb::freePrivateIndex (mIndex.index);
	    mIndex.initiated = false;
	    mIndex.failed = false;
	    mIndex.privIndex = privateHandlerIndex;
	    screen->eraseValue (keyName ());
	    privateHandlerIndex++;
	}
    }
}

template<class Tp, class Tb, int ABI>
Tp *
PrivateHandler<Tp,Tb,ABI>::get (Tb *base)
{
    if (mIndex.initiated && privateHandlerIndex == mIndex.privIndex)
    {
	return static_cast<Tp *>
	    (base->privates[mIndex.index].ptr);
    }
    if (mIndex.failed && privateHandlerIndex == mIndex.privIndex)
	return NULL;

    if (screen->hasValue (keyName ()))
    {
	mIndex.index     = screen->getValue (keyName ()).val;
	mIndex.initiated = true;
	mIndex.failed    = false;
	mIndex.privIndex = privateHandlerIndex;
	return static_cast<Tp *> (base->privates[mIndex.index].ptr);
    }
    else
    {
	mIndex.initiated = false;
	mIndex.failed    = true;
	mIndex.privIndex = privateHandlerIndex;
	return NULL;
    }
}

#endif
