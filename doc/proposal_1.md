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
// Return an error code (negative number) if something failed.
int FiRead(int fd, void* pBuf, size_t nBytes);

// Return an error code (negative number) if something failed. Make the pBuf pointer const.
int FiWrite(int fd, const void* pBuf, size_t nBytes);

// Return 0 for 'successfully retrieved an entry', 1 for 'directory reached its end', or a negative number for an error code.
int FiReadDir(DirEnt* pSpace, int dd);

// Put 'fdsOut' first, to be consistent with pipe2()'s definition
int FiCreatePipe(int pFDsOut[2], int nOpenFlags);
```

An additional proposal would be to update some definitions for internal calls that file system drivers may
provide:
```cpp
// Return an int instead of a uint32_t. Return an error code (negative number) if something failed.
int FsRead(FileNode* pNode, uint32_t offset, uint32_t size, void* pBuf, bool bBlock);

// Return an error code (negative number) if something failed. Make the pBuf pointer const.
int FsWrite(FileNode* pNode, uint32_t offset, uint32_t size, const void* pBuf, bool bBlock);

// Return an error code if a file could not be opened.
int FsOpen (FileNode* pNode, bool bRead, bool bWrite);

// Return an error code if a directory could not be opened.
int FsOpenDir(FileNode* pNode);

// Return an integer instead of echoing back the DirEnt* pointer. See FiReadDir for details.
int FsReadDir(FileNode* pNode, uint32_t* index, DirEnt* pOutputDent);

// Return an integer, and place the file node inside of a separate FileNode* pointer.
int FsFindDir(FileNode* pNode, const char* pName, FileNode** pOutNode);
```
\* - the "style" here being that return value pointers go first, then the rest of the function arguments

