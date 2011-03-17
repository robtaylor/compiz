/**
 * Compiz detection plugin
 *
 * detection.cpp
 *
 * Copyright (c) 2010 Canonical Ltd
 *
 * This plugin should does some quick extra checks to make sure we
 * are not running on quirky hardware or drivers.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * AUTHORS: Didier Roche <didrocks@ubuntu.com>
 * 	    Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include "detection.h"

COMPIZ_PLUGIN_20090315 (detection, DetectionPluginVTable);

static CompString HW_accell_blacklisted[] = {
    "8086:3577", /* Intel 830MG, 845G (LP: #259385) */
    "8086:2562"
};

bool
DetectionScreen::doDetection ()
{
    CompOption::Vector o (0);

    /* first, take care about the blacklisted cards */
    for (unsigned int i = 0; i < 2; i++)
    {
	CompString cmd = "lspci -n | grep -q " + HW_accell_blacklisted[i];

	if (system (cmd.c_str ()) == 0 && optionGetDetectBadPci ())
	{
	    compLogMessage ("detection",
			    CompLogLevelFatal,
			    "Accelerated blacklisted PCI ID %s detected",
			    HW_accell_blacklisted[i].c_str ());

	    screen->handleCompizEvent ("detection", "fatal_fallback", o);
	}
    }

    return false;
}

DetectionScreen::DetectionScreen (CompScreen *s) :
    PluginClassHandler <DetectionScreen, CompScreen> (s)
{
    CompTimer::CallBack cb = boost::bind (&DetectionScreen::doDetection,
					  this);

    mDetectionTimer.start (cb, 0, 0);
}

bool
DetectionPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;

    return true;
}
