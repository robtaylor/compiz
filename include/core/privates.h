#ifndef _PRIVATES_H
#define _PRIVATES_H

#include <vector>

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
	CompPrivateStorage (Indices *iList);

    public:
	std::vector<CompPrivate> privates;

    protected:
	static int allocatePrivateIndex (Indices *iList);
	static void freePrivateIndex (Indices *iList, int idx);
};

#endif
