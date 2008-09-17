#include <core/screen.h>
#include <core/window.h>
#include <core/atoms.h>
#include "privatescreen.h"
#include "privatewindow.h"

bool
CompScreen::closeWin (CompAction         *action,
		      CompAction::State  state,
		      CompOption::Vector &options)
{
    CompWindow   *w;
    Window       xid;
    unsigned int time;

    xid  = CompOption::getIntOptionNamed (options, "window");
    time = CompOption::getIntOptionNamed (options, "time", CurrentTime);

    w = screen->findTopLevelWindow (xid);
    if (w && (w->priv->actions  & CompWindowActionCloseMask))
	w->close (time);

    return true;
}

bool
CompScreen::mainMenu (CompAction         *action,
		      CompAction::State  state,
		      CompOption::Vector &options)
{
    unsigned int time;

    time = CompOption::getIntOptionNamed (options, "time", CurrentTime);

    if (screen->priv->grabs.empty ())
	screen->toolkitAction (Atoms::toolkitActionMainMenu, time,
			       screen->priv->root, 0, 0, 0);

    return true;
}

bool
CompScreen::runDialog (CompAction         *action,
		       CompAction::State  state,
		       CompOption::Vector &options)
{
    unsigned int time;

    time = CompOption::getIntOptionNamed (options, "time", CurrentTime);

    if (screen->priv->grabs.empty ())
	screen->toolkitAction (Atoms::toolkitActionRunDialog, time,
			       screen->priv->root , 0, 0, 0);

    return true;
}

bool
CompScreen::unmaximizeWin (CompAction         *action,
			   CompAction::State  state,
			   CompOption::Vector &options)
{
    CompWindow *w;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "window");

    w = screen->findTopLevelWindow (xid);
    if (w)
	w->maximize (0);

    return true;
}

bool
CompScreen::minimizeWin (CompAction         *action,
			 CompAction::State  state,
			 CompOption::Vector &options)
{
    CompWindow *w;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "window");

    w = screen->findTopLevelWindow (xid);
    if (w && (w->actions () & CompWindowActionMinimizeMask))
	w->minimize ();

    return true;
}

bool
CompScreen::maximizeWin (CompAction         *action,
			 CompAction::State  state,
			 CompOption::Vector &options)
{
    CompWindow *w;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "window");

    w = screen->findTopLevelWindow (xid);
    if (w)
	w->maximize (MAXIMIZE_STATE);

    return true;
}

bool
CompScreen::maximizeWinHorizontally (CompAction         *action,
				     CompAction::State  state,
				     CompOption::Vector &options)
{
    CompWindow *w;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "window");

    w = screen->findTopLevelWindow (xid);
    if (w)
	w->maximize (w->state () | CompWindowStateMaximizedHorzMask);

    return true;
}

bool
CompScreen::maximizeWinVertically (CompAction         *action,
				   CompAction::State  state,
				   CompOption::Vector &options)
{
    CompWindow *w;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "window");

    w = screen->findTopLevelWindow (xid);
    if (w)
	w->maximize (w->state () | CompWindowStateMaximizedVertMask);

    return true;
}

bool
CompScreen::showDesktop (CompAction         *action,
			 CompAction::State  state,
			 CompOption::Vector &options)
{
    if (screen->priv->showingDesktopMask == 0)
	screen->enterShowDesktopMode ();
    else
	screen->leaveShowDesktopMode (NULL);

    return true;
}

bool
CompScreen::raiseWin (CompAction         *action,
		      CompAction::State  state,
		      CompOption::Vector &options)
{
    CompWindow *w;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "window");

    w = screen->findTopLevelWindow (xid);
    if (w)
	w->raise ();

    return true;
}

bool
CompScreen::lowerWin (CompAction         *action,
		      CompAction::State  state,
		      CompOption::Vector &options)
{
    CompWindow *w;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "window");

    w = screen->findTopLevelWindow (xid);
    if (w)
	w->lower ();

    return true;
}

