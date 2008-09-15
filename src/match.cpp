/*
 * Copyright © 2007 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Novell, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#include <stdlib.h>
#include <string.h>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include <compiz-core.h>

#include <compscreen.h>
#include <compmatch.h>
#include <compwindow.h>
#include <compplugin.h>
#include "privatematch.h"

class CoreExp : public CompMatch::Expression {
    public:
	virtual ~CoreExp () {};

	typedef enum {
	    TypeXid,
	    TypeState,
	    TypeOverride,
	    TypeType
	} Type;
	
	CoreExp (CompString str)
	{
	    if (str.compare (0, 4, "xid=") == 0)
	    {
		mType = TypeXid;
		priv.val = strtol (str.substr (4).c_str (), NULL, 0);
	    }
	    else if (str.compare (0, 6, "state=") == 0)
	    {
		mType = TypeState;
		priv.uval = CompScreen::windowStateFromString
				(str.substr (6).c_str ());
	    }
	    else if (str.compare (0, 18, "override_redirect=") == 0)
	    {
		mType = TypeOverride;
		priv.val = strtol (str.substr (18).c_str (), NULL, 0);
	    }
	    else
	    {
		if (str.compare (0, 5, "type=") == 0)
		    str = str.substr (5);

		mType = TypeType;
		priv.uval = CompWindow::windowTypeFromString (str.c_str ());
	    }
	}

	bool evaluate (CompWindow *w)
	{

	    switch (mType)
	    {
		
		case TypeXid:
		    return ((unsigned int) priv.val == w->id ());
		case TypeState:
		    return (priv.uval & w->state ());
		case TypeOverride:
		{
		    bool overrideRedirect = w->overrideRedirect ();
		    return ((priv.val == 1 && overrideRedirect) ||
			    (priv.val == 0 && !overrideRedirect));
		}
		case TypeType:
		    return (priv.uval & w->wmType ());
	    }
	    return true;
	}

	Type        mType;
	CompPrivate priv;
};

CompMatch::Expression *
CompScreen::matchInitExp (const CompString str)
{
    WRAPABLE_HND_FUNC_RETURN(12, CompMatch::Expression *, matchInitExp, str)

    return new CoreExp (str);
}

static void
matchUpdateMatchOptions (CompOption::Vector options)
{
    foreach (CompOption &option, options)
    {
	switch (option.type ()) {
	    case CompOption::TypeMatch:
		option.value ().match ().update ();
		break;
	    case CompOption::TypeList:
		if (option.value ().listType () == CompOption::TypeMatch)
		{
		    foreach (CompOption::Value &value, option.value ().list ())
			value.match ().update ();
		}
	    default:
		break;
	}
    }
}

void
CompScreen::matchExpHandlerChanged ()
{
    WRAPABLE_HND_FUNC(13, matchExpHandlerChanged)

    foreach (CompPlugin *p, CompPlugin::getPlugins ())
    {
	CompOption::Vector &options = p->vTable->getOptions ();
	matchUpdateMatchOptions (options);
    }
}

void
CompScreen::matchPropertyChanged (CompWindow *w)
{
    WRAPABLE_HND_FUNC(14, matchPropertyChanged, w)
}



static void
matchResetOps (MatchOp::List &list)
{
    MatchExpOp *exp;
    foreach (MatchOp &op, list)
    {
	switch (op.type ()) {
	    case MatchOp::TypeGroup:
		matchResetOps (dynamic_cast <MatchGroupOp &> (op).op);
		break;
	    case MatchOp::TypeExp:
		exp = dynamic_cast <MatchExpOp *> (&op);
		if (exp && exp->e)
		{

		    exp->e.reset ();
		}
		break;
	    default:
		break;
	}
    }
}

static bool
matchOpsEqual (MatchOp::List &list1,
	       MatchOp::List &list2)
{
    MatchOp::List::iterator it1 = list1.begin (), it2 = list2.begin ();

    MatchGroupOp *g1, *g2;
    MatchExpOp *e1, *e2;
    
    if (list1.size () != list2.size ())
	return false;

    while (it1 != list1.end ())
    {
	if ((*it1).type () != (*it2).type ())
	    return false;

	if ((*it1).flags != (*it2).flags)
	    return false;

	switch ((*it1).type ()) {
	    case MatchOp::TypeGroup:
		g1 = dynamic_cast<MatchGroupOp *> (&(*it1));
		g2 = dynamic_cast<MatchGroupOp *> (&(*it2));

		if (!matchOpsEqual (g1->op, g2->op))
		    return false;

		break;
	    case MatchOp::TypeExp:
		e1 = dynamic_cast<MatchExpOp *> (&(*it1));
		e2 = dynamic_cast<MatchExpOp *> (&(*it2));

		if (e1->value != e2->value)
		    return false;

		break;
	    default:
		break;
	}

	it1++;
	it2++;
    }

    return true;
}


static unsigned int
nextIndex (CompString   &str,
	   unsigned int i)
{
    while (str[i] == '\\')
	if (str[++i] != '\0')
	    i++;

    return i;
}


static CompString
strndupValue (CompString str)
{
    CompString value;

    unsigned int i, j, n = str.length ();

    /* count trialing white spaces */
    i = j = 0;
    while (i < n)
    {
	if (str[i] != ' ')
	{
	    j = 0;
	    if (str[i] == '\\')
		i++;
	}
	else
	{
	    j++;
	}

	i++;
    }

    /* remove trialing white spaces */
    n -= j;

    i = j = 0;
    for (;;)
    {
	if (str[i] == '\\')
	    i++;

	value += str[i++];

	if (i >= n)
	{
	    return value;
	}
    }
}

