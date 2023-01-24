// CrappyScript (C) 2023 iProgramInCpp

#pragma once

typedef enum
{
	TK_NONE,
	TK_STRING,
	TK_NUMBER,

	TK_SYMBOL_START,

	TK_SEMICOLON,
	TK_OPENBLOCK,
	TK_CLOSEBLOCK,
	TK_OPENPAREN,
	TK_CLOSEPAREN,
	TK_COMMA,
	TK_EQUALS,

	TK_KEYWORD_START,
	TK_SYMBOL_END = TK_KEYWORD_START,

	TK_IF,
	TK_THEN,
	TK_ELSE,
	TK_WHILE,
	TK_DO,
	TK_FINALLY,
	TK_FUNCTION,
	TK_FUNCTION_SHORT,
	TK_LET,
	TK_VAR,
	TK_ASSIGN,
	TK_TO,
	TK_BE,
	TK_RETURN,

	TK_KEYWORD_END,

	TK_END,
}
eToken;

typedef struct Token
{
	eToken m_type;
	char*  m_data;
	int    m_line;
}
Token;

void Tokenise(const char * str);
void TokenDump();
void TokenTeardown();
