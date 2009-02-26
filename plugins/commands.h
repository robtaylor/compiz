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

#include <core/core.h>
#include <core/privatehandler.h>

#define COMMANDS_OPTION_COMMAND0              0
#define COMMANDS_OPTION_COMMAND1              1
#define COMMANDS_OPTION_COMMAND2              2
#define COMMANDS_OPTION_COMMAND3              3
#define COMMANDS_OPTION_COMMAND4              4
#define COMMANDS_OPTION_COMMAND5              5
#define COMMANDS_OPTION_COMMAND6              6
#define COMMANDS_OPTION_COMMAND7              7
#define COMMANDS_OPTION_COMMAND8              8
#define COMMANDS_OPTION_COMMAND9              9
#define COMMANDS_OPTION_COMMAND10            10
#define COMMANDS_OPTION_COMMAND11            11
#define COMMANDS_OPTION_RUN_COMMAND0_KEY     12
#define COMMANDS_OPTION_RUN_COMMAND1_KEY     13
#define COMMANDS_OPTION_RUN_COMMAND2_KEY     14
#define COMMANDS_OPTION_RUN_COMMAND3_KEY     15
#define COMMANDS_OPTION_RUN_COMMAND4_KEY     16
#define COMMANDS_OPTION_RUN_COMMAND5_KEY     17
#define COMMANDS_OPTION_RUN_COMMAND6_KEY     18
#define COMMANDS_OPTION_RUN_COMMAND7_KEY     19
#define COMMANDS_OPTION_RUN_COMMAND8_KEY     20
#define COMMANDS_OPTION_RUN_COMMAND9_KEY     21
#define COMMANDS_OPTION_RUN_COMMAND10_KEY    22
#define COMMANDS_OPTION_RUN_COMMAND11_KEY    23
#define COMMANDS_OPTION_RUN_COMMAND0_BUTTON  24
#define COMMANDS_OPTION_RUN_COMMAND1_BUTTON  25
#define COMMANDS_OPTION_RUN_COMMAND2_BUTTON  26
#define COMMANDS_OPTION_RUN_COMMAND3_BUTTON  27
#define COMMANDS_OPTION_RUN_COMMAND4_BUTTON  28
#define COMMANDS_OPTION_RUN_COMMAND5_BUTTON  29
#define COMMANDS_OPTION_RUN_COMMAND6_BUTTON  30
#define COMMANDS_OPTION_RUN_COMMAND7_BUTTON  31
#define COMMANDS_OPTION_RUN_COMMAND8_BUTTON  32
#define COMMANDS_OPTION_RUN_COMMAND9_BUTTON  33
#define COMMANDS_OPTION_RUN_COMMAND10_BUTTON 34
#define COMMANDS_OPTION_RUN_COMMAND11_BUTTON 35
#define COMMANDS_OPTION_RUN_COMMAND0_EDGE    36
#define COMMANDS_OPTION_RUN_COMMAND1_EDGE    37
#define COMMANDS_OPTION_RUN_COMMAND2_EDGE    38
#define COMMANDS_OPTION_RUN_COMMAND3_EDGE    39
#define COMMANDS_OPTION_RUN_COMMAND4_EDGE    40
#define COMMANDS_OPTION_RUN_COMMAND5_EDGE    41
#define COMMANDS_OPTION_RUN_COMMAND6_EDGE    42
#define COMMANDS_OPTION_RUN_COMMAND7_EDGE    43
#define COMMANDS_OPTION_RUN_COMMAND8_EDGE    44
#define COMMANDS_OPTION_RUN_COMMAND9_EDGE    45
#define COMMANDS_OPTION_RUN_COMMAND10_EDGE   46
#define COMMANDS_OPTION_RUN_COMMAND11_EDGE   47
#define COMMANDS_OPTION_NUM                  48

class CommandsScreen :
    public PrivateHandler<CommandsScreen, CompScreen>
{
    public:
	CommandsScreen (CompScreen *s);

	CompOption::Vector& getOptions ();
	bool setOption (const char *name, CompOption::Value& value);

	static bool runCommand (CompAction *action,
				CompAction::State state,
				CompOption::Vector& options,
				int commandOption);

    private:
	CompOption::Vector opt;
};

class CommandsPluginVTable :
    public CompPlugin::VTableForScreen<CommandsScreen>
{
    public:
	bool init ();

	PLUGIN_OPTION_HELPER (CommandsScreen);
};

