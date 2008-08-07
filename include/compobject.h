#ifndef _COMPOBJECT_H
#define _COMPOBJECT_H


typedef int CompObjectType;


#define COMP_OBJECT_TYPE_ALL     -1
#define COMP_OBJECT_TYPE_CORE    0
#define COMP_OBJECT_TYPE_DISPLAY 1
#define COMP_OBJECT_TYPE_SCREEN  2
#define COMP_OBJECT_TYPE_WINDOW  3


typedef bool (*ObjectCallBackProc) (CompObject *object,
				    void       *closure);

typedef bool (*ObjectTypeCallBackProc) (CompObjectType type,
					CompObject     *parent,
					void           *closure);

void
compObjectInit (CompObject     *object,
		CompPrivate    *privates,
		CompObjectType type);

void
compObjectFini (CompObject *object);

int
compObjectAllocatePrivateIndex (CompObject     *parent,
				CompObjectType type);

void
compObjectFreePrivateIndex (CompObject     *parent,
			    CompObjectType type,
			    int	           index);

CompBool
compObjectForEachType (CompObject	      *parent,
		       ObjectTypeCallBackProc proc,
		       void		      *closure);

const char *
compObjectTypeName (CompObjectType type);

char *
compObjectName (CompObject *object);

CompObject *
compObjectFind (CompObject     *parent,
		CompObjectType type,
		const char     *name);

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
	CompObject (CompObjectType type, const char* typeName);
	virtual ~CompObject ();

        const char *typeName ();
	CompObjectType type ();

	void addChild (CompObject *);

        bool forEachChild (ObjectCallBackProc proc,
			   void	              *closure = NULL,
			   int                type = -1);

	virtual CompString name () = 0;

    public:
	CompPrivate	   *privates;

    private:
	PrivateObject *priv;
};


#endif
