#include "lib_header.h"

static void ___swap(char*a, char*b) {
	char e;e=*a;*a=*b;*b=e;
}

void vsprintf(char* memory, const char* format, va_list list) {
	int padding_info = -1; char padding_char = ' ';
	while (*format) {
		char m = *format;
		format++;
		if (m == '%') {
			m = *format++;
			// parse the m
			if (!m) return;
			padding_info = -1;
			if (m == '0' || m == '.')
			{
				padding_info = m - '0';
				m = *format++;
				if (!m) return;
				if (m >= '0' && m <= '9')
				{
					padding_info = m - '0';
					padding_char = '0';
					m = *format++;
				}
			}
			else if (m >= '1' && m <= '9')
			{
				padding_info = m - '0';
				padding_char = ' ';
				m = *format++;
				if (!m) return;
			}
			switch (m) {
				case 's': {
					char* stringToPrint = va_arg(list, char*);
					if (stringToPrint == NULL) stringToPrint = "(null)";
					int length = strlen(stringToPrint);
					memcpy(memory, stringToPrint, length);
					memory += length;
					continue;
				}
				case 'c': {
					int chrToPrint = va_arg(list, int);
					*memory++ = chrToPrint;
					continue;
				}
				case 'd':case'i': {
					int32_t num = va_arg(list, int32_t);
					char str[17] = {0, };
					int i = 0;
					bool isNegative = false;
					if (num == 0) {
						str[i++] = '0'; 
						//str[i] = '\0'; 
						//goto skip1;
					}
					if (num == -2147483648) {
						char* e = "-2147483648";
						int length = strlen(e);
						memcpy(memory, e, length);
						memory += length;
						goto skip3;
					}
					
					// In standard itoa(), negative numbers are handled only with  
					// base 10. Otherwise numbers are considered unsigned. 
					if (num < 0) {
						isNegative = true;
						num = -num;
					}
					int base = 10;
					// Process individual digits 
					while (num != 0) 
					{ 
						int rem = num % base; 
						str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0'; 
						num = num/base; 
					}
					// add padding:
					for (; i < padding_info; )
						str[i++] = padding_char;
					// If number is negative, append '-' 
					if (isNegative) 
						str[i++] = '-'; 
					str[i] = '\0'; // Append string terminator 
					int start = 0; 
					int end = i -1; 
					while (start < end) 
					{ 
						___swap((str+start), (str+end)); 
						start++; 
						end--; 
					}
					int length = i;
					memcpy(memory, str, length);
					memory += length;
				skip3:;
					continue;
				}
				case 'u': {
					uint32_t num = va_arg(list, uint32_t);
					char str[14] = {0, };
					int i = 0;
					if (num == 0) {
						str[i++] = '0'; 
						//str[i] = '\0'; 
						//goto skip2;
					}
					int base = 10;
					// Process individual digits 
					while (num != 0) 
					{ 
						int rem = num % base; 
						str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0'; 
						num = num/base; 
					} 
					// add padding:
					for (; i < padding_info; )
						str[i++] = padding_char;
					str[i] = '\0'; // Append string terminator 
					int start = 0; 
					int end = i -1; 
					while (start < end) 
					{ 
						___swap((str+start), (str+end)); 
						start++; 
						end--; 
					}
					int length = i;
					memcpy(memory, str, length);
					memory += length;
					continue;
				}
				case 'b': {
					uint32_t toPrint = va_arg(list, uint32_t);
					uint32_t power = (15 << 4), pt = 4;
					for (; power != 0; power >>= 4, pt -= 4) {
						uint32_t p = toPrint & power;
						p >>= pt;
						*memory = "0123456789abcdef"[p];
						memory++;
					}
					continue;
				}
				case 'B': {
					uint32_t toPrint = va_arg(list, uint32_t);
					uint32_t power = (15 << 4), pt = 4;
					for (; power != 0; power >>= 4, pt -= 4) {
						uint32_t p = toPrint & power;
						p >>= pt;
						*memory = "0123456789ABCDEF"[p];
						memory++;
					}
					continue;
				}
				case 'x': {
					uint32_t toPrint = va_arg(list, uint32_t);
					uint32_t power = (15 << 28), pt = 28;
					for (; power != 0; power >>= 4, pt -= 4) {
						uint32_t p = toPrint & power;
						p >>= pt;
						*memory = "0123456789abcdef"[p];
						memory++;
					}
					continue;
				}
				case 'X': {
					uint32_t toPrint = va_arg(list, uint32_t);
					uint32_t power = (15 << 28), pt = 28;
					for (; power != 0; power >>= 4, pt -= 4) {
						uint32_t p = toPrint & power;
						p >>= pt;
						*memory = "0123456789ABCDEF"[p];
						memory++;
					}
					continue;
				}
			}
		} else {
			*memory++ = m;
		}
	}
	*memory = '\0';
	return;
}
void sprintf(char*a, const char*c, ...) {
	va_list list;
	va_start(list, c);
	vsprintf(a, c, list);
	va_end(list);
}
void LogMsg(const char* fmt, ...)
{
	char cr[8192];
	va_list list;
	va_start(list, fmt);
	vsprintf(cr, fmt, list);
	
	sprintf(cr+strlen(cr), "\n");
	PutString(cr);
	
	va_end(list);
}
void LogMsgNoCr(const char* fmt, ...)
{
	char cr[8192];
	va_list list;
	va_start(list, fmt);
	vsprintf(cr, fmt, list);
	
	PutString(cr);
	
	va_end(list);
}

//futureproofing here:
char g_VersionString[10] = "VX.XX";

const char* GetVersionString()
{
	if (g_VersionString[1] == 'X')
	{
		int ver = NsGetVersion();
		//major version and minor version:
		//NanoShell V1.00 (when that comes out) will have a version number of 100
		//Current version as of Feb 10,2022 (NanoShell V0.30) has a version code of 30.
		//Some software may naively just put a 0 in the major version number, but
		//we should expect an eventual V1.00 or more.
		sprintf(g_VersionString, "V%d.%02d", ver/100, ver%100);
	}
	return g_VersionString;
}

