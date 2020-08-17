/*************************************************************************************
 * Author: Mae LaPresta
 * Title: otp_enc
 * Description: Sends a message to encode and a key to encode it with to a server, and
 *              receives the encoded message in return.
 *************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

void error(const char *msg) { perror(msg); exit(1); } 
int verifyString(char *inputString, int length);


int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[1000];
    char *totalMsg;
    char *totalKey;
    char encodedMsg[70000];
    uint32_t msgLength;
    uint32_t msgLengthbeforeConvert;
    uint32_t keyLength;
    int file_descriptor;
    ssize_t nread;
    char confirm[3];
    int charsRecCount;
    char identityMsg[]="enc";
    
	if (argc < 4) { fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]); exit(0); } 

    //Open the file containing the message to encode
    file_descriptor = open(argv[1], O_RDONLY);
    if (file_descriptor<0){
        fprintf(stderr, "Could not open %s\n", argv[1]);
        exit(1);
    }


    lseek(file_descriptor, 0, SEEK_SET); 
    msgLengthbeforeConvert=lseek(file_descriptor, 0, SEEK_END);
    totalMsg=malloc(msgLengthbeforeConvert*sizeof(char));
    memset(totalMsg, '\0', msgLengthbeforeConvert);
    lseek(file_descriptor, 0, SEEK_SET);
    nread = read(file_descriptor, totalMsg, msgLengthbeforeConvert);
    close(file_descriptor);
    totalMsg[msgLengthbeforeConvert-1]='\0';    
    
    //Open the file containing the key to encode
    file_descriptor = open(argv[2], O_RDONLY);
    if (file_descriptor<0){
        fprintf(stderr, "Could not open %s\n", argv[2]);
        exit(1);
    }
    lseek(file_descriptor, 0, SEEK_SET);                
    keyLength=lseek(file_descriptor, 0, SEEK_END);     
    if (keyLength<msgLengthbeforeConvert) {           
        fprintf(stderr, "Error: key '%s' is too short\n", argv[2]);                
        exit(1);
    }
    lseek(file_descriptor, 0, SEEK_SET); 
    totalKey=malloc(msgLengthbeforeConvert*sizeof(char));  
    memset(totalKey, '\0', msgLengthbeforeConvert); 
    nread = read(file_descriptor, totalKey, msgLengthbeforeConvert);
    close(file_descriptor);
    totalKey[msgLengthbeforeConvert-1]='\0'; 
    //verify that all of the characters in the message string are either an uppercase alphabet character or a space
    if (!verifyString(totalMsg, msgLengthbeforeConvert)){
        fprintf(stderr, "otp_enc error: input contains bad characters\n" );
        exit(1);
    };
    //verify that all of the characters in the key string are either an uppercase alphabet character or a space
    if (!verifyString(totalKey, msgLengthbeforeConvert)){
        fprintf(stderr, "otp_enc error: input contains bad characters\n" );
        exit(1);
    }
    
    

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); 
	portNumber = atoi(argv[3]); 
	serverAddress.sin_family = AF_INET; 
	serverAddress.sin_port = htons(portNumber);
	serverHostInfo = gethostbyname("localhost"); 
	if (serverHostInfo == NULL) { fprintf(stderr, "Error: could not contact otp_enc_d on port %d", portNumber); exit(2); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); 

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); 
	if (socketFD < 0) error("CLIENT: ERROR opening socket");
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
		fprintf(stderr, "Error: could not contact otp_enc_d on port %d", portNumber);
        exit(2);
    }
    
    // Send message of client ID to server
    charsWritten = send(socketFD, identityMsg, strlen(identityMsg), 0); 
    if (charsWritten < 0) error("CLIENT: ERROR writing to socket\n");
    
    
    //get the server response to whether the connection is OK or not
    memset(confirm, '\0', sizeof(confirm));
    memset(buffer, '\0', sizeof(buffer));
    charsRead = recv(socketFD, buffer, sizeof(buffer)-1, 0); 
    if (charsRead < 0) error("CLIENT: ERROR reading from socket\n");
    strcpy(confirm, buffer);
    if (strcmp(confirm, "OK")){
        fprintf(stderr, "Error: could not contact otp_enc_d on port %d\n", portNumber);
        exit(2);
    }
  
    
    

	//send the message length to the server
    msgLength=htonl(msgLengthbeforeConvert);
    charsWritten = send(socketFD, &msgLength, sizeof(uint32_t), 0); 
    if (charsWritten < 0) error("CLIENT: ERROR writing to socket\n");
    
    //get the server confirmation that it is ok to send the message
    memset(buffer, '\0', sizeof(confirm));                  
    charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
    if (charsRead < 0) error("CLIENT: ERROR reading from socket\n");
    
    //send the message to be encoded to the server
    charsWritten = send(socketFD, totalMsg, strlen(totalMsg), 0); 
    if (charsWritten < 0) error("CLIENT: ERROR writing to socket\n");
    
    //get confimation that the server is ready to receive the key
    memset(buffer, '\0', sizeof(buffer));
    charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
    if (charsRead < 0) error("CLIENT: ERROR reading from socket\n");
    
    //send the key to the server
    charsWritten = send(socketFD, totalKey, strlen(totalKey), 0); 
    if (charsWritten < 0) error("CLIENT: ERROR writing to socket\n");
    
    //get the encoded message from the server
    charsRecCount=0;
    memset(encodedMsg, '\0', strlen(encodedMsg));  
    while (charsRecCount<msgLengthbeforeConvert-1){
        memset(buffer, '\0', 1000);
        charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
        if (charsRead < 0) error("ERROR reading from socket\n");
        charsRecCount+=charsRead;
        strcat(encodedMsg, buffer);
    }
    
    printf("%s\n", encodedMsg);

	close(socketFD);    
    free(totalMsg);
    free(totalKey);
	return 0;
}

int verifyString(char *inputString, int length){
    int i;
    int valid=1;
    for (i=0; i<length-1; i++){
        if (!((inputString[i]>='A'&&inputString[i]<='Z')||inputString[i]==' ')){
            valid=0;
            break;
        }
    }
    
    return valid;
}
