// CrappyScript (C) 2023 iProgramInCpp

#include "s_all.h"

extern jmp_buf g_errorJumpBuffer;

int g_parserLine = 0;
int ParserUpdateLine(int line)
{
	return g_parserLine = line;
}

NORETURN void ParserOnError(int error)
{
	LogMsg("At line %d:", g_parserLine);
	longjmp(g_errorJumpBuffer, error);
}

Statement* ParseBlockStatement  ();
Statement* ParseGenericStatement();
Statement* ParseStringStatement ();
Statement* ParseNumberStatement ();
Statement* ParseReturnStatement ();

#define IS(token, type) (token->m_type == type)

extern Token** tokens;
extern size_t ntokens;

Statement * g_mainBlock;

size_t g_currentToken;

bool ParserEndOfFile()
{
	return g_currentToken >= ntokens;
}

Token* PeekToken()
{
	if (ParserEndOfFile()) return NULL;
	return tokens[g_currentToken];
}

Token* ConsumeToken()
{
	if (ParserEndOfFile()) return NULL;
	return tokens[g_currentToken++];
}

Statement* ParserSetupBlockStatement(int line)
{
	Statement* pStmt = MemCAllocate(1, sizeof(Statement));
	if (!pStmt) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);
	
	pStmt->type = STMT_BLOCK;
	pStmt->m_firstLine = ParserUpdateLine(line);

	pStmt->m_blk_data = MemCAllocate(1, sizeof(StatementBlkData));
	if (!pStmt->m_blk_data) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);

	pStmt->m_blk_data->m_nstatements = 0;
	pStmt->m_blk_data->m_statements  = NULL;

	return pStmt;
}

Statement* ParserSetupCommandStatement(int line)
{
	Statement* pStmt = MemCAllocate(1, sizeof(Statement));
	if (!pStmt) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);

	pStmt->type = STMT_COMMAND;
	pStmt->m_firstLine = ParserUpdateLine(line);

	pStmt->m_cmd_data = MemCAllocate(1, sizeof(StatementCmdData));
	if (!pStmt->m_cmd_data) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);

	pStmt->m_cmd_data->m_name = NULL;
	pStmt->m_cmd_data->m_args = NULL;
	pStmt->m_cmd_data->m_nargs = 0;

	return pStmt;
}

Statement* ParserSetupIfStatement(int line)
{
	Statement* pStmt = MemCAllocate(1, sizeof(Statement));
	if (!pStmt) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);

	pStmt->type = STMT_IF;
	pStmt->m_firstLine = ParserUpdateLine(line);

	pStmt->m_if_data = MemCAllocate(1, sizeof(StatementIfData));
	if (!pStmt->m_if_data) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);

	pStmt->m_if_data->m_condition  = NULL;
	pStmt->m_if_data->m_true_part  = NULL;
	pStmt->m_if_data->m_false_part = NULL;

	return pStmt;
}

Statement* ParserSetupStringStatement(int line)
{
	Statement* pStmt = MemCAllocate(1, sizeof(Statement));
	if (!pStmt) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);

	pStmt->type = STMT_STRING;
	pStmt->m_firstLine = ParserUpdateLine(line);

	pStmt->m_str_data = MemCAllocate(1, sizeof(StatementStrData));
	if (!pStmt->m_str_data) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);

	pStmt->m_str_data->m_str = NULL;

	return pStmt;
}

Statement* ParserSetupNumberStatement(int line)
{
	Statement* pStmt = MemCAllocate(1, sizeof(Statement));
	if (!pStmt) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);

	pStmt->type = STMT_NUMBER;
	pStmt->m_firstLine = ParserUpdateLine(line);

	pStmt->m_num_data = MemCAllocate(1, sizeof(StatementNumData));
	if (!pStmt->m_num_data) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);

	pStmt->m_num_data->m_value = 0;

	return pStmt;
}

Statement* ParserSetupReturnStatement(int line)
{
	Statement* pStmt = MemCAllocate(1, sizeof(Statement));
	if (!pStmt) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);

	pStmt->type = STMT_RETURN;
	pStmt->m_firstLine = ParserUpdateLine(line);

	pStmt->m_ret_data = MemCAllocate(1, sizeof(StatementRetData));
	if (!pStmt->m_ret_data) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);

	pStmt->m_ret_data->m_statement = NULL;

	return pStmt;
}

