# Proposal to revamp file system related calls.

While the NanoShell file system API is inspired by POSIX, it in no way behaves like it and some stuff
that you may expect to behave in one way just doesn't. I plan to fix this.

The following proposal will fix that, by forcing an error number to be returned on ALL APIs.

For instance, right now FiRead returns a `size_t` (for how many bytes were read). This is stupid. If we couldn't
read the full amount, we have no idea why, and `errno` isn't defined either.

The reason every API returns an error code is to avert the kernel of having to handle an error number.
(Although this would be trivial, thinking about it. We already track plenty of stuff per thread)

Here is a list of the new function prototypes that will be changed. (The comment immediately preceding
the function prototype will describe what has changed in the prototype.)

```cpp
// Return the file descriptor into an output pointer, instead of directly.
int FiOpen(int* pOutFD, const char* pFileName, int nOpenFlags);

// No change.
int FiClose(int fd);

// Return the amount of bytes that have been read into a separate output pointer.
int FiRead(size_t* pOutRead, int fd, void* pBuf, size_t nBytes);

// Return the amount of bytes that have been written into a separate output pointer.
int FiWrite(size_t* pOutWritten, int fd, const void* pBuf, size_t nBytes);

// Return the offset in the file that the read-write head is at in a separate output pointer.
int FiTell(int* pOutTold, int fd);

// Return the size of the file in a separate output pointer.
int FiTellSize(int* pOutToldSize, int fd);

// No change.
int FiSeek(int fd, int offset, int whence);

// Return the directory descriptor into a separate output pointer.
int FiOpenDir(int* pOutDD, const char* pFileName);

// No change.
int FiCloseDir(int dd);

// Allow the user to provide their own space for directory entries.
int FiReadDir(DirEnt* pSpace, int dd);

// No change.
int FiSeekDir(int dd, int place);

// No change.
int FiRewindDir(int dd);

// Returns the directory 'told' value in a separate output pointer.
int FiTellDir(int* pOutTold, int dd);

// Put 'pOut' first to be consistent with the style.*
int FiStatAt(StatResult* pOut, int dd, const char* pFileName);

// Put 'pOut' first.
int FiStat(StatResult* pOut, const char* pFileName);

// Put 'pOut' first.
int FiLinkStat(StatResult* pOut, const char* pFileName);

// No change.
int FiChangeDir(const char* pFileName);

// No change.
int FiUnlinkFile(const char* pFileName);

// No change.
int FiRename(const char* pFileNameOld, const char* pFileNameNew);

// No change.
int FiMakeDir(const char* pPath);

// No change.
int FiRemoveDir(const char* pPath);

// Put 'fdsOut' first. Remove the 'friendly name' as that's an outdated notion. (FileNodes no longer carry a 'name' property)
int FiCreatePipe(int pFDsOut[2], int nOpenFlags);

// No change.
int FiIoControl(int fd, unsigned long nRequest, void* pArg);
```
\* - the "style" here being that return value pointers go first, then the rest of the function arguments

