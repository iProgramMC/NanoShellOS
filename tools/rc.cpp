// NanoShell SDK Resource Compiler
// Copyright (C) 2023 iProgramInCpp

// Note. This is pretty disorganized, I wrote it in a hurry :)

#include <algorithm>
#include <csetjmp>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <ctime>
#include <map>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// I'm probably not going to define a `string` class here any time soon.
using std::vector;
using std::string;

#define RED "\x1B[31m"
#define YEL "\x1B[33m"
#define DEF "\x1B[0m"

#define TRANSPARENT 0xFFFFFFFF
void PelProcess(uint32_t* ptr)
{
	uint8_t* bptr = (uint8_t*)ptr;
	uint8_t temp;
	
	temp    = bptr[2];
	bptr[2] = bptr[0];
	bptr[0] = temp;
	
	if (bptr[3] == 0x00) // alpha
	{
		*ptr = TRANSPARENT;
	}
	else if ((* ptr & 0xFFFFFF) == 0xFFFFFF)
	{
		*ptr = 0xFFFFFF;
	}
}

bool g_bErrorMode = false;

const char* pArg0 = "rc";

std::string g_OutputFileContents = "";
std::string g_FileHeaderStuff = "";

struct Image
{
	short width, height;
	const uint32_t* buffer;
}
__attribute__((packed));

// Command flags.
enum
{
	FLAG_NOLOGO       = (1 << 0), // /S
	FLAG_NOHEADERDATA = (1 << 1), // /H
};

// Resource types, as defined in NanoShell
enum eResourceType
{
	RES_NONE,
	RES_STRING,
	RES_ICON,
	RES_BITMAP,
	RES_BLOB,
	RES_MAX,
};

std::map<int, bool> g_bDoesIDExistYet;

struct Resource
{
	int id = 0;
	eResourceType type = RES_NONE;
	
	void* pData = NULL; size_t szData = 0;
	
	bool operator< (const Resource& res) const
	{
		// really, this should NOT happen at all, since we don't
		// allow more than one resource per ID number.
		if (id == res.id)
		{
			if (szData == res.szData)
				return type < res.type;
			
			return szData < res.szData;
		}
		
		return id < res.id;
	}
	
	std::string Stringify() const
	{
		std::string str = "";
		str.reserve(type == RES_STRING ? 200 : 10000);
		
		size_t shownSzData = szData;
		
		if (type == RES_BITMAP || type == RES_ICON)
		{
			// neutralize a difference in address size
			shownSzData -= (sizeof(uint32_t*) - sizeof(uint32_t));
		}
		
		str += "dd " + std::to_string(id) + " ; id\n";
		str += "dd " + std::to_string(int(type)) + " ; type\n";
		str += "dd " + std::to_string(int(shownSzData)) + " ; size\n";
		
		if (type == RES_STRING)
		{
			str += "db \"" + string((const char*)pData) + "\", 0";
		}
		else if (type == RES_BITMAP || type == RES_ICON)
		{
			const Image* pImg = (const Image*)pData;
			str += "dw " + std::to_string(pImg->width)  + " ; width\n";
			str += "dw " + std::to_string(pImg->height) + " ; height\n";
			str += "dd RES_" + std::to_string(id) + "_PIXELS ; pixels pointer\nRES_" + std::to_string(id) + "_PIXELS:\n";
			
			int wh = pImg->width * pImg->height;
			int iwm1 = pImg->width - 1;
			
			for (int i = 0; i < wh; i++)
			{
				if (i == 0 || (pImg->width && i % pImg->width == 0))
					str += "dd";
				
				char buf[20];
				snprintf(buf, sizeof buf, " 0x%08X,", pImg->buffer[i]);
				
				str += buf;
				if (pImg->width && i % pImg->width == iwm1)
					str += "\n";
			}
		}
		else
		{
			for (int i = 0; i < szData; i++)
			{
				if (i % 16 == 0)
					str += "db";
				
				char buf[10];
				snprintf(buf, sizeof buf, " 0x%02X,", ((const uint8_t*)pData)[i]);
				
				str += buf;
				if (i % 16 == 15)
					str += "\n";
			}
		}
		
		return str;
	}
};

