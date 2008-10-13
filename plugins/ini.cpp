/*
 * Copyright © 2008 Danny Baumann
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
 * NO EVENT SHALL DANNY BAUMANN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Danny Baumann <dannybaumann@web.de>
 *
 * based on ini.c by :
 *                       Mike Dransfield <mike@blueroot.co.uk>
 */

#include "ini.h"
#include <boost/lexical_cast.hpp>

static CompMetadata *iniMetadata;

IniFile::IniFile (CompScreen *s, CompPlugin *p) :
    screen (s), plugin (p)
{
}

IniFile::~IniFile ()
{
    if (optionFile.is_open ())
	optionFile.close ();
}

bool
IniFile::open (bool write)
{
    CompString              homeDir;
    std::ios_base::openmode mode;

    homeDir = IniScreen::getHomeDir ();
    if (homeDir.empty ())
	return false;

    filePath =  homeDir;
    if (strcmp (plugin->vTable->name (), "core") == 0)
	filePath += "general";
    else
	filePath += plugin->vTable->name ();
    filePath += FILE_SUFFIX;

    mode = write ? std::ios::out : std::ios::in;
    optionFile.open (filePath.c_str (), mode);
    if (optionFile.fail ())
    {
	mkdir (homeDir.c_str (), 0700);
	optionFile.open (filePath.c_str (), mode);
    }

    return true;
}

void
IniFile::load ()
{
    bool resave = false;

    if (!plugin)
	return;

    if (!open (false))
	return;

    if (optionFile.fail ())
    {
	compLogMessage ("ini", CompLogLevelWarn,
			"Could not open config for plugin %s - using defaults.",
			plugin->vTable->name ());

	save ();

	if (!open (false))
	    return;
    }
    else
    {
	CompOption::Vector& options = plugin->vTable->getOptions ();
	CompString          line, optionValue;
	CompOption          *option;
	unsigned int        pos;

	while (std::getline (optionFile, line))
	{
	    pos = line.find_first_of ('=');
	    if (pos == CompString::npos)
		continue;

	    option = CompOption::findOption (options, line.substr (pos + 1));
	    if (!option)
		continue;

	    optionValue = line.substr (0, pos);
	    if (!stringToOption (option, optionValue))
		resave = true;
	}
    }

    /* re-save whole file if we encountered invalid lines */
    if (resave)
	save ();
}

void
IniFile::save ()
{
    if (!plugin)
	return;

    CompOption::Vector& options = plugin->vTable->getOptions ();
    if (options.empty ())
	return;

    if (!open (true))
    {
	compLogMessage ("ini", CompLogLevelError,
			"Failed to write to config file %s, please "
			"check if you have sufficient permissions.",
			filePath.c_str ());
	return;
    }

    foreach (CompOption& option, options)
    {
	CompString optionValue;
	bool       valid;

	optionValue = optionToString (option, valid);
	if (valid)
	    optionFile << option.name () << "=" << optionValue << std::endl;
    }
}

CompString
IniFile::optionValueToString (CompOption::Value &value,
			      CompOption::Type  type)
{
    CompString retval;

    switch (type) {
    case CompOption::TypeBool:
	retval = value.b () ? "true" : "false";
	break;
    case CompOption::TypeInt:
	retval = boost::lexical_cast<CompString> (value.i ());
	break;
    case CompOption::TypeFloat:
	retval = boost::lexical_cast<CompString> (value.f ());
	break;
    case CompOption::TypeString:
	retval = value.s ();
	break;
    case CompOption::TypeColor:
	retval = CompOption::colorToString (value.c ());
	break;
    case CompOption::TypeKey:
	retval = value.action ().keyToString ();
	break;
    case CompOption::TypeButton:
	retval = value.action ().buttonToString ();
	break;
    case CompOption::TypeEdge:
	retval = value.action ().edgeMaskToString ();
	break;
    case CompOption::TypeBell:
	retval = value.action ().bell () ? "true" : "false";
	break;
    case CompOption::TypeMatch:
	retval = value.match ().toString ();
	break;
    }

    return retval;
}

