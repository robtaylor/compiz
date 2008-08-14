#ifndef _PRIVATEMATCH_H
#define _PRIVATEMATCH_H

#include <compmatch.h>
#include <boost/shared_ptr.hpp>

#define MATCH_OP_AND_MASK (1 << 0)
#define MATCH_OP_NOT_MASK (1 << 1)

class MatchOp {
    public:
	typedef enum {
	    TypeNone,
	    TypeGroup,
	    TypeExp
	} Type;

	typedef std::list<MatchOp> List;

	MatchOp ();
	virtual ~MatchOp ();

	virtual Type type () { return TypeNone; };

	unsigned int flags;
};

class MatchExpOp : public MatchOp {
    public:
	MatchExpOp ();

	MatchOp::Type type () { return MatchOp::TypeExp; };

	CompString	      value;

    	boost::shared_ptr<CompMatch::Expression> e;
};

class MatchGroupOp : public MatchOp {
    public:
	MatchGroupOp ();

	MatchOp::Type type () { return MatchOp::TypeGroup; };

	MatchOp::List op;
};

class PrivateMatch {
    public:
	PrivateMatch ();

    public:
	MatchGroupOp op;
	CompDisplay  *display;
};

#endif