enum eSubsystem
{
	SUBSYS_CONSOLE,
	SUBSYS_GUI,
};

struct HeaderData
{
	eSubsystem ss;
	short build;
	char  minor;
	char  major;
	string appName;
	string appAuth;
	string appCopr;
	string proName;
	
	string Stringify()
	{
		string result = "";
		
		result += "dd " + std::to_string(int(ss)) + " ; subsystem\n";
		result += "dw " + std::to_string(build) + " ; version build num\n";
		result += "db " + std::to_string(minor) + " ; version minor\n";
		result += "db " + std::to_string(major) + " ; version major\n";
		
		result += string("dd ") + ((appName.size()) ? "_App_Name" : "0") + " ; file description\n";
		result += string("dd ") + ((appAuth.size()) ? "_App_Auth" : "0") + " ; author\n";
		result += string("dd ") + ((appCopr.size()) ? "_App_Copr" : "0") + " ; copyright\n";
		result += string("dd ") + ((proName.size()) ? "_Pro_Name" : "0") + " ; project name\n";
		
		if (appName.size()) result += "_App_Name: db \"" + appName + "\", 0\n";
		if (appAuth.size()) result += "_App_Auth: db \"" + appAuth + "\", 0\n";
		if (appCopr.size()) result += "_App_Copr: db \"" + appCopr + "\", 0\n";
		if (proName.size()) result += "_Pro_Name: db \"" + proName + "\", 0\n";
		
		return result;
	}
};

HeaderData g_headerData;

vector<Resource> g_resources;

uint32_t g_cmdFlags;

//taken from Gamedeveloper magazine's InnerProduct (Sean Barrett 2005-03-15)

// circular shift hash -- produces good results if modding by a prime;
// longword at a time would be faster (need alpha-style "is any byte 0"),
// or just use the first longword
constexpr uint32_t HashString(const char *str)
{
	if (!str) return 0;
	
	uint32_t acc = 0x55555555;
	
	while (*str)
		acc = (acc >> 27) + (acc << 5) + uint8_t(*str++);
	
	return acc;
}

// Tokenize a string 
vector<string> StringTokenize(const string& str, char delimiter)
{
	string currentStr;
	bool bQuoteMode = false;
	const size_t strSize = str.size();
	
	vector<string> result;
	
	for (size_t i = 0; i < strSize; i++)
	{
		if (str[i] == '"' && delimiter == ' ')
		{
			if (!bQuoteMode)
			{
				if (currentStr.size())
					result.push_back(currentStr);
				currentStr.clear();
				bQuoteMode = true;
			}
			else
			{
				bQuoteMode = false;
				result.push_back(currentStr);
				currentStr.clear();
			}
		}
		else if (str[i] == delimiter)
		{
			if (bQuoteMode && delimiter == ' ')
			{
				currentStr += delimiter;
			}
			else
			{
				if (currentStr.size())
					result.push_back(currentStr);
				currentStr.clear();
			}
		}
		else if (str[i] == '\r' && delimiter == '\n')
		{
		}
		else
		{
			currentStr += str[i];
		}
	}
	
	if (currentStr.size())
		result.push_back(currentStr);
	
	return result;
}

const char* GetErrorString(int error)
{
	return "Huh?";
}

void Help()
{
	printf("Usage: %s [resource file] [output assembly file] (flags)\n", pArg0);
	printf("Compile a NanoShell resource file.\n");
	printf("Usable flags:\n");
	printf("/?  - Shows this list\n");
	printf("/S  - Doesn't show the 'logo'\n");
	printf("/H  - Doesn't emit info header data\n");
}

