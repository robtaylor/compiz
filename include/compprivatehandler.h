
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
#include <core/privates.h>

#if !defined(PLUGIN)
#define PLUGIN
#define _COMPPRIVATEHANDLER_
#endif

#define INDICESNAME BOOST_PP_CAT(PLUGIN, PrivateIndicies)
#define CLASSNAME BOOST_PP_CAT(PLUGIN, PrivateHandler)
			 
static CompPrivateIndex INDICESNAME[MAX_PRIVATE_STORAGE];

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
    if (mBase->storageIndex () >= MAX_PRIVATE_STORAGE ||
        INDICESNAME[base->storageIndex ()].failed)
    {
	mFailed = true;
	mPrivFailed = true;
    }
    else
    {
	if (!INDICESNAME[mBase->storageIndex ()].initiated)
	{
	    INDICESNAME[mBase->storageIndex ()].index = Tb::allocPrivateIndex ();
	    if (INDICESNAME[mBase->objectType ()].index >= 0)
	    {
		INDICESNAME[mBase->storageIndex ()].initiated = true;

		CompPrivate p;
		p.val = INDICESNAME[mBase->storageIndex ()].index;

		if (!screen->hasValue (keyName ()))
		{
		    screen->storeValue (keyName (), p);
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
		INDICESNAME[mBase->storageIndex ()].failed = true;
		mPrivFailed = true;
		mFailed = true;
	    }
	}

	if (!INDICESNAME[mBase->storageIndex ()].failed)
	{
	    INDICESNAME[mBase->storageIndex ()].refCount++;
	    mBase->privates[INDICESNAME[mBase->storageIndex ()].index].ptr =
		static_cast<Tp *> (this);
	}
    }
}

template<class Tp, class Tb, int ABI>
CLASSNAME<Tp,Tb,ABI>::~CLASSNAME ()
{
    if (!mPrivFailed && !INDICESNAME[mBase->storageIndex ()].failed)
    {
	INDICESNAME[mBase->storageIndex ()].refCount--;

	if (INDICESNAME[mBase->storageIndex ()].refCount == 0)
	{
	    Tb::freePrivateIndex (INDICESNAME[mBase->storageIndex ()].index);
	    INDICESNAME[mBase->storageIndex ()].initiated = false;
	    screen->eraseValue (keyName ());
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
	    (base->privates[INDICESNAME[base->storageIndex ()].index].ptr);
    }
    if (INDICESNAME[base->storageIndex ()].failed)
	return NULL;

    if (screen->hasValue (keyName ()))
    {
	INDICESNAME[base->storageIndex ()].index =
	    screen->getValue (keyName ()).val;
	INDICESNAME[base->storageIndex ()].initiated = true;
	INDICESNAME[base->storageIndex ()].refCount  = -1;
	return static_cast<Tp *>
	    (base->privates[INDICESNAME[base->storageIndex ()].index].ptr);
    }
    else
    {
	INDICESNAME[base->storageIndex ()].failed = true;
	return NULL;
    }
}

#undef INDICESNAME
#undef CLASSNAME
#undef PLUGIN
#endif
