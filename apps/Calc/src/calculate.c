#include <nsstandard.h>

enum
{
	CALC_SUCCESS,
	CALC_FAIL,
	CALC_TOO_BIG,
	CALC_DIV_BY_ZERO,
	CALC_EXPECTED_NUM,
	CALC_EXPECTED_OP,
	CALC_UNK_CHAR,
};

typedef struct Token
{
	int type; int value; float valuef;
}
Token;

enum {
    TYPE_NONE,TYPE_NUMBER,TYPE_OPERATOR,

    TYPE_EOF=255
};
enum
{
	OP_NONE, OP_ADD, OP_SUB, OP_MUL, OP_DIV,
};
const int g_precedence [] = { 0,1,1,2,2 };//Multiplicaiton and division have higher precedence than addition and subtraction.
bool IsPrecedenceLower(int type1, int type2)
{
	return g_precedence[type1] < g_precedence[type2];
}
bool IsSpace(char c)
{
    return c == ' ';
}
bool IsDigit(char c)
{
    return c >= '0' && c <= '9';
}
Token outputStack[100]; int outputStackP = 1;
Token operatStack[100]; int operatStackP = 1;

#define PUSH_TO_OUTPUT(x) do { outputStack[outputStackP++] = x; } while (0)
#define PUSH_TO_OPERAT(x) do { operatStack[operatStackP++] = x; } while (0)

#define POP_OUTPUT do {--outputStackP; }while (0)
#define POP_OPERAT do {--operatStackP; }while (0)

#define TOP_OUTPUT outputStack[outputStackP-1]
#define TOP_OPERAT operatStack[operatStackP-1]

#define EMPTY_OUTPUT (outputStackP <= 1)
#define EMPTY_OPERAT (operatStackP <= 1)

void OnFinishThisToken(Token *pToken)
{
    if (pToken->type == TYPE_NUMBER)
    {
        PUSH_TO_OUTPUT(*pToken);
    }
    if (pToken->type == TYPE_OPERATOR)
    {
        if (IsPrecedenceLower(pToken->value, TOP_OPERAT.value))
        {
            //push everything to op-stack
            while (!EMPTY_OPERAT)
            {
                PUSH_TO_OUTPUT(TOP_OPERAT);
                POP_OPERAT;
            }
        }
        PUSH_TO_OPERAT(*pToken);
    }
    if (pToken->type == TYPE_EOF)
    {
        while (!EMPTY_OPERAT)
        {
            PUSH_TO_OUTPUT(TOP_OPERAT);
            POP_OPERAT;
        }
    }
    pToken->type  = TYPE_NONE;
    pToken->value = 0;
}

float Calculate(const char*pText, int *errorCode)
{
	if (strlen (pText) > 1000) return 0.0f;
	memset (outputStack, 0, sizeof (outputStack));
	memset (operatStack, 0, sizeof (operatStack));

	outputStackP = 1;
    operatStackP = 1;

	outputStack[0].type = TYPE_NONE;
	operatStack[0].type = TYPE_NONE;

	Token t;
	t.type  = TYPE_NONE;
	t.value = 0;

	int lastTokenType = TYPE_NONE;

	//convert this crap to RPN
    bool wasNeg = false;
    while (*pText)
    {
        char c = *pText;
        pText++;
        if (IsSpace(c))
        {
            lastTokenType = t.type;
            OnFinishThisToken(&t);
        }
        else if (IsDigit(c))
        {
            if (t.type == TYPE_NONE && lastTokenType == TYPE_NUMBER)
            {
                *errorCode = CALC_EXPECTED_OP;
                return 0.0f;
            }
            if (t.type != TYPE_NONE && t.type != TYPE_NUMBER)
            {
                lastTokenType = t.type;
                OnFinishThisToken(&t);
            }
            t.type = TYPE_NUMBER;
			if (!wasNeg) wasNeg = (t.value < 0);
            t.value = (t.value * 10) + (wasNeg ? -1 : 1) * (c - '0');
            wasNeg = false;
        }
        else
        {
            if (t.type == TYPE_NONE && lastTokenType == TYPE_OPERATOR)
            {
                *errorCode = CALC_EXPECTED_NUM;
                return 0.0f;
            }
            if (t.type != TYPE_NONE && t.type != TYPE_OPERATOR)
            {
                lastTokenType = t.type;
                OnFinishThisToken(&t);
            }

            t.type = TYPE_OPERATOR;
            switch (c)
            {
                case '+': t.value = OP_ADD; break;
                case '-': t.value = OP_ADD; wasNeg = true; break;
                case '*': t.value = OP_MUL; break;
                case '/': t.value = OP_DIV; break;
                default:
                    *errorCode = CALC_UNK_CHAR;
                    return 0.0f;
            }
            lastTokenType = t.type;
            OnFinishThisToken(&t);
        }
    }
    if (t.type == TYPE_OPERATOR)
    {
        *errorCode = CALC_EXPECTED_NUM;
        return 0.0f;
    }
    OnFinishThisToken(&t);
    t.type  = TYPE_EOF;
    t.value = 0;
    OnFinishThisToken(&t);

    //parse the RPN now.

    //print all ops so far:
    for (int i = 1; i < outputStackP; i++)
    {
        outputStack[i].valuef = (float)outputStack[i].value;

        if (outputStack[i].type == TYPE_OPERATOR)
        {
            Token* tb1 = &outputStack[i-1];
            Token* tb2 = &outputStack[i-2];

            if (tb1->type != TYPE_NUMBER)
            {
                *errorCode = CALC_EXPECTED_NUM;
                return 0.0f;
            }
            if (tb2->type != TYPE_NUMBER)
            {
                *errorCode = CALC_EXPECTED_NUM;
                return 0.0f;
            }

            //process the result:
            float result = tb2->valuef;
            switch (outputStack[i].value)
            {
                case OP_ADD: result += tb1->valuef; break;
                case OP_SUB: result -= tb1->valuef; break;
                case OP_MUL: result *= tb1->valuef; break;
                case OP_DIV:
                    if (tb1->valuef == 0.0f)
                    {
                        *errorCode = CALC_DIV_BY_ZERO;
                        return 0.0f;
                    }
                    result /= tb1->valuef;
                    break;
            }

            tb2->type   = TYPE_NUMBER;
            tb2->valuef = result;
            tb2->value  = (int)result;

            //compact the stack
            memmove(&outputStack[i-1],&outputStack[i+1],sizeof (outputStack) - sizeof(Token) * (i+1));

            outputStackP-=2;
            i -= 2;
        }
    }

    if (EMPTY_OUTPUT)
    {
        *errorCode = CALC_FAIL;
        return 0.0f;
    }

    *errorCode = CALC_SUCCESS;
    return outputStack[1].valuef;
}
const char* ErrorCodeToString (int ec)
{
	switch (ec)
	{
		case CALC_SUCCESS:      return "Success";
		case CALC_FAIL:         return "Calculation failure";
		case CALC_TOO_BIG:      return "Calculation too big to compute";
		case CALC_DIV_BY_ZERO:  return "Cannot divide by zero";
		case CALC_EXPECTED_NUM: return "Expected number";
		case CALC_EXPECTED_OP:  return "Expected operator";
		case CALC_UNK_CHAR:     return "Invalid character. Valid characters are '0-9' and '+*/-'.";
	}
	return "Unknown Calculation Error";
}
