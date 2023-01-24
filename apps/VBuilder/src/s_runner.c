// CrappyScript (C) 2023 iProgramInCpp

#include "s_all.h"

extern jmp_buf g_errorJumpBuffer;

Statement* g_pCurrentStatement;
static CallStackFrame g_callStack[C_MAX_STACK];
static int g_callStackPointer;

extern char g_ErrorBuffer[ERROR_BUFFER_SIZE];

NORETURN void RunnerOnError(int error)
{
	g_ErrorBuffer[0] = 0;
	
	char buff[4096];
	if (g_pCurrentStatement)
	{
		snprintf(buff, sizeof buff, "At line %d:\n", g_pCurrentStatement->m_firstLine);
		strcat(g_ErrorBuffer, buff);
	}
	snprintf(buff, sizeof buff, "Runtime Error %c%04d: %s\nCall stack: \n", GetErrorCategory(error), GetErrorNo(error), GetErrorMessage(error));
	strcat(g_ErrorBuffer, buff);
	
	for (int i = g_callStackPointer; i >= 0; i--)
	{
		const char* fName = "Entry point";
		Function* pFn = g_callStack[i].m_pFunction;
		if (pFn != NULL)
			fName = pFn->m_name;

		snprintf(buff, sizeof buff, "%s %s", i == g_callStackPointer ? "->" : " *", fName);
		strcat(g_ErrorBuffer, buff);
	}

	longjmp(g_errorJumpBuffer, error);
}

// The runner is very simple - it looks through the statements and does decisions based on them.

// This could be optimized. By a lot.
Function* g_functionsList;

Variant* RunStatement(Statement* pStatement);

void RunnerFreeFunction(Function * pFunc)
{
	if (pFunc->type == FUNCTION_VARIABLE)
	{
		MemFree(pFunc->m_pContents);
	}

	// if it's a pointer, we shouldn't free it, and if it's a statement, it's managed by the parser code

	MemFree(pFunc);
}

void RunnerRemoveFunctionFromList(Function* pFunc)
{
	if (pFunc == g_functionsList)
	{
		g_functionsList = pFunc->m_nlink;
		if (g_functionsList)
			g_functionsList->m_plink = NULL;
	}
	else
	{
		if (pFunc->m_nlink) pFunc->m_nlink->m_plink = pFunc->m_plink;
		if (pFunc->m_plink) pFunc->m_plink->m_nlink = pFunc->m_nlink;
	}

	RunnerFreeFunction(pFunc);
}

void RunnerCleanup()
{
	while (g_functionsList)
		RunnerRemoveFunctionFromList(g_functionsList);
}

void RunnerAddFunctionToList(Function* pFunc)
{
	if (g_functionsList == NULL)
	{
		g_functionsList = pFunc;
	}
	else
	{
		pFunc->m_nlink = g_functionsList;
		g_functionsList->m_plink = pFunc;
		g_functionsList = pFunc;
	}
}

void RunnerAddFunctionPtr(CallableFunPtr fp, const char* fname, int nargs, bool returns)
{
	if (nargs >= C_MAX_BUILTIN_ARGS)
	{
		LogMsg("ERROR: Too many arguments specified in this function. Not a great idea!");
	}

	Function* pFunc = MemCAllocate(1, sizeof (Function));
	if (!pFunc)
	{
		RunnerOnError(ERROR_R_MEMORY_ALLOC_FAILURE);
	}

	RunnerAddFunctionToList(pFunc);

	pFunc->type = FUNCTION_POINTER;
	pFunc->m_name     = fname;
	pFunc->m_bReturns = returns;
	pFunc->m_nArgs    = nargs;
	pFunc->m_args     = NULL; // this only applies to statement-functions
	pFunc->m_pFunction = fp;
}

