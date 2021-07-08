#include<sys/socket.h>
#include<sys/types.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<netinet/in.h>
#include<errno.h>
#include<arpa/inet.h>
#include <signal.h>
#include<stdint.h>
#include<sys/time.h>

//Timeout Feature of Server (Waiting for ACK from DATA packet sent)
int timeout(int fd, int sec){
	fd_set rset;
	struct timeval tv;
	
	FD_ZERO(&rset);
	FD_SET(fd, &rset);
	
	tv.tv_sec = sec;
	tv.tv_usec = 0;
	
	return (select(fd + 1, &rset, NULL, NULL, &tv));
}

//Zombie process handling
void sigchld_handler(int s)
{
	 int saved_errno= errno;
	 while(waitpid(-1, NULL, WNOHANG )>0);
	 errno =saved_errno;
}



int main(int argc, char *argv[]){ //command line: echos port (echos is name of program, port is port number)

	//Declare parameters/variables used:
	int timer = 5;
	int sckt; //initial socket descriptor
	struct sockaddr_in my_addr; //server socket address
	struct sockaddr_in nxt_client; //client socket address wanting to connect in port
	socklen_t client_size; // size of socket address
	
	//intialize addresses:
	memset(&my_addr,0,sizeof(my_addr)); 
	memset(&nxt_client, 0, sizeof nxt_client); 
	
	char s[INET_ADDRSTRLEN];
	struct sigaction sa;
  
	//buffer to store recieved datagrams:
	char recv_buff[520];
	memset(recv_buff, 0, sizeof(recv_buff));
	int recvbytes;

	//buffer to store packets to send as datagrams:
	char send_buff[520];
	memset(send_buff, 0, sizeof(send_buff));
	int sendbytes;

	int new_sckt; //child socket descriptor to connect to client

	int yes = 1;
	
	int port = 0; //store port of server

  //Get information from command line:
	if (argc != 3){ //make sure command line sets proper number of arguments
		printf("Please specify the IP address and port number\n");
		return 0;
	}
	else{
		port = atoi(argv[2]);
	}
 
	my_addr.sin_family = AF_INET; //host byte order
	my_addr.sin_port = htons(port); // short, network byte order (atoi to convert input argument from char to integer)
	my_addr.sin_addr.s_addr = inet_addr(argv[1]); //assign server's IP with specified one in command line

	//Establish socket descriptor:
	if ((sckt = socket(AF_INET, SOCK_DGRAM, 0)) <0){ //assign socket descriptor
		printf("Server: Socket could not be created\n"); // handling error
		exit(-1);
	}

	memset(&my_addr.sin_zero, '\0', 8); //clean lingering bytes used previously in address

	//Bind:
	if (bind(sckt, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0){
		printf("Server: Error in binding\n");
		close(sckt);
		exit(-1);
	}
	
	printf("Server: Waiting for connection\n");
 
	//keep receiving requests from clients:
	while(1){
		client_size = sizeof(nxt_client); //
		
		//recieve requests from clients:
		if((recvbytes = recvfrom(sckt, recv_buff, sizeof(recv_buff), 0, (struct sockaddr *)&nxt_client, &client_size)) <0){
			printf("Server: Could not receive over socket\n" );
			exit(-1);
		}
		
		printf("Server: got packet from %s\n",inet_ntop(AF_INET, &nxt_client.sin_addr, s, INET_ADDRSTRLEN));
	  
		//child process to handle:
		if(!fork()){
			
			close(sckt);  //Closing parent socket
      
			sa.sa_handler = sigchld_handler; // reap all dead processes
			sigemptyset(&sa.sa_mask);
			sa.sa_flags = SA_RESTART;
			if (sigaction(SIGCHLD, &sa, NULL) == -1){
			  perror("sigaction");
			  exit(1);
			}

			//create new socket for exchanging datagrams with client:
			struct sockaddr_in servaddr;
			servaddr.sin_family = AF_INET; //host byte order
			servaddr.sin_addr.s_addr = inet_addr(argv[1]); //assign server's IP with specified one in command line
			servaddr.sin_port = htons(0); // short, network byte order (atoi to convert input argument from char to integer)
      
	  
			if ((new_sckt = socket(AF_INET, SOCK_DGRAM, 0)) == -1){ //assign socket descriptor
			 printf("Server: Unable to create socket for child.\n"); // handling error
			 exit(-1);
			}
			
			if (setsockopt(new_sckt, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){ //reuse port if address is already in use
				printf("Server: Error in setsockopt\n"); // handling error
				exit(-1);
			}
			
			//Bind:
			if (bind(new_sckt, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
				printf("Server: Error in binding child socket.\n");
				close(new_sckt);
				exit(-1);
			}
			
			printf("Server: Socket ready\n");
      
			//extract opcode and mode of transfer
			int opcode,mode;
			char fileName[] = " ";
			size_t len_fileName;
			opcode = recv_buff[1];
			strcpy(fileName, &recv_buff[2]);
			len_fileName = strlen(fileName);
			fileName[len_fileName]='\0';
			
			
			//Check for correct mode of transfer:
			if(strcasecmp(&recv_buff[len_fileName+3] , "netascii" ) == 0)
				mode =1 ;
			else if(strcasecmp(&recv_buff[len_fileName+3],"octet") == 0)
				mode =2;
			else{
				
				//Error due to inaccurate mode:
				printf("Server: Inaccurate mode  \n ");
				memset(send_buff , 0, sizeof(send_buff));
        

				send_buff[1] = 5; // Opcode for error ;
				send_buff[3] = 4; // Error number for illegal operation

				stpcpy(&send_buff[4], "Mode is inaccurate!" );
				len_fileName = strlen("Mode is inaccurate!");
					 
				if(sendto(new_sckt,send_buff,len_fileName+5,0,(struct sockaddr *)&nxt_client, client_size)<0){
					printf("Server: Could not send over socket\n" );
					close(new_sckt);
					return(1);
				}
			} 
						
			//For RRQ:
			
				//initialize parameters:
			int timeout_count = 0; //timer for waiting for ACK from client
			FILE *fp; //file pointer
			unsigned short int sent_packnum; //number of packets sent
			sent_packnum = 1; 
			long int offset = 0; //keeps track of position of file pointer from beginning of file
			char nextchar; //store next character to be placed in the packet
			nextchar = -1;
			
			//open file according to file type: binary file for octet and text file for netascii:
			if(mode == 1){
				fp = fopen(fileName,"r");
			}
			else{
				fp = fopen(fileName,"rb");
			}
			
			//file not found error handling:
			if(fp == NULL)
			{
				printf("Server: File does not exist \n");
				memset(send_buff,0,sizeof(send_buff));

				send_buff[1] = 5; // opcode for error
				send_buff[3] = 1; //errnumber for file not found

				strcpy(&send_buff[4],"File does not exist");
				len_fileName = strlen("File does not exist");

				if(sendto(new_sckt,send_buff,len_fileName+5,0,(struct sockaddr *)&nxt_client, client_size)<0)
				{
					printf("Server: Could not send over socket\n" );
					close(new_sckt);
					return(1); 	
					
				}
				//close(new_sckt);
				printf("Client Disconnected\n" );
				continue;
			 
			}
     
			//Interpret transfer mode:
			if (opcode == 1){ //only for RRQs
			
				while(1){ //keep reading until EOF
					
					//parameters to read from intended file:
					int count; //number of bytes read
					char c; //character read
					char translate[512]; //buffer to store data as packets
					memset(&translate, '\0', sizeof(translate)/sizeof(char));
					
					if (mode == 2){ //Handle Octet
						count = fread(translate, 1, 512, fp); //read 512 byte chunks of data from binary file
						
						if (count < 512){ //if data read is less than maximum size of packet, this is the last data packet to send
							c = EOF;
						}
						offset += count; //increment position of file pointer
					}
					else if (mode == 1){ //Handle Netascii
					
						//extract each character in the text file:
						for (count = 0; count < 512; count++){
							
							if (nextchar >= 0){
								translate[count] = nextchar;
								//strncat(translate, &nextchar, 1);
								nextchar = -1;
								continue;
							}
							
							c = getc(fp);
							
							if (c == EOF){
								if (ferror(fp)){
									perror("Server: Read error from getc on local file");
									printf("Server: File byte count: %d", count);
								}
								
								break;
							}
							else if (c == '\n'){ //LR, add CR before
								c = '\r';	
								nextchar = '\n';
							}
							else if (c == '\r'){ //CR, add null after
								nextchar = '\0';
							}
							else{
								nextchar = -1;
							}
							
							translate[count] = c;
							offset += 1;
						}
					}
					
					//prepare packet to be sent:
					char packet[520]; 
					memset(packet,'\0',sizeof(packet));
					packet[0] = 0x00;
					packet[1] = 0x03;
					packet[2] = (sent_packnum & 0xFF00) >>8;
					packet[3] = (sent_packnum & 0x00FF);  
					memcpy(&packet[4],translate,count); 
                 
					//sending packet:
					if(sendto(new_sckt,packet,count+4,0,(struct sockaddr *)&nxt_client, client_size)<0){
						printf("Server: Could not send over socket\n" );
						close(new_sckt);
						exit(-1);
					}
					printf("Server: Packet# %d sent\n", sent_packnum);
					printf("Server: Waiting for ACK from client\n");
					
					//Timeout Feature:
						//use select() to wait for ACK from client with timeout:
					int time_bool = timeout(new_sckt, timer);
					if (time_bool == -1){
						perror("Server: Select Error");
						exit(1);
					}
					
						//recieve ACK from client:
					if (time_bool > 0){ //if not timed out, recieve ACK packet
						memset(recv_buff, 0, sizeof(recv_buff));
						if((recvbytes = recvfrom(new_sckt, recv_buff, sizeof(recv_buff), 0, (struct sockaddr *)&nxt_client, &client_size)) <0){
							printf("Server: Could not receive over socket\n" );
							exit(-1);
						}
						printf("Server: Packet# %d acknowledged\n", sent_packnum);
					}
					else{ //if timed out, count number of times it timed out
						if (timeout_count == 10) //maximum count to time out is 10
						{
							printf("Server: Maximum timeouts have been reached.\n");
							break;
						}
						else
						{
							printf("Server: Timed out. Retry sending packet\n");
							timeout_count++; //increment number of times it timed out
							
							//try to read back the recent packet to be sent:
							offset -= count; 
							fseek(fp, offset, SEEK_SET);//
							continue;
						}
					}
					
					//Check for last packet:
					if (c == EOF){
						printf("Server: Transfer Done\n");
						fclose(fp);
						break;
					}
					
					sent_packnum += 1;
				}
			}
			close(new_sckt); //close socket of child process
			exit(0);
		}   
	}
	close(sckt);
	return 0;
 }