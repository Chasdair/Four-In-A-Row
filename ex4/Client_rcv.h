#ifndef CLIENT_RCV_H
#define CLIENT_RCV_H

#include "Client_h.h"

#define RED_PLAYER 1
#define YELLOW_PLAYER 2

#define BOARD_HEIGHT 6
#define BOARD_WIDTH  7

#define BLACK  15
#define RED    204
#define YELLOW 238

void PrintBoard(int board[][BOARD_WIDTH], HANDLE consoleHandle);

struct String100 {
	char word[101];
} string100;

DWORD RecvDataThread(void);
BOOL gameIsOn;

#endif // CLIENT_RCV_H