void RunnerAddFunctionStatement(Statement* statement, const char* fname, int nargs, char** args, bool returns)
{
	if (nargs >= C_MAX_ARGS)
	{
		RunnerOnError(ERROR_TOO_MANY_ARGS_DEF);
	}

	Function* pFunc = MemCAllocate(1, sizeof(Function));
	if (!pFunc)
	{
		RunnerOnError(ERROR_R_MEMORY_ALLOC_FAILURE);
	}

	RunnerAddFunctionToList(pFunc);

	pFunc->type = FUNCTION_STATEMENT;
	pFunc->m_name = fname;
	pFunc->m_bReturns = returns;
	pFunc->m_nArgs = nargs;
	pFunc->m_args = args;
	pFunc->m_pStatement = statement;
}

void RunnerAddFunctionVariable(Statement* statement, const char* fname)
{
	Function* pFunc = MemCAllocate(1, sizeof(Function));
	if (!pFunc)
	{
		RunnerOnError(ERROR_R_MEMORY_ALLOC_FAILURE);
	}

	RunnerAddFunctionToList(pFunc);

	pFunc->type = FUNCTION_VARIABLE;
	pFunc->m_name = fname;
	pFunc->m_bReturns = true;
	pFunc->m_nArgs = 0;
	pFunc->m_args = NULL; // this only applies to statement-functions
	if (statement)
	{
		pFunc->m_pContents = RunStatement(statement);
	}
	else
	{
		pFunc->m_pContents = VariantCreateInt(0);
	}
}

Function * RunnerLookUpFunction(const char * name)
{
	Function* fn = g_functionsList;
	while (fn)
	{
		if (strcmp(fn->m_name, name) == 0)
			return fn;

		fn = fn->m_nlink;
	}

	return NULL;
}

extern Statement* g_mainBlock;