bool
IniFile::validItemType (CompOption::Type type)
{
    switch (type) {
    case CompOption::TypeBool:
    case CompOption::TypeInt:
    case CompOption::TypeFloat:
    case CompOption::TypeString:
    case CompOption::TypeColor:
    case CompOption::TypeKey:
    case CompOption::TypeButton:
    case CompOption::TypeEdge:
    case CompOption::TypeBell:
    case CompOption::TypeMatch:
	return true;
    default:
	break;
    }

    return false;
}

bool
IniFile::validListItemType (CompOption::Type type)
{
    switch (type) {
    case CompOption::TypeBool:
    case CompOption::TypeInt:
    case CompOption::TypeFloat:
    case CompOption::TypeString:
    case CompOption::TypeColor:
    case CompOption::TypeMatch:
	return true;
    default:
	break;
    }

    return false;
}

CompString
IniFile::optionToString (CompOption &option,
			 bool       &valid)
{
    CompString       retval;
    CompOption::Type type;

    valid = true;
    type  = option.type ();

    if (validItemType (type))
    {
	retval = optionValueToString (option.value (), option.type ());
    }
    else if (type == CompOption::TypeList)
    {
	type = option.value ().listType ();
	if (validListItemType (type))
	{
	    CompOption::Value::Vector& list = option.value ().list ();

	    foreach (CompOption::Value& listOption, list)
	    {
		retval += optionValueToString (listOption, type);
		retval += ",";
	    }

	    /* strip off dangling comma at the end */
	    if (!retval.empty ())
		retval.erase (retval.length () - 1);
	}
	else
	{
	    compLogMessage ("ini", CompLogLevelWarn,
			    "Unknown list option type %d on option %s.",
			    type, option.name ().c_str ());
	    valid = false;
	}
    }
    else
    {
	compLogMessage ("ini", CompLogLevelWarn,
			"Unknown option type %d found on option %s.",
			type, option.name ().c_str ());
	valid = false;
    }

    return retval;
}

bool
IniFile::stringToOptionValue (CompString        &string,
			      CompOption::Type  type,
			      CompOption::Value &value)
{
    bool retval = true;

    switch (type) {
    case CompOption::TypeBool:
	if (string == "true")
	    value.set (true);
	else if (string == "false")
	    value.set (false);
	else
	    retval = false;
	break;
    case CompOption::TypeInt:
	try
	{
	    int intVal;
	    intVal = boost::lexical_cast<int> (string);
	    value.set (intVal);
	}
	catch (boost::bad_lexical_cast)
	{
	    retval = false;
	};
    case CompOption::TypeFloat:
	try
	{
	    float floatVal;
	    floatVal = boost::lexical_cast<float> (string);
	    value.set (floatVal);
	}
	catch (boost::bad_lexical_cast)
	{
	    retval = false;
	};
	break;
    case CompOption::TypeString:
	value.set (string);
	break;
    case CompOption::TypeColor:
	{
	    unsigned short c[4];
	    retval = CompOption::stringToColor (string, c);
	    if (retval)
		value.set (c);
	}
	break;
    case CompOption::TypeKey:
    case CompOption::TypeButton:
    case CompOption::TypeEdge:
    case CompOption::TypeBell:
	{
	    CompAction action;

	    switch (type) {
	    case CompOption::TypeKey:
		action.keyFromString (string);
		retval = (action.type () != CompAction::BindingTypeNone);
		break;
	    case CompOption::TypeButton:
		action.buttonFromString (string);
		retval = (action.type () != CompAction::BindingTypeNone);
		break;
	    case CompOption::TypeEdge:
		action.edgeMaskFromString (string);
		break;
	    case CompOption::TypeBell:
		if (string == "true")
		    action.setBell (true);
		else if (string == "false")
		    action.setBell (false);
		else
		    retval = false;
	    }

	    if (retval)
		value.set (action);
	}
	break;
    case CompOption::TypeMatch:
	{
	    CompMatch match (string);
	    value.set (match);
	}
	break;
    }

    return retval;
}

