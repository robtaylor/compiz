#ifndef _PRIVATES_H
#define _PRIVATES_H

#include <vector>

#define MAX_PRIVATE_STORAGE 2

union CompPrivate {
    void	  *ptr;
    long	  val;
    unsigned long uval;
    void	  *(*fptr) (void);
};

class CompPrivateStorage {

    public:
	typedef std::vector<bool> Indices;

    public:
	CompPrivateStorage (Indices *iList, unsigned int index);

	unsigned int storageIndex ();

    public:
	std::vector<CompPrivate> privates;

    protected:
	static int allocatePrivateIndex (Indices *iList);
	static void freePrivateIndex (Indices *iList, int idx);

    private:
	unsigned int mIndex;
};

#endif
