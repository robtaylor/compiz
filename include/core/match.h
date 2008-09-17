#ifndef _COMPMATCH_H
#define _COMPMATCH_H

#include <core/core.h>

class PrivateMatch;
class CompWindow;
class CompDisplay;

class CompMatch {
    public:

	class Expression {
	    public:
		virtual ~Expression () {};
		virtual bool evaluate (CompWindow *window) = 0;
	};
	
    public:
	CompMatch ();
	CompMatch (const CompString);
	CompMatch (const CompMatch &);
	~CompMatch ();

	void update ();
	bool evaluate (CompWindow *window);

	CompString toString ();

	CompMatch & operator= (const CompMatch &);
	CompMatch & operator&= (const CompMatch &);
	CompMatch & operator|= (const CompMatch &);

	const CompMatch & operator& (const CompMatch &);
	const CompMatch & operator| (const CompMatch &);
	const CompMatch & operator! ();

	CompMatch & operator= (const CompString &);
	CompMatch & operator&= (const CompString &);
	CompMatch & operator|= (const CompString &);

	const CompMatch & operator& (const CompString &);
	const CompMatch & operator| (const CompString &);

	bool operator== (const CompMatch &);

    private:
	PrivateMatch *priv;
};

#endif