/*
  Add match expressions from string. Special characters are
  '(', ')', '!', '&', '|'. Escape character is '\'.

  Example:

  "type=desktop | !type=dock"
  "!type=dock & (state=fullscreen | state=shaded)"
*/

static void
matchAddFromString (MatchOp::List &list,
		    CompString    str)
{
    CompString value;
    int	 j, i = 0;
    int	 flags = 0;

    str += "\0";

    while (str[i] != '\0')
    {
	while (str[i] == ' ')
	    i++;

	if (str[i] == '!')
	{
	    flags |= MATCH_OP_NOT_MASK;

	    i++;
	    while (str[i] == ' ')
		i++;
	}

	if (str[i] == '(')
	{
	    int	level = 1;
	    int length;

	    j = ++i;

	    while (str[j] != '\0')
	    {
		if (str[j] == '(')
		{
		    level++;
		}
		else if (str[j] == ')')
		{
		    level--;
		    if (level == 0)
			break;
		}

		j = nextIndex (str, ++j);
	    }

	    length = j - i;

	    MatchGroupOp group;
	    matchAddFromString (group.op, str.substr (i, length));
	    group.flags = flags;
	    list.push_back (group);

	    while (str[j] != '\0' && str[j] != '|' && str[j] != '&')
		j++;
	}
	else
	{
	    j = i;

	    while (str[j] != '\0' && str[j] != '|' && str[j] != '&')
		j = nextIndex (str, ++j);

	    if (j > i)
	    {
		MatchExpOp exp;
		exp.value = strndupValue (str.substr(i, j - i));
		exp.flags = flags;
		list.push_back (exp);
	    }
	}

	i = j;

	if (str[i] != '\0')
	{
	    if (str[i] == '&')
		flags = MATCH_OP_AND_MASK;

	    i++;
	}
    }

    if (list.size ())
	list.front ().flags &= ~MATCH_OP_AND_MASK;

}

static CompString
matchOpsToString (MatchOp::List &list)
{
    CompString value (""), group;

    foreach (MatchOp &op, list)
    {
	switch (op.type ()) {
	    case MatchOp::TypeGroup:
		group = matchOpsToString (dynamic_cast <MatchGroupOp &> (op).op);
		if (group.length ())
		{
		    if (value.length ())
		    {
			value += ((op.flags & MATCH_OP_AND_MASK) ?
				  "& " : "| ");
		    }
		    if (op.flags & MATCH_OP_NOT_MASK)
			value += "!";
		    value += "(" + group + ") ";
		}
		break;
	    case MatchOp::TypeExp:

		if (value.length ())
		{
		    value += ((op.flags & MATCH_OP_AND_MASK) ?
			      "& " : "| ");
		}
		if (op.flags & MATCH_OP_NOT_MASK)
		    value += "!";
		    value += dynamic_cast <MatchExpOp &> (op).value;
		break;
	    default:
		break;
	}
    }

    value.erase (value.length () - 1);

    return value;
}

static void
matchUpdateOps (MatchOp::List &list)
{
    MatchExpOp *exp;
    foreach (MatchOp &op, list)
    {
	switch (op.type ()) {
	    case MatchOp::TypeGroup:
		matchUpdateOps (dynamic_cast <MatchGroupOp &> (op).op);
		break;
	    case MatchOp::TypeExp:
		exp = dynamic_cast <MatchExpOp *> (&op);
		if (exp && screen)
		    exp->e.reset (screen->matchInitExp (exp->value));
		break;
	    default:
		break;
	}
    }
}