Statement* ParserSetupFunctionStatement(int line)
{
	Statement* pStmt = MemCAllocate(1, sizeof(Statement));
	if (!pStmt) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);

	pStmt->type = STMT_FUNCTION;
	pStmt->m_firstLine = ParserUpdateLine(line);

	pStmt->m_fun_data = MemCAllocate(1, sizeof(StatementFunData));
	if (!pStmt->m_fun_data) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);

	pStmt->m_fun_data->m_name      = NULL;
	pStmt->m_fun_data->m_statement = NULL;
	pStmt->m_fun_data->m_args      = NULL;
	pStmt->m_fun_data->m_nargs     = 0;

	return pStmt;
}

Statement* ParserSetupVariableStatement(int line)
{
	Statement* pStmt = MemCAllocate(1, sizeof(Statement));
	if (!pStmt) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);

	pStmt->type = STMT_VARIABLE;
	pStmt->m_firstLine = ParserUpdateLine(line);

	pStmt->m_var_data = MemCAllocate(1, sizeof(StatementFunData));
	if (!pStmt->m_var_data) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);

	pStmt->m_var_data->m_name      = NULL;
	pStmt->m_var_data->m_statement = NULL;

	return pStmt;
}

Statement* ParseSetupAssignmentStatement(int line)
{
	Statement* pStmt = MemCAllocate(1, sizeof(Statement));
	if (!pStmt) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);

	pStmt->type = STMT_ASSIGNMENT;
	pStmt->m_firstLine = ParserUpdateLine(line);

	pStmt->m_var_data = MemCAllocate(1, sizeof(StatementFunData));
	if (!pStmt->m_var_data) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);

	pStmt->m_asg_data->m_varName   = NULL;
	pStmt->m_asg_data->m_statement = NULL;

	return pStmt;
}

void ParserAddStmtToBlockStmt(Statement* pBlockStmt, Statement* pAddedStmt)
{
	if (pBlockStmt->type != STMT_BLOCK) ParserOnError(ERROR_INTERNAL_NOT_A_BLOCK_STMT);

	// To the m_blk_data, add a statement.
	Statement** stmts = (Statement**)MemReAllocate(pBlockStmt->m_blk_data->m_statements, (pBlockStmt->m_blk_data->m_nstatements + 1) * sizeof(Statement*));
	if (!stmts) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);

	pBlockStmt->m_blk_data->m_statements = stmts;
	pBlockStmt->m_blk_data->m_statements[pBlockStmt->m_blk_data->m_nstatements++] = pAddedStmt;
}

void ParserAddArgToCmdStmt(Statement* pCmdStmt, Statement* arg)
{
	if (pCmdStmt->type != STMT_COMMAND) ParserOnError(ERROR_INTERNAL_NOT_A_COMMAND_STMT);

	// To the m_blk_data, add a statement.
	Statement** args = (Statement**)MemReAllocate(pCmdStmt->m_cmd_data->m_args, (pCmdStmt->m_cmd_data->m_nargs + 1) * sizeof(Statement*));
	if (!args) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);

	pCmdStmt->m_cmd_data->m_args = args;
	pCmdStmt->m_cmd_data->m_args[pCmdStmt->m_cmd_data->m_nargs++] = arg;
}

void ParserAddArgToFunStmt(Statement* pFunStmt, char* arg)
{
	if (pFunStmt->type != STMT_FUNCTION) ParserOnError(ERROR_INTERNAL_NOT_A_FUNCTION_STMT);

	// To the m_blk_data, add a statement.
	char** args = (char**)MemReAllocate(pFunStmt->m_fun_data->m_args, (pFunStmt->m_fun_data->m_nargs + 1) * sizeof(char*));
	if (!args) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);

	pFunStmt->m_fun_data->m_args = args;
	pFunStmt->m_fun_data->m_args[pFunStmt->m_fun_data->m_nargs++] = arg;
}

