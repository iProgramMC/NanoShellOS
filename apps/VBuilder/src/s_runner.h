// CrappyScript (C) 2023 iProgramInCpp

#pragma once

#include "s_all.h"

#define C_MAX_ARGS (64)
#define C_MAX_BUILTIN_ARGS (8)
#define C_MAX_STACK (128)

typedef enum
{
	FUNCTION_STATEMENT,
	FUNCTION_POINTER, //Built in function
	FUNCTION_VARIABLE,//Actually just a variable
}
eFunctionType;

// note: This function isn't just "void"
// note: This returns a malloc'ed pointer. Free it properly.
// note: All parameters are also malloc'ed pointers.
typedef Variant* (*CallableFunPtr) ();

typedef struct Function Function;

struct Function
{
	Function* m_nlink;
	Function* m_plink;

	eFunctionType type;
	const char* m_name;
	int m_nArgs;
	char** m_args; // The names of the arguments.
	bool m_bReturns;

	union
	{
		Statement* m_pStatement;
		CallableFunPtr m_pFunction;
		Variant* m_pContents;
	};
};

void RunnerGo(int argc, char** argv);
void RunnerCleanup();
void RunnerAddFunctionPtr(CallableFunPtr fp, const char* fname, int nargs, bool returns);