void ParseFlag(const char *pFlag)
{
	if (*pFlag != '/') return;
	
	switch (pFlag[1])
	{
		case 'S':
			g_cmdFlags |= FLAG_NOLOGO;
			break;
		case 'H':
			g_cmdFlags |= FLAG_NOHEADERDATA;
			break;
		case '?':
			Help();
			break;
		default:
			printf(YEL "warning" DEF ": unknown flag '%s'\n", pFlag);
			break;
	}
}

std::string ReadFile(const std::string& fileName)
{
	FILE* f = fopen(fileName.c_str(), "r");
	if (!f)
	{
		char buffer[512];
		snprintf(buffer, sizeof buffer, "%s: %s: " RED "error" DEF, pArg0, fileName.c_str());
		perror(buffer);
		return "";
	}
	
	// this typically works, although sometimes it won't
	int fileSize = 0;
	fseek(f, 0, SEEK_END);
	fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	char* buf = new char[fileSize + 1];
	size_t readIn = fread(buf, sizeof (char), fileSize, f);
	buf[readIn] = 0;
	
	fclose(f);
	
	std::string s(buf);
	delete[] buf;
	return s;
}

uint8_t* ReadEntireFile(const std::string& fileName, size_t& szOut)
{
	FILE* f = fopen(fileName.c_str(), "r");
	if (!f)
	{
		char buffer[512];
		snprintf(buffer, sizeof buffer, "%s: %s: " RED "error" DEF, pArg0, fileName.c_str());
		perror(buffer);
		return NULL;
	}
	
	// this typically works, although sometimes it won't
	int fileSize = 0;
	fseek(f, 0, SEEK_END);
	fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	uint8_t* buf = new uint8_t[fileSize + 1];
	size_t readIn = fread(buf, sizeof (char), fileSize, f);
	buf[readIn] = 0;
	
	szOut = readIn;
	
	fclose(f);
	
	return buf;
}

void WriteFile(const std::string& fileName, const std::string& fileContents)
{
	FILE* f = fopen(fileName.c_str(), "w");
	if (!f)
	{
		char buffer[512];
		snprintf(buffer, sizeof buffer, "%s: %s: " RED "error" DEF, pArg0, fileName.c_str());
		perror(buffer);
		return;
	}
	
	fwrite(fileContents.c_str(), sizeof(char), fileContents.size(), f);
	
	fclose(f);
}

void PrintError(const char* strerr, const char* fileName, int lineNum, const char* line)
{
	printf("%s:%d: " RED "error" DEF ": %s\n  %s\n", fileName, lineNum, strerr, line);
	g_bErrorMode = true;
}

void ProcessResource(int rid, eResourceType rtype, const std::string& text, const std::string& fullLine, const std::string& fileName, int lineNum)
{
	Resource res;
	res.id = rid;
	res.type = rtype;
	
	switch (rtype)
	{
		case RES_STRING:
		{
			res.pData = strdup(text.c_str());
			res.szData = text.size() + 1;
			break;
		}
		case RES_BLOB:
		{
			res.pData = ReadEntireFile(text, res.szData);
			if (!res.pData)
				return;
			break;
		}
		case RES_BITMAP:
		case RES_ICON:
		{
			int width, height, bpp;
			uint8_t* data = stbi_load(text.c_str(), &width, &height, &bpp, 0);
			
			if (!data)
			{
				PrintError("could not load image file", fileName.c_str(), lineNum, fullLine.c_str());
				break;
			}
			
			if (bpp != 4 && bpp != 3)
			{
				PrintError("image file must be 24- or 32-bit", fileName.c_str(), lineNum, fullLine.c_str());
				
				stbi_image_free(data);
				
				break;
			}
			
			// convert it to a NanoShell style framebuffer
			uint32_t* fb = nullptr;
			res.szData = sizeof(Image) + width * height * sizeof(uint32_t);
			Image *pImg = (Image*)malloc(res.szData);
			pImg->buffer = fb = (uint32_t*)&pImg[1];
			pImg->width = width;
			pImg->height = height;
			
			res.pData = pImg;
			
			size_t pitch = width * bpp;
			pitch = (pitch + 3) & ~3;   // align to nearest 4. Probably not necessary since we expect 32-bit format though
			
			// copy each row
			for (int i = 0; i < height; i++)
			{
				if (bpp == 4)
				{
					memcpy(fb + i * width, data + i * pitch, pitch);
					// byte swap
					for (int x = 0; x < width; x++)
					{
						PelProcess(&fb[i * width + x]);
					}
				}
				else if (bpp == 3)
				{
					for (int x = 0; x < width; x++)
					{
						uint32_t tmp = *((uint32_t*)(data + i * pitch + 3 * x));
						tmp |= 0xFF000000;
						PelProcess(&tmp);
						fb[i * width + x] = tmp;
					}
				}
			}
			
			stbi_image_free(data);
			
			break;
		}
	}
	
	g_resources.push_back(res);
	g_bDoesIDExistYet[res.id] = true;
}

