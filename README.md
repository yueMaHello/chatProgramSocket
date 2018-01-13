A client-server program that utilizes TCP sockets for communication. Multithreading technique is also used during the implementation.
Design Overview:
The program supports one server and several clients. Each client can send some commands to the server and clients can communicate with each other through a proper input format.
1. The server can be invoked by ‘a3chat -s port nclient’. ‘nclient’ should be no more than 5.
2. The client can be executed by ‘a3chat –c port serverAddress’.
3. After invoking the server, the server will start listen to the client’s connecting request.
The server assigns a thread and a socket to a client to communicate. The server also creates a thread to report the activity periodically. Each client creates a separate thread to keep sending keepalive message to the server.
4. If the server doesn’t receive the keepalive message for five times, the server will say the client has lost its connection.
5. On the server side, an activity report will be displayed periodically every 15 seconds automatically.
6. When the client sends ‘open username’, a new user will open and one inFifo will be locked by this user.
7. ‘who’ command can check who is online.
8. ‘close’ will close the current user without terminating the program.
9. ‘exit’ from the client side will terminate the client process, and ‘exit’ from the server
side will terminate the server.
10. ‘to user1 user2 ....’ Will adds specified users to the list of recipients.
11. ‘<chat line’ will send the message to all specified recipients.
12. Through ‘Makefile’, the user can make, clean and zip the program easily.
Project Status:
The project has been completed with all the functionality and features. There are no known bugs existing in the program currently. There are several difficulties during the implementation:
1. It’s a little bit tricky to totally understand the keepalive message part. I always thought I can directly use some powerful function to finish this part. However, I finally decided to use a thread to handle it.
2. The activity report is also hard to implement. There are quite a few dynamic parameters. I finally learned to use a ‘struct’ to store every message.
Testing and Results:
I tested the application feature by feature. Firstly, I tested whether the socket could be connected correctly. Then I tested whether the client’s messages could get proper responses. I opened one server and several clients, and made clients communicate with the server at the same time. I also tested whether clients could talk to each other through the server. I also tested the activity reports and the keepalive messages by printing some specific characters.
Results: The clients can connect to the server properly. The clients’ commands can get proper responses from the server, such as ‘who’, ‘open’, ‘to’ and etc. The activity reports and the keepalive messages work properly.
Acknowledgements:
1. https://www.tutorialspoint.com/cprogramming/c_strings.htm
2. https://en.wikipedia.org/wiki/Keepalive
3. https://www.cnblogs.com/javawebsoa/archive/2013/05/30/3109122.html 4. http://www.csc.villanova.edu/~mdamian/threads/posixsem.html
5. http://man7.org/linux/man-pages/man7/pthreads.7.html
6. Textbook: Advanced Programming in UNIX Environment
