
//Legacy errors
enum
{
	ENOTHING,      // No Error
	EACCES,        // Permission denied
	EEXIST,        // File exists
	EINTR,         // Interrupted system call
	EINVAL,        // Invalid argument
	EIO,           // I/O error
	EISDIR,        // Is a directory
	ELOOP,         // Symbolic link chain too deep
	EMFILE,        // Too many open files
	ENAMETOOLONG,  // File or path name too long
	ENFILE,        // Too many open files in system
	ENOENT,        // No such file or directory
	ENOSR,         // Out of stream resources
	ENOSPC,        // No space left on device
	ENOTDIR,       // Not a directory
	ENXIO,         // No such device or address
	EOVERFLOW,     // Value too large for defined data type
	EROFS,         // Read only file system
	EAGAIN,        // No more processes
	ENOMEM,        // Not enough memory
	ETXTBUSY,      // Text file busy
	EBADF,         // Bad file descriptor
	ESPIPE,        // Illegal seek
	EIEIO,         // Computer bought the farm
	ENOTSUP,       // Operation not supported
	EXDEV,         // Cross device operation not supported
	EBUSY,         // Resource is busy
	ENOTEMPTY,     // Directory is not empty
	ENOTTY,        // Invalid input/output control request
	ERANGE,        // Range error
	EDOM,          // Domain error
	ECOUNT,
};
	
#define ERR_NOTHING                (-ENOTHING)
#define ERR_ACCESS_DENIED          (-EACCES)
#define ERR_FILE_EXISTS            (-EEXIST)
#define ERR_INTERRUPTED            (-EINTR)
#define ERR_INVALID_PARM           (-EINVAL)
#define ERR_IO_ERROR               (-EIO)
#define ERR_IS_DIRECTORY           (-EISDIR)
#define ERR_TOO_MANY_SYMLINKS      (-ELOOP)
#define ERR_TOO_MANY_OPEN_FILES    (-EMFILE)
#define ERR_NAME_TOO_LONG          (-ENAMETOOLONG)
#define ERR_TOO_MANY_OPEN_FILES_S  (-ENFILE)
#define ERR_NO_FILE                (-ENOENT)
#define ERR_NO_STREAM_RESOURCES    (-ENOSR)
#define ERR_NO_SPACE_LEFT          (-ENOSPC)
#define ERR_NOT_DIRECTORY          (-ENOTDIR)
#define ERR_NO_SUCH_DEVICE         (-ENXIO)
#define ERR_OVERFLOW               (-EOVERFLOW)
#define ERR_READ_ONLY_FS           (-EROFS)
#define ERR_NO_MORE_PROCESSES      (-EAGAIN)
#define ERR_NO_MEMORY              (-ENOMEM)
#define ERR_TEXT_FILE_BUSY         (-ETXTBUSY)
#define ERR_BAD_FILE_DES           (-EBADF)
#define ERR_ILLEGAL_SEEK           (-ESPIPE)
#define ERR_NOT_SUPPORTED          (-ENOTSUP)
#define ERR_CROSS_DEVICE           (-EXDEV)

enum
{
	ERR_CANCELED      = 0x80000002, ECANCELED = -ERR_CANCELED,
	ERR_FILE_TOO_BIG  = 0x80000003, EFBIG     = -ERR_FILE_TOO_BIG,
};

const char *GetErrNoString(int errno);
