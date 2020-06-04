#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues

int validateText(char c)
{
	char *accepted = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	int length = strlen(accepted);

	int i;
	for(i=0; i < length; i++)
	{
		if (c == accepted[i])
		{
			return 0;
		}
	}
	return 1;
}

void encrypt(char textFromFile[], char keyFromFile[])
{
		int length = strlen(textFromFile)-1;
		int textInts[length];
		int keyInts[length];

		//char to int the original message
		int i;
		for(i=0; i<length; i++)
		{
			int num = textFromFile[i] - '0';
			textInts[i] = num;
			//char to int key
			num = keyFromFile[i] - '0';
			keyInts[i] = num;
			//add them together and mod 27 the result
			int temp = (textInts[i] + keyInts[i]) % 27;
			//store result as a char
			textFromFile[i] = temp;
		}
		//tack on newline after encryption
		textFromFile[i] = '\n';
}

int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[256];
    
	if (argc < 3) { fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); exit(0); } // Check usage & args

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	if(strcmp(argv[1], "post") == 0)
	{
		portNumber = atoi(argv[5]); // Get the port number, convert to an integer from a string
	}
	else // it is in get mode
	{
		portNumber = atoi(argv[4]);
	}
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
	{
		error("CLIENT: ERROR connecting to server. Exiting...\n"); 
		exit(2);
	}

	//-----------
	
	if(strcmp(argv[1], "post") == 0)
	{
		int finalSend;
		char postMessage[1000];
		memset(postMessage, '\0', sizeof(postMessage)); 
		char *text = NULL;
		char *key = NULL;
		size_t textSize = 0;
		size_t keySize = 0;
		
		//syntax is: otp post user plaintext key port
		strcpy(postMessage, argv[2]);
		strcat(postMessage, " post");
		//printf("post message so far is: %s\n", postMessage);

		//https://stackoverflow.com/questions/22697407/reading-text-file-into-char-array
		//encrypt the plaintext usin gkey (remember to strip off newline)
		FILE* textFile = fopen(argv[3], "r");
		if(textFile == NULL)
		{
			fprintf(stderr, "could not open text file!");
		}
		//get size of text to compare to key
		fseek(textFile, 0, SEEK_END);
		textSize = ftell(textFile);
		//set file pointer back to beginning
		fseek(textFile, 0, SEEK_SET);
		//get size of key to compare to text
		FILE* keyFile = fopen(argv[4], "r");
		if(keyFile == NULL)
		{
			fprintf(stderr, "could not open key file!");
		}
		fseek(keyFile, 0, SEEK_END);
		keySize = ftell(keyFile);
		//set file pointer back to beginning
		fseek(keyFile, 0, SEEK_SET);
		//error if key is too short for the file
		if (textSize > keySize)
		{
			fprintf(stderr,"error: key is too short.\n");
			exit(1);
		}
		
		//------
		//PUT CONNECT HERE?!

		text = malloc((textSize+1)*sizeof(*text));
		key = malloc((keySize+1)*sizeof(*key));

		//copy contents of file into an array for manipulation
		fread(text, textSize, 1, textFile);
		fread(key, keySize, 1, keyFile);

		printf("Text so far is: %s\n", text);
		printf("Key so far is: %s\n", key);

		//check if text is valid to be encrypted
		int length = strlen(text)-1;
		int i;
		for(i=0; i < length; i++)
		{
			if (validateText(text[i]) == 1)
			{
				fprintf(stderr, "input contains bad characters\n");
				exit(1);
			}
		}

		//encrypt message
		encrypt(text, key);
		
		//do only 1000 to not overflow the send buffer
		strncpy(postMessage, text, 1000);
		postMessage[1000] = '\0';

		//check what we got
		printf("After encryption, the text is: %s\n", text);
		
		printf("Message being sent is: %s\n", postMessage);
		
		//send username and the encrypted message to otp_d
		finalSend = write(socketFD, postMessage, strlen(postMessage)*sizeof(char));
		
		fclose(textFile);
		fclose(keyFile);
		free(text);
		free(key);
	}
	else //argv[1] = get
	{
		printf("you have ended up in get somehow.\n");
		//syntax is: otp get user key port
		//user is the name you want otp_d to retrieve an encrypted message for
		//send request for a message for user to otp_d
		//if msg has bad characters or key file is shorter than plaintext, send error to stderr and set exit value to 1
		//use key to decrypt message
		//print decrypted message to stdout
		//if no message, report an error stderr
	}

	close(socketFD); // Close the socket
	return 0;
}