void ParseResources(const std::string& fileName, const std::string& input)
{
	vector<string> lines = StringTokenize(input, '\n');
	
	int lineNum = 0;
	for (const string& line : lines)
	{
		if (line.empty()) continue;
		if (line[0] == '/')
			if (line.size() >= 2 && line[1] == '/')
				continue;
		
		lineNum++;
		
		vector<string> toks = StringTokenize(line, ' ');
		if (toks.size() == 0) continue;
		
		switch (HashString(toks[0].c_str()))
		{
			case HashString("SubSystem"):
			{
				if (toks.size() != 2) { PrintError("invalid nr of args", fileName.c_str(), lineNum, line.c_str()); break; }
				
				if (toks[1] == "Gui")
					g_headerData.ss = SUBSYS_GUI;
				else
					g_headerData.ss = SUBSYS_CONSOLE;
				
				break;
			}
			case HashString("Version"):
			{
				if (toks.size() != 4) { PrintError("invalid nr of args", fileName.c_str(), lineNum, line.c_str()); break; }
				
				try
				{
					g_headerData.major = (char) std::stoi(toks[1].c_str());
					g_headerData.minor = (char) std::stoi(toks[2].c_str());
					g_headerData.build = (short)std::stoi(toks[3].c_str());
				}
				catch (...)
				{
					PrintError("invalid version number", fileName.c_str(), lineNum, line.c_str());
				}
				
				break;
			}
			case HashString("AppName"):
			{
				if (toks.size() != 2) { PrintError("invalid nr of args", fileName.c_str(), lineNum, line.c_str()); break; }
				g_headerData.appName = toks[1];
				
				break;
			}
			case HashString("ProjectName"):
			{
				if (toks.size() != 2) { PrintError("invalid nr of args", fileName.c_str(), lineNum, line.c_str()); break; }
				g_headerData.proName = toks[1];
				break;
			}
			case HashString("AppAuthor"):
			{
				if (toks.size() != 2) { PrintError("invalid nr of args", fileName.c_str(), lineNum, line.c_str()); break; }
				g_headerData.appAuth = toks[1];
				
				break;
			}
			case HashString("AppCopyright"):
			{
				if (toks.size() != 2) { PrintError("invalid nr of args", fileName.c_str(), lineNum, line.c_str()); break; }
				g_headerData.appCopr = toks[1];
				break;
			}
			case HashString("Resource"):
			{
				if (toks.size() != 4) { PrintError("invalid nr of args", fileName.c_str(), lineNum, line.c_str()); break; }
				int resid = 0;
				
				try
				{
					resid = std::stoi(toks[1].c_str());
				}
				catch (...)
				{
					char buffer[1024];
					snprintf(buffer, sizeof buffer, "invalid integer '%s'", toks[1].c_str());
					PrintError(buffer, fileName.c_str(), lineNum, line.c_str());
				}
				
				auto iter = g_bDoesIDExistYet.find(resid);
				if (iter != g_bDoesIDExistYet.end())
				{
					if (iter->second)
					{
						char buffer[1024];
						snprintf(buffer, sizeof buffer, "resource with ID '%d' already exists", resid);
						PrintError(buffer, fileName.c_str(), lineNum, line.c_str());
						break;
					}
				}
				
				eResourceType rtype(eResourceType(-1));
				
				switch (HashString(toks[2].c_str()))
				{
					case HashString("String"): rtype = RES_STRING; break;
					case HashString("Bitmap"): rtype = RES_BITMAP; break;
					case HashString("Blob"):   rtype = RES_BLOB;   break;
					case HashString("Icon"):   rtype = RES_ICON;   break;
				}
				
				if (rtype <= RES_NONE || rtype >= RES_MAX)
				{
					char buffer[1024];
					snprintf(buffer, sizeof buffer, "invalid resource type '%s'", toks[2].c_str());
					PrintError(buffer, fileName.c_str(), lineNum, line.c_str());
					break;
				}
				
				ProcessResource(resid, rtype, toks[3], line, fileName, lineNum);
				
				break;
			}
		}
	}
}

