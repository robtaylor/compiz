#ifndef _PRIVATEACTION_H
#define _PRIVATEACTION_H

class PrivateAction {
    public:
	PrivateAction ();
	PrivateAction (const PrivateAction&);
	
	CompAction::CallBack initiate;
	CompAction::CallBack terminate;

	CompAction::State state;

	CompAction::BindingType   type;

	CompAction::KeyBinding    key;
	CompAction::ButtonBinding button;

	bool bell;

	unsigned int edgeMask;

	CompPrivate priv;
};

#endif