bool
CompScreen::runCommandDispatch (CompAction         *action,
				CompAction::State  state,
				CompOption::Vector &options)
{
    int index = -1;
    int i = COMP_OPTION_RUN_COMMAND0_KEY;

    while (i <= COMP_OPTION_RUN_COMMAND11_KEY)
    {
	if (action == &screen->priv->opt[i].value ().action ())
	{
	    index = i - COMP_OPTION_RUN_COMMAND0_KEY +
		COMP_OPTION_COMMAND0;
	    break;
	}

	i++;
    }

    if (index > 0)
	screen->runCommand (screen->priv->opt[index].value ().s ());

    return true;
}

bool
CompScreen::runCommandScreenshot (CompAction         *action,
				  CompAction::State  state,
				  CompOption::Vector &options)
{
    screen->runCommand (
	screen->priv->opt[COMP_OPTION_SCREENSHOT].value ().s ());

    return true;
}

bool
CompScreen::runCommandWindowScreenshot (CompAction         *action,
					CompAction::State  state,
					CompOption::Vector &options)
{
    screen->runCommand (
	screen->priv->opt[COMP_OPTION_WINDOW_SCREENSHOT].value ().s ());

    return true;
}

bool
CompScreen::runCommandTerminal (CompAction         *action,
				CompAction::State  state,
				CompOption::Vector &options)
{
    screen->runCommand (
	screen->priv->opt[COMP_OPTION_TERMINAL].value ().s ());

    return true;
}

bool
CompScreen::windowMenu (CompAction         *action,
			CompAction::State  state,
			CompOption::Vector &options)
{
    CompWindow *w;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "window");

    w = screen->findTopLevelWindow (xid);
    if (w && screen->priv->grabs.empty ())
    {
	int  x, y, button;
	Time time;

	time   = CompOption::getIntOptionNamed (options, "time", CurrentTime);
	button = CompOption::getIntOptionNamed (options, "button", 0);
	x      = CompOption::getIntOptionNamed (options, "x",
						w->geometry ().x ());
	y      = CompOption::getIntOptionNamed (options, "y",
						w->geometry ().y ());

	screen->toolkitAction (Atoms::toolkitActionWindowMenu,
			       time, w->id (), button, x, y);
    }

    return true;
}

bool
CompScreen::toggleWinMaximized (CompAction         *action,
				CompAction::State  state,
				CompOption::Vector &options)
{
    CompWindow *w;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "window");

    w = screen->findTopLevelWindow (xid);
    if (w)
    {
	if ((w->priv->state & MAXIMIZE_STATE) == MAXIMIZE_STATE)
	    w->maximize (0);
	else
	    w->maximize (MAXIMIZE_STATE);
    }

    return true;
}

bool
CompScreen::toggleWinMaximizedHorizontally (CompAction         *action,
					    CompAction::State  state,
					    CompOption::Vector &options)
{
    CompWindow *w;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "window");

    w = screen->findTopLevelWindow (xid);
    if (w)
	w->maximize (w->priv->state ^ CompWindowStateMaximizedHorzMask);

    return true;
}

bool
CompScreen::toggleWinMaximizedVertically (CompAction         *action,
					  CompAction::State  state,
					  CompOption::Vector &options)
{
    CompWindow *w;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "window");

    w = screen->findTopLevelWindow (xid);
    if (w)
	w->maximize (w->priv->state ^ CompWindowStateMaximizedVertMask);

    return true;
}

bool
CompScreen::shadeWin (CompAction         *action,
		      CompAction::State  state,
		      CompOption::Vector &options)
{
    CompWindow *w;
    Window     xid;

    xid = CompOption::getIntOptionNamed (options, "window");

    w = screen->findTopLevelWindow (xid);
    if (w && (w->priv->actions & CompWindowActionShadeMask))
    {
	w->priv->state ^= CompWindowStateShadedMask;
	w->updateAttributes (CompStackingUpdateModeNone);
    }

    return true;
}
