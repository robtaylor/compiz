#ifndef _PRIVATEREGION_H
#define _PRIVATEREGION_H

#include <core/rect.h>
#include <core/region.h>

class PrivateRegion {
    public:
	PrivateRegion ();
	~PrivateRegion ();

	void makeReal ();
	
    public:
	Region region;
	REGION box;
};

#endif
