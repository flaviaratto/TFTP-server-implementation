# TFTP-server-implementation
Implement a Trivial File Transfer Protocol (TFTP) server


ECEN602 Programming Assignment 3
1. Flavia Ratto
2. Eric Lloyd Robles

## Assignment Overview
TFTP is a simple method of transferring files between two systems that uses the User Datagram Protocol (UDP). The implemented server must be capable of handling multiple simultaneous clients. A standard TFTP client is used.

The below test have been done on the TFTP server - 
* Transfer a binary file of 2048 bytes and check that it matches the source file
* Transfer a binary file of 2047 bytes and check that it matches the source file
* Transfer a netascii file that includes two CR’s and check that the resulting file matches the input file
* Transfer a binary file of 34 MB and see if block number wrap-around work
* Check that you receive an error message if you try to transfer a file that does not exist and that your server cleans up and the child process exits
* Connect to the TFTP server with three clients simultaneously and test that the transfers work correctly
* Terminate the TFTP client in the middle of a transfer and see if your TFTP server recognizes after 10 timeouts that the client is no longer there 

## Steps to compile and run

1. In order to compile the code, make sure that the files you want to read from the server are in the same folder as that of the TFTP server. Make sure that the makefile is also in the same directory/folder/. Type in the command:
    ```
    make
    ```
2. To start the server, type in the command line:
    ```
    ./server server_ip server_port
    ```
   The server is ready to accept connections. 
3. Now, open another terminal and go to the client directory. Make sure the client and server folders are separate. The client and server should be running in different folders.    Type in the terminal: 
   ```
    tftp
   ```
   Connect to the server by typing in the terminal:
   ```
    connect IP_Address Port_Number
   ```
   You can check the status of the client using the command status, verbose mode on using the command verbose and packet tracing on using trace. You can also change the mode of    transfer by using the command binary for octet and ascii for netascii. 
6. The client can send a RRQ to the server using the command 
   ```
   get file_name
   ```

**Note:** - The code is written in C and is compiled and tested in a Linux environment.
