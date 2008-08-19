/*
 * Copyright © 2005 Novell, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <compiz.h>
#include <compiz-core.h>

#include <compaction.h>
#include <privateaction.h>

struct _Modifier {
    const char *name;
    int        modifier;
} modifiers[] = {
    { "<Shift>",      ShiftMask		 },
    { "<Control>",    ControlMask	 },
    { "<Mod1>",	      Mod1Mask		 },
    { "<Mod2>",	      Mod2Mask		 },
    { "<Mod3>",	      Mod3Mask		 },
    { "<Mod4>",	      Mod4Mask		 },
    { "<Mod5>",	      Mod5Mask		 },
    { "<Alt>",	      CompAltMask        },
    { "<Meta>",	      CompMetaMask       },
    { "<Super>",      CompSuperMask      },
    { "<Hyper>",      CompHyperMask	 },
    { "<ModeSwitch>", CompModeSwitchMask }
};

#define N_MODIFIERS (sizeof (modifiers) / sizeof (struct _Modifier))

struct _Edge {
    const char *name;
    const char *modifierName;
} edges[] = {
    { "Left",	     "<LeftEdge>"	 },
    { "Right",	     "<RightEdge>"	 },
    { "Top",	     "<TopEdge>"	 },
    { "Bottom",	     "<BottomEdge>"	 },
    { "TopLeft",     "<TopLeftEdge>"	 },
    { "TopRight",    "<TopRightEdge>"	 },
    { "BottomLeft",  "<BottomLeftEdge>"	 },
    { "BottomRight", "<BottomRightEdge>" }
};

static CompString
modifiersToString (CompDisplay  *d,
		   unsigned int modMask)
{
    CompString binding = "";

    for (unsigned int i = 0; i < N_MODIFIERS; i++)
    {
	if (modMask & modifiers[i].modifier)
	    binding += modifiers[i].name;
    }

    return binding;
}

static unsigned int
stringToModifiers (CompDisplay *d,
		   CompString  str)
{
    unsigned int mods = 0;

    for (unsigned int i = 0; i < N_MODIFIERS; i++)
    {
	if (str.find (modifiers[i].name) != std::string::npos)
	    mods |= modifiers[i].modifier;
    }

    return mods;
}

static unsigned int
bindingStringToEdgeMask (CompDisplay *d,
			 CompString  str)
{
    unsigned int edgeMask = 0;

    for (int i = 0; i < SCREEN_EDGE_NUM; i++)
	if (str.find (edges[i].modifierName) != std::string::npos)
	    edgeMask |= 1 << i;

    return edgeMask;
}

static CompString
edgeMaskToBindingString (CompDisplay  *d,
			 unsigned int edgeMask)
{
    CompString binding = "";
    int  i;

    for (i = 0; i < SCREEN_EDGE_NUM; i++)
	if (edgeMask & (1 << i))
	    binding += edges[i].modifierName;

    return binding;
}

CompAction::KeyBinding::KeyBinding () :
    mModifiers (0),
    mKeycode (0)
{
}

CompAction::KeyBinding::KeyBinding (const KeyBinding& k) :
    mModifiers (k.mModifiers),
    mKeycode (k.mKeycode)
{
}

unsigned int
CompAction::KeyBinding::modifiers ()
{
    return mModifiers;
}

int
CompAction::KeyBinding::keycode ()
{
    return mKeycode;
}

bool
CompAction::KeyBinding::fromString (CompDisplay *d, const CompString str)
{
    CompString   sStr;
    unsigned int mods, pos;
    KeySym	 keysym;

    mods = stringToModifiers (d, str);

    pos = str.rfind ('>');
    if (pos != std::string::npos)
	pos++;

    while (pos < str.size () && !isalnum (str[pos]))
	pos++;

    if (pos == std::string::npos || pos == str.size ())
    {
	if (mods)
	{
	    mKeycode   = 0;
	    mModifiers = mods;

	    return true;
	}

	return false;
    }

    sStr = str.substr (pos);
    keysym = XStringToKeysym (sStr.c_str ());
    if (keysym != NoSymbol)
    {
	KeyCode keycode;

	keycode = XKeysymToKeycode (d->dpy (), keysym);
	if (keycode)
	{
	    mKeycode   = keycode;
	    mModifiers = mods;

	    return true;
	}
    }

    if (sStr.compare (0, 2, "0x") == 0)
    {
	mKeycode   = strtol (sStr.c_str (), NULL, 0);
	mModifiers = mods;

	return true;
    }

    return false;
}

CompString
CompAction::KeyBinding::toString (CompDisplay *d)
{
    CompString binding = "";

    binding = modifiersToString (d, mModifiers);

    if (mKeycode != 0)
    {
	KeySym keysym;
	char   *keyname;

	keysym  = XKeycodeToKeysym (d->dpy (), mKeycode, 0);
	keyname = XKeysymToString (keysym);

	if (keyname)
	{
	    binding += keyname;
	}
	else
	{
	    binding += compPrintf ("0x%x", mKeycode);
	}
    }

    return binding;
}

CompAction::ButtonBinding::ButtonBinding () :
    mModifiers (0),
    mButton (0)
{
}

CompAction::ButtonBinding::ButtonBinding (const ButtonBinding& b) :
    mModifiers (b.mModifiers),
    mButton (b.mButton)
{
}

unsigned int
CompAction::ButtonBinding::modifiers ()
{
    return mModifiers;
}

int
CompAction::ButtonBinding::button ()
{
    return mButton;
}

bool
CompAction::ButtonBinding::fromString (CompDisplay *d, const CompString str)
{
    unsigned int mods, pos;

    mods = stringToModifiers (d, str);

    pos = str.rfind ('>');
    if (pos != std::string::npos)
	pos++;

    while (pos < str.size () && !isalnum (str[pos]))
	pos++;

    if (pos != std::string::npos && pos != str.size () &&
        str.compare (pos, 6, "Button") == 0)
    {
	int buttonNum;

	if (sscanf (str.substr (pos + 6).c_str (), "%d", &buttonNum) == 1)
	{
	    mButton    = buttonNum;
	    mModifiers = mods;

	    return true;
	}
    }

    return false;
}

CompString
CompAction::ButtonBinding::toString (CompDisplay *d)
{
    CompString binding;

    if (!mModifiers && !mButton)
	return "";

    binding = modifiersToString (d, mModifiers);
    binding += compPrintf ("Button%d", mButton);

    return binding;
}

CompAction::CompAction () :
    priv (new PrivateAction ())
{
}

CompAction::CompAction (const CompAction & a) :
    priv (new PrivateAction (*a.priv))
{
}

CompAction::~CompAction ()
{
    delete priv;
}

CompAction::CallBack
CompAction::initiate ()
{
    return priv->initiate;
}

CompAction::CallBack
CompAction::terminate ()
{
    return priv->terminate;
}

void
CompAction::setInitiate (const CompAction::CallBack &initiate)
{
    priv->initiate = initiate;
}

void
CompAction::setTerminate (const CompAction::CallBack &terminate)
{
    priv->terminate = terminate;
}

CompAction::State
CompAction::state ()
{
    return priv->state;
}

CompAction::BindingType
CompAction::type ()
{
    return priv->type;
}

CompAction::KeyBinding &
CompAction::key ()
{
    return priv->key;
}

CompAction::ButtonBinding &
CompAction::button ()
{
    return priv->button;
}

unsigned int
CompAction::edgeMask ()
{
    return priv->edgeMask;
}

void
CompAction::setEdgeMask (unsigned int edge)
{
    priv->edgeMask = edge;
}

bool
CompAction::bell ()
{
    return priv->bell;
}

void
CompAction::setBell (bool bell)
{
    priv->bell = bell;
}

void
CompAction::setState (CompAction::State state)
{
    priv->state = state;
}

void
CompAction::copyState (const CompAction &action)
{
    priv->initiate = action.priv->initiate;
    priv->terminate = action.priv->terminate;
    priv->state = action.priv->state;
    memcpy (&priv->priv, &action.priv->priv, sizeof (CompPrivate));
}

bool
CompAction::operator== (const CompAction& val)
{
    if (priv->state != val.priv->state)
	return false;
    if (priv->type != val.priv->type)
	return false;
    if (priv->key.modifiers () != val.priv->key.modifiers ())
	return false;
    if (priv->key.keycode () != val.priv->key.keycode ())
	return false;
    if (priv->button.modifiers () != val.priv->button.modifiers ())
	return false;
    if (priv->button.button () != val.priv->button.button ())
	return false;
    if (priv->bell != val.priv->bell)
	return false;
    if (priv->edgeMask != val.priv->edgeMask)
	return false;
    if (memcmp (&priv->priv, &val.priv->priv, sizeof (CompPrivate)) != 0)
	return false;

    return true;
}

CompAction &
CompAction::operator= (const CompAction &action)
{
    delete priv;
    priv = new PrivateAction (*action.priv);
    return *this;
}

void
CompAction::keyFromString (CompDisplay *d, const CompString str)
{
    if (priv->key.fromString (d, str))
	priv->type = CompAction::BindingTypeKey;
    else
	priv->type = CompAction::BindingTypeNone;
}

void
CompAction::buttonFromString (CompDisplay *d, const CompString str)
{
    if (priv->button.fromString (d, str))
    {
	priv->edgeMask = bindingStringToEdgeMask (d, str);
	if (priv->edgeMask)
	    priv->type = CompAction::BindingTypeEdgeButton;
	else
	    priv->type = CompAction::BindingTypeButton;
    }
    else
    {
	priv->type = CompAction::BindingTypeNone;
    }
}

void
CompAction::edgeMaskFromString (const CompString str)
{
    unsigned int edgeMask = 0, pos;

    for (int i = 0; i < SCREEN_EDGE_NUM; i++)
    {
	pos = 0;
	while ((pos = str.find (edgeToString (i), pos)) != std::string::npos)
	{
	    if (pos > 0 && isalnum (str[pos - 1]))
	    {
		pos++;
		continue;
	    }

	    pos += edgeToString (i).size ();

	    if (pos < str.size () && isalnum (str[pos]))
		continue;

	    edgeMask |= 1 << i;
	}
    }

    priv->edgeMask = edgeMask;
}

CompString
CompAction::keyToString (CompDisplay *d)
{
    CompString binding;

    binding = priv->key.toString (d);
    if (binding.size () == 0)
	return "Disabled";

    return binding;
}

CompString
CompAction::buttonToString (CompDisplay *d)
{
    CompString binding = "", edge = "";

    binding = modifiersToString (d, priv->button.modifiers ());
    binding += edgeMaskToBindingString (d, priv->edgeMask);

    binding += compPrintf ("Button%d", priv->button.button ());

    if (priv->button.button () == 0)
	return "Disabled";

    return binding;
}

CompString
CompAction::edgeMaskToString ()
{
    CompString edge = "";

    for (int i = 0; i < SCREEN_EDGE_NUM; i++)
    {
	if (priv->edgeMask & (1 << i))
	{
	    edge += " | ";

	    edge += edgeToString (i);
	}
    }

    return edge;
}


CompString
CompAction::edgeToString (unsigned int edge)
{
    return CompString (edges[edge].name);
}

PrivateAction::PrivateAction () :
    initiate (),
    terminate (),
    state (0),
    type (0),
    key (),
    button (),
    bell (false),
    edgeMask (0)
{
    memset (&priv, 0, sizeof (CompPrivate));
}

PrivateAction::PrivateAction (const PrivateAction& a) :
    initiate (a.initiate),
    terminate (a.terminate),
    state (a.state),
    type (a.type),
    key (a.key),
    button (a.button),
    bell (a.bell),
    edgeMask (a.edgeMask)
{
    memcpy (&priv, &a.priv, sizeof (CompPrivate));
}
