#ifndef _COMPSESSION_H
#define _COMPSESSION_H

#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>

namespace CompSession {

    enum Event {
	EventSaveYourself = 0,
	EventSaveComplete,
	EventDie,
	EventShutdownCancelled
    };

    enum ClientIdType {
	ClientId = 0,
	PrevClientId
    };

    void initSession (char *smPrevClientId);

    void closeSession (void);

    char *getSessionClientId (ClientIdType type);
};

#endif