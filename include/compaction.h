#ifndef _COMPACTION_H
#define _COMPACTION_H

#include <boost/function.hpp>

#include <X11/Xlib-xcb.h>

#include <compoption.h>

class PrivateAction;

#define CompModAlt        0
#define CompModMeta       1
#define CompModSuper      2
#define CompModHyper      3
#define CompModModeSwitch 4
#define CompModNumLock    5
#define CompModScrollLock 6
#define CompModNum        7

#define CompAltMask        (1 << 16)
#define CompMetaMask       (1 << 17)
#define CompSuperMask      (1 << 18)
#define CompHyperMask      (1 << 19)
#define CompModeSwitchMask (1 << 20)
#define CompNumLockMask    (1 << 21)
#define CompScrollLockMask (1 << 22)

#define CompNoMask         (1 << 25)

class CompAction {
    public:
	typedef enum {
	    StateInitKey     = 1 <<  0,
	    StateTermKey     = 1 <<  1,
	    StateInitButton  = 1 <<  2,
	    StateTermButton  = 1 <<  3,
	    StateInitBell    = 1 <<  4,
	    StateInitEdge    = 1 <<  5,
	    StateTermEdge    = 1 <<  6,
	    StateInitEdgeDnd = 1 <<  7,
	    StateTermEdgeDnd = 1 <<  8,
	    StateCommit      = 1 <<  9,
	    StateCancel      = 1 << 10,
	    StateAutoGrab    = 1 << 11,
	    StateNoEdgeDelay = 1 << 12
	} StateEnum;

	typedef enum {
	    BindingTypeNone       = 0,
	    BindingTypeKey        = 1 << 0,
	    BindingTypeButton     = 1 << 1,
	    BindingTypeEdgeButton = 1 << 2
	} BindingTypeEnum;

	class KeyBinding {
	    public:
		KeyBinding ();
		KeyBinding (const KeyBinding&);

		unsigned int modifiers ();
		int keycode ();

		bool fromString (CompDisplay *d, const CompString str);
		CompString toString (CompDisplay *d);

	    private:
		unsigned int mModifiers;
		int          mKeycode;
	};

	class ButtonBinding {
	    public:
		ButtonBinding ();
		ButtonBinding (const ButtonBinding&);

		unsigned int modifiers ();
		int button ();

		bool fromString (CompDisplay *d, const CompString str);
		CompString toString (CompDisplay *d);

	    private:
		unsigned int mModifiers;
		int          mButton;
	};

	typedef unsigned int State;
	typedef unsigned int BindingType;
	typedef boost::function <bool (CompDisplay *, CompAction *, State, CompOption::Vector &)> CallBack;

    public:
	CompAction ();
	CompAction (const CompAction &);
	~CompAction ();

	CallBack initiate ();
	CallBack terminate ();

	void setInitiate (const CallBack &initiate);
	void setTerminate (const CallBack &terminate);

	State state ();
	BindingType type ();

	KeyBinding & key ();
	ButtonBinding & button ();

	unsigned int edgeMask ();
	void setEdgeMask (unsigned int edge);

	bool bell ();
	void setBell (bool bell);

	void setState (State state);

	void copyState (const CompAction &action);

	bool operator== (const CompAction& val);
	CompAction & operator= (const CompAction &action);

	void keyFromString (CompDisplay *d, const CompString str);
	void buttonFromString (CompDisplay *d, const CompString str);
	void edgeMaskFromString (const CompString str);

	CompString keyToString (CompDisplay *d);
	CompString buttonToString (CompDisplay *d);
	CompString edgeMaskToString ();

	static CompString edgeToString (unsigned int edge);

    private:
	PrivateAction *priv;
};

#endif
