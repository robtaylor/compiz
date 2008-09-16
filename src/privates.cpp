
#include <core/privates.h>

CompPrivateStorage::CompPrivateStorage (CompPrivateStorage::Indices *iList,
				        unsigned int                index) :
    privates (0),
    mIndex (index)
{
    if (iList->size() > 0)
	privates.resize (iList->size ());
}

int
CompPrivateStorage::allocatePrivateIndex (CompPrivateStorage::Indices *iList)
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

    return i;
}

void
CompPrivateStorage::freePrivateIndex (CompPrivateStorage::Indices *iList,
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
}

unsigned int
CompPrivateStorage::storageIndex ()
{
    return mIndex;
}
