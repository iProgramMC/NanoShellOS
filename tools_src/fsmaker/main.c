#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <windows.h>

//TODO: linux port?

#define FILE_NAME_MAX 64

typedef struct
{
    unsigned int m_magic;
    char m_fileName[FILE_NAME_MAX];
    unsigned int m_offset, m_length;
} InitRdHeader;

#define MAX_HEADERS 64

int main(int argc, char** argv)
{
    InitRdHeader m_headers[MAX_HEADERS];
    memset(m_headers, 0, sizeof(m_headers));
    printf("NanoShell Initrd builder V1.00 (C) 2022 iProgramInCpp\n");
    if (argc != 3)
    {
        printf("Usage: %s <directory name> <output initrd image>\n", argv[0]);
        return -1;
    }
    char dir[MAX_PATH], dir1[MAX_PATH];
    if (strlen(argv[1]) > (MAX_PATH - 3))
    {
        printf("ERROR: Directory name too long.\n");
        return -1;
    }
    printf("* Target directory: %s\n", argv[1]);
    strcpy(dir, argv[1]);
    strcpy(dir1, dir);
    strcat(dir1, "\\");
    strcat(dir, "\\*");

    HANDLE h_find = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAA file_data;
    h_find = FindFirstFileA(dir, &file_data);

    if (h_find == INVALID_HANDLE_VALUE)
    {
        printf("ERROR: Could not FindFirstFileA directory: %s\n", dir);
    }

    int eindex = 0;
    int offsetoffset = sizeof(int), offsetcurrent = 0;
    do
    {
        if (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            printf("1>%s\t(is a directory, ignoring)\n", file_data.cFileName);
            continue;
        }
        //write the filename:
        InitRdHeader* p = &m_headers[eindex];

        printf("1>%s\t(size %d)\n", file_data.cFileName, (int)file_data.nFileSizeLow);
        int fnlength = strlen(file_data.cFileName);
        if (fnlength > FILE_NAME_MAX - 1)
        {
            printf("1>WARN: File name's length is %d, truncating to %d\n", fnlength, FILE_NAME_MAX - 1);
            fnlength = FILE_NAME_MAX - 1;
        }
        memcpy(p->m_fileName, file_data.cFileName, fnlength + 1LL);
        p->m_offset = offsetcurrent;
        p->m_length = file_data.nFileSizeLow;
        p->m_magic = 0x2A2054E1;
        offsetcurrent += p->m_length;
        offsetoffset  += sizeof(InitRdHeader);

        eindex++;
        if (eindex >= MAX_HEADERS)
        {
            printf("* ERROR: Too many files.  Stopping.\n");
            break;
        }
    }
    while (FindNextFileA(h_find, &file_data) != 0);

    for (int i = 0; i < eindex; i++)
    {
        InitRdHeader* p = &m_headers[i];
        p->m_offset += offsetoffset;
    }

    if (GetLastError() != ERROR_NO_MORE_FILES)
    {
        printf("1>WARN: GetLastError returned: %d\n", GetLastError());
    }

    FILE* pFile = fopen(argv[2], "wb");
    if (!pFile)
    {
        printf("* ERROR: Cannot open `%s' for writing.\n", argv[2]);
        return 1;
    }

    //write the number of headers present:
    fwrite(&eindex, sizeof(int), 1, pFile);

    for (int i = 0; i < eindex; i++)
    {
        //write each header:
        fwrite(&m_headers[i], sizeof(m_headers[i]), 1, pFile);
    }

    for (int i = 0; i < eindex; i++)
    {
        char file[1000];
        strcpy(file, dir1);
        strcat(file, m_headers[i].m_fileName);
        //write each file's contents
        FILE* pStream = fopen(file, "rb");
        if (!pStream)
        {
            printf("* FATAL:  File %s not found\n", file);
            fclose(pFile);
            return -1;
        }
        unsigned char* pBuf = malloc(m_headers[i].m_length);
        if (!pBuf)
        {
            printf("* FATAL: Out of memory while allocating memory of size %d for file %s?\n", m_headers[i].m_length, file);
            fclose(pFile);
            return -1;
        }
        fread(pBuf, 1, m_headers[i].m_length, pStream);
        fclose(pStream);
        fwrite(pBuf, 1, m_headers[i].m_length, pFile);
        free(pBuf);
    }
    printf("* Finished job.  %d files written, %d total size.\n", eindex, offsetcurrent);
    fclose(pFile);

    FindClose(h_find);
}