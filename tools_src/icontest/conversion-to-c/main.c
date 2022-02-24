#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(int argc, char *argv[])
{
    //if (argc < 2)
    //{
    //    printf("Usage: iconconv <bitmapfile> <output_name>\n");
    //    return 0;
    //}

    int size = sizeof(argv[1]);


    printf("input: '%s'\n", argv[0]);
    printf("size: %d\n", size);
    printf("argv[size]: %c\n", argv[size]);

    printf("'");
    for (int i = 0; i < size; i++)
    {
        printf("%c", argv[i]);
    }
    printf("'\n");

    //if (argv[size] == "h")
    //{
    //    printf("HEADER\n");
    //}
    //else
    //{
    //    printf("YOU STINK\n");
    //}
    

    //printf("bmp: %s\n", bmp);
    //printf("output: %s\n", output);
    //printf("outputFile: %s\n", outputFile);
}