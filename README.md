                  Four-In-A-Row
This a classic 8x8 four-in-a-row-game.
The .exe should be executed firstly in server mode, and than twice in client mode.
                 Thread Division
```
  Client1                     Server                    Client2 

-----------          -----------------------          -----------
|         |          |          |          |          |         |
| Sender  | -------> | Reciever | Reciever | <------- | Sender  |
| Thread  |          | Thread   | Thread   |          | Thread  | 
|         |          |          |          |          |         |
-----------          |----------|----------|          -----------
|         |          |          |          |          |         |
| Reciever|          | Sender   |  Sender  |          | Reciever|
| Thread  | <------- | Thread   |  Thread  | -------> | Thread  |
|         |          |          |          |          |         |
-----------          -----------------------          -----------
```
