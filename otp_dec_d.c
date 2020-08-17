/*************************************************************************************
 * Author: Mae LaPresta
 * Title: otp_dec_d.c
 * Description: Communicates with opt_dec, gets sent an encoded message and the key
 *              used to encode the message. Decodes the  message with the given key,
 *              and returns the decoded message.
 *************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void error(const char *msg) { perror(msg); exit(1); }

int main(int argc, char *argv[])
{
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead, charsWritten;
	socklen_t sizeOfClientInfo;
    int numChildren = 0;
    int i;
    char buffer[1000];
    char totalMsg[70000];
    char totalKey[70000];
    char decodedMsg[70000];
    char clientID[4];
    char verifyID[]="dec";
    char ready[]="OK";
    char refuse[]="NO";
    uint32_t msgLength;
    uint32_t msgLength_beforeConvert;
    pid_t activeSpawn[5]={-5};
	struct sockaddr_in serverAddress, clientAddress;
    int charsRecCount;
    
	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } 

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); 
	portNumber = atoi(argv[1]); 
	serverAddress.sin_family = AF_INET; 
	serverAddress.sin_port = htons(portNumber); 
	serverAddress.sin_addr.s_addr = INADDR_ANY; 

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) 
		error("ERROR on binding");
	listen(listenSocketFD, 5); 

    while(1){
	
	    sizeOfClientInfo = sizeof(clientAddress); 
	    establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
	    if (establishedConnectionFD < 0) error("ERROR on accept");
        
        pid_t spawnPid = -5;
        int childExitStatus = -5;
        pid_t childPID;
        
        do {
            childPID = waitpid(-1, &childExitStatus, WNOHANG);
            if (childPID>0){
                numChildren--;
                for (i=0; i<5; i++){
                    if (activeSpawn[i]==childPID){
                        activeSpawn[i]=-5;
                        break;
                    }
                }
                
            }
                
        }while (childPID>0);
        
        if (numChildren>4){
            childPID = waitpid(-1, &childExitStatus, 0);
            numChildren--;
                for (i=0; i<5; i++){
                    if (activeSpawn[i]==childPID){
                        activeSpawn[i]=-5;
                        break;
                    }
                }
        }
        spawnPid=fork();
        switch (spawnPid) {
            case -1: { perror("Hull Breach!\n"); exit(1); break; }
            case 0: {
                memset(clientID, '\0', 4);
                memset(totalKey, '\0', sizeof(totalKey));
                memset(totalMsg, '\0', sizeof(totalMsg));
                memset(decodedMsg, '\0', sizeof(decodedMsg));

                //Obtain the client identity:
                charsRead = recv(establishedConnectionFD, clientID, strlen(clientID)-1, 0);
                if (charsRead < 0) error("ERROR reading from socket");
                //Refuse the connection if the client is not "enc"
                if (strcmp(verifyID, clientID)) {
                    charsWritten = send(establishedConnectionFD, refuse, strlen(refuse), 0);
                    close(establishedConnectionFD);
                    exit(1);
                }
                
                //Send confirmation that server is ready to receive message length
                charsWritten = send(establishedConnectionFD, ready, strlen(ready), 0); 
                if (charsRead < 0) error("ERROR writing to socket");
                
                msgLength_beforeConvert=0;
                msgLength=0;
                charsRead = recv(establishedConnectionFD, &msgLength_beforeConvert, sizeof(uint32_t), 0);
                if (charsRead < 0) error("ERROR reading from socket");
                msgLength=ntohl(msgLength_beforeConvert);
                
                //Send confirmation that server is ready to receive message:
                charsWritten = send(establishedConnectionFD, ready, strlen(ready), 0); 
                if (charsWritten < 0) error("ERROR writing to socket");
                
                charsRecCount=0;                                                   
                //continue receiving the message from client until entire message is received.
                while (charsRecCount<msgLength-1){
                    memset(buffer, '\0', 1000);                                    
                    charsRead = recv(establishedConnectionFD, buffer, sizeof(buffer) - 1, 0); 
                    if (charsRead < 0) error("ERROR reading from socket");
                    charsRecCount+=charsRead;                                          
                    strcat(totalMsg, buffer);                                          
                }
                
                //Send confirmation that server is ready to receive key:
                charsWritten = send(establishedConnectionFD, ready, strlen(ready), 0); // Send success back
                if (charsWritten < 0) error("ERROR writing to socket");
                
                charsRecCount=0;                                                        
                //continue receiving the message from client until entire message is received.
                while (charsRecCount<msgLength-1){
                    memset(buffer, '\0', 1000);                                    
                    charsRead = recv(establishedConnectionFD, buffer, sizeof(buffer) - 1, 0);
                    if (charsRead < 0) error("ERROR reading from socket");
                    charsRecCount+=charsRead;                                          
                    strcat(totalKey, buffer);                                         
                }
                
                for (i=0; i<msgLength-1; i++){
                    char difference;
                    //for each character in message received
                    //if character is a space, set to numerical value 26
                    if (totalMsg[i]==32) { totalMsg[i]=26; }
                    //else it is an alphbet characted A-Z. subtract 65 to have 'A'=0, 'B'=1... 'Y'=24, 'Z'=25
                    else totalMsg[i]-=65;
                    if (totalKey[i]==32) { totalKey[i]=26; }
                    else totalKey[i]-=65;
                    
                    difference=(totalMsg[i]-totalKey[i]);
                    if (difference<0){difference+=27;}
                    //add the the msg character to the key character, and modulo 27. Then add 65 to get ASCII char
                    // if the numerical value is equal to 91 then it is a space. Set to correct ASCII number
                    decodedMsg[i]=((difference)%27)+65;
                    if (decodedMsg[i]==91) {decodedMsg[i]=32;}
                }
                
                charsWritten = send(establishedConnectionFD, decodedMsg, strlen(decodedMsg), 0); // Send encrypted message back
                if (charsWritten < 0) error("ERROR writing to socket");
                
                close(establishedConnectionFD);
                exit(0);
            }
            
            default:{
                pid_t childPID_actual = waitpid(spawnPid, &childExitStatus, WNOHANG);
 
                if (childPID_actual==0){
                    numChildren++;
                    for (i=0; i<5; i++){
                        if (activeSpawn[i]==-5){ 
                            activeSpawn[i]=spawnPid;
                            break;
                        }
                    }
                }
            }
            
        }
    
	
    }
	close(listenSocketFD); // Close the listening socket
	return 0; 
}