static bool
matchEvalOps (MatchOp::List &list,
	      CompWindow    *w)
{
    bool       value, result = false;
    MatchExpOp *exp;

    foreach (MatchOp &op, list)
    {
	/* fast evaluation */
	if (op.flags & MATCH_OP_AND_MASK)
	{
	    /* result will never be true */
	    if (!result)
		return false;
	}
	else
	{
	    /* result will always be true */
	    if (result)
		return true;
	}

	switch (op.type ()) {
	    case MatchOp::TypeGroup:
		value = matchEvalOps (dynamic_cast <MatchGroupOp &> (op).op, w);
		break;
	    case MatchOp::TypeExp:
		exp = dynamic_cast <MatchExpOp *> (&op);
		if (exp->e.get ())
		    value = exp->e->evaluate (w);
		else
		    value = true;
		break;
	    default:
		value = true;
		break;
	}

	if (op.flags & MATCH_OP_NOT_MASK)
	    value = !value;

	if (op.flags & MATCH_OP_AND_MASK)
	    result = (result && value);
	else
	    result = (result || value);
    }

    return result;
}

MatchOp::MatchOp () :
    flags (0)
{
}

MatchOp::~MatchOp ()
{
}

MatchExpOp::MatchExpOp () :
    value (""),
    e ()
{
}

MatchGroupOp::MatchGroupOp () :
    op (0)
{
}

PrivateMatch::PrivateMatch () :
    op ()
{
}


CompMatch::CompMatch () :
    priv (new PrivateMatch ())
{
}

CompMatch::CompMatch (const CompString str) :
    priv (new PrivateMatch ())
{
    matchAddFromString (priv->op.op, str);
}

CompMatch::CompMatch (const CompMatch &match) :
    priv (new PrivateMatch ())
{
    priv->op = match.priv->op;
}
	

CompMatch::~CompMatch ()
{
    delete priv;
}

void
CompMatch::update ()
{
    matchResetOps (priv->op.op);
    matchUpdateOps (priv->op.op);
}
	
bool
CompMatch::evaluate (CompWindow *window)
{
    return matchEvalOps (priv->op.op, window);
}

CompString
CompMatch::toString ()
{
    return matchOpsToString (priv->op.op);
}

CompMatch &
CompMatch::operator= (const CompMatch &match)
{
    priv->op = match.priv->op;
    return *this;
}

CompMatch &
CompMatch::operator&= (const CompMatch &match)
{
    MatchGroupOp g1;
    MatchGroupOp g2;

    g1.op = priv->op.op;
    g2.op = match.priv->op.op;
    g2.flags = MATCH_OP_AND_MASK;

    priv->op.op.clear ();
    priv->op.op.push_back (g1);
    priv->op.op.push_back (g2);

    return *this;
}

CompMatch &
CompMatch::operator|= (const CompMatch &match)
{
    MatchGroupOp g1;
    MatchGroupOp g2;

    g1.op = priv->op.op;
    g2.op = match.priv->op.op;

    priv->op.op.clear ();
    priv->op.op.push_back (g1);
    priv->op.op.push_back (g2);

    return *this;
}

const CompMatch &
CompMatch::operator& (const CompMatch &match)
{
    return CompMatch (*this) &= match;
}

const CompMatch &
CompMatch::operator| (const CompMatch &match)
{
    return CompMatch (*this) |= match;
}

const CompMatch &
CompMatch::operator! ()
{
    MatchGroupOp g;
    g.op = priv->op.op;
    g.flags ^= MATCH_OP_NOT_MASK;
    priv->op.op.clear ();
    priv->op.op.push_back (g);
    return *this;
}

CompMatch &
CompMatch::operator= (const CompString &str)
{
    priv->op.op.clear ();
    matchAddFromString (priv->op.op, str);
    return *this;
}

CompMatch &
CompMatch::operator&= (const CompString &str)
{
    return CompMatch (*this) &= CompMatch (str);
}

CompMatch &
CompMatch::operator|= (const CompString &str)
{
    return CompMatch (*this) |= CompMatch (str);
}

const CompMatch &
CompMatch::operator& (const CompString &str)
{
    return CompMatch(*this) &= str;
}

const CompMatch &
CompMatch::operator| (const CompString &str)
{
    return CompMatch(*this) |= str;
}

bool
CompMatch::operator== (const CompMatch &match)
{
    return matchOpsEqual (priv->op.op, match.priv->op.op);
}
