#ifndef MAIN_H
#define MAIN_H

#include "Client_h.h"

int main(int argc, char** argv);
void mutexedPrintToLogFile(char* errorMessage, DWORD lastError);

int board[BOARD_HEIGHT][BOARD_WIDTH];
BOOL endFlag;
BOOL terminateFlag;
#define SERVER_ADDRESS_STR "127.0.0.1"
#define STRINGS_ARE_EQUAL( Str1, Str2 ) ( strcmp( (Str1), (Str2) ) == 0 )

#endif //MAIN_H
