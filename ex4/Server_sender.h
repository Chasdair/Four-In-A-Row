#ifndef SERVER_SND_H
#define SERVER_SND_H

#include "Client_h.h"
#define BOARD_HEIGHT 6
#define BOARD_WIDTH  7
DWORD ServerSendThread(struct thread_data *inData);
int CheckMove(char* move, int playerIdx, int turnOfPlayer);
int PlayMove(char* move, int playerNum);
int CheckIfWinner(int playerNum, int widthCoord, int hightCoord);
int CheckDirection(int playerNum, int widthCoord, int hightCoord, int upDown, int leftRight);
int CheckIfTie();
void SendCommand(char* SendStr, struct thread_data *inData);
void propogateStartFlag();
int AssignUserNames(char* userName, struct thread_data *inData);
HANDLE GetGameStartEventHandle();
#endif