Statement* ParseCommandStatementInside(bool bCanExpectSemicolon)
{
	Token* token = PeekToken();
	if (token == NULL)
	{
		ParserOnError(ERROR_EXPECTED_STATEMENT);
		return NULL;
	}

	if (IS(token, TK_STRING))
		return ParseStringStatement();

	if (IS(token, TK_NUMBER))
		return ParseNumberStatement();

	if (!IS(token, TK_KEYWORD_START))
	{
		ParserOnError(ERROR_EXPECTED_KEYWORD);
	}

	Statement* pCmdStmt = ParserSetupCommandStatement(ConsumeToken()->m_line);

	pCmdStmt->m_cmd_data->m_name = StrDuplicate(token->m_data);

	token = PeekToken();

	if (!token) ParserOnError(ERROR_EXPECTED_SEMICOLON_OR_ARGUMENTS);

	if (!IS(token, TK_OPENPAREN))
	{
		// If we're starting to close up..
		if (!IS(token, TK_CLOSEPAREN))
		{
			if (IS(token, TK_COMMA))
			{
				if (bCanExpectSemicolon)
					ParserOnError(ERROR_EXPECTED_CLOSE_PAREN_OR_ARGUMENTS);
				else
					return pCmdStmt;
			}
			
			if (IS(token, TK_SEMICOLON))
			{
				if (!bCanExpectSemicolon)
					ParserOnError(ERROR_EXPECTED_CLOSE_PAREN_OR_ARGUMENTS);
				else
					return pCmdStmt;
			}
		}

		return pCmdStmt;
	}
	else
	{
		ConsumeToken();

		token = PeekToken();

		if (IS(token, TK_CLOSEPAREN))
		{
			ConsumeToken();
			return pCmdStmt;
		}

		// Parse the arguments
		while (true)
		{
			token = PeekToken();
			if (!token) ParserOnError(ERROR_UNTERMINATED_COMMAND_STMT);

			Statement* pSubArg = ParseCommandStatementInside(false);

			ParserAddArgToCmdStmt(pCmdStmt, pSubArg);

			// check if we have a comma
			token = PeekToken();
			if (!token) ParserOnError(ERROR_UNTERMINATED_COMMAND_STMT);

			if (IS(token, TK_COMMA))
			{
				ConsumeToken();
			}
			else if (IS(token, TK_CLOSEPAREN))
			{
				ConsumeToken();
				break;
			}
		}
	}

	return pCmdStmt;
}

Statement* ParseCommandStatement()
{
	if (!PeekToken()) ParserOnError(ERROR_EXPECTED_COMMAND_STATEMENT);

	Statement* pStmt = ParseCommandStatementInside(true);

	Token* token = ConsumeToken();
	if (!token) ParserOnError(ERROR_EXPECTED_SEMICOLON);
	if (!IS(token, TK_SEMICOLON)) ParserOnError(ERROR_EXPECTED_SEMICOLON);

	return pStmt;
}

Statement* ParseIfStatement()
{
	if (!PeekToken()) ParserOnError(ERROR_EXPECTED_IF_STATEMENT);
	if (!IS(PeekToken(), TK_IF)) ParserOnError(ERROR_EXPECTED_IF_STATEMENT);

	Statement* pIfStmt = ParserSetupIfStatement(ConsumeToken()->m_line);

	// get the condition
	pIfStmt->m_if_data->m_condition = ParseCommandStatementInside(false);

	Token* token;

	token = PeekToken();
	if (!IS(token, TK_THEN))
	{
		ParserOnError(ERROR_EXPECTED_THEN);
	}

	ConsumeToken();

	pIfStmt->m_if_data->m_true_part = ParseGenericStatement();

	token = PeekToken();
	if (!IS(token, TK_ELSE))
	{
		return pIfStmt;
	}

	ConsumeToken();

	pIfStmt->m_if_data->m_false_part = ParseGenericStatement();

	return pIfStmt;
}

Statement* ParseWhileStatement()
{
	if (!PeekToken()) ParserOnError(ERROR_EXPECTED_WHILE_STATEMENT);
	if (!IS(PeekToken(), TK_WHILE)) ParserOnError(ERROR_EXPECTED_WHILE_STATEMENT);
	
	Statement* pIfStmt = ParserSetupIfStatement(ConsumeToken()->m_line);
	pIfStmt->type = STMT_WHILE;

	// get the condition
	pIfStmt->m_if_data->m_condition = ParseCommandStatementInside(false);

	Token* token;

	token = PeekToken();
	if (!IS(token, TK_DO))
	{
		ParserOnError(ERROR_EXPECTED_DO);
	}

	ConsumeToken();

	pIfStmt->m_if_data->m_true_part = ParseGenericStatement();

	token = PeekToken();
	if (!IS(token, TK_FINALLY))
	{
		return pIfStmt;
	}

	ConsumeToken();

	pIfStmt->m_if_data->m_false_part = ParseGenericStatement();

	return pIfStmt;
}