Variant* RunStatementSub(Statement* pStatement)
{
	// well, it depends on the type of statement
	switch (pStatement->type)
	{
		case STMT_NULL: return VariantCreateNull();
		case STMT_BLOCK:
		{
			// For each statement within the block, run it with zero arguments.
			StatementBlkData* pData = pStatement->m_blk_data;

			for (size_t i = 0; i < pData->m_nstatements; i++)
			{
				Variant* returnValue = RunStatement(pData->m_statements[i]);

				// TempleOS style. If this statement was a simple string, just print it.
				if (pData->m_statements[i]->type == STMT_STRING)
				{
					LogMsg("%s", pData->m_statements[i]->m_str_data->m_str);
				}

				// if it returned something, we most likely won't use it. Free the memory.
				if (returnValue)
				{
					// However if this is a return statement, return it right away.
					if (pData->m_statements[i]->type == STMT_RETURN)
					{
						return returnValue;
					}
					VariantFree(returnValue);
				}
			}

			break;
		}
		case STMT_RETURN:
		{
			// This executes the statement afterwards and passes its return value through.
			StatementRetData* pData = pStatement->m_ret_data;

			return RunStatement(pData->m_statement);
		}
		case STMT_FUNCTION:
		{
			// Add the function to the list of known functions.
			StatementFunData* pData = pStatement->m_fun_data;

			Function* pPreExistingFunc = RunnerLookUpFunction(pData->m_name);
			if (pPreExistingFunc)
			{
				// If the function has the same body, it should be okay.
				if (pPreExistingFunc->type != FUNCTION_STATEMENT || pPreExistingFunc->m_pStatement != pData->m_statement)
				{
					RunnerOnError(ERROR_FUNCTION_ALREADY_EXISTS);
				}
				else
				{
					break;
				}
			}

			RunnerAddFunctionStatement(pData->m_statement, pData->m_name, (int)pData->m_nargs, pData->m_args, true);

			break;
		}
		case STMT_VARIABLE:
		{
			StatementVarData* pData = pStatement->m_var_data;

			Function* pPreExistingFunc = RunnerLookUpFunction(pData->m_name);
			if (pPreExistingFunc)
			{
				// If the variable has the same body, it should be okay.
				if (pPreExistingFunc->type != FUNCTION_STATEMENT || pPreExistingFunc->m_pStatement != pData->m_statement)
				{
					RunnerOnError(ERROR_VARIABLE_ALREADY_EXISTS);
				}
				else
				{
					break;
				}
			}
			
			RunnerAddFunctionVariable(pData->m_statement, pData->m_name);

			break;
		}
		case STMT_ASSIGNMENT:
		{
			StatementVarData* pData = pStatement->m_var_data;

			Function* pPreExistingFunc = RunnerLookUpFunction(pData->m_name);

			//well, I'm going to be nice and allow this behavior
			if (!pPreExistingFunc)
			{
				RunnerAddFunctionVariable(pData->m_statement, pData->m_name);
				pPreExistingFunc = RunnerLookUpFunction(pData->m_name);
			}

			if (pPreExistingFunc->type != FUNCTION_VARIABLE)
			{
				RunnerOnError(ERROR_ASSIGNEE_IS_NOT_VARIABLE);
			}

			// Note : The variable may get accessed during the modification. This is why we perform a backup here.

			Variant* pOldContents = pPreExistingFunc->m_pContents;

			pPreExistingFunc->m_pContents = RunStatement(pData->m_statement);

			if (pOldContents)
				VariantFree(pOldContents);

			break;
		}
		case STMT_NUMBER:
		{
			return VariantCreateInt(pStatement->m_num_data->m_value);
		}
		case STMT_STRING:
		{
			return VariantCreateString(pStatement->m_str_data->m_str);
		}
		case STMT_IF:
		case STMT_WHILE:
		{
			bool bIsIf = pStatement->type == STMT_IF;
			bool bHitOnce = false;

			while (!bHitOnce || !bIsIf)
			{
				bHitOnce = true;

				Variant* pVar = RunStatement(pStatement->m_if_data->m_condition);
				if (pVar->m_type != VAR_INT)
				{
					RunnerOnError(bIsIf ? ERROR_IF_EXPECTS_INT : ERROR_WHILE_EXPECTS_INT);
				}

				long long value = pVar->m_intValue;
				VariantFree(pVar);

				// If this value is false, break out of the loop.
				if (value)
				{
					Variant* pVar = RunStatement(pStatement->m_if_data->m_true_part);
					VariantFree(pVar);
				}
				else
				{
					if (pStatement->m_if_data->m_false_part)
					{
						Variant* pVar = RunStatement(pStatement->m_if_data->m_false_part);
						VariantFree(pVar);
					}
					break;
				}
			}
			break;
		}
		case STMT_COMMAND:
		{
			StatementCmdData* pData = pStatement->m_cmd_data;

			Function fakeFuncObject; // keep the fake on the stack despite that we only need it for argument searching

			Function* pFunc = RunnerLookUpFunction(pData->m_name);
			if (!pFunc)
			{
				// Okay, well maybe this is an argument. Scan through all the arguments of this function
				CallStackFrame* callStackFrame = &g_callStack[g_callStackPointer];
				Function* pFunction = callStackFrame->m_pFunction;

				if (pFunction)
				{
					for (int i = 0; i < pFunction->m_nArgs; i++)
					{
						if (strcmp(pFunction->m_args[i], pData->m_name) == 0)
						{
							// Found our match!! Let's make the fakeFuncObject point to the argument.
							memset(&fakeFuncObject, 0, sizeof fakeFuncObject);

							fakeFuncObject.m_args = NULL;
							fakeFuncObject.m_nArgs = 0;
							fakeFuncObject.m_bReturns = true;
							fakeFuncObject.m_name = pData->m_name;
							fakeFuncObject.m_pContents = callStackFrame->m_args[i];
							fakeFuncObject.type = FUNCTION_VARIABLE;

							pFunc = &fakeFuncObject;
							break;
						}
					}
				}

				if (!pFunc)
				{
					RunnerOnError(ERROR_UNKNOWN_FUNCTION);
				}
			}

			if (pFunc->type == FUNCTION_VARIABLE)
			{
				if (pData->m_nargs != 0)
				{
					RunnerOnError(ERROR_SPECIFIED_ARGUMENTS);
				}

				return VariantDuplicate(pFunc->m_pContents);
			}

			// If the command's argument calls don't match..
			if ((int)pData->m_nargs < pFunc->m_nArgs)
				RunnerOnError(ERROR_TOO_FEW_ARGUMENTS);
			if ((int)pData->m_nargs > pFunc->m_nArgs)
				RunnerOnError(ERROR_TOO_MANY_ARGUMENTS);

			Variant* args[C_MAX_ARGS] = { 0 };

			if (pData->m_nargs >= C_MAX_BUILTIN_ARGS)
			{
				RunnerOnError(ERROR_TOO_MANY_ARGUMENTS);
			}

			for (size_t i = 0; i < pData->m_nargs; i++)
			{
				args[i] = RunStatement(pData->m_args[i]);
			}

			Variant* returnValue = NULL;

			switch (pFunc->type)
			{
				case FUNCTION_POINTER:
				{
					returnValue = pFunc->m_pFunction(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
					break;
				}
				case FUNCTION_STATEMENT:
				{
					// This is a statement function.
					if (g_callStackPointer + 1 >= C_MAX_STACK)
					{
						RunnerOnError(ERROR_STACK_OVERFLOW);
					}

					// Add a new value to the call stack
					CallStackFrame* pItem = &g_callStack[g_callStackPointer + 1];
					pItem->m_pFunction = pFunc;
					memcpy(pItem->m_args, args, C_MAX_ARGS * sizeof(Variant*));

					g_callStackPointer++;
					returnValue = RunStatement(pFunc->m_pStatement);
					g_callStackPointer--;

					break;
				}
				default:
				{
					LogMsg("Don't know how to run function type %d with name %s", pFunc->type, pFunc->m_name);
					RunnerOnError(ERROR_UNKNOWN_STATEMENT); 
				}
			}

			// free the args
			for (size_t i = 0; i < pData->m_nargs; i++)
			{
				if (args[i])
				{
					VariantFree(args[i]);
				}
				args[i] = NULL;
			}

			if (!returnValue)
			{
				return VariantCreateNull();
			}

			return returnValue;
		}

		default:
		{
			LogMsg("Don't know how to run statement type %d", pStatement->type);
			RunnerOnError(ERROR_UNKNOWN_STATEMENT);
		}
	}

	return VariantCreateNull();
}

void PrepareGlobals(Statement* pStatement)
{
	// first, ensure that the main block is a block statement. It better be!
	if (pStatement->type != STMT_BLOCK) return;

	for (size_t i = 0; i < pStatement->m_blk_data->m_nstatements; i++)
	{
		Statement* pStmt = pStatement->m_blk_data->m_statements[i];
		if (pStmt->type == STMT_FUNCTION || pStmt->type == STMT_VARIABLE)
		{
			RunStatement(pStmt);
		}
	}
}

Variant* RunStatement(Statement* pStatement)
{
	Statement* pStmtBkp = g_pCurrentStatement;
	g_pCurrentStatement = pStatement;
	Variant* pRes = RunStatementSub(pStatement);
	g_pCurrentStatement = pStmtBkp;
	return pRes;
}

// Note: This is added here instead of in builtin.c because we need access to the call stack.
Variant* BuiltInArgs(Variant* index)
{
	if (index->m_type != VAR_INT)
		RunnerOnError(ERROR_EXPECTED_INT_PARM);

	if (index->m_intValue < 0 || index->m_intValue >= g_callStack[0].m_nargs - 1)
		RunnerOnError(ERROR_ARRAY_INDEX_OUT_OF_BOUNDS);

	return VariantDuplicate(g_callStack[0].m_args[1+index->m_intValue]);
}

void RunnerPrepareRunTime()
{
	// This prepares the runtime.
	RunnerAddStandardFunctions();
	PrepareGlobals(g_mainBlock);
}

void RunFunction(Function* pFunc)
{
	switch (pFunc->type)
	{
		case FUNCTION_STATEMENT:
			RunStatement(pFunc->m_pStatement);
			break;
		case FUNCTION_POINTER:
			RunnerOnError(ERROR_UNKNOWN_FUNCTION);
			break;
		case FUNCTION_VARIABLE:
			break;
	}
}

// Calls a function.
// The argument string is something like "iiss", for two ints and two strings.
bool RunnerCall(const char* pFuncName, const char* argStr, ...)
{
	size_t strl = strlen(argStr);
	
	g_callStackPointer = 0;
	memset(&g_callStack[g_callStackPointer], 0, sizeof(CallStackFrame));
	
	Function * pFunc = RunnerLookUpFunction(pFuncName);
	if (!pFunc)
	{
		//RunnerOnError(ERROR_UNKNOWN_FUNCTION);
		LogMsg("Function '%s' not found.", pFuncName);
		return false;
	}
	
	if ((int)strl > pFunc->m_nArgs)
		RunnerOnError(ERROR_TOO_MANY_ARGUMENTS);
	if ((int)strl < pFunc->m_nArgs)
		RunnerOnError(ERROR_TOO_FEW_ARGUMENTS);
	
	va_list lst;
	va_start(lst, argStr);
	
	// Set up the arguments
	CallStackFrame* pFrame = &g_callStack[0];
	pFrame->m_pFunction = pFunc;
	pFrame->m_nargs = strl;
	
	for (size_t i = 0; i < strl; i++)
	{
		switch (argStr[i])
		{
			case 'i':
				// This argument shall be an int.
				pFrame->m_args[i] = VariantCreateInt(va_arg(lst, int));
				break;
			case 's':
				// This argument shall be a string.
				pFrame->m_args[i] = VariantCreateString(va_arg(lst, const char *));
				break;
			case 'n':
				// This argument shall be null
				pFrame->m_args[i] = VariantCreateNull();
				break;
			default:
				RunnerOnError(ERROR_UNKNOWN_VARIANT_TYPE);
				break;
		}
	}
	
	RunFunction(pFunc);
	
	// Free the arguments.
	pFrame->m_nargs = 0;
	for (size_t i = 0; i < strl; i++)
	{
		VariantFree(pFrame->m_args[i]);
		pFrame->m_args[i] = NULL;
	}
	
	return true;
}

/*
void RunnerGo(int argc, char** argv)
{
	if (argc >= C_MAX_ARGS - 1)
	{
		LogMsg("A maximum of %d arguments may be provided to the main function.", C_MAX_ARGS - 2);
		return;
	}

	RunnerAddStandardFunctions();

	// Build a 'main' function.
	char* argNames[C_MAX_ARGS];

	argNames[0] = StrDuplicate("argc");
	for (int i = 0; i < argc; i++)
	{
		char* argName = MemAllocate(64);
		snprintf(argName, sizeof argName, "arg%d", i);
		argNames[i + 1] = argName;
	}

	RunnerAddFunctionStatement(g_mainBlock, "main", argc+1, argNames, false);
	RunnerAddFunctionPtr(BuiltInArgs, "args", 1, true);
	
	g_callStackPointer = 0;
	memset(&g_callStack[g_callStackPointer], 0, sizeof(CallStackFrame));

	// Set up the arguments.
	g_callStack[0].m_nargs = argc + 1;
	g_callStack[0].m_args[0] = VariantCreateInt(argc);
	for (int i = 0; i < argc; i++)
	{
		g_callStack[0].m_args[i+1] = VariantCreateString(argv[i]);
	}
	g_callStack[0].m_pFunction = RunnerLookUpFunction("main");

	// prepare global functions.
	PrepareGlobals(g_mainBlock);

	Variant* chr = RunStatement(g_mainBlock);

	// If this happens to return anything, free it
	if (chr)
	{
		VariantFree(chr);
	}

	// Free the argument variants
	for (int i = 0; i < g_callStack[0].m_nargs; i++)
	{
		VariantFree(g_callStack[0].m_args[i]);
		free(argNames[i]);
	}
	g_callStack[0].m_nargs = 0;
}
*/
