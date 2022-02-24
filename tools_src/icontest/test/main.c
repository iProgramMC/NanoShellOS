#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
	char* str = argv[2];
	int init_size = strlen(str);
	char delim[] = ".";

	char *ptr = strtok(str, delim);

	while(ptr != NULL)
	{
		printf("'%s'\n", ptr);
		ptr = strtok(NULL, delim);
	}
	return 0;
}