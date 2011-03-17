#include "debugspew.h"
#include "../../src/privatescreen.h"
#include "../../src/privatewindow.h"
#include "../../plugins/composite/src/privates.h"
#include "../../plugins/opengl/src/privates.h"

#include <signal.h>
#include <stdio.h>
#include <fcntl.h>

COMPIZ_PLUGIN_20090315 (debugspew, SpewPluginVTable);

static void
handle_crash (int sig)
{
    SpewScreen *ss = SpewScreen::get (screen);

    signal (SIGSEGV, SIG_DFL);
    signal (SIGFPE, SIG_DFL);
    signal (SIGILL, SIG_DFL);
    signal (SIGABRT, SIG_DFL);

    ss->spew ();

    raise (sig); // die
}

void
SpewWindow::spew (CompString &f)
{
    f = f + compPrintf (" == Window Information 0x%x==\n", w->id ());
    f = f + compPrintf ("  Geometry received from server: x %i y %i w %i h %i b %i\n", w->geometry ().x (), w->geometry ().y (), w->geometry ().width (), w->geometry ().height (), w->geometry ().border ());
    f = f + compPrintf ("  Geometry last sent to server: x %i y %i w %i h %i b %i\n", w->serverGeometry ().x (), w->serverGeometry ().y (), w->serverGeometry ().width (), w->serverGeometry ().height (), w->serverGeometry ().border ());
    f = f + compPrintf ("  Input Rect: x %i y %i w %i h %i\n", w->inputRect ().x (), w->inputRect ().y (), w->inputRect ().width (), w->inputRect ().height ());
    f = f + compPrintf ("  Output Rect: x %i y %i w %i h %i\n", w->outputRect ().x (), w->outputRect ().y (), w->outputRect ().width (), w->outputRect ().height ());
    f = f + compPrintf ("  Window id 0x%x Reparented into Wrapper id 0x%x Reparented into Frame id 0x%x\n", w->id (), w->priv->wrapper, w->priv->frame);
    if (w->resName ().size ())
	f = f + compPrintf ("  Window name: %s\n", w->resName ().c_str ());
    f = f + compPrintf ("  === WINDOW REGION ===\n");
    f = f + compPrintf ("   Bounding Rect: %i %i %i %i\n", w->region ().boundingRect ().x (), w->region ().boundingRect ().y (), w->region ().boundingRect ().width (),w->region ().boundingRect ().height ());
    foreach (const CompRect &r, w->region ().rects ())
	f = f + compPrintf ("   Rect: %i %i %i %i\n", r.x (), r.y (), r.width (), r.height ());
    f = f + compPrintf ("  === WINDOW FRAME REGION ===\n");
    f = f + compPrintf ("   Bounding Rect: %i %i %i %i\n", w->frameRegion ().boundingRect ().x (), w->frameRegion ().boundingRect ().y (), w->frameRegion ().boundingRect ().width (),w->frameRegion ().boundingRect ().height ());
    foreach (const CompRect &r, w->frameRegion ().rects ())
	f = f + compPrintf ("   Rect: %i %i %i %i\n", r.x (), r.y (), r.width (), r.height ());
    f = f + compPrintf ("  Set _NET_WM_WINDOW_TYPE: ");

#define SPEW_WINDOW_TYPE(type) \
    case type: \
	f = f + compPrintf (#type) + compPrintf ("\n"); \
	break; \

    switch (w->wmType ())
    {
	SPEW_WINDOW_TYPE (CompWindowTypeDesktopMask)
	SPEW_WINDOW_TYPE (CompWindowTypeDockMask)
	SPEW_WINDOW_TYPE (CompWindowTypeToolbarMask)
	SPEW_WINDOW_TYPE (CompWindowTypeMenuMask)
	SPEW_WINDOW_TYPE (CompWindowTypeUtilMask)
	SPEW_WINDOW_TYPE (CompWindowTypeSplashMask)
	SPEW_WINDOW_TYPE (CompWindowTypeDialogMask)
	SPEW_WINDOW_TYPE (CompWindowTypeNormalMask)
	SPEW_WINDOW_TYPE (CompWindowTypeDropdownMenuMask)
	SPEW_WINDOW_TYPE (CompWindowTypePopupMenuMask)
	SPEW_WINDOW_TYPE (CompWindowTypeTooltipMask)
	SPEW_WINDOW_TYPE (CompWindowTypeNotificationMask)
	SPEW_WINDOW_TYPE (CompWindowTypeComboMask)
	SPEW_WINDOW_TYPE (CompWindowTypeDndMask)
	SPEW_WINDOW_TYPE (CompWindowTypeModalDialogMask)
	SPEW_WINDOW_TYPE (CompWindowTypeFullscreenMask)
	SPEW_WINDOW_TYPE (CompWindowTypeUnknownMask)
	default:
	    f = f + compPrintf ("WARNING: Typeless window\n");
	    break;
    }

#define SPEW_INTERNAL_TYPE(type) \
    case type: \
	f = f + compPrintf (#type) + compPrintf ("\n"); \
	break; \

    f = f + compPrintf ("  Internal Type: ");

    switch (w->type ())
    {
	SPEW_INTERNAL_TYPE (CompWindowTypeDesktopMask)
	SPEW_INTERNAL_TYPE (CompWindowTypeDockMask)
	SPEW_INTERNAL_TYPE (CompWindowTypeToolbarMask)
	SPEW_INTERNAL_TYPE (CompWindowTypeMenuMask)
	SPEW_INTERNAL_TYPE (CompWindowTypeUtilMask)
	SPEW_INTERNAL_TYPE (CompWindowTypeSplashMask)
	SPEW_INTERNAL_TYPE (CompWindowTypeDialogMask)
	SPEW_INTERNAL_TYPE (CompWindowTypeNormalMask)
	SPEW_INTERNAL_TYPE (CompWindowTypeDropdownMenuMask)
	SPEW_INTERNAL_TYPE (CompWindowTypePopupMenuMask)
	SPEW_INTERNAL_TYPE (CompWindowTypeTooltipMask)
	SPEW_INTERNAL_TYPE (CompWindowTypeNotificationMask)
	SPEW_INTERNAL_TYPE (CompWindowTypeComboMask)
	SPEW_INTERNAL_TYPE (CompWindowTypeDndMask)
	SPEW_INTERNAL_TYPE (CompWindowTypeModalDialogMask)
	SPEW_INTERNAL_TYPE (CompWindowTypeFullscreenMask)
	SPEW_INTERNAL_TYPE (CompWindowTypeUnknownMask)
	default:
	    f = f + compPrintf ("WARNING: Typeless window\n");
	    break;
    }

    f = f + compPrintf ("  Window State: ");

#define SPEW_WINDOW_STATE(state) \
    case state: \
	f = f + compPrintf (#state) + compPrintf (" "); \
	break; \


    switch (w->state ())
    {
	SPEW_WINDOW_STATE (CompWindowStateModalMask)
	SPEW_WINDOW_STATE (CompWindowStateStickyMask)
	SPEW_WINDOW_STATE (CompWindowStateMaximizedVertMask)
	SPEW_WINDOW_STATE (CompWindowStateMaximizedHorzMask)
	SPEW_WINDOW_STATE (CompWindowStateShadedMask)
	SPEW_WINDOW_STATE (CompWindowStateSkipTaskbarMask)
	SPEW_WINDOW_STATE (CompWindowStateSkipPagerMask)
	SPEW_WINDOW_STATE (CompWindowStateHiddenMask)
	SPEW_WINDOW_STATE (CompWindowStateFullscreenMask)
	SPEW_WINDOW_STATE (CompWindowStateAboveMask)
	SPEW_WINDOW_STATE (CompWindowStateBelowMask)
	SPEW_WINDOW_STATE (CompWindowStateDemandsAttentionMask)
	SPEW_WINDOW_STATE (CompWindowStateDisplayModalMask)
	default:
	    f = f + compPrintf ("Stateless window");
	    break;
    }

    f = f + compPrintf ("\n");
    f = f + compPrintf ("  Available  actions: ");

#define SPEW_WINDOW_ACTIONS(action) \
    if (w->actions () & action) \
	f = f + compPrintf (#action) + compPrintf (" ");

    {
	SPEW_WINDOW_ACTIONS (CompWindowActionMoveMask)
	SPEW_WINDOW_ACTIONS (CompWindowActionResizeMask)
	SPEW_WINDOW_ACTIONS (CompWindowActionStickMask)
	SPEW_WINDOW_ACTIONS (CompWindowActionMinimizeMask)
	SPEW_WINDOW_ACTIONS (CompWindowActionMaximizeHorzMask)
	SPEW_WINDOW_ACTIONS (CompWindowActionMaximizeVertMask)
	SPEW_WINDOW_ACTIONS (CompWindowActionFullscreenMask)
	SPEW_WINDOW_ACTIONS (CompWindowActionCloseMask)
	SPEW_WINDOW_ACTIONS (CompWindowActionShadeMask)
	SPEW_WINDOW_ACTIONS (CompWindowActionChangeDesktopMask)
	SPEW_WINDOW_ACTIONS (CompWindowActionAboveMask)
	SPEW_WINDOW_ACTIONS (CompWindowActionBelowMask)
    }

    f = f + compPrintf ("\n");
    
    f = f + compPrintf ("  WM Protocols :");

#define SPEW_WINDOW_PROTOCOL(protocol) \
    if (w->protocols () & protocol) \
	f = f + compPrintf (#protocol) + compPrintf (" ");

    {
	SPEW_WINDOW_PROTOCOL (CompWindowProtocolDeleteMask)
	SPEW_WINDOW_PROTOCOL (CompWindowProtocolTakeFocusMask)
	SPEW_WINDOW_PROTOCOL (CompWindowProtocolPingMask)
	SPEW_WINDOW_PROTOCOL (CompWindowProtocolSyncRequestMask)
    }

    f = f + compPrintf ("  Showdesktop mode: %i\n", w->inShowDesktopMode ());
    f = f + compPrintf ("  Grabbed: %i\n", w->grabbed ());
    f = f + compPrintf ("  Pending maps: %i\n", w->pendingMaps ());
    f = f + compPrintf ("  Active Num: %i\n", w->activeNum ());
    f = f + compPrintf ("  Struts:\n");
    if (w->struts ())
    {
	f = f + compPrintf ("   Left: %i %i %i %i\n", w->struts ()->left.x, w->struts ()->left.y, w->struts ()->left.width, w->struts ()->left.height);
	f = f + compPrintf ("   Left: %i %i %i %i\n", w->struts ()->right.x, w->struts ()->right.y, w->struts ()->right.width, w->struts ()->right.height);
	f = f + compPrintf ("   Left: %i %i %i %i\n", w->struts ()->top.x, w->struts ()->top.y, w->struts ()->top.width, w->struts ()->top.height);
	f = f + compPrintf ("   Left: %i %i %i %i\n", w->struts ()->bottom.x, w->struts ()->bottom.y, w->struts ()->bottom.width, w->struts ()->bottom.height);
    }
    f = f + compPrintf ("  SaveMask CWX %i CWY %i CWWidth %i CWHeight %i CWBorder %i CWStackMode %i CWSibling %i\n", w->saveMask () & CWX, w->saveMask () & CWY, w->saveMask () & CWWidth, w->saveMask () & CWHeight, w->saveMask () & CWBorderWidth, w->saveMask () & CWStackMode, w->saveMask () & CWSibling);
    f = f + compPrintf ("  Save Window Changes x %i y %i width %i height %i border %i stack_mode %i sibling 0x%x\n", w->saveWc ().x, w->saveWc ().y, w->saveWc ().width, w->saveWc ().height, w->saveWc ().border_width, w->saveWc ().stack_mode, w->saveWc ().sibling);
    f = f + compPrintf ("  Startup Notification Id %s\n", w->startupId ());
    f = f + compPrintf ("  Desktop: %i\n", w->desktop ());
    f = f + compPrintf ("  ClientLeader (ancestor): 0x%x\n", w->clientLeader (true));
    f = f + compPrintf ("  ClientLeader (non-ancestor): 0x%x\n", w->clientLeader (false));
    //f = f + compPrintf ("Sync Alarm TODO\n");
    f = f + compPrintf ("  Default viewport %i %i\n", w->defaultViewport ().x (), w->defaultViewport ().y ());
    f = f + compPrintf ("  Initial viewport %i %i\n", w->initialViewport ().x (), w->initialViewport ().y ());
    f = f + compPrintf ("  Window icon : Geometry with width %i height %i\n", w->iconGeometry ().width (), w->iconGeometry ().height ());
    f = f + compPrintf ("  OutputDevice %i\n", w->outputDevice ());
    f = f + compPrintf ("  On current desktop: %i\n", w->onCurrentDesktop ());
    f = f + compPrintf ("  OnAllViewports %i\n", w->onAllViewports ());
    f = f + compPrintf ("  Transient window 0x%x\n", w->transientFor ());
    f = f + compPrintf ("  Pending unmaps 0x%x\n", w->pendingUnmaps ());
    f = f + compPrintf ("  Placed window: %i\n", w->placed ());
    f = f + compPrintf ("  Shaded: %i\n", w->shaded ());
    f = f + compPrintf ("  Input extents %i %i %i %i\n", w->input ().left, w->input ().right, w->input ().top, w->input ().bottom);
    f = f + compPrintf ("  Output Extents %i %i %i %i\n", w->input ().left, w->input ().right, w->input ().top, w->input ().bottom);
    //f = f + compPrintf ("  Size Hints %i %i %i %i \n");
    f = f + compPrintf ("  Destroyed window %i\n", w->destroyed ());
    f = f + compPrintf ("  Inivisible window %i \n", w->invisible ());
    f = f + compPrintf ("  Waiting for sync %i\n", w->syncWait ());
    f = f + compPrintf ("  Alive window %i\n", w->alive ());
    f = f + compPrintf ("  Override Redirect %i\n", w->overrideRedirect ());
    f = f + compPrintf ("  Window is mapped %i\n", w->isMapped ());
    f = f + compPrintf ("  Window is viewable %i\n", w->isViewable ());
    f = f + compPrintf ("  Window class %i\n", w->windowClass ());
    f = f + compPrintf ("  Window depth %i\n", w->depth ());

#define SPEW_MWM_DECOR(decor) \
    if (w->mwmDecor () & decor) \
	f = f + compPrintf (#decor) + compPrintf (" ");

    f = f + compPrintf ("Motif WM Decor \n");

    SPEW_MWM_DECOR (MwmDecorAll);
    SPEW_MWM_DECOR (MwmDecorBorder);
    SPEW_MWM_DECOR (MwmDecorHandle);
    SPEW_MWM_DECOR (MwmDecorTitle);
    SPEW_MWM_DECOR (MwmDecorMenu);
    SPEW_MWM_DECOR (MwmDecorMinimize);
    SPEW_MWM_DECOR (MwmDecorMaximize);

    f = f + compPrintf ("MOTIF WM Func \n");

#define SPEW_MWM_FUNC(func) \
    if (w->mwmFunc () & func) \
	f = f + compPrintf (#func) + compPrintf (" ");

    SPEW_MWM_FUNC (MwmFuncAll);
    SPEW_MWM_FUNC (MwmFuncResize);
    SPEW_MWM_FUNC (MwmFuncMove);
    SPEW_MWM_FUNC (MwmFuncIconify);
    SPEW_MWM_FUNC (MwmFuncClose);

    f = f + compPrintf ("\n   === PRIVATE WINDOW ===\n");
    f = f + compPrintf ("    MapNum: %i\n", w->priv->mapNum);
    f = f + compPrintf ("    ActiveNum: %i\n", w->priv->activeNum);
    f = f + compPrintf ("    Window attributes x %i y %i width %i height %i border_width %i depth %i visual 0x%x root 0x%x 0x%x bit_grabity 0x%x win_grabity 0x%x " \
		"colormap 0x%x map_installed %i map_state %i event_masks 0x%x all_event_masks 0x%x, do_not_protogate_mask 0x%x override_redirect %i\n",
	     w->priv->attrib.x, w->priv->attrib.y,
	     w->priv->attrib.width, w->priv->attrib.height, w->priv->attrib.border_width, w->priv->attrib.depth,
	     w->priv->attrib.visual, w->priv->attrib.root, w->priv->attrib.bit_gravity, w->priv->attrib.win_gravity,
	     &w->priv->attrib.colormap, w->priv->attrib.map_installed, w->priv->attrib.map_state, w->priv->attrib.your_event_mask,
	     w->priv->attrib.all_event_masks, w->priv->attrib.your_event_mask,
	     w->priv->attrib.do_not_propagate_mask, w->priv->attrib.override_redirect);
    f = f + compPrintf ("    Size hints mask USPosition %i USSize %i PPosition %i PSize %i PMinSize %i  PMaxSize %i PResizeInc %i PAspect %i PBaseSize %i PWinGravity %i " \
		" x %i y %i w %i h %i min_w %i  min_h %i " \
	        " max_w %i max_h %i width_inc %i height_inc %i base_w %i base_h %i win_gravity 0x%x\n",
	     w->priv->sizeHints.flags & USPosition, w->priv->sizeHints.flags & USSize,
	     w->priv->sizeHints.flags & PPosition, w->priv->sizeHints.flags & PSize, w->priv->sizeHints.flags & PMinSize, w->priv->sizeHints.flags & PMaxSize,
	     w->priv->sizeHints.flags & PResizeInc, w->priv->sizeHints.flags & PAspect, w->priv->sizeHints.flags & PBaseSize, w->priv->sizeHints.flags & PWinGravity,
	     w->priv->sizeHints.x, w->priv->sizeHints.y, w->priv->sizeHints.width,
	     w->priv->sizeHints.height, w->priv->sizeHints.min_width, w->priv->sizeHints.min_height,
	     w->priv->sizeHints.max_width, w->priv->sizeHints.max_height, w->priv->sizeHints.width_inc, w->priv->sizeHints.height_inc,
	     w->priv->sizeHints.base_width, w->priv->sizeHints.base_height, w->priv->sizeHints.win_gravity);
    if (w->priv->hints)
    {
        f = f + compPrintf ("    WM hints mask InputHint %i StateHint %i IconPixmapHint %i IconWindowHint %i IconPositionHint %i IconMaskHint %i"\
			"WindowGroupHint %i XUrgencyHint %i  input %i initial_state %i icon_window 0x%x icon_x %i icon_y %i window_group 0x%x\n",
			w->priv->hints->flags & InputHint,
			w->priv->hints->flags & StateHint, w->priv->hints->flags & IconPixmapHint,
			w->priv->hints->flags & IconWindowHint,  w->priv->hints->flags & IconPositionHint,
			w->priv->hints->flags & IconMaskHint, w->priv->hints->flags & WindowGroupHint, w->priv->hints->flags & XUrgencyHint,
			w->priv->hints->input, w->priv->hints->initial_state,
			w->priv->hints->icon_window, w->priv->hints->icon_x, w->priv->hints->icon_y, w->priv->hints->window_group);
    }
    f = f + compPrintf ("    InputHint %i\n", w->priv->inputHint);
    f = f + compPrintf ("    Has alpha channel: %i\n", w->priv->alpha);
    f = f + compPrintf ("    Input Region :\n");
    foreach (const CompRect &r, w->frameRegion ().rects ())
	f = f + compPrintf ("   Rect: %i %i %i %i\n", r.x (), r.y (), r.width (), r.height ());
    f = f + compPrintf ("    Manged %i Unmanaged %i\n", w->priv->managed, w->priv->unmanaging);
    f = f + compPrintf ("    Destroy refcount %i\n", w->priv->destroyRefCnt);
    f = f + compPrintf ("    Unmap refcount %i\n", w->priv->unmapRefCnt);
    f = f + compPrintf ("    Initial timestamp %i set %i\n", w->priv->initialTimestamp, w->priv->initialTimestampSet);
    f = f + compPrintf ("    Hidden window %i\n", w->priv->hidden);
    f = f + compPrintf ("    Grabbed %i\n", w->grabbed ());
    f = f + compPrintf ("    Pending Unmaps %i\n", w->pendingUnmaps ());
    f = f + compPrintf ("    Pending Maps %i\n", w->priv->pendingMaps);
    f = f + compPrintf ("    Last pong from window %i\n", w->priv->lastPong);
    f = f + compPrintf ("    Close requests: %i\n", w->priv->closeRequests);
    f = f + compPrintf ("    Last close request time %i\n", w->priv->lastCloseRequestTime);

    f = f + compPrintf ("= OPENGL WINDOW =\n");
    f = f + compPrintf ("ClipRegion, bRect %i %i %i %i\n", gWindow->clip ().boundingRect ().x (), gWindow->clip ().boundingRect ().y (), gWindow->clip ().boundingRect ().width (), gWindow->clip ().boundingRect ().height ());
    foreach (const CompRect &r, gWindow->clip ().rects ())
	f = f + compPrintf (" Rect: %i %i %i %i\n", r.x (), r.y (), r.width (), r.height ());
    f = f + compPrintf ("Current PAttrib opacity %i brightness %i saturation %i xScale %i yScale %i xTranslate %i, yTranslate %i\n", gWindow->paintAttrib ().opacity, gWindow->paintAttrib ().brightness, gWindow->paintAttrib ().saturation, gWindow->paintAttrib ().xScale, gWindow->paintAttrib ().yScale, gWindow->paintAttrib ().xTranslate, gWindow->paintAttrib ().yTranslate);
    f = f + compPrintf ("Last PAttrib opacity %i brightness %i saturation %i xScale %i yScale %i xTranslate %i, yTranslate %i\n", gWindow->lastPaintAttrib ().opacity, gWindow->lastPaintAttrib ().brightness, gWindow->lastPaintAttrib ().saturation, gWindow->lastPaintAttrib ().xScale, gWindow->lastPaintAttrib ().yScale, gWindow->lastPaintAttrib ().xTranslate, gWindow->lastPaintAttrib ().yTranslate);

#define SPEW_LAST_PAINT_MASK(mask) \
    if (gWindow->lastMask () & mask) \
	f = f + compPrintf (#mask) + compPrintf (" ");

    SPEW_LAST_PAINT_MASK (PAINT_WINDOW_ON_TRANSFORMED_SCREEN_MASK);
    SPEW_LAST_PAINT_MASK (PAINT_WINDOW_OCCLUSION_DETECTION_MASK);
    SPEW_LAST_PAINT_MASK (PAINT_WINDOW_WITH_OFFSET_MASK);
    SPEW_LAST_PAINT_MASK (PAINT_WINDOW_TRANSLUCENT_MASK);
    SPEW_LAST_PAINT_MASK (PAINT_WINDOW_TRANSFORMED_MASK);
    SPEW_LAST_PAINT_MASK (PAINT_WINDOW_NO_CORE_INSTANCE_MASK);
    SPEW_LAST_PAINT_MASK (PAINT_WINDOW_BLEND_MASK);

    f = f + compPrintf (" == TEXTURES ==\n");
    foreach (GLTexture *tex, gWindow->textures ())
    {
	f = f + compPrintf ("  Texture name: %i target %i, matrix.xx %i matrix.xy %i, matrix.x0 %i matrix.yy %i, matrix.yx %i, matrix.y0 %i, mipmapping %i \n", tex->name (), tex->target (), tex->matrix ().xx, tex->matrix ().xy, tex->matrix ().x0, tex->matrix ().yy, tex->matrix ().yx, tex->matrix ().y0, tex->mipmap ());
    }

    GLTexture *tex = gWindow->getIcon (128, 128);

    if (tex)
	f = f + compPrintf (" Icon texture: name: %i target %i, matrix.xx %i matrix.xy %i, matrix.x0 %i matrix.yy %i, matrix.yx %i, matrix.y0 %i, mipmapping %i \n", tex->name (), tex->target (), tex->matrix ().xx, tex->matrix ().xy, tex->matrix ().x0, tex->matrix ().yy, tex->matrix ().yx, tex->matrix ().y0, tex->mipmap ());

    f = f + compPrintf (" == GEOMETRY ==\n");
    f = f + compPrintf ("  Vertex Size %i Vertex Stride %i\n", gWindow->geometry ().vertexSize, gWindow->geometry ().vertexStride);
    for (int i  = 0; i < gWindow->geometry ().vertexSize; i++)
	f = f + compPrintf ("Vertex %f\n", gWindow->geometry ().vertices[i]);
    f = f + compPrintf ("  Index Size %i Index count %i \n", gWindow->geometry ().indexSize, gWindow->geometry ().indexCount);
    for (int i  = 0; i < gWindow->geometry ().indexSize; i++)
	f = f + compPrintf ("Vertex %f\n", gWindow->geometry ().indices[i]);
    f = f + compPrintf ("  vCount %i texUnits %i texCoordSize %i\n", gWindow->geometry ().vCount, gWindow->geometry ().texUnits, gWindow->geometry ().texCoordSize);

    f = f + compPrintf (" == REGIONS == \n");
    foreach (CompRegion &reg, gWindow->priv->regions)
    { 
	f = f + compPrintf ("  Region, bRect %i %i %i %i\n", reg.boundingRect ().x (), reg.boundingRect ().y (), reg.boundingRect ().width (), reg.boundingRect ().height ());
	foreach (const CompRect &r, reg.rects ())
	{
	    f = f + compPrintf ("   Rect: %i %i %i %i\n", r.x (), r.y (), r.width (), r.height ());
	}
    }

    f = f + compPrintf ("== COMPOSITE WINDOW==\n");

    f = f + compPrintf (" Pixmap id 0x%x\n", cWindow->pixmap ());

    Window root;
    int    x, y;
    unsigned int width, height, border, depth;

    if (XGetGeometry (screen->dpy (), cWindow->pixmap (), &root, &x, &y, &width, &height, &border, &depth))
    {
	f = f + compPrintf (" XGetGeometry succeeded: x %i y %i width %i height %i border %i depth %i\n", x, y, width, height, border, depth);
    }
    else
    {
	f = f + compPrintf (" XGetGeometry failed\n");
    }

    f = f + compPrintf (" redirected: %i\n", cWindow->redirected ());
    f = f + compPrintf (" overlay: %i\n", cWindow->overlayWindow ());
    f = f + compPrintf (" damaged: %i\n", cWindow->damaged ());
    f = f + compPrintf (" bound: %i\n", !cWindow->priv->bindFailed);
    f = f + compPrintf (" == DAMAGE RECTS ==\n");
    f = f + compPrintf ("  current damage rect %i\n", cWindow->priv->nDamage);
    for (int i = 0; i < cWindow->priv->sizeDamage; i++)
	f = f + compPrintf ("  rect: %i %i %i %i\n", cWindow->priv->damageRects[i].x, cWindow->priv->damageRects[i].y, cWindow->priv->damageRects[i].width, cWindow->priv->damageRects[i].height);

}

void
SpewScreen::spewScreenEdge (CompString &f, unsigned int i)
{
    struct screenEdgeGeometry {
	int xw, x0;
	int yh, y0;
	int ww, w0;
	int hh, h0;
    } geometry[SCREEN_EDGE_NUM] = {
	{ 0,  0,   0,  2,   0,  2,   1, -4 }, /* left */
	{ 1, -2,   0,  2,   0,  2,   1, -4 }, /* right */
	{ 0,  2,   0,  0,   1, -4,   0,  2 }, /* top */
	{ 0,  2,   1, -2,   1, -4,   0,  2 }, /* bottom */
	{ 0,  0,   0,  0,   0,  2,   0,  2 }, /* top-left */
	{ 1, -2,   0,  0,   0,  2,   0,  2 }, /* top-right */
	{ 0,  0,   1, -2,   0,  2,   0,  2 }, /* bottom-left */
	{ 1, -2,   1, -2,   0,  2,   0,  2 }  /* bottom-right */
    };
    int x, y, w, h;

    x = geometry[i].xw * screen->width () + geometry[i].x0;
    y = geometry[i].yh * screen->height () + geometry[i].y0;
    w = geometry[i].ww * screen->width () + geometry[i].w0;
    h = geometry[i].hh * screen->height () + geometry[i].h0;

    f = compPrintf ("  Screen edge %i Window id 0x%x, Geometry %i %i %i %i\n", i, screen->priv->screenEdge[i].id, x, y, w, h);
}

bool
SpewScreen::spew ()
{
    CompString f;
    int pid = getpid ();

    CompString filename = compPrintf ("/tmp/compiz_internal_state%i.log", pid);
    int fd;
    /* CompScreen */

    f = f + compPrintf ("\n");

    f = f + compPrintf ("= Screen Information =\n");
    f = f + compPrintf ("Startup display option: %s\n", screen->displayString ());
    f = f + compPrintf ("Currently active window: 0x%x\n", screen->activeWindow ());
    f = f + compPrintf ("Window we want to auto-raise: 0x%x\n", screen->autoRaiseWindow ());
    f = f + compPrintf ("== Root window information ==\n");
    f = f + compPrintf (" Root window id: 0x%x\n", screen->root ());
    f = f + compPrintf (" === X Window attributes of the root window ===\n");
    f = f + compPrintf ("  attributes x %i y %i width %i height %i border_width %i depth %i visual 0x%x root 0x%x 0x%x bit_grabity 0x%x win_grabity 0x%x " \
		"colormap 0x%x map_installed %i map_state %i event_masks 0x%x all_event_masks 0x%x, do_not_protogate_mask 0x%x override_redirect %i\n", screen->priv->attrib.x, screen->priv->attrib.y,
	     screen->priv->attrib.width, screen->priv->attrib.height, screen->priv->attrib.border_width, screen->priv->attrib.depth,
	     screen->priv->attrib.visual, screen->priv->attrib.root, screen->priv->attrib.bit_gravity, screen->priv->attrib.win_gravity,
	     &screen->priv->attrib.colormap, screen->priv->attrib.map_installed, screen->priv->attrib.map_state, screen->priv->attrib.your_event_mask,
	     screen->priv->attrib.all_event_masks, screen->priv->attrib.your_event_mask,
	     screen->priv->attrib.do_not_propagate_mask, screen->priv->attrib.override_redirect);
    f = f + compPrintf ("Screen Number: %i\n", screen->screenNum ());
    f = f + compPrintf ("Window that has the selection 0x%x\n", screen->selectionWindow ());
    f = f + compPrintf ("Current output extents x %i y %i width %i height %i\n", screen->getCurrentOutputExtents ().x (), screen->getCurrentOutputExtents ().y (), screen->getCurrentOutputExtents ().width (), screen->getCurrentOutputExtents ().height ());
    f = f + compPrintf ("Current active output number: %i\n", screen->currentOutputDev ().id ());
    f = f + compPrintf ("= OUTPUT DEVICES=\n");
    foreach (CompOutput &o, screen->outputDevs ())
	f = f + compPrintf ( " id %i name %s x %i y %i width %i height %i wa x %i wa y %i wa width %i wa height %i\n", o.id (), o.name ().c_str (), o.x (), o.y (), o.width (), o.height (), o.workArea().x (), o.workArea ().y (), o.workArea ().width (), o.workArea ().height ());
    f = f + compPrintf ("Current viewport: %i %i\n", screen->vp ().x (), screen->vp ().y ());
    f = f + compPrintf ("Current viewport size: %i %i\n", screen->vpSize ().width (), screen->vpSize ().height ());
    f = f + compPrintf ("Number of desktop windows %i\n", screen->desktopWindowCount ());
    f = f + compPrintf ("Current number of _NET_WM_DESKTOPs %i\n", screen->nDesktop ());
    f = f + compPrintf (" _NET_WM_DESKTOP %i\n", screen->currentDesktop ());
    f = f + compPrintf ("= ACTIVE WINDOW HISTORY =\n");
    f = f + compPrintf (" current history is number: %i\n", screen->priv->currentHistory);
    for (unsigned int i = 0; i < ACTIVE_WINDOW_HISTORY_NUM; i++)
    {
	f = f + compPrintf (" checking history %i x %i y %i activeNum %i\n", i, screen->currentHistory ()[i].x, screen->currentHistory ()[i].y, screen->currentHistory ()[i].activeNum);
	for (unsigned int j = 0; j < ACTIVE_WINDOW_HISTORY_SIZE; j++)
	    f = f + compPrintf ("  window id 0x%x\n", screen->currentHistory ()[i].id[j]);
    }


    f = f + compPrintf ("User has overlapping outputs %i\n", screen->hasOverlappingOutputs ());
    f = f + compPrintf ("Fullscreen output geometry x %i y %i width %i height %i\n", screen->fullscreenOutput ().x (), screen->fullscreenOutput ().y (), screen->fullscreenOutput ().width (), screen->fullscreenOutput ().height ());
    f = f + compPrintf ("= XINERAMA SCREEN INFORMATION =\n");
    foreach (XineramaScreenInfo &si, screen->priv->screenInfo)
	f = f + compPrintf (" Screen number %i x_org %i y_org %i width %i height %i\n", si.screen_number, si.x_org, si.y_org, si.width, si.height);
    f = f + compPrintf ("Serialize Bit: %i\n", screen->shouldSerializePlugins ());
    f = f + compPrintf ("= PRIVATE SCREEN =\n");
    f = f + compPrintf (" Last time pinged 0x%x\n", screen->priv->lastPing);
    f = f + compPrintf (" Next Active window 0x%x\n", screen->priv->nextActiveWindow);
    f = f + compPrintf (" Current below window 0x%x\n", screen->priv->below);
    f = f + compPrintf (" Escape KeyCode 0x%x\n", screen->priv->escapeKeyCode);
    f = f + compPrintf (" Return KeyCode 0x%x\n", screen->priv->returnKeyCode);
    f = f + compPrintf (" Current internal plugin load list : ");
    foreach (CompOption::Value &v, screen->priv->plugin.list ())
	f = f + compPrintf ("%s", v.s ().c_str ());
    f = f + compPrintf ("\n");
    f = f + compPrintf (" Screen region: %i %i %i %i\n", screen->priv->region.boundingRect ().x (), screen->priv->region.boundingRect ().y (), screen->priv->region.boundingRect ().width (), screen->priv->region.boundingRect ().height ());
    f = f + compPrintf (" == REGION INFO ==\n");
    foreach (const CompRect &r, screen->priv->region.rects ())
	f = f + compPrintf ("  Rect %i %i %i %i\n", r.x (), r.y (), r.width (), r.height ());
    f = f + compPrintf (" lastViewport %i %i %i %i\n", screen->priv->lastViewport.x, screen->priv->lastViewport.y, screen->priv->lastViewport.width, screen->priv->lastViewport.height);
    f = f + compPrintf (" == SCREEN EDGES ==\n");
    for (unsigned int i = 0; i < SCREEN_EDGE_NUM; i++)
	spewScreenEdge (f, i);
    f = f + compPrintf (" == STARTUP SEQUENCES ==\n");
    foreach (CompStartupSequence *start, screen->priv->startupSequences)
	f = f + compPrintf ( "vp x %i vp y %i\n", start->viewportX, start->viewportY);//spewStartupSequence (start);
    f = f + compPrintf (" == WINDOW GROUPS ==\n");
    foreach (CompGroup *g, screen->priv->groups)
	f = f + compPrintf ( "id %i refCnt %i\n", g->id, g->refCnt);//spewGroup (g);
    f = f + compPrintf (" == MAPPING ORDER ==\n");
    foreach (CompWindow *w, screen->priv->clientList)
    {
	if (!w->resName ().empty ())
	    f = f + compPrintf ( "  Window ID: 0x%x Window Title: %s\n", w->id (), w->resName ().c_str ());
	else
	    f = f + compPrintf ( "  Window ID: 0x%x\n", w->id ());
    }
    f = f + compPrintf (" == STACKING ORDER ==\n");
    foreach (CompWindow *w, screen->priv->clientListStacking)
    {
	if (!w->resName ().empty ())
	    f = f + compPrintf ( "  Window ID: 0x%x Name: %s\n", w->id (), w->resName ().c_str ());
	else
	    f = f + compPrintf ( "  Window ID: 0x%x\n", w->id ());
    }
    f = f + compPrintf (" == MAPPING ID ORDER ==\n");
    foreach (Window &id, screen->priv->clientIdList)
	f = f + compPrintf ( "  Window ID: 0x%x\n", id);
    f = f + compPrintf (" == STACKING ID ORDER ==\n");
    foreach (Window &id, screen->priv->clientIdListStacking)
	f = f + compPrintf ( "  Window ID: 0x%x\n", id);
    f = f + compPrintf (" == BUTTON GRABS ==\n");
    foreach (PrivateScreen::ButtonGrab &b, screen->priv->buttonGrabs)
	f = f + compPrintf ( "  button %i mods 0x%x, count %i\n", b.button, b.modifiers, b.count);
    f = f + compPrintf (" == KEY GRABS ==\n");
    foreach (PrivateScreen::KeyGrab &k, screen->priv->keyGrabs)
	f = f + compPrintf ( "  keycode 0x%x mods 0x%x, count %i\n", k.keycode, k.modifiers, k.count);
    f = f + compPrintf (" == ACTIVE GRABS ==\n");
    foreach (PrivateScreen::Grab *g, screen->priv->grabs)
	f = f + compPrintf ( "  Grab 0x%x, CursorId 0x%x, Name 0x%x\n", g, g->cursor, g->name);
    f = f + compPrintf ("Screen Grabbed %i\n", screen->priv->grabbed);
    f = f + compPrintf ("Number of pending destroys %i\n", screen->priv->pendingDestroys);
    f = f + compPrintf ("Showdesktop Mask 0x%x\n", screen->priv->showingDesktopMask);

    f = f + compPrintf ("= Composite Screen =\n");
    f = f + compPrintf (" Compositing Active: %i\n", cScreen->compositingActive ());
    f = f + compPrintf (" DamageMask Pending %i Region %i All %i\n", cScreen->damageMask () & COMPOSITE_SCREEN_DAMAGE_PENDING_MASK, cScreen->damageMask () & COMPOSITE_SCREEN_DAMAGE_REGION_MASK, cScreen->damageMask () & COMPOSITE_SCREEN_DAMAGE_ALL_MASK); 
    f = f + compPrintf (" Current Damage region bRect %i %i %i %i\n", cScreen->priv->damage.boundingRect ().x (), cScreen->priv->damage.boundingRect ().y (), cScreen->priv->damage.boundingRect ().width (), cScreen->priv->damage.boundingRect ().height ());
    foreach (const CompRect &r, cScreen->priv->damage.rects ())
	f = f + compPrintf ("  Rect: %i %i %i %i\n", r.x (), r.y (), r.width (), r.height ());

    f = f + compPrintf (" Overlay window 0x%x\n", cScreen->overlay ());
    f = f + compPrintf (" Output 0x%x\n", cScreen->output ());
    f = f + compPrintf (" overlaywindowcount %i\n", cScreen->overlayWindowCount ());
    f = f + compPrintf (" Window Paint offset %i %i\n", cScreen->windowPaintOffset ().x (), cScreen->windowPaintOffset ().y ());
    f = f + compPrintf (" FPS Limiter mode %i\n", cScreen->FPSLimiterMode ());
    f = f + compPrintf (" Redraw time %i\n", cScreen->redrawTime ());
    f = f + compPrintf (" Optimal redraw time %i\n", cScreen->optimalRedrawTime ());
    f = f + compPrintf (" Exposed Rects \n");
    foreach (CompRect &r, cScreen->priv->exposeRects)
    {
	f = f + compPrintf (" Rect %i %i %i %i\n", r.x (), r.y (), r.width (), r.height ());
    }

    //f = f + compPrintf (" Last redraw happened at %i.%i\n", cScreen->priv->lastRedraw.tv_sec, cScreen->priv->lastRedraw.tv_msec);
    f = f + compPrintf (" Next redraw at %i\n", cScreen->priv->nextRedraw);
    f = f + compPrintf (" Frame status %i\n", cScreen->priv->frameStatus);
    f = f + compPrintf (" Time multiplier: %i\n", cScreen->priv->timeMult);
    f = f + compPrintf (" Idle %i\n", cScreen->priv->idle);
    f = f + compPrintf (" timeLeft %i\n", cScreen->priv->timeLeft);
    f = f + compPrintf (" slowAnimations %i\n", cScreen->priv->slowAnimations);
    f = f + compPrintf (" active %i\n", cScreen->priv->active);
    f = f + compPrintf (" frame time accumulator: %i\n", cScreen->priv->frameTimeAccumulator);

    f = f + compPrintf ("= GLScreen =\n");
    f = f + compPrintf (" Texture Filter: %i\n", gScreen->textureFilter ());
    //f = f + compPrintf (" Compiz Texture Filter: %i\n", gScreen->filter ());
    f = f + compPrintf (" Lighting: %i\n", gScreen->lighting ());
    f = f + compPrintf (" FBConfigs:\n");

    for (unsigned int i = 0; i < MAX_DEPTH; i++)
    {
	GLFBConfig *fbc = gScreen->glxPixmapFBConfig (i);
	f = f + compPrintf (" GLFBConfig %i: GLXFBConfigRec TODO yInverted: %i, mipmap %i, textureFormat %i, textureTarget %i\n", i, fbc->yInverted, fbc->mipmap, fbc->textureFormat, fbc->textureTargets);
    }

    const float *m = gScreen->projectionMatrix ();

    f = f + compPrintf ("Projection Matrix: [[%i %i %i %i]\n"\
			"		     [%i %i %i %i]\n"\
			"		     [%i %i %i %i]\n"\
			"		     [%i %i %i %i]\n"\
			"		     [%i %i %i %i]]\n",
			m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], m[9], m[10], m[11], m[12], m[13], m[14], m[15]);

    f = f + compPrintf ("  Backgrounds loaded %i nBackgrounds %i\n", gScreen->priv->backgroundLoaded, gScreen->priv->backgroundTextures.size ());
    f = f + compPrintf ("  ClearBuffers %i\n", gScreen->priv->clearBuffers);
    f = f + compPrintf ("  lighting %i\n", gScreen->priv->lighting);
    f = f + compPrintf ("  GLXContext TODO\n");
    f = f + compPrintf ("  pendingCommands %i\n", gScreen->priv->pendingCommands);
    f = f + compPrintf ("  lastViewport: x %i y %i width %i height %i\n", gScreen->priv->lastViewport.x, gScreen->priv->lastViewport.y, gScreen->priv->lastViewport.width, gScreen->priv->lastViewport.height);
    f = f + compPrintf ("  hasCompositing: %i\n", gScreen->priv->hasCompositing);

    f = f + compPrintf ("== EACH WINDOW ==\n");
    foreach (CompWindow *w, screen->priv->windows)
	SpewWindow::get (w)->spew (f);

    fprintf (stderr, "%s", f.c_str ());

    /* Write it to a file called comp_internal_state */

    fd = open (filename.c_str (), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if (fd)
    {
	lseek (fd, 0, SEEK_SET);
	int result = write (fd, (void *) f.c_str (), f.size ());
	if (result)
	    fsync (fd);
	close (fd);
    }
    else
    {
	f = f + compPrintf ( "%m\n");
    }

    return true;
}

void
SpewScreen::handleCompizEvent (const char *plugin, const char *action, CompOption::Vector &opts)
{
    if (strcmp ("fatal_fallback", action) == 0)
    {
	spew ();
    }

    screen->handleCompizEvent (plugin, action, opts);
}

SpewScreen::SpewScreen (CompScreen *s) :
    PluginClassHandler <SpewScreen, CompScreen> (s),
    cScreen (CompositeScreen::get (s)),
    gScreen (GLScreen::get (s))
{
    ScreenInterface::setHandler (s);

    optionSetSpewKeyInitiate (boost::bind (&SpewScreen::spew, this));

    //signal (SIGSEGV, handle_crash);
    //signal (SIGFPE, handle_crash);
    //signal (SIGILL, handle_crash);
    //signal (SIGABRT, handle_crash);
}

SpewWindow::SpewWindow (CompWindow *w) :
    PluginClassHandler <SpewWindow, CompWindow> (w),
    w (w),
    cWindow (CompositeWindow::get (w)),
    gWindow (GLWindow::get (w))
{
}

bool
SpewPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
	!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	return false;

    return true;
}
