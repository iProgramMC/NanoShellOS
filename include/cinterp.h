/*****************************************
		NanoShell Operating System
	   (C) 2021, 2022 iProgramInCpp

  NSScript Interpreter module headerfile
******************************************/
#ifndef _CINTERP_H
#define _CINTERP_H

#include <task.h>

typedef int CCSTATUS;

#define MAX_ALLOCS 1024

typedef struct {
    char* pSource, * pLastSource, * pSourceStart, // current position in source code
        * pData, * pDataStart;   // data/bss pointer

        //#define e pText
        //#define le pLastText

    int* pText, * pLastText, * pTextStart,  // current position in emitted code
    //int* e, * le,  // current position in emitted code
        * pCurrIdentifier, * pStartOfIdentifiers,      // currently parsed identifier
        * pCurrSymbol, * pStartOfSymbols,     // symbol table (simple list of identifiers)
        currentToken,       // current token
        currentTokenValue,     // current token value
        currentExprType,       // current expression type
        localVariableOffset,      // local variable offset
        lineNum,     // current line number
        printAssembly,      // print source and assembly flag
        printCycles;    // print executed instructions

    int g_memoryAllocCount;
    void* g_memoryAllocatedPointers[MAX_ALLOCS];

    bool m_halted;

    const char* g_pErrorString;
    int g_pErrorLine;
    bool g_pErrorExists;

    int main_baseType, main_tokenType, main_poolSize, * main_idMain;
    int* main_instPtr, * main_stackPtr, * main_stackStart, * main_basePtr, main_theAReg, main_cycle; // vm registers
    int main_tempI, * main_tempT; // temps

    int retnVal;
	
	bool m_bHookedConsole;
	
	JumpBuffer m_jumpError;

} CMachine;

enum {
    CCSTATUS_SUCCESS,
    CCSTATUS_CTRL_C = 0xFFFFFFFC,
    CCSTATUS_NO_MALLOC_POOL_AREA = 0x80000001,
    CCSTATUS_NO_MALLOC_TEXT_AREA,
    CCSTATUS_NO_MALLOC_DATA_AREA,
    CCSTATUS_NO_MALLOC_STACK_AREA,
    CCSTATUS_NO_MALLOC_SRC_AREA,
    CCSTATUS_BAD_ENUM_IDENTIFIER,
    CCSTATUS_BAD_ENUM_INITIALIZER,
    CCSTATUS_BAD_GLOBAL_DECLARATION,
    CCSTATUS_DUP_GLOBAL_DECLARATION,
    CCSTATUS_BAD_PARM_DECLARATION,
    CCSTATUS_DUP_PARM_DECLARATION,
    CCSTATUS_BAD_FUNC_DECLARATION,
    CCSTATUS_BAD_LOCAL_DECLARATION,
    CCSTATUS_DUP_LOCAL_DECLARATION,
    CCSTATUS_MAIN_NOT_DEFINED,
    CCSTATUS_UNKNOWN_INSTRUCTION,//due to memory corruption?
	CCSTATUS_ERROR_FOUND,
	// source code errors
	CCSTATUS_UNEXPECTED_EOF,
	CCSTATUS_OPEN_PAREN_EXPECTED,
	CCSTATUS_CLOSE_PAREN_EXPECTED,
	CCSTATUS_BAD_FUNCTION_CALL,
	CCSTATUS_UNDEFINED_VARIABLE,
	CCSTATUS_BAD_CAST,
	CCSTATUS_BAD_DEREFERENCE,
	CCSTATUS_BAD_REFERENCE,
	CCSTATUS_BAD_LVALUE,
	CCSTATUS_BAD_EXPRESSION,
	CCSTATUS_COND_MISSING_COLON,
	CCSTATUS_CLOSE_BRACK_EXPECTED,
	CCSTATUS_POINTER_TYPE_EXPECTED,
	CCSTATUS_COMPILER_ERROR,
	CCSTATUS_SEMICOLON_EXPECTED,
};

/**
 * Initializes a C interpreter machine.
 */
CCSTATUS CcInitMachine(CMachine* pMachine);

/**
 * Compiles a string of text (the code).
 */
CCSTATUS CcCompileCode(CMachine* pMachine, const char* pCode, int length);

/**
 * Runs the machine for a certain amount of cycles.
 */
void CcRunMachine(CMachine* pMachine, int cycs_per_call);

/**
 * Kill the C interpreter machine.
 */
void CcKillMachine(CMachine* pMachine);

/**
 * Integrates a C interpreter machine and runs it synchronously. The function will only return when execution is finished.
 */
int CcRunCCode(const char* pCode, int length);

#endif//_CINTERP_H