bool
IniFile::stringToOption (CompOption *option,
			 CompString &valueString)
{
    CompOption::Value value;
    bool              valid = false;
    CompOption::Type  type = option->type ();

    if (validItemType (type))
    {
	valid = stringToOptionValue (valueString, option->type (), value);
    }
    else if (type == CompOption::TypeList)
    {
	type = option->value ().listType ();
	if (validListItemType (type))
	{
	    CompString        listItem;
	    unsigned int      delim, pos = 0;
	    CompOption::Value item;

	    do
	    {
		delim = valueString.find_first_of (',', pos);

		if (delim != CompString::npos)
		    listItem = valueString.substr (pos, delim - pos);
		else
		    listItem = valueString.substr (pos);

		valid = stringToOptionValue (listItem, type, item);
		if (valid)
		    value.list ().push_back (item);

		pos = delim + 1;
	    }
	    while (delim != CompString::npos);

	    valid = true;
	}
    }

    if (valid)
	screen->setOptionForPlugin (plugin->vTable->name (),
				    option->name ().c_str (), value);

    return valid;
}

void
IniScreen::fileChanged (const char *name)
{
    CompString   fileName (name);
    unsigned int length;
    CompString   plugin;
    CompPlugin   *p;

    if (fileName.length () <= strlen (FILE_SUFFIX))
	return;

    length = fileName.length () - strlen (FILE_SUFFIX);
    if (strcmp (fileName.c_str () + length, FILE_SUFFIX) != 0)
	return;

    p = CompPlugin::find (fileName.substr (0, length).c_str ());
    if (p)
    {
	IniFile ini (screen, p);
	ini.load ();
    }
}

CompString
IniScreen::getHomeDir ()
{
    char       *home = getenv("HOME");
    CompString retval;

    if (home)
    {
	retval += home;
	retval += "/";
	retval += HOME_OPTIONDIR;
	retval += "/";
    }

    return retval;
}

bool
IniScreen::setOptionForPlugin (const char        *plugin,
			       const char        *name,
			       CompOption::Value &v)
{
    bool status = screen->setOptionForPlugin (plugin, name, v);

    if (status && !blockWrites)
    {
	CompPlugin *p;

	p = CompPlugin::find (plugin);
	if (p)
	{
	    IniFile ini (screen, p);
	    ini.save ();
	}
    }

    return status;
}

bool
IniScreen::initPluginForScreen (CompPlugin *p)
{
    bool status = screen->initPluginForScreen (p);

    if (status)
    {
	IniFile ini (screen, p);

	blockWrites = true;
	ini.load ();
	blockWrites = false;
    }

    return status;
}

IniScreen::IniScreen (CompScreen *screen) :
    PrivateHandler<IniScreen, CompScreen> (screen)
{
    CompString homeDir;
    int        mask;

    homeDir = getHomeDir ();
    if (homeDir.empty ())
    {
	setFailed ();
	return;
    }

    mask = NOTIFY_CREATE_MASK | NOTIFY_DELETE_MASK | NOTIFY_MODIFY_MASK;

    directoryWatchHandle =
	screen->addFileWatch (homeDir.c_str (), mask,
			      boost::bind (&IniScreen::fileChanged, this, _1));

    ScreenInterface::setHandler (screen, true);

    /* FIXME: timer? */
    IniFile ini (screen, CompPlugin::find ("core"));

    blockWrites = true;
    ini.load ();
    blockWrites = false;
}

IniScreen::~IniScreen ()
{
    if (directoryWatchHandle)
	screen->removeFileWatch (directoryWatchHandle);
}

bool
IniPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;

    iniMetadata = new CompMetadata (name ());
    if (!iniMetadata)
	return false;

    iniMetadata->addFromFile (name ());

    return true;
}

void
IniPluginVTable::fini ()
{
    delete iniMetadata;
}

CompMetadata *
IniPluginVTable::getMetadata ()
{
    return iniMetadata;
}

IniPluginVTable iniVTable;

CompPlugin::VTable *
getCompPluginInfo20080805 (void)
{
    return &iniVTable;
}
