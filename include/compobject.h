#ifndef _COMPOBJECT_H
#define _COMPOBJECT_H

class CompObject {
    public :

    CompObjectType type;
    CompPrivate	   *privates;
    CompObject	   *parent;
};


#endif
