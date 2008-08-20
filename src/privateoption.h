#ifndef _PRIVATEOPTION_H
#define _PRIVATEOPTION_H

#include <vector>

#include <compaction.h>
#include <compmatch.h>
#include <compdisplay.h>
#include <compscreen.h>

typedef struct _CompOptionIntRestriction {
    int min;
    int max;
} IntRestriction;

typedef struct _CompOptionFloatRestriction {
    float min;
    float max;
    float precision;
} FloatRestriction;

typedef union {
    IntRestriction    i;
    FloatRestriction  f;
} RestrictionUnion;

class PrivateRestriction {
    public:
	CompOption::Type type;
	RestrictionUnion rest;
};

typedef union {
    bool	   b;
    int		   i;
    float	   f;
    unsigned short c[4];
} ValueUnion;

class PrivateValue {
    public:
	PrivateValue ();
	PrivateValue (const PrivateValue&);

	void reset ();
	
	CompOption::Type          type;
	ValueUnion                value;
	CompString                string;
	CompAction                action;
	CompMatch                 match;
	CompOption::Type          listType;
	CompOption::Value::Vector list;
};

class PrivateOption
{
    public:
	PrivateOption ();
	PrivateOption (const PrivateOption &);
	
	CompString              name;
	CompOption::Type        type;
	CompOption::Value       value;
	CompOption::Restriction rest;
};

#endif
