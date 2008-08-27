
#ifndef _COMPPRIVATEINDEX_
#define _COMPPRIVATEINDEX_
class CompPrivateIndex {
    public:
	CompPrivateIndex () : index(-1), refCount (0),
			      initiated (false), failed (false) {}
			
	int  index;
	int  refCount;
	bool initiated;
	bool failed;
};

#endif

#if defined(PLUGIN) || !defined(_COMPPRIVATEHANDLER_)

#include <typeinfo>
#include <boost/preprocessor/cat.hpp>

#include <compiz.h>
#include <compobject.h>
#include <compcore.h>

#if !defined(PLUGIN)
#define PLUGIN
#define _COMPPRIVATEHANDLER_
#endif

#define INDICESNAME BOOST_PP_CAT(PLUGIN, PrivateIndicies)
#define CLASSNAME BOOST_PP_CAT(PLUGIN, PrivateHandler)
			 
static CompPrivateIndex INDICESNAME[COMP_OBJECT_TYPE_NUM];

template<class Tp, class Tb, int ABI = 0>
class CLASSNAME {
    public:
	CLASSNAME (Tb *);
	~CLASSNAME ();

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
	bool mPrivFailed;
	Tb   *mBase;
};

template<class Tp, class Tb, int ABI>
CLASSNAME<Tp,Tb,ABI>::CLASSNAME (Tb *base) :
    mFailed (false),
    mPrivFailed (false),
    mBase (base)
{
    if (mBase->objectType () >= COMP_OBJECT_TYPE_NUM ||
        INDICESNAME[base->objectType ()].failed)
    {
	mFailed = true;
	mPrivFailed = true;
    }
    else
    {
	if (!INDICESNAME[mBase->objectType ()].initiated)
	{
	    INDICESNAME[mBase->objectType ()].index = Tb::allocPrivateIndex ();
	    if (INDICESNAME[mBase->objectType ()].index >= 0)
	    {
		INDICESNAME[mBase->objectType ()].initiated = true;

		CompPrivate p;
		p.val = INDICESNAME[mBase->objectType ()].index;

		if (!core->hasValue (keyName ()))
		{
		    core->storeValue (keyName (), p);
		}
		else
		{
		    compLogMessage (0, "core", CompLogLevelFatal,
			"Private index value \"%s\" already stored in core.",
			keyName ().c_str ());
		}
	    }
	    else
	    {
		INDICESNAME[mBase->objectType ()].failed = true;
		mPrivFailed = true;
		mFailed = true;
	    }
	}

	if (!INDICESNAME[mBase->objectType ()].failed)
	{
	    INDICESNAME[mBase->objectType ()].refCount++;
	    mBase->privates[INDICESNAME[mBase->objectType ()].index].ptr =
		static_cast<Tp *> (this);
	}
    }
}

template<class Tp, class Tb, int ABI>
CLASSNAME<Tp,Tb,ABI>::~CLASSNAME ()
{
    if (!mPrivFailed && !INDICESNAME[mBase->objectType ()].failed)
    {
	INDICESNAME[mBase->objectType ()].refCount--;

	if (INDICESNAME[mBase->objectType ()].refCount == 0)
	{
	    Tb::freePrivateIndex (INDICESNAME[mBase->objectType ()].index);
	    INDICESNAME[mBase->objectType ()].initiated = false;
	    core->eraseValue (keyName ());
	}
    }
}

template<class Tp, class Tb, int ABI>
Tp *
CLASSNAME<Tp,Tb,ABI>::get (Tb *base)
{
    if (INDICESNAME[base->objectType ()].initiated)
    {
	return static_cast<Tp *>
	    (base->privates[INDICESNAME[base->objectType ()].index].ptr);
    }
    if (INDICESNAME[base->objectType ()].failed)
	return NULL;

    if (core->hasValue (keyName ()))
    {
	INDICESNAME[base->objectType ()].index =
	    core->getValue (keyName ()).val;
	INDICESNAME[base->objectType ()].initiated = true;
	INDICESNAME[base->objectType ()].refCount  = -1;
	return static_cast<Tp *>
	    (base->privates[INDICESNAME[base->objectType ()].index].ptr);
    }
    else
    {
	INDICESNAME[base->objectType ()].failed = true;
	return NULL;
    }
}

#undef INDICESNAME
#undef CLASSNAME
#undef PLUGIN
#endif