std::string GetTimeString()
{
	time_t t = time(NULL);
	tm time;
	
	// note: Not sure this will work everywhere
	localtime_r(&t, &time);
	
	char buffer[1024];
	strftime(buffer, sizeof buffer, "%c", &time);
	
	return std::string(buffer);
}

void AddHeader()
{
	g_OutputFileContents += "; NanoShell resource file. Compiled on " + GetTimeString() + "\n\n";
}

int main(int argc, char ** argv)
{
	if (argc > 1)
		pArg0 = argv[0];
	
	if (argc < 3)
	{
		Help();
		return 0;
	}
	
	for (int i = 3; i < argc; i++)
		ParseFlag(argv[i]);
	
	if (~g_cmdFlags & FLAG_NOLOGO)
	{
		printf("NanoShell Resource File Compiler V1.00 Copyright (C) 2023 iProgramInCpp\n");
		printf("The following software is licensed under the GNU GPLv3.0 license.\n");
	}
	
	// try to read the content of the input file
	std::string inputFile = ReadFile(argv[1]);
	if (inputFile.empty())
	{
		printf("%s: %s: " YEL "warning" DEF ": file is empty\n", pArg0, argv[1]);
		return 0;
	}
	
	g_FileHeaderStuff.reserve(100000);
	g_OutputFileContents.reserve(100000);
	
	ParseResources(argv[1], inputFile);
	
	bool bAddedHeader = false;
	
	if (~g_cmdFlags & FLAG_NOHEADERDATA)
	{
		bAddedHeader = true;
		AddHeader();
		
		g_OutputFileContents += "section .nanoshell\n\n" + g_headerData.Stringify() + "\n\n";
	}
	
	if (!g_resources.empty())
	{
		if (!bAddedHeader)
		{
			AddHeader();
			bAddedHeader = true;
		}
		
		g_OutputFileContents += "section .nanoshell_resources\n\ndd " + std::to_string(g_resources.size()) +
			" ; # of resources\n\n";
		
		// sort the resources by ID
		std::sort(g_resources.begin(), g_resources.end());
		
		for (const Resource& res : g_resources)
		{
			g_OutputFileContents += res.Stringify() + "\n\n";
		}
	}
	
	if (g_OutputFileContents.empty())
	{
		printf("%s: %s: " YEL "warning" DEF ": nothing was generated\n", pArg0, argv[2]);
		return 0;
	}
	
	if (!g_bErrorMode)
	{
		WriteFile(argv[2], g_OutputFileContents);
	}
	
	return g_bErrorMode;
}
