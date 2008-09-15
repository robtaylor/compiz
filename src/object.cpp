/*
 * Copyright © 2007 Novell, Inc.
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

#include <compiz.h>

#include <algorithm>
#include <boost/bind.hpp>

#include <compiz-core.h>
#include "privateobject.h"

PrivateObject::PrivateObject () :
    typeName (0),
    parent (NULL),
    children (0)
{
}


CompObject::CompObject (CompObject::Type type, const char* typeName,
		        CompObject::indices *iList) :
    privates (0)
{
    priv = new PrivateObject ();
    assert (priv);

    priv->type     = type;
    priv->typeName = typeName;

    if (iList && iList->size() > 0)
	privates.resize (iList->size ());
}
	
CompObject::~CompObject ()
{
    std::list<CompObject *>::iterator it;

    while (!priv->children.empty ())
    {
	CompObject *o = priv->children.front ();
	priv->children.pop_front ();
	o->priv->parent = NULL;
	delete o;
    }
    if (priv->parent)
    {
	it = std::find (priv->parent->priv->children.begin (),
			priv->parent->priv->children.end (),
			this);

	if (it != priv->parent->priv->children.end ())
	{
	    priv->parent->priv->children.erase (it);
	}
    }
    delete priv;
}

const char *
CompObject::objectTypeName ()
{
    return priv->typeName;
}

CompObject::Type
CompObject::objectType ()
{
    return priv->type;
}

void
CompObject::addChild (CompObject *object)
{
    if (!object)
	return;
    object->priv->parent = this;
    priv->children.push_back (object);
    screen->objectAdd (this, object);
}

void
CompObject::removeFromParent ()
{
    std::list<CompObject *>::iterator it;

    if (priv->parent)
    {
	it = std::find (priv->parent->priv->children.begin (),
			priv->parent->priv->children.end (),
			this);

	if (it != priv->parent->priv->children.end ())
	{
	    priv->parent->priv->children.erase (it);
	    screen->objectRemove (priv->parent, this);
	}
    }
}

bool
CompObject::forEachChild (CompObject::CallBack proc,
			  CompObject::Type     type)
{
    bool rv = true;

    std::list<CompObject *>::iterator it;
    for (it = priv->children.begin (); it != priv->children.end (); it++)
    {
	if (type > 0 && (*it)->objectType () != type)
	    continue;
	rv &= proc ((*it));
    }

    return rv;
}

static bool
resizePrivates (CompObject *o, CompObject::Type type, unsigned int size)
{
    if (o->objectType () == type)
    {
	o->privates.resize (size);
    }
    o->forEachChild (boost::bind (resizePrivates, _1, type, size));

    return true;
}

int
CompObject::allocatePrivateIndex (CompObject::Type    type,
				  CompObject::indices *iList)
{
    if (!iList)
	return -1;

    for (unsigned int i = 0; i < iList->size(); i++)
    {
	if (!iList->at (i))
	{
	    iList->at (i) = true;
	    return i;
	}
    }
    unsigned int i = iList->size ();
    iList->resize (i + 1);
    iList->at (i) = true;

    resizePrivates (screen, type, i + 1);

    return i;
}

void
CompObject::freePrivateIndex (CompObject::Type    type,
			      CompObject::indices *iList,
	 		      int idx)
{
    if (!iList || idx < 0 || idx >= (int) iList->size())
	return;

    if (idx < (int) iList->size () - 1)
    {
	iList->at(idx) = false;
	return;
    }

    unsigned int i = iList->size () - 1;
    iList->resize (i);

    resizePrivates (screen, type, i);
}

