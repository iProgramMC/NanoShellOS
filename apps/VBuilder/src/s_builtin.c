// CrappyScript (C) 2023 iProgramInCpp

#include <errno.h>
#include "s_all.h"

NORETURN void RunnerOnError(int error);

Variant* BuiltInNull()
{
	return NULL;
}

Variant* BuiltInHelp()
{
	LogMsg("CrappyScript Help");
	LogMsg("\nBuilt-in functions:");
	LogMsg("help()        - Shows this list");
	LogMsg("ver()         - Shows the script version");
	LogMsg("getver()      - Returns the script version as a string");
	LogMsg("echo()        - Prints a string or integer and a new line");
	LogMsg("equals(a,b)   - Checks if two objects are equal in value");
	LogMsg("concat(a,b)   - Concatenates two strings and returns a string");
	LogMsg("str(a)        - Converts any object into its string representation");
	LogMsg("int(a)        - Converts any object into its integer representation if it can");
	LogMsg("add(a,b)      - Adds two numbers");
	LogMsg("sub(a,b)      - Subtracts two numbers");
	LogMsg("mul(a,b)      - Multiplies two numbers");
	LogMsg("div(a,b)      - Divides two numbers");
	LogMsg("and(a,b)      - Performs a binary AND on two numbers");
	LogMsg("or(a,b)       - Performs a binary OR on two numbers");
	LogMsg("lt(a,b)       - Returns 1 if A is less than B, 0 otherwise");
	LogMsg("gt(a,b)       - Returns 1 if A is more than B, 0 otherwise");
	LogMsg("substr(a,b,c) - Returns a substring part of the original string 'a', starting");
	LogMsg("                at index 'b' with length 'c'.");
	LogMsg("argc          - Returns the number of arguments passed into the script");
	LogMsg("args(a)       - Returns the a'th (zero-indexed) argument.");
	LogMsg("arg ## a      - Returns the a'th (zero-indexed) argument.");
	LogMsg("\nLanguage constructions:");
	LogMsg("\n* if <condition> then <statement> [else <statement>]");
	LogMsg("   The usual 'if' statement. The condition statement must always return an");
	LogMsg("integer. If the condition statement results in a zero, the 'else' branch is");
	LogMsg("run. Otherwise, the 'then' branch is run, and the 'else' branch is skipped.");
	LogMsg("\n* while <condition> do <statement> [finally <statement>]");
	LogMsg("   The 'while' statement is just like the 'if' statement in terms of its");
	LogMsg("construction. While <condition> returns a non-zero value, the 'do' branch");
	LogMsg("is run, but once the condition returns zero, the 'finally' branch will run.");
	LogMsg("\n* Note that this language is still under construction, so not all features");
	LogMsg("have been implemented yet.\n");
	return NULL;
}

Variant* BuiltInVersion()
{
	LogMsg("CrappyScript Version %s", SHELL_VERSION_STRING);
	return NULL;
}

Variant* BuiltInGetVer()
{
	return VariantCreateString(SHELL_VERSION_STRING);
}

Variant* BuiltInEcho(Variant* str)
{
	switch (str->m_type)
	{
		case VAR_INT:
			LogMsg("%lld", str->m_intValue);
			break;
		case VAR_STRING:
			LogMsg("%s", str->m_strValue);
			break;
		case VAR_NULL:
			LogMsg("(null)");
			break;
		default:
			RunnerOnError(ERROR_UNKNOWN_VARIANT_TYPE);
			break;
	}
	return NULL;
}

Variant* BuiltInEquals(Variant* str1, Variant* str2)
{
	if (str1->m_type != str2->m_type) return VariantCreateInt(0);

	switch (str1->m_type)
	{
		case VAR_INT:
			return VariantCreateInt(str1->m_intValue == str2->m_intValue);
		case VAR_STRING:
			return VariantCreateInt(!strcmp(str1->m_strValue, str2->m_strValue));
		case VAR_NULL:
			return VariantCreateInt(1); // all such variants are the same
		default:
			RunnerOnError(ERROR_UNKNOWN_VARIANT_TYPE);
	}
}

Variant* BuiltInConcat(Variant* str1, Variant* str2)
{
	if (str1->m_type != VAR_STRING || str2->m_type != VAR_STRING)
		RunnerOnError(ERROR_EXPECTED_STRING_PARM);

	size_t len1 = strlen(str1->m_strValue), len2 = strlen(str2->m_strValue);

	char* cpy = MemAllocate(len1 + len2 + 1);
	if (!cpy) RunnerOnError(ERROR_R_MEMORY_ALLOC_FAILURE);

	memcpy(cpy,        str1->m_strValue, len1);
	memcpy(cpy + len1, str2->m_strValue, len2 + 1);

	Variant* pVar = VariantCreateString(cpy);
	MemFree(cpy);

	return pVar;
}

Variant* BuiltInToString(Variant* var)
{
	switch (var->m_type)
	{
		case VAR_STRING:
		{
			return VariantDuplicate(var);
		}
		case VAR_INT:
		{
			char buffer[64];
			snprintf(buffer, sizeof buffer, "%lld", var->m_intValue);
			return VariantCreateString(buffer);
		}
		case VAR_NULL:
		{
			return VariantCreateString("");
		}
		default:
		{
			RunnerOnError(ERROR_UNKNOWN_VARIANT_TYPE);
		}
	}
}

Variant* BuiltInToInt(Variant* var)
{
	switch (var->m_type)
	{
		case VAR_STRING:
		{
			errno = 0;
			long long value = strtoll(var->m_strValue, NULL, 0);
			if (errno != 0)
				RunnerOnError(ERROR_INT_CONVERSION_FAILURE);

			return VariantCreateInt(value);
		}
		case VAR_INT:
		{
			return VariantDuplicate(var);
		}
		case VAR_NULL:
		{
			return VariantCreateInt(0);
		}
		default:
		{
			RunnerOnError(ERROR_UNKNOWN_VARIANT_TYPE);
		}
	}
}

