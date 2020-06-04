#include <math.h>
#include <stdlib.h>
#include <time.h> 
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
	srand(time(NULL));
	char randChar;
	
	if(argc < 2)
	{
		fprintf(stderr, "not enough arguments!");
		exit(0);
	}

	int keyLength = atoi(argv[1]);
	
	int i;
	//stackoverflow.com/questions/19724346/generate-random-characters-in-c
	for(i=0; i < keyLength; i++)
	{
		randChar = "ABCDEFGHIJKLMNOPQRSTUVWXYZ$"[random () % 27];
		if(randChar == '$')
		{
			randChar = ' ';
		}
		fprintf(stdout, "%c", randChar);
	}
	
	//add newLine
	fprintf(stdout, "\n");

	return 0;
}

