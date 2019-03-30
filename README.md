                 "# Four-In-A-Row" - Thread Division

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