Variant* BuiltInAdd(Variant* var1, Variant* var2)
{
	if (var1->m_type != VAR_INT || var2->m_type != VAR_INT)
		RunnerOnError(ERROR_EXPECTED_INT_PARM);

	return VariantCreateInt(var1->m_intValue + var2->m_intValue);
}

Variant* BuiltInSub(Variant* var1, Variant* var2)
{
	if (var1->m_type != VAR_INT || var2->m_type != VAR_INT)
		RunnerOnError(ERROR_EXPECTED_INT_PARM);

	return VariantCreateInt(var1->m_intValue - var2->m_intValue);
}

Variant* BuiltInMul(Variant* var1, Variant* var2)
{
	if (var1->m_type != VAR_INT || var2->m_type != VAR_INT)
		RunnerOnError(ERROR_EXPECTED_INT_PARM);

	return VariantCreateInt(var1->m_intValue * var2->m_intValue);
}

Variant* BuiltInDiv(Variant* var1, Variant* var2)
{
	if (var1->m_type != VAR_INT || var2->m_type != VAR_INT)
		RunnerOnError(ERROR_EXPECTED_INT_PARM);

	if (var2->m_intValue == 0)
		RunnerOnError(ERROR_DIVISION_BY_ZERO);

	return VariantCreateInt(var1->m_intValue / var2->m_intValue);
}

Variant* BuiltInLessThan(Variant* var1, Variant* var2)
{
	if (var1->m_type != VAR_INT || var2->m_type != VAR_INT)
		RunnerOnError(ERROR_EXPECTED_INT_PARM);

	return VariantCreateInt(var1->m_intValue < var2->m_intValue);
}

Variant* BuiltInMoreThan(Variant* var1, Variant* var2)
{
	if (var1->m_type != VAR_INT || var2->m_type != VAR_INT)
		RunnerOnError(ERROR_EXPECTED_INT_PARM);

	return VariantCreateInt(var1->m_intValue > var2->m_intValue);
}

Variant* BuiltInAnd(Variant* var1, Variant* var2)
{
	if (var1->m_type != VAR_INT || var2->m_type != VAR_INT)
		RunnerOnError(ERROR_EXPECTED_INT_PARM);

	return VariantCreateInt(var1->m_intValue & var2->m_intValue);
}

Variant* BuiltInOr(Variant* var1, Variant* var2)
{
	if (var1->m_type != VAR_INT || var2->m_type != VAR_INT)
		RunnerOnError(ERROR_EXPECTED_INT_PARM);

	return VariantCreateInt(var1->m_intValue | var2->m_intValue);
}

Variant* BuiltInSubstr(Variant* str, Variant* start, Variant* length)
{
	if (str->m_type != VAR_STRING)
		RunnerOnError(ERROR_EXPECTED_STRING_PARM);

	if (start->m_type != VAR_INT || length->m_type != VAR_INT)
		RunnerOnError(ERROR_EXPECTED_INT_PARM);

	size_t strLen = strlen(str->m_strValue);

	if (start->m_intValue < 0) RunnerOnError(ERROR_SUBSTRING_FAILURE);
	if (start->m_intValue + length->m_intValue > strLen) RunnerOnError(ERROR_SUBSTRING_FAILURE);

	char* strp = MemAllocate(length->m_intValue + 1);
	memcpy(strp, str->m_strValue + start->m_intValue, length->m_intValue);
	strp[length->m_intValue] = 0;

	Variant* pVar = VariantCreateString(strp);
	MemFree(strp);
	return pVar;
}

Variant* BuiltInLength(Variant* str)
{
	if (str->m_type != VAR_STRING)
		RunnerOnError(ERROR_EXPECTED_STRING_PARM);

	return VariantCreateInt(strlen(str->m_strValue));
}

void RunnerAddStandardFunctions()
{
	RunnerAddFunctionPtr(BuiltInHelp,     "help",   0, false);
	RunnerAddFunctionPtr(BuiltInVersion,  "ver",    0, false);
	RunnerAddFunctionPtr(BuiltInEcho,     "echo",   1, false);
	RunnerAddFunctionPtr(BuiltInGetVer,   "getver", 0, true);
	RunnerAddFunctionPtr(BuiltInEquals,   "equals", 2, true);
	RunnerAddFunctionPtr(BuiltInConcat,   "concat", 2, true);
	RunnerAddFunctionPtr(BuiltInToString, "str",    1, true);
	RunnerAddFunctionPtr(BuiltInToInt,    "int",    1, true);
	RunnerAddFunctionPtr(BuiltInNull,     "null",   0, true);
	RunnerAddFunctionPtr(BuiltInSubstr,   "substr", 3, true);
	RunnerAddFunctionPtr(BuiltInLength,   "length", 1, true);

	// Arithmetic operations
	RunnerAddFunctionPtr(BuiltInAdd,      "add", 2, true);
	RunnerAddFunctionPtr(BuiltInSub,      "sub", 2, true);
	RunnerAddFunctionPtr(BuiltInMul,      "mul", 2, true);
	RunnerAddFunctionPtr(BuiltInDiv,      "div", 2, true);
	RunnerAddFunctionPtr(BuiltInLessThan, "lt",  2, true);
	RunnerAddFunctionPtr(BuiltInMoreThan, "gt",  2, true);
	RunnerAddFunctionPtr(BuiltInAnd,      "and", 2, true);
	RunnerAddFunctionPtr(BuiltInOr,       "or",  2, true);
}