Statement* ParseEmptyStatement()
{
	if (!PeekToken()) ParserOnError(ERROR_EXPECTED_EMPTY_STATEMENT);
	if (!IS(PeekToken(), TK_SEMICOLON)) ParserOnError(ERROR_EXPECTED_SEMICOLON);

	Statement* pStmt = MemCAllocate(1, sizeof(Statement));
	if (!pStmt) ParserOnError(ERROR_P_MEMORY_ALLOC_FAILURE);

	pStmt->type = STMT_NULL;
	pStmt->m_firstLine = ParserUpdateLine(ConsumeToken()->m_line);

	return pStmt;
}

Statement* ParseStringStatement()
{
	if (!PeekToken()) ParserOnError(ERROR_EXPECTED_STRING_STATEMENT);
	if (!IS(PeekToken(), TK_STRING)) ParserOnError(ERROR_EXPECTED_STRING_STATEMENT);

	Statement* pStmt = ParserSetupStringStatement(PeekToken()->m_line);

	const char* text = ConsumeToken()->m_data;
	if (!text) text = "";
		
	pStmt->m_str_data->m_str = StrDuplicate(text);

	return pStmt;
}

Statement* ParseNumberStatement()
{
	if (!PeekToken()) ParserOnError(ERROR_EXPECTED_STRING_STATEMENT);
	if (!IS(PeekToken(), TK_NUMBER)) ParserOnError(ERROR_EXPECTED_STRING_STATEMENT);

	Statement* pStmt = ParserSetupNumberStatement(PeekToken()->m_line);

	long long value = *((long long*)ConsumeToken()->m_data);

	pStmt->m_num_data->m_value = value;

	return pStmt;
}

Statement* ParseReturnStatement()
{
	if (!PeekToken()) ParserOnError(ERROR_EXPECTED_RETURN_STATEMENT);
	if (!IS(PeekToken(), TK_RETURN)) ParserOnError(ERROR_EXPECTED_RETURN_STATEMENT);
	
	Statement* pStmt = ParserSetupReturnStatement(ConsumeToken()->m_line);
	pStmt->m_ret_data->m_statement = ParseGenericStatement();
	// hack for now: If this is a STMT_COMMAND statement, it already ate the semicolon
	if (pStmt->m_ret_data->m_statement->type == STMT_COMMAND)
		return pStmt;

	// Ensure this declaration is finished off with a semicolon
	Token* tk = PeekToken();
	if (!IS(tk, TK_SEMICOLON))
	{
		ParserOnError(ERROR_EXPECTED_SEMICOLON);
	}

	return pStmt;
}

Statement* ParseFunctionStatement()
{
	if (!PeekToken()) ParserOnError(ERROR_EXPECTED_FUNCTION_STATEMENT);
	if (!IS(PeekToken(), TK_FUNCTION) && !IS(PeekToken(), TK_FUNCTION_SHORT)) ParserOnError(ERROR_EXPECTED_FUNCTION_STATEMENT);

	Token* pFunctionKeyWord = ConsumeToken(); // consume the 'function' word

	Token* token;

	token = PeekToken();
	if (!token) ParserOnError(ERROR_EXPECTED_FUNCTION_NAME);
	if (!IS(token, TK_KEYWORD_START)) ParserOnError(ERROR_EXPECTED_FUNCTION_NAME);

	Statement* pFunction = ParserSetupFunctionStatement(pFunctionKeyWord->m_line);
	pFunction->m_fun_data->m_name = StrDuplicate(token->m_data);

	// Consume the name token.
	ConsumeToken();

	// Peek into the next token.
	token = PeekToken();

	// Check if this is the start of an argument list.
	if (IS(token, TK_OPENPAREN))
	{
		ConsumeToken();

		while (true)
		{
			token = PeekToken();

			if (!token) ParserOnError(ERROR_UNTERMINATED_FUNCTION_DECL);
			if (!IS(token, TK_KEYWORD_START)) ParserOnError(ERROR_EXPECTED_ARGUMENTS);

			ParserAddArgToFunStmt(pFunction, StrDuplicate(token->m_data));

			ConsumeToken();

			// The next token should be a comma.
			token = PeekToken();
			if (!token) ParserOnError(ERROR_UNTERMINATED_FUNCTION_DECL);
			if (!IS(token, TK_COMMA))
			{
				if (IS(token, TK_CLOSEPAREN))
					break;

				ParserOnError(ERROR_EXPECTED_COMMA);
			}

			ConsumeToken();
		}

		ConsumeToken();
	}

	// Now get the function body.
	pFunction->m_fun_data->m_statement = ParseGenericStatement();

	return pFunction;
}

