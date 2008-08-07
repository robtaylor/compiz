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

#include <algorithm>

#include <compiz-core.h>
#include "privateobject.h"

typedef CompBool (*AllocObjectPrivateIndexProc) (CompObject *parent);

typedef void (*FreeObjectPrivateIndexProc) (CompObject *parent,
					    int	       index);

typedef CompBool (*ForEachObjectProc) (CompObject	  *parent,
				       ObjectCallBackProc proc,
				       void		  *closure);

typedef char *(*NameObjectProc) (CompObject *object);

typedef CompObject *(*FindObjectProc) (CompObject *parent,
				       const char *name);

struct _CompObjectInfo {
    const char			*name;
    AllocObjectPrivateIndexProc allocPrivateIndex;
    FreeObjectPrivateIndexProc  freePrivateIndex;
} objectInfo[] = {
    {
	"core",
	allocCoreObjectPrivateIndex,
	freeCoreObjectPrivateIndex
    }, {
	"display",
	allocDisplayObjectPrivateIndex,
	freeDisplayObjectPrivateIndex
    }, {
	"screen",
	allocScreenObjectPrivateIndex,
	freeScreenObjectPrivateIndex
    }, {
	"window",
	allocWindowObjectPrivateIndex,
	freeWindowObjectPrivateIndex
    }
};

void
compObjectInit (CompObject     *object,
		CompPrivate    *privates,
		CompObjectType type)
{
    object->privates = privates;
}

int
compObjectAllocatePrivateIndex (CompObject     *parent,
				CompObjectType type)
{
    return (*objectInfo[type].allocPrivateIndex) (parent);
}

void
compObjectFreePrivateIndex (CompObject     *parent,
			    CompObjectType type,
			    int	           index)
{
    (*objectInfo[type].freePrivateIndex) (parent, index);
}


const char *
compObjectTypeName (CompObjectType type)
{
    return objectInfo[type].name;
}

CompBool
compObjectForEachType (CompObject            *parent,
		       ObjectTypeCallBackProc proc,
		       void                   *closure)
{
    int i;

    for (i = 0; i < sizeof (objectInfo) / sizeof (objectInfo[0]); i++)
	if (!(*proc) (i, parent, closure))
	    return FALSE;

    return TRUE;
}


PrivateObject::PrivateObject () :
    typeName (0),
    parent (NULL),
    children (0)
{
}


CompObject::CompObject (CompObjectType type, const char* typeName) :
    privates (0)
{
    priv = new PrivateObject ();
    assert (priv);

    priv->type     = type;
    priv->typeName = typeName;
}
	
CompObject::~CompObject ()
{
    std::list<CompObject *>::iterator it;

    while (!priv->children.empty ())
    {
	CompObject *o = priv->children.front ();
	priv->children.pop_front ();
	o->priv->parent = NULL;
	core->objectRemove (this, o);
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
	    core->objectRemove (priv->parent, this);
	}
    }
}

const char *
CompObject::typeName ()
{
    return priv->typeName;
}

CompObjectType
CompObject::type ()
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
    core->objectAdd (this, object);
}

bool
CompObject::forEachChild (ObjectCallBackProc proc,
			  void	             *closure,
			  CompObjectType     type)
{
    bool rv = true;

    std::list<CompObject *>::iterator it;
    for (it = priv->children.begin (); it != priv->children.end (); it++)
    {
	if (type > 0 && (*it)->type () != type)
	    continue;
	rv &= (*proc) ((*it), closure);
    }

    return rv;
}

