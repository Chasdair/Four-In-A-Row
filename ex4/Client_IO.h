#ifndef CLIENT_IO_H
#define CLIENT_IO_H

#include "Client_h.h"

HANDLE GetClearToSendMsgEvent();
DWORD IOThread(void);
void getUsername();


#endif // CLIENT_IO_H
