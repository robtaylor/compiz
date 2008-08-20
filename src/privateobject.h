#ifndef _PRIVATEOBJECT_H
#define _PRIVATEOBJECT_H

#include <list>
#include <compiz-core.h>
#include <compobject.h>

class PrivateObject {
    public :
	PrivateObject ();

	CompObject::Type type;

	const char *typeName;

	CompObject	        *parent;
	std::list<CompObject *> children;
};


#endif