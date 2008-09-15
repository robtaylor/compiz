#ifndef _ATOMS_H
#define _ATOMS_H

#include <X11/Xlib-xcb.h>

namespace Atoms {
    extern Atom supported;
    extern Atom supportingWmCheck;

    extern Atom utf8String;

    extern Atom wmName;

    extern Atom winType;
    extern Atom winTypeDesktop;
    extern Atom winTypeDock;
    extern Atom winTypeToolbar;
    extern Atom winTypeMenu;
    extern Atom winTypeUtil;
    extern Atom winTypeSplash;
    extern Atom winTypeDialog;
    extern Atom winTypeNormal;
    extern Atom winTypeDropdownMenu;
    extern Atom winTypePopupMenu;
    extern Atom winTypeTooltip;
    extern Atom winTypeNotification;
    extern Atom winTypeCombo;
    extern Atom winTypeDnd;

    extern Atom winOpacity;
    extern Atom winBrightness;
    extern Atom winSaturation;
    extern Atom winActive;
    extern Atom winDesktop;

    extern Atom workarea;

    extern Atom desktopViewport;
    extern Atom desktopGeometry;
    extern Atom currentDesktop;
    extern Atom numberOfDesktops;

    extern Atom winState;
    extern Atom winStateModal;
    extern Atom winStateSticky;
    extern Atom winStateMaximizedVert;
    extern Atom winStateMaximizedHorz;
    extern Atom winStateShaded;
    extern Atom winStateSkipTaskbar;
    extern Atom winStateSkipPager;
    extern Atom winStateHidden;
    extern Atom winStateFullscreen;
    extern Atom winStateAbove;
    extern Atom winStateBelow;
    extern Atom winStateDemandsAttention;
    extern Atom winStateDisplayModal;

    extern Atom winActionMove;
    extern Atom winActionResize;
    extern Atom winActionStick;
    extern Atom winActionMinimize;
    extern Atom winActionMaximizeHorz;
    extern Atom winActionMaximizeVert;
    extern Atom winActionFullscreen;
    extern Atom winActionClose;
    extern Atom winActionShade;
    extern Atom winActionChangeDesktop;
    extern Atom winActionAbove;
    extern Atom winActionBelow;

    extern Atom wmAllowedActions;

    extern Atom wmStrut;
    extern Atom wmStrutPartial;

    extern Atom wmUserTime;

    extern Atom wmIcon;
    extern Atom wmIconGeometry;

    extern Atom clientList;
    extern Atom clientListStacking;

    extern Atom frameExtents;
    extern Atom frameWindow;

    extern Atom wmState;
    extern Atom wmChangeState;
    extern Atom wmProtocols;
    extern Atom wmClientLeader;

    extern Atom wmDeleteWindow;
    extern Atom wmTakeFocus;
    extern Atom wmPing;
    extern Atom wmSyncRequest;

    extern Atom wmSyncRequestCounter;

    extern Atom closeWindow;
    extern Atom wmMoveResize;
    extern Atom moveResizeWindow;
    extern Atom restackWindow;

    extern Atom showingDesktop;

    extern Atom xBackground[2];

    extern Atom toolkitAction;
    extern Atom toolkitActionMainMenu;
    extern Atom toolkitActionRunDialog;
    extern Atom toolkitActionWindowMenu;
    extern Atom toolkitActionForceQuitDialog;

    extern Atom mwmHints;

    extern Atom xdndAware;
    extern Atom xdndEnter;
    extern Atom xdndLeave;
    extern Atom xdndPosition;
    extern Atom xdndStatus;
    extern Atom xdndDrop;

    extern Atom manager;
    extern Atom targets;
    extern Atom multiple;
    extern Atom timestamp;
    extern Atom version;
    extern Atom atomPair;

    extern Atom startupId;

    void init (Display *dpy);
};

#endif
