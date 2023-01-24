// CrappyScript (C) 2023 iProgramInCpp

#include "s_all.h"

FILE* g_file;
jmp_buf g_errorJumpBuffer;

const char* g_errorMsgs[] = {
	"Success",
	"",
	"Tokeniser could not allocate memory",
	"Unterminated string",
	"Unterminated escape sequence",
	"Unknown escape sequence",
	"Internal error: unknown symbol token",
	"Internal error: unknown keyword token",
	"",
	"Parser could not allocate memory",
	"Unterminated block statement",
	"Unterminated command statement",
	"Unterminated function declaration",
	"Internal error: not a block statement",
	"Internal error: not a command statement",
	"Internal error: not a function statement",
	"Expected statement",
	"Expected '('",
	"Expected ')'",
	"Expected ';'",
	"Expected 'then'",
	"Expected 'do'",
	"Expected '{'",
	"Expected '}'",
	"Expected ';' or arguments",
	"Expected ')' or arguments",
	"Expected ','",
	"Expected '='",
	"Expected '=' or 'to'",
	"Expected '=' or 'be'",
	"Expected string",
	"Expected function name",
	"Expected arguments",
	"Expected block",
	"Expected function definition",
	"Expected command",
	"Expected 'if' statement",
	"Expected 'while' statement",
	"Expected string",
	"Expected number",
	"Expected ';' statement",
	"Expected variable definition",
	"Expected assignment",
	"Expected return",
	"Expected variable name",
	"Expected name",
	"",
	"Runner could not allocate memory",
	"Internal error: unknown statement type",
	"Unknown function or variable name",
	"Arguments specified for a variable as if it were a function call",
	"Too many arguments in function call",
	"Too few arguments in function call",
	"Too many arguments in function definition",
	"Function is already defined",
	"Variable is already defined",
	"Assignee is not a variable",
	"Call stack overflow",
	"Invalid parameter type. The correct type is 'string'",
	"Invalid parameter type. The correct type is 'int'",
	"Unknown variant type",
	"'if' statement expects 'int' condition",
	"'while' statement expects 'int' condition",
	"Cannot convert string value to int",
	"Array index out of bounds",
	"Division by zero",
	"Substring out of bounds",
};

char GetErrorCategory(int error)
{
	if (error <= ERROR_NONE || error >= ERROR_END) return 'U';

	if (error >= ERROR_RUNTIME_START) return 'R';
	if (error >= ERROR_PARSER_START) return 'P';
	if (error >= ERROR_TOKENIZER_START) return 'T';

	return 'U';
}

int GetErrorNo(int error)
{
	if (error <= ERROR_NONE || error >= ERROR_END) return error;

	if (error >= ERROR_RUNTIME_START) return error - ERROR_RUNTIME_START;
	if (error >= ERROR_PARSER_START) return error - ERROR_PARSER_START;
	if (error >= ERROR_TOKENIZER_START) return error - ERROR_TOKENIZER_START;

	return error;
}

const char* GetErrorMessage(int error)
{
	if (error < ERROR_NONE || error >= ERROR_END) return "Unknown error";

	return g_errorMsgs[error];
}

void CompileSource(const char* src)
{
	Tokenise(src);
	Parse();
	RunnerPrepareRunTime();
}

void CompilerTeardown()
{
	RunnerCleanup();
	ParserTeardown();
	TokenTeardown();
}

void ShellExecuteFile(const char* pfn, UNUSED int argc, UNUSED char** argv)
{
	FILE* f = fopen(pfn, "r");
	if (!f)
	{
		LogMsg("File %s not found.", pfn);
		return;
	}
	
	char* chr;
	fseek(f, 0, SEEK_END);
	int szint = ftell(f);
	size_t sz = (size_t)szint;
	fseek(f, 0, SEEK_SET);
	chr = malloc(sz + 1);
	chr[sz] = 0;
	size_t readin = fread(chr, 1, sz, f);
	if (readin > sz) readin = 0;
	chr[readin] = 0;
	
	int error = setjmp(g_errorJumpBuffer);
	if (error)
	{
		LogMsg("ERROR %c%04d: %s", GetErrorCategory(error), GetErrorNo(error), GetErrorMessage(error));
		
		fclose(f);
		return;
	}

	Tokenise(chr);
	Parse();
	//RunnerGo(argc, argv);
	RunnerPrepareRunTime();
	RunnerCall("main", "iiii", 1,2,3,4);
	RunnerCleanup();
	ParserTeardown();
	TokenTeardown();
	
	#ifdef _WIN32
	MemDebugPrint();
	#endif

	fclose(f);
	return;
}
