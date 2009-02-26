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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <compiz.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#include <core/core.h>
#include "privatescreen.h"

char *programName;
char **programArgv;
int  programArgc;

char *backgroundImage = NULL;

bool shutDown = false;
bool restartSignal = false;

CompWindow *lastFoundWindow = 0;

bool replaceCurrentWm = false;
bool indirectRendering = false;
bool noDetection = false;
bool useDesktopHints = false;
bool debugOutput = false;

bool useCow = true;

CompMetadata *coreMetadata = NULL;

unsigned int privateHandlerIndex = 0;

static void
usage (void)
{
    printf ("Usage: %s "
	    "[--replace] "
	    "[--display DISPLAY]\n       "
	    "[--indirect-rendering] "
	    "[--sm-disable] "
	    "[--sm-client-id ID]\n       "
	    "[--bg-image PNG] "
	    "[--no-detection] "
	    "[--keep-desktop-hints]\n       "
	    "[--use-root-window] "
	    "[--debug] "
	    "[--version] "
	    "[--help] "
	    "[PLUGIN]...\n",
	    programName);
}


static void
signalHandler (int sig)
{
    int status;

    switch (sig) {
    case SIGCHLD:
	waitpid (-1, &status, WNOHANG | WUNTRACED);
	break;
    case SIGHUP:
	restartSignal = TRUE;
	break;
    case SIGINT:
    case SIGTERM:
	shutDown = TRUE;
    default:
	break;
    }
}

typedef struct _CompIOCtx {
    unsigned int offset;
    char         *pluginData;
} CompIOCtx;

static int
readCoreXmlCallback (void *context,
		     char *buffer,
		     int  length)
{
    CompIOCtx    *ctx = (CompIOCtx *) context;
    unsigned int offset = ctx->offset;
    unsigned int i, j;

    i = CompMetadata::readXmlChunk ("<compiz><plugin name=\"core\"><options>",
				    &offset, buffer, length);

    for (j = 0; j < COMP_OPTION_NUM; j++)
    {
	CompMetadata::OptionInfo info = coreOptionInfo[j];

	switch (j) {
	case COMP_OPTION_ACTIVE_PLUGINS:
	    if (ctx->pluginData)
		info.data = ctx->pluginData;
	    break;
	default:
	    break;
	}

	i += CompMetadata::readXmlChunkFromOptionInfo (&info, &offset,
						       buffer + i, length - i);
    }

    i += CompMetadata::readXmlChunk ("</options></plugin></compiz>", &offset,
				     buffer + i, length - 1);

    if (!offset && length > (int)i)
	buffer[i++] = '\0';

    ctx->offset += i;

    return i;
}

int
main (int argc, char **argv)
{
    CompIOCtx ctx;
    char      *displayName = 0;
    char      *plugin[256];
    int	      i, nPlugin = 0;
    Bool      disableSm = FALSE;
    char      *clientId = NULL;

    programName = argv[0];
    programArgc = argc;
    programArgv = argv;

    signal (SIGHUP, signalHandler);
    signal (SIGCHLD, signalHandler);
    signal (SIGINT, signalHandler);
    signal (SIGTERM, signalHandler);

    memset (&ctx, 0, sizeof (ctx));

    for (i = 1; i < argc; i++)
    {
	if (!strcmp (argv[i], "--help"))
	{
	    usage ();
	    return 0;
	}
	else if (!strcmp (argv[i], "--version"))
	{
	    printf (PACKAGE_STRING "\n");
	    return 0;
	}
	else if (!strcmp (argv[i], "--debug"))
	{
	    debugOutput = true;
	}
	else if (!strcmp (argv[i], "--display"))
	{
	    if (i + 1 < argc)
		displayName = argv[++i];
	}
	else if (!strcmp (argv[i], "--indirect-rendering"))
	{
	    indirectRendering = TRUE;
	}
	else if (!strcmp (argv[i], "--keep-desktop-hints"))
	{
	    useDesktopHints = true;
	}
	else if (!strcmp (argv[i], "--ignore-desktop-hints"))
	{
	    /* backward compatibility */
	}
	else if (!strcmp (argv[i], "--use-root-window"))
	{
	    useCow = FALSE;
	}
	else if (!strcmp (argv[i], "--replace"))
	{
	    replaceCurrentWm = TRUE;
	}
	else if (!strcmp (argv[i], "--sm-disable"))
	{
	    disableSm = TRUE;
	}
	else if (!strcmp (argv[i], "--sm-client-id"))
	{
	    if (i + 1 < argc)
		clientId = argv[++i];
	}
	else if (!strcmp (argv[i], "--no-detection"))
	{
	    noDetection = TRUE;
	}
	else if (!strcmp (argv[i], "--bg-image"))
	{
	    if (i + 1 < argc)
		backgroundImage = argv[++i];
	}
	else if (*argv[i] == '-')
	{
	    compLogMessage ("core", CompLogLevelWarn,
			    "Unknown option '%s'\n", argv[i]);
	}
	else
	{
	    if (nPlugin < 256)
		plugin[nPlugin++] = argv[i];
	}
    }

    if (nPlugin)
    {
	int size = 256;

	for (i = 0; i < nPlugin; i++)
	    size += strlen (plugin[i]) + 16;

	ctx.pluginData = (char *) malloc (size);
	if (ctx.pluginData)
	{
	    char *ptr = ctx.pluginData;

	    ptr += sprintf (ptr, "<type>string</type><default>");

	    for (i = 0; i < nPlugin; i++)
		ptr += sprintf (ptr, "<value>%s</value>", plugin[i]);

	    ptr += sprintf (ptr, "</default>");
	}
    }

    xmlInitParser ();

    LIBXML_TEST_VERSION;

    coreMetadata = getCoreVTable ()->getMetadata ();

    if (!coreMetadata->addFromIO (readCoreXmlCallback, NULL, &ctx))
	return 1;

    if (ctx.pluginData)
	free (ctx.pluginData);

    coreMetadata->addFromFile ("core");

    screen = new CompScreen();

    if (!screen)
	return 1;

    if (!screen->init (displayName))
	return 1;

    if (!disableSm)
	CompSession::initSession (clientId);

    screen->eventLoop ();

    if (!disableSm)
	CompSession::closeSession ();

    delete screen;
    delete coreMetadata;

    xmlCleanupParser ();

    if (restartSignal)
    {
	execvp (programName, programArgv);
	return 1;
    }

    return 0;
}