Statement* ParseLetStatement()
{
	Token* tk = PeekToken();
	if (!tk) ParserOnError(ERROR_EXPECTED_LET_STATEMENT);
	if (!IS(tk, TK_LET) && !IS(tk, TK_VAR)) ParserOnError(ERROR_EXPECTED_LET_STATEMENT);

	ConsumeToken();

	tk = PeekToken();
	if (!IS(tk, TK_KEYWORD_START)) ParserOnError(ERROR_EXPECTED_VARIABLE_NAME);

	Statement* pVarStmt = ParserSetupVariableStatement(ConsumeToken()->m_line);

	pVarStmt->m_var_data->m_name = StrDuplicate(tk->m_data);

	// Check the next token. If it's an equals sign, that means afterwards there will be a new statement.
	tk = PeekToken();

	if (IS(tk, TK_EQUALS) || IS(tk, TK_BE))
	{
		ConsumeToken();

		// okay, now parse a statement
		pVarStmt->m_var_data->m_statement = ParseGenericStatement();

		// hack for now: If this is a STMT_COMMAND statement, it already ate the semicolon
		if (pVarStmt->m_var_data->m_statement->type == STMT_COMMAND)
			return pVarStmt;
	}

	// Ensure this declaration is finished off with a semicolon
	tk = PeekToken();
	if (!IS(tk, TK_SEMICOLON))
	{
		ParserOnError(ERROR_EXPECTED_SEMICOLON);
	}

	return pVarStmt;
}

Statement* ParseAssignmentStatement()
{
	Token* tk = PeekToken();
	if (!tk) ParserOnError(ERROR_EXPECTED_ASSIGN_STATEMENT);
	if (!IS(tk, TK_ASSIGN)) ParserOnError(ERROR_EXPECTED_ASSIGN_STATEMENT);

	Statement* pStmt = ParseSetupAssignmentStatement(tk->m_line);

	ConsumeToken();

	tk = PeekToken();
	if (!IS(tk, TK_KEYWORD_START)) ParserOnError(ERROR_EXPECTED_VARIABLE_NAME);

	pStmt->m_asg_data->m_varName = StrDuplicate(tk->m_data);

	ConsumeToken();
	tk = PeekToken();
	if (!IS(tk, TK_EQUALS) && !IS(tk, TK_TO)) ParserOnError(ERROR_EXPECTED_EQUALS_OR_TO);

	ConsumeToken();

	// okay, now parse a statement
	pStmt->m_asg_data->m_statement = ParseGenericStatement();

	// hack for now: If this is a STMT_COMMAND statement, it already ate the semicolon
	if (pStmt->m_asg_data->m_statement->type == STMT_COMMAND)
		return pStmt;

	// Ensure this declaration is finished off with a semicolon
	tk = PeekToken();
	if (!IS(tk, TK_SEMICOLON))
	{
		ParserOnError(ERROR_EXPECTED_SEMICOLON);
	}

	return pStmt;
}

