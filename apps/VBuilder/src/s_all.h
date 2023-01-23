// CrappyScript (C) 2023 iProgramInCpp
#pragma once

#include "s_main.h"
#include "s_shell.h"
#include "s_builtin.h"
#include "s_tokeniser.h"
#include "s_parser.h"
#include "s_runner.h"
#include "s_variant.h"

typedef struct
{
	Function* m_pFunction;
	Variant* m_args[C_MAX_ARGS];
	int m_nargs;
}
CallStackFrame;

typedef struct
{
	// tokenizer
	jmp_buf m_errorJumpBuffer;
	FILE*   m_file;
	
	int     m_lineNum;
	
	Token** m_tokens;
	size_t  m_nTokens;
	
	// parser
	Statement* m_mainBlock;
	
	int     m_parserLine;
	size_t  m_currentToken;
	
	// runner
	Statement* m_pCurrentStatement;
	CallStackFrame m_callStack[C_MAX_STACK];
	int        m_callStackPointer;
	
	Function*  m_functionsList;
}
CrappyScriptContext;

