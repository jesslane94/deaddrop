#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

char *receivedArgs[64] = { NULL };

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

/*
* void parseLine
* code from csl.mtu.edu/cs4411.ck/www/NOTES/process/fork/exec.html
* this function sets the arguments to pass to exec. 
*/
void parseLine(char* line, char **argv)
{
	while(*line != '\0') 
	{
		//replace whitespaces/tabs/endline with terminating null character
		while(*line == ' ' || *line == '\t' || *line == '\n')
		{
			*line++ = '\0';
		}
		//add to argument array
		*argv++ = line;
		//iterate along line
		while (*line != '\0' && *line != ' ' && *line != '\t' && *line != '\n')
		{
			line++;
		}
	}
	//get rid of trailing character/set last argument to null
	*argv--;
	*argv = NULL;
}

int main(int argc, char *argv[])
{
	pid_t childPid;
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	char buffer[1001];
	struct sockaddr_in serverAddress, clientAddress;

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	//from 4.3 Network Servers - multiserver.c
	while(1)
	{
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept

		if (establishedConnectionFD < 0) 
		{
			error("ERROR on accept");
			exit(1);
		}

		//fork process after acceptance
		int childExitStat;
		childPid = fork();
		switch(childPid)
		{
			case -1:
			{
				perror("problem forking...\n");
				exit(1);
			}
			//in child
			case 0:
			{
				sleep(2);
				// Get the message from the client and display it
				memset(buffer, '\0', 1001);
				charsRead = read(establishedConnectionFD, buffer, 1000); // Read the client's message from the socket
				if (charsRead < 0) error("ERROR reading from socket");
				printf("SERVER: I received this from the client: \"%s\"\n", buffer);
				
				//parse out otp message 
				//post format: user post encryptedtext
				parseLine(buffer, receivedArgs);

				//check if arguments are good
				printf("The received arguments are:\n");
				int i;
				while(receivedArgs[i] != NULL)
				{
					printf("Argument %d is %s\n", i, receivedArgs[i]);
				}

				if(strcmp(receivedArgs[1], "post") == 0)
				{
					char filename[15] = "cipherFile.txt";
					char path[256];
					memset(path, '\0', sizeof(path));
					//write encrypted message to a file	
					FILE* messageFile = fopen(filename, "w");
					int result = fputs(receivedArgs[2], messageFile);
					if(result == EOF)
					{
						fprintf(stderr, "error with fputs.\n");
					}
					//get current directory
					if(getcwd(path, sizeof(path)) != NULL)
					{
						printf("current dir is: %s\n", path);
					}
					else
					{
						perror("error with getcwd\n");
					}
					//print path to the file to STDOUT
					fprintf(stdout, path);
				}
				else // if receivedArgs[1] == get
				{
					//get form: otp get name key port
					char getMessage[1001];
					memset(getMessage, '\0', sizeof(getMessage));
					char userName[256];
					memset(userName, '\0', sizeof(userName));
					//receive user name
					strcpy(userName, receivedArgs[2]);
					//retrieve oldest file for this user
					//send file to otp
					charsRead = write(establishedConnectionFD, getMessage, strlen(getMessage)*sizeof(char));
					if(charsRead < 0) error("ERROR writing to socket");
					//delete cipher textfile
				}
			
				close(establishedConnectionFD); // Close the existing socket which is connected to the client
				exit(0);
			}
			//parent
			default:
			{
				 childPid = waitpid(-1, &childExitStat, WNOHANG);
			}
		}
	}

	close(listenSocketFD); // Close the listening socket
	return 0; 
}

