/*****************************************
		NanoShell Operating System
		  (C) 2021 iProgramInCpp

      Printing and formatting module
******************************************/
#include <print.h>
#include <string.h>
#include <vga.h>

void uns_to_str(uint64_t num, char* str, int paddingInfo, char paddingChar)
{
	// print the actual digits themselves
	int i = 0;
	while (num || i == 0)
	{
		str[i++] = '0' + (num % 10);
		str[i]   = '\0';
		num /= 10;
	}

	// append padding too
	for (; i < paddingInfo; )
	{
		str[i++] = paddingChar;
		str[i]   = '\0';
	}

	// reverse the string
	int start = 0, end = i - 1;
	while (start < end)
	{
		char
		temp       = str[start];
		str[start] = str[end];
		str[end]   = temp;
		start++;
		end--;
}
	}
void int_to_str(int64_t num, char* str, int paddingInfo, char paddingChar)
{
	if (num < 0)
	{
		str[0] = '-';
		uns_to_str((uint64_t)(-num), str + 1, paddingInfo, paddingChar);
	}
	else
		uns_to_str((uint64_t)  num,  str,     paddingInfo, paddingChar);
}

// features:
// %s, %S = prints a string
// %c, %C = prints a char
// %d, %i, %D, %I = prints an int32
// %u, %U = prints a uint32
// %l = prints a uint64
// %L = prints an int64
// %x, %X = prints a uint32 in hex in lowercase or uppercase respectively
// %q, %Q = prints a uint64 in hex in lowercase or uppercase respectively
// %b, %B = prints a uint16 in hex in lowercase or uppercase respectively
// %w, %W = prints a uint8  in hex in lowercase or uppercase respectively
// %p, %P = prints a pointer address (fully platform dependent)

size_t vsnprintf(char* buf, size_t sz, const char* fmt, va_list args)
{
	int  paddingInfo = -1;
	char paddingChar = ' ';
	size_t currentIndex = 0;
	while (*fmt)
	{
		char m = *fmt;
		if (!m) goto finished;
		fmt++;

		if (m == '%')
		{
			m = *(fmt++);

			// if hit end, return
			if (!m) goto finished;

			// handle %0 or %.
			if (m == '0' || m == '.')
			{
				// this by default handles %0<anything> too, though it does nothing
				paddingInfo = 0;
				m = *(fmt++);

				// if hit end, return
				if (!m) goto finished;

				// handle %0D<anything> cases (D = digit)
				if (m >= '0' && m <= '9')
				{
					paddingInfo = m - '0';
					paddingChar = '0';
					m = *(fmt++);
				}
			}
			else if (m >= '1' && m <= '9')
			{
				paddingInfo = m - '0';
				paddingChar = ' ';
				m = *(fmt++);

				// if hit end, return
				if (!m) goto finished;
			}

			switch (m)
			{
				// Format a string
				case 's': case 'S':
				{
					const char* pString = va_arg(args, const char*);

					//allow user to print null
					if (pString == NULL)
						pString = "(null)";

					while (*pString)
					{
						if (currentIndex >= sz - 1)
							goto finished;

						// place this character here
						buf[currentIndex++] = *pString;

						pString++;
					}

					break;
				}
				// Escape a percentage symbol
				case '%':
				{
					if (currentIndex >= sz - 1)
						goto finished;

					buf[currentIndex++] = '%';
					break;
				}
				// Format a char
				case 'c': case 'C':
				{
					// using va_arg(args, char) has undefined behavior, because
					// char arguments will be promoted to int.
					char character = (char)va_arg(args, int);

					if (currentIndex >= sz - 1)
						goto finished;

					buf[currentIndex++] = character;
					break;
				}
				// Format an int
				case 'd': case 'i': case 'D': case 'I':
				{
					int num = va_arg(args, int);
					char buffer[20];
					
					int_to_str(num, buffer, paddingInfo, paddingChar);

					const char* pString = buffer;
					while (*pString)
					{
						if (currentIndex >= sz - 1)
							goto finished;

						// place this character here
						buf[currentIndex++] = *pString;

						pString++;
					}

					break;
				}
				// Format an unsigned int
				case 'u': case 'U':
				{
					uint32_t num = va_arg(args, uint32_t);
					char buffer[20];
					
					uns_to_str(num, buffer, paddingInfo, paddingChar);

					const char* pString = buffer;
					while (*pString)
					{
						if (currentIndex >= sz - 1)
							goto finished;

						// place this character here
						buf[currentIndex++] = *pString;

						pString++;
					}

					break;
				}
				// Format an unsigned 64-bit int
				case 'l':
				{
					uint64_t num = va_arg(args, uint64_t);
					char buffer[30];
					
					uns_to_str(num, buffer, paddingInfo, paddingChar);

					const char* pString = buffer;
					while (*pString)
					{
						if (currentIndex >= sz - 1)
							goto finished;

						// place this character here
						buf[currentIndex++] = *pString;

						pString++;
					}

					break;
				}
				// Format a signed 64-bit int
				case 'L':
				{
					int64_t num = va_arg(args, int64_t);
					char buffer[30];
					
					int_to_str(num, buffer, paddingInfo, paddingChar);

					const char* pString = buffer;
					while (*pString)
					{
						if (currentIndex >= sz - 1)
							goto finished;

						// place this character here
						buf[currentIndex++] = *pString;

						pString++;
					}

					break;
				}
				// Format a uint8_t as lowercase/uppercase hexadecimal
				case 'b':
				case 'B':
				{
					const char* charset = "0123456789abcdef";
					if (m == 'B')
						charset = "0123456789ABCDEF";

					// using va_arg(args, uint8_t) has undefined behavior, because
					// uint8_t arguments will be promoted to int.
					uint8_t p = (uint8_t) va_arg(args, uint32_t);

					if (currentIndex >= sz - 1) goto finished;
					buf[currentIndex++] = charset[(p & 0xF0) >> 4];
					if (currentIndex >= sz - 1) goto finished;
					buf[currentIndex++] = charset[p & 0x0F];

					break;
				}
				// Format a uint16_t as lowercase/uppercase hexadecimal
				case 'w':
				case 'W':
				{
					const char* charset = "0123456789abcdef";
					if (m == 'W')
						charset = "0123456789ABCDEF";

					// using va_arg(args, uint16_t) has undefined behavior, because
					// uint16_t arguments will be promoted to int.
					uint16_t p = (uint16_t) va_arg(args, uint32_t);

					for (uint32_t mask = 0xF000, bitnum = 12; mask; mask >>= 4, bitnum -= 4)
					{
						if (currentIndex >= sz - 1)
							goto finished;

						// place this character here
						buf[currentIndex++] = charset[(p & mask) >> bitnum];
					}

					break;
				}
				// Format a uint32_t as lowercase/uppercase hexadecimal
				case 'x':
				case 'X':
#if SIZE_MAX == 0xFFFFFFFFu //todo: a proper way
				case 'p': case 'P':
#endif
				{
					const char* charset = "0123456789abcdef";
					if (m == 'X' || m == 'P')
						charset = "0123456789ABCDEF";

					uint32_t p = va_arg(args, uint32_t);

					for (uint32_t mask = 0xF0000000, bitnum = 28; mask; mask >>= 4, bitnum -= 4)
					{
						if (currentIndex >= sz - 1)
							goto finished;

						// place this character here
						buf[currentIndex++] = charset[(p & mask) >> bitnum];
					}

					break;
				}
				// Format a uint64_t as lowercase/uppercase hexadecimal
				case 'q':
				case 'Q':
#if SIZE_MAX == 0xFFFFFFFFFFFFFFFFull //todo: a proper way
				case 'p': case 'P':
#endif
				{
					const char* charset = "0123456789abcdef";
					if (m == 'Q' || m == 'P')
						charset = "0123456789ABCDEF";

					uint32_t p = va_arg(args, uint32_t);

					for (uint64_t mask = 0xF000000000000000ULL, bitnum = 60; mask; mask >>= 4, bitnum -= 4)
					{
						if (currentIndex >= sz - 1)
							goto finished;

						// place this character here
						buf[currentIndex++] = charset[(p & mask) >> bitnum];
					}

					break;
				}
			}
		}
		else
		{
			if (currentIndex >= sz - 1)
				goto finished;
			buf[currentIndex++] = m;
		}
	}
finished:
	buf[currentIndex] = '\0';
	return currentIndex;
}

