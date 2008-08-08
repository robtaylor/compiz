#ifndef _COMPOBJECT_H
#define _COMPOBJECT_H

#include <vector>

typedef int CompObjectType;


#define COMP_OBJECT_TYPE_ALL     -1
#define COMP_OBJECT_TYPE_CORE    0
#define COMP_OBJECT_TYPE_DISPLAY 1
#define COMP_OBJECT_TYPE_SCREEN  2
#define COMP_OBJECT_TYPE_WINDOW  3


typedef bool (*ObjectCallBackProc) (CompObject *object,
				    void       *closure);

#define ARRAY_SIZE(array)		 \
    (sizeof (array) / sizeof (array[0]))

#define DISPATCH_CHECK(object, dispTab, tabSize)	      \
    ((object)->type () < (tabSize) && (dispTab)[(object)->type ()])

#define DISPATCH(object, dispTab, tabSize, args)   \
    if (DISPATCH_CHECK (object, dispTab, tabSize)) \
	(*(dispTab)[(object)->type ()]) args

#define RETURN_DISPATCH(object, dispTab, tabSize, def, args) \
    if (DISPATCH_CHECK (object, dispTab, tabSize))	     \
	return (*(dispTab)[(object)->type ()]) args;	     \
    else						     \
	return (def)

class PrivateObject;

class CompObject {

    public:
	typedef std::vector<bool> indices;

    public:
	CompObject (CompObjectType type, const char* typeName,
		    indices *iList = NULL);
	virtual ~CompObject ();

        const char *typeName ();
	CompObjectType type ();

	void addChild (CompObject *);

        bool forEachChild (ObjectCallBackProc proc,
			   void	              *closure = NULL,
			   int                type = -1);

	virtual CompString name () = 0;

    public:

	std::vector<CompPrivate> privates;

    protected:
	static int allocatePrivateIndex (CompObjectType type, indices *iList);
	static void freePrivateIndex (CompObjectType type,
				      indices *iList, int idx);

    private:
	PrivateObject *priv;
};


#endif
