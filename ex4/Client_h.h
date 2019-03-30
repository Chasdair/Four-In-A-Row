#ifndef CLIENT_H_H
#define CLIENT_H_H
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#define ERROR_CODE ((int)(-1))

#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <stdlib.h>
#include <Windows.h>

#include "SocketSendRecvTools.h"
#include "Client_IO.h"
#include "Client_rcv.h"
#include "Client_send.h"
#include "Server_rcv.h"
#include "Server_sender.h"
#include "FifoUtil.h"
#include "main.h"

SOCKET m_socket;
char userName[31];
char playerNum[2];
char *inputMode;
char* inputFile;
char* logFile;
char toSend[101];
FILE *logFileHandle;
FILE *inFileHandle;
HANDLE logFileMutexHandle;
HANDLE GetClearToAcceptMsgEvent();
HANDLE GetClearToAcceptMsgEventFile();
HANDLE GetEndOfTheWorldEvent();
HANDLE ThreadHandles[4];

void printToLogMutexed(char* toPrint);
int MainClient(int serverPort);
#endif // CLIENT_H