Statement* ParseGenericStatement()
{
	if (!PeekToken()) ParserOnError(ERROR_EXPECTED_STATEMENT);

	Token* tk = PeekToken();

	Statement* pStmt = NULL;

	if (IS(tk, TK_SEMICOLON))
		pStmt = ParseEmptyStatement();
	else if (IS(tk, TK_OPENBLOCK))
		pStmt = ParseBlockStatement();
	else if (IS(tk, TK_IF))
		pStmt = ParseIfStatement();
	else if (IS(tk, TK_WHILE))
		pStmt = ParseWhileStatement();
	else if (IS(tk, TK_STRING))
		pStmt = ParseStringStatement();
	else if (IS(tk, TK_NUMBER))
		pStmt = ParseNumberStatement();
	else if (IS(tk, TK_FUNCTION) || IS(tk, TK_FUNCTION_SHORT))
		pStmt = ParseFunctionStatement();
	else if (IS(tk, TK_LET) || IS(tk, TK_VAR))
		pStmt = ParseLetStatement();
	else if (IS(tk, TK_ASSIGN))
		pStmt = ParseAssignmentStatement();
	else if (IS(tk, TK_RETURN))
		pStmt = ParseReturnStatement();
	else
		pStmt = ParseCommandStatement();

	return pStmt;
}

void ParseBlockStatementInside(Statement* pBlockStmt)
{
	Token* tk = PeekToken();

	while (tk && !IS(tk, TK_CLOSEBLOCK))
	{
		ParserAddStmtToBlockStmt(pBlockStmt, ParseGenericStatement());

		tk = PeekToken();
	}
}

Statement* ParseBlockStatement()
{
	if (!PeekToken()) ParserOnError(ERROR_EXPECTED_BLOCK_STATEMENT);
	if (!IS(PeekToken(), TK_OPENBLOCK)) ParserOnError(ERROR_EXPECTED_BLOCK_STATEMENT);

	Statement* subBlockStmt = ParserSetupBlockStatement(ConsumeToken()->m_line);
	
	ParseBlockStatementInside(subBlockStmt);
	
	Token* tkn = ConsumeToken();
	if (tkn == NULL)
	{
		ParserOnError(ERROR_UNTERMINATED_BLOCK_STMT);
	}

	return subBlockStmt;
}

static void PadLineTo(int padding)
{
	char paddingStr[42];
	snprintf(paddingStr, sizeof paddingStr, "%%%ds", padding);
	LogMsgNoCr(paddingStr, "");
}

static const char* const g_ts[] =
{
	"STMT_NULL",
	"STMT_COMMAND",
	"STMT_BLOCK",
	"STMT_IF",
	"STMT_WHILE",
	"STMT_STRING",
	"STMT_FUNCTION",
	"STMT_VARIABLE",
	"STMT_ASSIGNMENT",
	"STMT_NUMBER",
	"STMT_RETURN",
};

const char* GetTypeString(eStatementType type)
{
	return g_ts[type];
}

void ParserDumpStatement(Statement* pStmt, int padding)
{
	PadLineTo(padding);

	if (!pStmt)
	{
		LogMsg("NULL statement");
		return;
	}

	LogMsgNoCr("Type %s", GetTypeString(pStmt->type));
	//LogMsgNoCr("Statement %p. Type %s", pStmt, GetTypeString(pStmt->type));

	switch (pStmt->type)
	{
		case STMT_BLOCK:
		{
			LogMsg("");
			for (size_t i = 0; i < pStmt->m_blk_data->m_nstatements; i++)
			{
				Statement* pStmtSub = pStmt->m_blk_data->m_statements[i];

				ParserDumpStatement(pStmtSub, padding + 4);
			}
			break;
		}
		case STMT_COMMAND:
		{
			LogMsg("  Name: %s   Arguments:", pStmt->m_cmd_data->m_name);

			for (size_t i = 0; i < pStmt->m_cmd_data->m_nargs; i++)
			{
				Statement* pStmtSub = pStmt->m_cmd_data->m_args[i];

				ParserDumpStatement(pStmtSub, padding + 4);
			}
			break;
		}
		case STMT_STRING:
		{
			LogMsg("   Contents: '%s'", pStmt->m_str_data->m_str);

			break;
		}
		case STMT_IF:
		case STMT_WHILE:
		{
			LogMsg("");
			PadLineTo(padding); LogMsg("Condition: ");
			ParserDumpStatement(pStmt->m_if_data->m_condition, padding + 4);

			PadLineTo(padding); LogMsg("True branch: ");
			ParserDumpStatement(pStmt->m_if_data->m_true_part, padding + 4);

			PadLineTo(padding); LogMsg("False branch: ");
			ParserDumpStatement(pStmt->m_if_data->m_false_part, padding + 4);
			break;
		}
		case STMT_FUNCTION:
		{
			LogMsgNoCr("  Name: %s Arg:", pStmt->m_fun_data->m_name);
			for (size_t i = 0; i < pStmt->m_fun_data->m_nargs; i++)
				LogMsgNoCr("%s%s", i == 0 ? "" : ",", pStmt->m_fun_data->m_args[i]);
			LogMsg("");

			PadLineTo(padding); LogMsg("Function body:");
			ParserDumpStatement(pStmt->m_fun_data->m_statement, padding + 4);
			break;
		}
		case STMT_VARIABLE:
		{
			if (!pStmt->m_var_data->m_statement)
			{
				LogMsg("  New Variable Name: %s  No Default Value", pStmt->m_var_data->m_name);
			}
			else
			{
				LogMsg("  New Variable Name: %s  Assignee:", pStmt->m_var_data->m_name);
				ParserDumpStatement(pStmt->m_var_data->m_statement, padding + 4);
			}
			break;
		}
		case STMT_NUMBER:
		{
			LogMsg("  Value: %lld", pStmt->m_num_data->m_value);
			break;
		}
		case STMT_ASSIGNMENT:
		{
			LogMsg("  Destination: %s  Assignee:", pStmt->m_asg_data->m_varName);
			ParserDumpStatement(pStmt->m_asg_data->m_statement, padding + 4);
			break;
		}
		case STMT_RETURN:
		{
			LogMsg("  Returns:");
			ParserDumpStatement(pStmt->m_ret_data->m_statement, padding + 4);
			break;
		}
		default:
		{
			LogMsg("");
			break;
		}
	}

	PadLineTo(padding);
	LogMsg("End-Statement");
}

