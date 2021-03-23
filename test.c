#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	if (argc < 2)
       	{
		printf("Incorrect number of arguments\n");
		return 1;
	}

	int num = atoi(argv[1]);
	if (num == 0) 
	{
		printf("Invalid Argument\n");
		return 1;
	}

	for (int i = 0; i < num; i++) 
	{
		printf("Hello World\n");
	}	
	return 0;
}