size_t vsprintf(char* buf, const char* fmt, va_list args)
{
	return vsnprintf(buf, SIZE_MAX, fmt, args);
}

size_t snprintf(char* buf, size_t sz, const char* fmt, ...)
{
	va_list lst;
	va_start(lst, fmt);

	size_t val = vsnprintf(buf, sz, fmt, lst);

	va_end(lst);

	return val;
}

size_t sprintf(char* buf, const char* fmt, ...)
{
	va_list lst;
	va_start(lst, fmt);

	size_t val = vsprintf(buf, fmt, lst);

	va_end(lst);

	return val;
}


extern int g_textX, g_textY;

void DumpBytesAsHex (void *nAddr, size_t nBytes, bool as_bytes)
{
	int ints = nBytes/4;
	if (ints > 1024) ints = 1024;
	if (ints < 4) ints = 4;
	
	uint32_t *pAddr  = (uint32_t*)nAddr;
	uint8_t  *pAddrB = (uint8_t*) nAddr;
	for (int i = 0; i < ints; i += (8 >> as_bytes))
	{
		for (int j = 0; j < (8 >> as_bytes); j++)
		{
			if (as_bytes)
			{
				LogMsgNoCr("%b %b %b %b ", pAddrB[((i+j)<<2)+0], pAddrB[((i+j)<<2)+1], pAddrB[((i+j)<<2)+2], pAddrB[((i+j)<<2)+3]);
			}
			else
				LogMsgNoCr("%x ", pAddr[i+j]);
		}
		for (int j = 0; j < (8 >> as_bytes); j++)
		{
			#define FIXUP(c) ((c<32||c>126)?'.':c)
			char c1 = pAddrB[((i+j)<<2)+0], c2 = pAddrB[((i+j)<<2)+1], c3 = pAddrB[((i+j)<<2)+2], c4 = pAddrB[((i+j)<<2)+3];
			LogMsgNoCr("%c%c%c%c", FIXUP(c1), FIXUP(c2), FIXUP(c3), FIXUP(c4));
		}
		LogMsg("");
	}
}