void ParserFreeStatement(Statement* pStatement)
{
	if (!pStatement) return;

	switch (pStatement->type)
	{
		case STMT_NULL: break;
		case STMT_STRING:
		{
			MemFree(pStatement->m_str_data->m_str);
			break;
		}
		case STMT_COMMAND:
		{
			MemFree(pStatement->m_cmd_data->m_name);

			for (size_t i = 0; i < pStatement->m_cmd_data->m_nargs; i++)
			{
				ParserFreeStatement(pStatement->m_cmd_data->m_args[i]);
			}

			MemFree(pStatement->m_cmd_data->m_args);
			break;
		}
		case STMT_BLOCK:
		{
			for (size_t i = 0; i < pStatement->m_blk_data->m_nstatements; i++)
			{
				ParserFreeStatement(pStatement->m_blk_data->m_statements[i]);
			}

			MemFree(pStatement->m_blk_data->m_statements);
			break;
		}
		case STMT_FUNCTION:
		{
			for (size_t i = 0; i < pStatement->m_fun_data->m_nargs; i++)
			{
				MemFree(pStatement->m_fun_data->m_args[i]);
			}
			MemFree(pStatement->m_fun_data->m_args);
			ParserFreeStatement(pStatement->m_fun_data->m_statement);
			MemFree(pStatement->m_fun_data->m_name);
			break;
		}
		case STMT_VARIABLE:
		{
			ParserFreeStatement(pStatement->m_var_data->m_statement);
			MemFree(pStatement->m_var_data->m_name);
			break;
		}
		case STMT_IF:
		case STMT_WHILE:
		{
			ParserFreeStatement(pStatement->m_if_data->m_condition);
			ParserFreeStatement(pStatement->m_if_data->m_true_part);
			ParserFreeStatement(pStatement->m_if_data->m_false_part);
			break;
		}
		case STMT_ASSIGNMENT:
		{
			MemFree(pStatement->m_asg_data->m_varName);
			ParserFreeStatement(pStatement->m_asg_data->m_statement);
			break;
		}
		case STMT_RETURN:
		{
			ParserFreeStatement(pStatement->m_ret_data->m_statement);
			break;
		}
		case STMT_NUMBER:
		{
			break;
		}
	}

	MemFree(pStatement->m_data);
	MemFree(pStatement);
}

void Parse()
{
	g_currentToken = 0;

	g_mainBlock = ParserSetupBlockStatement(1);

	ParseBlockStatementInside(g_mainBlock);

	// dump the main block
	//ParserDumpStatement(g_mainBlock, 0);
}

void ParserTeardown()
{
	ParserFreeStatement(g_mainBlock);

	g_mainBlock = NULL;
}
