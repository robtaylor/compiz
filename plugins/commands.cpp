/*
 * Copyright © 2009 Danny Baumann
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Danny Baumann not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Danny Baumann makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DANNY BAUMANN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DENNIS KASPRZYK BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Danny Baumann <dannybaumann@web.de>
 */

#include "commands.h"

COMPIZ_PLUGIN_20081216 (commands, CommandsPluginVTable);

bool
CommandsScreen::runCommand (CompAction          *action,
			    CompAction::State   state,
			    CompOption::Vector& options,
			    int                 commandOption)
{
    CommandsScreen *cs;
    Window         xid;

    xid = CompOption::getIntOptionNamed (options, "root", 0);
    if (xid != screen->root ())
	return false;

    cs = CommandsScreen::get (screen);

    screen->runCommand (cs->opt[commandOption].value (). s());

    return true;
}

#define DISPATCH(opt) boost::bind (CommandsScreen::runCommand, _1, _2, _3, opt)

static const CompMetadata::OptionInfo commandsOptionInfo[] = {
    { "command0", "string", 0, 0, 0 },
    { "command1", "string", 0, 0, 0 },
    { "command2", "string", 0, 0, 0 },
    { "command3", "string", 0, 0, 0 },
    { "command4", "string", 0, 0, 0 },
    { "command5", "string", 0, 0, 0 },
    { "command6", "string", 0, 0, 0 },
    { "command7", "string", 0, 0, 0 },
    { "command8", "string", 0, 0, 0 },
    { "command9", "string", 0, 0, 0 },
    { "command10", "string", 0, 0, 0 },
    { "command11", "string", 0, 0, 0 },
    { "run_command0_key", "key", 0, DISPATCH (COMMANDS_OPTION_COMMAND0), 0 },
    { "run_command1_key", "key", 0, DISPATCH (COMMANDS_OPTION_COMMAND1), 0 },
    { "run_command2_key", "key", 0, DISPATCH (COMMANDS_OPTION_COMMAND2), 0 },
    { "run_command3_key", "key", 0, DISPATCH (COMMANDS_OPTION_COMMAND3), 0 },
    { "run_command4_key", "key", 0, DISPATCH (COMMANDS_OPTION_COMMAND4), 0 },
    { "run_command5_key", "key", 0, DISPATCH (COMMANDS_OPTION_COMMAND5), 0 },
    { "run_command6_key", "key", 0, DISPATCH (COMMANDS_OPTION_COMMAND6), 0 },
    { "run_command7_key", "key", 0, DISPATCH (COMMANDS_OPTION_COMMAND7), 0 },
    { "run_command8_key", "key", 0, DISPATCH (COMMANDS_OPTION_COMMAND8), 0 },
    { "run_command9_key", "key", 0, DISPATCH (COMMANDS_OPTION_COMMAND9), 0 },
    { "run_command10_key", "key", 0, DISPATCH (COMMANDS_OPTION_COMMAND10), 0 },
    { "run_command11_key", "key", 0, DISPATCH (COMMANDS_OPTION_COMMAND11), 0 },
    { "run_command0_button", "button", 0,
      DISPATCH (COMMANDS_OPTION_COMMAND0), 0 },
    { "run_command1_button", "button", 0,
      DISPATCH (COMMANDS_OPTION_COMMAND1), 0 },
    { "run_command2_button", "button", 0,
      DISPATCH (COMMANDS_OPTION_COMMAND2), 0 },
    { "run_command3_button", "button", 0,
      DISPATCH (COMMANDS_OPTION_COMMAND3), 0 },
    { "run_command4_button", "button", 0,
      DISPATCH (COMMANDS_OPTION_COMMAND4), 0 },
    { "run_command5_button", "button", 0,
      DISPATCH (COMMANDS_OPTION_COMMAND5), 0 },
    { "run_command6_button", "button", 0,
      DISPATCH (COMMANDS_OPTION_COMMAND6), 0 },
    { "run_command7_button", "button", 0,
      DISPATCH (COMMANDS_OPTION_COMMAND7), 0 },
    { "run_command8_button", "button", 0,
      DISPATCH (COMMANDS_OPTION_COMMAND8), 0 },
    { "run_command9_button", "button", 0,
      DISPATCH (COMMANDS_OPTION_COMMAND9), 0 },
    { "run_command10_button", "button", 0,
      DISPATCH (COMMANDS_OPTION_COMMAND10), 0 },
    { "run_command11_button", "button", 0,
      DISPATCH (COMMANDS_OPTION_COMMAND11), 0 },
    { "run_command0_edge", "edge", 0, DISPATCH (COMMANDS_OPTION_COMMAND0), 0 },
    { "run_command1_edge", "edge", 0, DISPATCH (COMMANDS_OPTION_COMMAND1), 0 },
    { "run_command2_edge", "edge", 0, DISPATCH (COMMANDS_OPTION_COMMAND2), 0 },
    { "run_command3_edge", "edge", 0, DISPATCH (COMMANDS_OPTION_COMMAND3), 0 },
    { "run_command4_edge", "edge", 0, DISPATCH (COMMANDS_OPTION_COMMAND4), 0 },
    { "run_command5_edge", "edge", 0, DISPATCH (COMMANDS_OPTION_COMMAND5), 0 },
    { "run_command6_edge", "edge", 0, DISPATCH (COMMANDS_OPTION_COMMAND6), 0 },
    { "run_command7_edge", "edge", 0, DISPATCH (COMMANDS_OPTION_COMMAND7), 0 },
    { "run_command8_edge", "edge", 0, DISPATCH (COMMANDS_OPTION_COMMAND8), 0 },
    { "run_command9_edge", "edge", 0, DISPATCH (COMMANDS_OPTION_COMMAND9), 0 },
    { "run_command10_edge", "edge", 0,
      DISPATCH (COMMANDS_OPTION_COMMAND10), 0 },
    { "run_command11_edge", "edge", 0,
      DISPATCH (COMMANDS_OPTION_COMMAND11), 0 }
};

CommandsScreen::CommandsScreen (CompScreen *s) :
    PrivateHandler<CommandsScreen, CompScreen> (s)
{
    if (!commandsVTable->getMetadata ()->initOptions (commandsOptionInfo,
						      COMMANDS_OPTION_NUM,
						      opt))
	setFailed ();
}

CompOption::Vector&
CommandsScreen::getOptions ()
{
    return opt;
}

bool
CommandsScreen::setOption (const char         *name,
			   CompOption::Value& value)
{
    CompOption *o;

    o = CompOption::findOption (opt, name, NULL);
    if (!o)
	return false;

    return CompOption::setOption (*o, value);
}

bool
CommandsPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;

    getMetadata ()->addFromOptionInfo (commandsOptionInfo, COMMANDS_OPTION_NUM);
    getMetadata ()->addFromFile (name ());

    return true;
}

