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


