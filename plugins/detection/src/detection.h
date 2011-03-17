/**
 * Compiz detection plugin
 *
 * detection.h
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

#include "core/core.h"
#include "core/pluginclasshandler.h"
#include "core/timer.h"

#include "detection_options.h"

class DetectionScreen :
    public PluginClassHandler <DetectionScreen, CompScreen>,
    public DetectionOptions
{
    public:

	DetectionScreen (CompScreen *);

    private:

	bool doDetection ();

	CompTimer mDetectionTimer;
};

class DetectionPluginVTable:
    public CompPlugin::VTableForScreen <DetectionScreen>
{
    public:

	bool init ();
};
