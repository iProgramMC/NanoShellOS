/*****************************************
		NanoShell Operating System
	   (C) 2021, 2022 iProgramInCpp

       NSScript Interpreter  module
******************************************/

#include <main.h>
#include <console.h>
#include <video.h>
#include <vfs.h>
#include <keyboard.h>
#include <string.h>
#include <memory.h>
#include <misc.h>
#include <cinterp.h>
#include <print.h>
#include <idt.h>

// c4.c - C in four functions
// char, int, and pointer types
// if, while, return, and expression statements
// just enough features to allow self-compilation and a bit more
// Written by Robert Swierczek, ported to NanoShell by iProgramInCpp
extern Console* g_currentConsole;

// tokens and classes (operators last and in precedence order)
enum {
    Num = 128, Fun, Sys, Glo, Loc, Id, FunPro,
    Char, Else, Enum, If, Int, Return, Sizeof, While,
    Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};

// opcodes
enum {
    LEA, IMM, JMP, JSR, BZ, BNZ, ENT, ADJ, LEV, LI, LC, SI, SC, PSH,
    OR, XOR, AND, EQ, NE, LT, GT, LE, GE, SHL, SHR, ADD, SUB, MUL, DIV, MOD,
    OPEN, READ, CLOS, PRTF, PRTN, MALC, FREE, MSET, MCMP, RAND, DRPX, EXSC, RDCH, RDIN, CLSC,
    FLSC, FLRC, DRRC, SSCY, DRST, SPTF, MVCR, SLEP, EXIT
};

// types
enum { CHAR, INT, PTR };

// identifier offsets (since we can't create an ident struct)
enum { Tk, Hash, Name, Class, Type, Val, HClass, HType, HVal, Idsz };


#define exit(code) do {\
	LogMsg("Exit requested with code %d. We can't exit straight away so we will just let the application keep running, which might result in a crash or something.");\
	return;\
} while (0)

#define Except(mach, lo) do {\
	mach->g_pErrorExists = true;\
	mach->g_pErrorLine = __LINE__;\
	mach->g_pErrorString = lo;\
    mach->m_halted = true;\
	return;\
} while (0)

#define ExitIfAnythingWrong(mach) \
do {\
	if (mach->g_pErrorExists)\
		return;\
} while(0)
	

void CcAdvanceOpc(CMachine* pMachine) { ++pMachine->pText; }
void CcAdvanceImm(CMachine* pMachine) { ++pMachine->pText; }
void CcSetOpCodeHere(CMachine* pMachine, int opc) { *pMachine->pText = opc; }
void CcSetImmHere   (CMachine* pMachine, int imm) { *pMachine->pText = imm; }

void CcPrintOpCode(int opc, int optional/*opc <= ADJ*/) {
    LogMsgNoCr("%s",
        &"LEA \0IMM \0JMP \0JSR \0BZ  \0BNZ \0ENT \0ADJ \0LEV \0LI  \0LC  \0SI  \0SC  \0PSH \0"
        "OR  \0XOR \0AND \0EQ  \0NE  \0LT  \0GT  \0LE  \0GE  \0SHL \0SHR \0ADD \0SUB \0MUL \0DIV \0MOD \0"
        "OPEN\0READ\0CLOS\0PRTF\0PRTN\0MALC\0FREE\0MSET\0MCMP\0RAND\0DRPX\0EXSC\0RDCH\0RDIN\0CLSC\0"
        "FLSC\0FLRC\0DRRC\0SSCY\0DRST\0SPTF\0MVCR\0SLEP\0EXIT\0"[opc * 5]);
    if (opc <= ADJ && optional != (int)0xDDEEAAFF/*hack!!!*/) {
        LogMsg(" %d (%#x)", optional, optional);
    }
    else LogMsg("");
}

// equivalent to *++e = opc;
void CcPushOpCode(CMachine* pMachine, int opc) {
    CcAdvanceOpc(pMachine);
    CcSetOpCodeHere(pMachine, opc);
    //LogMsg("Pushed opcode: ");
    //CcPrintOpCode(opc, 0xDDEEAAFF);
}
// equivalent to *++e = imm;
void CcPushImm(CMachine* pMachine, int imm) {
    CcAdvanceImm(pMachine);
    CcSetImmHere(pMachine, imm);
    //LogMsg("Pushed immediate: %d\n", imm);
}
int CcGetOpCodeHere(CMachine* pMachine) { return *pMachine->pText; }
int CcGetImmHere   (CMachine* pMachine) { return *pMachine->pText; }
void CcGoBackOpc   (CMachine* pMachine) { pMachine->pText--; }
void CcGoBackImm   (CMachine* pMachine) { pMachine->pText--; }

void CcNextToken(CMachine* pMachine)
{
    char* pp;

    while ((pMachine->currentToken = *pMachine->pSource) != 0) {
        ++pMachine->pSource;
        if (pMachine->currentToken == '\n') {
            if (pMachine->printAssembly) {
                LogMsg("%d: %.*s", pMachine->lineNum, pMachine->pSource - pMachine->pLastSource, pMachine->pLastSource);
                pMachine->pLastSource = pMachine->pSource;
                while (pMachine->pLastText < pMachine->pText) {
                    int optional = 0, opcode = *++pMachine->pLastText;
                    if (opcode <= ADJ) optional = *++pMachine->pLastText;
                    CcPrintOpCode(opcode, optional);
                }
            }
            ++pMachine->lineNum;
        }
        else if (pMachine->currentToken == '#') {
            while (*pMachine->pSource != 0 && *pMachine->pSource != '\n') ++pMachine->pSource;
        }
        else if ((pMachine->currentToken >= 'a' && pMachine->currentToken <= 'z') || (pMachine->currentToken >= 'A' && pMachine->currentToken <= 'Z') || pMachine->currentToken == '_') {
            pp = pMachine->pSource - 1;
            while ((*pMachine->pSource >= 'a' && *pMachine->pSource <= 'z') || (*pMachine->pSource >= 'A' && *pMachine->pSource <= 'Z') || (*pMachine->pSource >= '0' && *pMachine->pSource <= '9') || *pMachine->pSource == '_')
                pMachine->currentToken = pMachine->currentToken * 147 + *pMachine->pSource++;
            pMachine->currentToken = (pMachine->currentToken << 6) + (pMachine->pSource - pp);
            pMachine->pCurrIdentifier = pMachine->pCurrSymbol;
            while (pMachine->pCurrIdentifier[Tk]) {
                if (pMachine->currentToken == pMachine->pCurrIdentifier[Hash] && !memcmp((char*)pMachine->pCurrIdentifier[Name], pp, pMachine->pSource - pp)) { pMachine->currentToken = pMachine->pCurrIdentifier[Tk]; return; }
                pMachine->pCurrIdentifier = pMachine->pCurrIdentifier + Idsz;
            }
            pMachine->pCurrIdentifier[Name] = (int)pp;
            pMachine->pCurrIdentifier[Hash] = pMachine->currentToken;
            pMachine->currentToken = pMachine->pCurrIdentifier[Tk] = Id;
            return;
        }
        else if (pMachine->currentToken >= '0' && pMachine->currentToken <= '9') {
            if ((pMachine->currentTokenValue = pMachine->currentToken - '0')) { while (*pMachine->pSource >= '0' && *pMachine->pSource <= '9') pMachine->currentTokenValue = pMachine->currentTokenValue * 10 + *pMachine->pSource++ - '0'; }
            else if (*pMachine->pSource == 'x' || *pMachine->pSource == 'X') {
                while ((pMachine->currentToken = *++pMachine->pSource) && ((pMachine->currentToken >= '0' && pMachine->currentToken <= '9') || (pMachine->currentToken >= 'a' && pMachine->currentToken <= 'f') || (pMachine->currentToken >= 'A' && pMachine->currentToken <= 'F')))
                    pMachine->currentTokenValue = pMachine->currentTokenValue * 16 + (pMachine->currentToken & 15) + (pMachine->currentToken >= 'A' ? 9 : 0);
            }
            else { while (*pMachine->pSource >= '0' && *pMachine->pSource <= '7') pMachine->currentTokenValue = pMachine->currentTokenValue * 8 + *pMachine->pSource++ - '0'; }
            pMachine->currentToken = Num;
            return;
        }
        else if (pMachine->currentToken == '/') {
            if (*pMachine->pSource == '/') {
                ++pMachine->pSource;
                while (*pMachine->pSource != 0 && *pMachine->pSource != '\n') ++pMachine->pSource;
            }
            else {
                pMachine->currentToken = Div;
                return;
            }
        }
        else if (pMachine->currentToken == '\'' || pMachine->currentToken == '"') {
            pp = pMachine->pData;
            while (*pMachine->pSource != 0 && *pMachine->pSource != pMachine->currentToken) {
                if ((pMachine->currentTokenValue = *pMachine->pSource++) == '\\') {
                    if ((pMachine->currentTokenValue = *pMachine->pSource++) == 'n') pMachine->currentTokenValue = '\n';
                }
                if (pMachine->currentToken == '"') *pMachine->pData++ = pMachine->currentTokenValue;
            }
            ++pMachine->pSource;
            if (pMachine->currentToken == '"') pMachine->currentTokenValue = (int)pp; else pMachine->currentToken = Num;
            return;
        }
        else if (pMachine->currentToken == '=') { if (*pMachine->pSource == '=') { ++pMachine->pSource; pMachine->currentToken = Eq; } else pMachine->currentToken = Assign; return; }
        else if (pMachine->currentToken == '+') { if (*pMachine->pSource == '+') { ++pMachine->pSource; pMachine->currentToken = Inc; } else pMachine->currentToken = Add; return; }
        else if (pMachine->currentToken == '-') { if (*pMachine->pSource == '-') { ++pMachine->pSource; pMachine->currentToken = Dec; } else pMachine->currentToken = Sub; return; }
        else if (pMachine->currentToken == '!') { if (*pMachine->pSource == '=') { ++pMachine->pSource; pMachine->currentToken = Ne; } return; }
        else if (pMachine->currentToken == '<') { if (*pMachine->pSource == '=') { ++pMachine->pSource; pMachine->currentToken = Le; } else if (*pMachine->pSource == '<') { ++pMachine->pSource; pMachine->currentToken = Shl; } else pMachine->currentToken = Lt; return; }
        else if (pMachine->currentToken == '>') { if (*pMachine->pSource == '=') { ++pMachine->pSource; pMachine->currentToken = Ge; } else if (*pMachine->pSource == '>') { ++pMachine->pSource; pMachine->currentToken = Shr; } else pMachine->currentToken = Gt; return; }
        else if (pMachine->currentToken == '|') { if (*pMachine->pSource == '|') { ++pMachine->pSource; pMachine->currentToken = Lor; } else pMachine->currentToken = Or; return; }
        else if (pMachine->currentToken == '&') { if (*pMachine->pSource == '&') { ++pMachine->pSource; pMachine->currentToken = Lan; } else pMachine->currentToken = And; return; }
        else if (pMachine->currentToken == '^') { pMachine->currentToken = Xor; return; }
        else if (pMachine->currentToken == '%') { pMachine->currentToken = Mod; return; }
        else if (pMachine->currentToken == '*') { pMachine->currentToken = Mul; return; }
        else if (pMachine->currentToken == '[') { pMachine->currentToken = Brak; return; }
        else if (pMachine->currentToken == '?') { pMachine->currentToken = Cond; return; }
        else if (pMachine->currentToken == '~' || pMachine->currentToken == ';' || pMachine->currentToken == '{' || pMachine->currentToken == '}' || pMachine->currentToken == '(' || pMachine->currentToken == ')' || pMachine->currentToken == ']' || pMachine->currentToken == ',' || pMachine->currentToken == ':') return;
    }
}
void CcOnExpression(CMachine* pMachine, int lev)
{
    int t, * d;

    if (!pMachine->currentToken) { LogMsg("%d: unexpected eof in expression", pMachine->lineNum); Except(pMachine, "Error in CcOnExpression"); }
    else if (pMachine->currentToken == Num) { CcPushOpCode(pMachine, IMM); CcPushImm(pMachine, pMachine->currentTokenValue); CcNextToken(pMachine); pMachine->currentExprType = INT; }
    else if (pMachine->currentToken == '"') {
        CcPushOpCode(pMachine, IMM); CcPushImm(pMachine, pMachine->currentTokenValue); CcNextToken(pMachine);
        while (pMachine->currentToken == '"') CcNextToken(pMachine);
		//FIXME
        pMachine->pData = (char*)((int)pMachine->pData + sizeof(int) & (unsigned int)(-(int)(sizeof(int)))); pMachine->currentExprType = PTR;
    }
    else if (pMachine->currentToken == Sizeof) {
        CcNextToken(pMachine); if (pMachine->currentToken == '(') CcNextToken(pMachine); else { LogMsg("%d: open paren expected in sizeof", pMachine->lineNum); Except(pMachine, "Error in CcOnExpression"); }
        pMachine->currentExprType = INT; if (pMachine->currentToken == Int) CcNextToken(pMachine); else if (pMachine->currentToken == Char) { CcNextToken(pMachine); pMachine->currentExprType = CHAR; }
        while (pMachine->currentToken == Mul) { CcNextToken(pMachine); pMachine->currentExprType = pMachine->currentExprType + PTR; }
        if (pMachine->currentToken == ')') CcNextToken(pMachine); else { LogMsg("%d: close paren expected in sizeof", pMachine->lineNum); Except(pMachine, "Error in CcOnExpression"); }
        CcPushOpCode(pMachine, IMM); CcPushImm(pMachine, (pMachine->currentExprType == CHAR) ? sizeof(char) : sizeof(int));
        pMachine->currentExprType = INT;
    }
    else if (pMachine->currentToken == Id) {
        d = pMachine->pCurrIdentifier; CcNextToken(pMachine);
        if (pMachine->currentToken == '(') {
            CcNextToken(pMachine);
            t = 0;
            while (pMachine->currentToken != ')') {
                CcOnExpression(pMachine, Assign);
                ExitIfAnythingWrong(pMachine);
                CcPushOpCode(pMachine, PSH);
                ++t;
                if (pMachine->currentToken == ',')
                    CcNextToken(pMachine);
            }
            CcNextToken(pMachine);
            if (d[Class] == Sys) CcPushImm(pMachine, d[Val]);
            else if (d[Class] == Fun) { CcPushImm(pMachine, JSR); CcPushImm(pMachine, d[Val]); }
            else {
                LogMsg("%d: bad function call", pMachine->lineNum);
                Except(pMachine, "Error in CcOnExpression");
            }
            if (t) { CcPushOpCode(pMachine, ADJ); CcPushImm(pMachine, t); }
            pMachine->currentExprType = d[Type];
        }
        else if (d[Class] == Num) { CcPushOpCode(pMachine, IMM); CcPushImm(pMachine, d[Val]); pMachine->currentExprType = INT; }
        else {
            if (d[Class] == Loc) { CcPushOpCode(pMachine, LEA); CcPushImm(pMachine, pMachine->localVariableOffset - d[Val]); }
            else if (d[Class] == Glo) { CcPushOpCode(pMachine, IMM); CcPushImm(pMachine, d[Val]); }
            else { LogMsg("%d: undefined variable", pMachine->lineNum); Except(pMachine, "Error in CcOnExpression"); }
            CcPushImm(pMachine, ((pMachine->currentExprType = d[Type]) == CHAR) ? LC : LI);
        }
    }
    else if (pMachine->currentToken == '(') {
        CcNextToken(pMachine);
        if (pMachine->currentToken == Int || pMachine->currentToken == Char) {
            t = (pMachine->currentToken == Int) ? INT : CHAR; CcNextToken(pMachine);
            while (pMachine->currentToken == Mul) { CcNextToken(pMachine); t = t + PTR; }
            if (pMachine->currentToken == ')') CcNextToken(pMachine); else { LogMsg("%d: bad cast", pMachine->lineNum); Except(pMachine, "Error in CcOnExpression"); }
            CcOnExpression(pMachine, Inc);
            ExitIfAnythingWrong(pMachine);
            pMachine->currentExprType = t;
        }
        else {
            CcOnExpression(pMachine, Assign);
            ExitIfAnythingWrong(pMachine);
            if (pMachine->currentToken == ')') CcNextToken(pMachine); else { LogMsg("%d: close paren expected", pMachine->lineNum); Except(pMachine, "Error in CcOnExpression"); }
        }
    }
    else if (pMachine->currentToken == Mul) {
        CcNextToken(pMachine); CcOnExpression(pMachine, Inc);
        ExitIfAnythingWrong(pMachine);
        if (pMachine->currentExprType > INT) pMachine->currentExprType = pMachine->currentExprType - PTR; else { LogMsg("%d: bad dereference", pMachine->lineNum); Except(pMachine, "Error in CcOnExpression"); }
        CcPushOpCode(pMachine, (pMachine->currentExprType == CHAR) ? LC : LI);
    }
    else if (pMachine->currentToken == And) {
        CcNextToken(pMachine); CcOnExpression(pMachine, Inc);
        ExitIfAnythingWrong(pMachine);
        if (CcGetOpCodeHere(pMachine) == LC || CcGetOpCodeHere(pMachine) == LI)
            CcGoBackOpc(pMachine);
        else { LogMsg("%d: bad address-of", pMachine->lineNum); Except(pMachine, "Error in CcOnExpression"); }
        pMachine->currentExprType = pMachine->currentExprType + PTR;
    }
    else if (pMachine->currentToken == '!') { CcNextToken(pMachine); CcOnExpression(pMachine, Inc); ExitIfAnythingWrong(pMachine); CcPushOpCode(pMachine, PSH); CcPushOpCode(pMachine, IMM); CcPushImm(pMachine, 0); CcPushOpCode(pMachine, EQ);  pMachine->currentExprType = INT; }
    else if (pMachine->currentToken == '~') { CcNextToken(pMachine); CcOnExpression(pMachine, Inc); ExitIfAnythingWrong(pMachine); CcPushOpCode(pMachine, PSH); CcPushOpCode(pMachine, IMM); CcPushImm(pMachine, -1); CcPushOpCode(pMachine, XOR); pMachine->currentExprType = INT; }
    else if (pMachine->currentToken == Add) { CcNextToken(pMachine); CcOnExpression(pMachine, Inc); ExitIfAnythingWrong(pMachine); pMachine->currentExprType = INT; }
    else if (pMachine->currentToken == Sub) {
        CcNextToken(pMachine); CcPushOpCode(pMachine, IMM);
        if (pMachine->currentToken == Num) { CcPushImm(pMachine, -pMachine->currentTokenValue); CcNextToken(pMachine); }
        else { CcPushImm(pMachine, -1); CcPushOpCode(pMachine, PSH); CcOnExpression(pMachine, Inc); ExitIfAnythingWrong(pMachine); CcPushOpCode(pMachine, MUL); }
        pMachine->currentExprType = INT;
    }
    else if (pMachine->currentToken == Inc || pMachine->currentToken == Dec) {
        t = pMachine->currentToken; CcNextToken(pMachine); CcOnExpression(pMachine, Inc); ExitIfAnythingWrong(pMachine);
        if (CcGetOpCodeHere(pMachine) == LC) { CcSetOpCodeHere(pMachine, PSH); CcPushOpCode(pMachine, LC); }
        else if (CcGetOpCodeHere(pMachine) == LI) { CcSetOpCodeHere(pMachine, PSH); CcPushOpCode(pMachine, LI); }
        else { LogMsg("%d: bad lvalue in pre-increment", pMachine->lineNum); Except(pMachine, "Error in CcOnExpression"); }
        CcPushOpCode(pMachine, PSH);
        CcPushOpCode(pMachine, IMM); CcPushImm(pMachine, (pMachine->currentExprType > PTR) ? sizeof(int) : sizeof(char));
        CcPushOpCode(pMachine, (t == Inc) ? ADD : SUB);
        CcPushOpCode(pMachine, (pMachine->currentExprType == CHAR) ? SC : SI);
    }
    else { LogMsg("%d: bad expression", pMachine->lineNum); Except(pMachine, "Error in CcOnExpression"); }

    while (pMachine->currentToken >= lev) { // "precedence climbing" or "Top Down Operator Precedence" method
        t = pMachine->currentExprType;
        if (pMachine->currentToken == Assign) {
            CcNextToken(pMachine);
            if (CcGetOpCodeHere(pMachine) == LC || CcGetOpCodeHere(pMachine) == LI) CcSetOpCodeHere(pMachine, PSH); else { LogMsg("%d: bad lvalue in assignment", pMachine->lineNum); Except(pMachine, "Error in CcOnExpression"); }
            CcOnExpression(pMachine, Assign); ExitIfAnythingWrong(pMachine); CcPushOpCode(pMachine, ((pMachine->currentExprType = t) == CHAR) ? SC : SI);
        }
        else if (pMachine->currentToken == Cond) {
            CcNextToken(pMachine);
            CcPushOpCode(pMachine, BZ);
            d = ++pMachine->pText;
            CcOnExpression(pMachine, Assign); ExitIfAnythingWrong(pMachine);
            if (pMachine->currentToken == ':') CcNextToken(pMachine); else { LogMsg("%d: conditional missing colon", pMachine->lineNum); Except(pMachine, "Error in CcOnExpression"); }
            *d = (int)(pMachine->pText + 3); CcPushOpCode(pMachine, JMP); d = ++pMachine->pText;
            CcOnExpression(pMachine, Cond); ExitIfAnythingWrong(pMachine);
            *d = (int)(pMachine->pText + 1);
        }
        else if (pMachine->currentToken == Lor) { CcNextToken(pMachine); CcPushOpCode(pMachine, BNZ); CcAdvanceOpc(pMachine); d = pMachine->pText; CcOnExpression(pMachine, Lan); ExitIfAnythingWrong(pMachine); *d = (int)(pMachine->pText + 1); pMachine->currentExprType = INT; }
        else if (pMachine->currentToken == Lan) { CcNextToken(pMachine); CcPushOpCode(pMachine, BZ);  CcAdvanceOpc(pMachine); d = pMachine->pText; CcOnExpression(pMachine, Or);  ExitIfAnythingWrong(pMachine); *d = (int)(pMachine->pText + 1); pMachine->currentExprType = INT; }
        else if (pMachine->currentToken == Or) { CcNextToken(pMachine); CcPushOpCode(pMachine, PSH);  CcOnExpression(pMachine, Xor); ExitIfAnythingWrong(pMachine); CcPushOpCode(pMachine, OR);  pMachine->currentExprType = INT; }
        else if (pMachine->currentToken == Xor) { CcNextToken(pMachine); CcPushOpCode(pMachine, PSH); CcOnExpression(pMachine, And); ExitIfAnythingWrong(pMachine); CcPushOpCode(pMachine, XOR); pMachine->currentExprType = INT; }
        else if (pMachine->currentToken == And) { CcNextToken(pMachine); CcPushOpCode(pMachine, PSH); CcOnExpression(pMachine, Eq);  ExitIfAnythingWrong(pMachine); CcPushOpCode(pMachine, AND); pMachine->currentExprType = INT; }
        else if (pMachine->currentToken == Eq) { CcNextToken(pMachine); CcPushOpCode(pMachine, PSH);  CcOnExpression(pMachine, Lt);  ExitIfAnythingWrong(pMachine); CcPushOpCode(pMachine, EQ);  pMachine->currentExprType = INT; }
        else if (pMachine->currentToken == Ne) { CcNextToken(pMachine); CcPushOpCode(pMachine, PSH);  CcOnExpression(pMachine, Lt);  ExitIfAnythingWrong(pMachine); CcPushOpCode(pMachine, NE);  pMachine->currentExprType = INT; }
        else if (pMachine->currentToken == Lt) { CcNextToken(pMachine); CcPushOpCode(pMachine, PSH);  CcOnExpression(pMachine, Shl); ExitIfAnythingWrong(pMachine); CcPushOpCode(pMachine, LT);  pMachine->currentExprType = INT; }
        else if (pMachine->currentToken == Gt) { CcNextToken(pMachine); CcPushOpCode(pMachine, PSH);  CcOnExpression(pMachine, Shl); ExitIfAnythingWrong(pMachine); CcPushOpCode(pMachine, GT);  pMachine->currentExprType = INT; }
        else if (pMachine->currentToken == Le) { CcNextToken(pMachine); CcPushOpCode(pMachine, PSH);  CcOnExpression(pMachine, Shl); ExitIfAnythingWrong(pMachine); CcPushOpCode(pMachine, LE);  pMachine->currentExprType = INT; }
        else if (pMachine->currentToken == Ge) { CcNextToken(pMachine); CcPushOpCode(pMachine, PSH);  CcOnExpression(pMachine, Shl); ExitIfAnythingWrong(pMachine); CcPushOpCode(pMachine, GE);  pMachine->currentExprType = INT; }
        else if (pMachine->currentToken == Shl) { CcNextToken(pMachine); CcPushOpCode(pMachine, PSH); CcOnExpression(pMachine, Add); ExitIfAnythingWrong(pMachine); CcPushOpCode(pMachine, SHL); pMachine->currentExprType = INT; }
        else if (pMachine->currentToken == Shr) { CcNextToken(pMachine); CcPushOpCode(pMachine, PSH); CcOnExpression(pMachine, Add); ExitIfAnythingWrong(pMachine); CcPushOpCode(pMachine, SHR); pMachine->currentExprType = INT; }
        else if (pMachine->currentToken == Add) {
            CcNextToken(pMachine); CcPushOpCode(pMachine, PSH); CcOnExpression(pMachine, Mul); ExitIfAnythingWrong(pMachine);
            if ((pMachine->currentExprType = t) > PTR) { CcPushOpCode(pMachine, PSH); CcPushOpCode(pMachine, IMM); CcPushImm(pMachine, sizeof(int)); CcPushOpCode(pMachine, MUL); }
            CcPushOpCode(pMachine, ADD);
        }
        else if (pMachine->currentToken == Sub) {
            CcNextToken(pMachine); CcPushOpCode(pMachine, PSH); CcOnExpression(pMachine, Mul); ExitIfAnythingWrong(pMachine);
            if (t > PTR && t == pMachine->currentExprType) { CcPushOpCode(pMachine, SUB); CcPushOpCode(pMachine, PSH); CcPushOpCode(pMachine, IMM); CcPushImm(pMachine, sizeof(int)); CcPushOpCode(pMachine, DIV); pMachine->currentExprType = INT; }
            else if ((pMachine->currentExprType = t) > PTR) { CcPushOpCode(pMachine, PSH); CcPushOpCode(pMachine, IMM); CcPushImm(pMachine, sizeof(int)); CcPushOpCode(pMachine, MUL); CcPushOpCode(pMachine, SUB); }
            else CcPushOpCode(pMachine, SUB);
        }
        else if (pMachine->currentToken == Mul) { CcNextToken(pMachine); CcPushOpCode(pMachine, PSH); CcOnExpression(pMachine, Inc); ExitIfAnythingWrong(pMachine); CcPushOpCode(pMachine, MUL); pMachine->currentExprType = INT; }
        else if (pMachine->currentToken == Div) { CcNextToken(pMachine); CcPushOpCode(pMachine, PSH); CcOnExpression(pMachine, Inc); ExitIfAnythingWrong(pMachine); CcPushOpCode(pMachine, DIV); pMachine->currentExprType = INT; }
        else if (pMachine->currentToken == Mod) { CcNextToken(pMachine); CcPushOpCode(pMachine, PSH); CcOnExpression(pMachine, Inc); ExitIfAnythingWrong(pMachine); CcPushOpCode(pMachine, MOD); pMachine->currentExprType = INT; }
        else if (pMachine->currentToken == Inc || pMachine->currentToken == Dec) {
            if (CcGetOpCodeHere(pMachine) == LC) { CcSetOpCodeHere(pMachine, PSH); CcPushOpCode(pMachine, LC); }
            else if (CcGetOpCodeHere(pMachine) == LI) { CcSetOpCodeHere(pMachine, PSH); CcPushOpCode(pMachine, LI); }
            else { LogMsg("%d: bad lvalue in post-increment", pMachine->lineNum); Except(pMachine, "Error in CcOnExpression"); }
            CcPushOpCode(pMachine, PSH); CcPushOpCode(pMachine, IMM); CcPushImm(pMachine, (pMachine->currentExprType > PTR) ? sizeof(int) : sizeof(char));
            CcPushOpCode(pMachine, (pMachine->currentToken == Inc) ? ADD : SUB);
            CcPushOpCode(pMachine, (pMachine->currentExprType == CHAR) ? SC : SI);
            CcPushOpCode(pMachine, PSH); CcPushOpCode(pMachine, IMM); CcPushImm(pMachine, (pMachine->currentExprType > PTR) ? sizeof(int) : sizeof(char));
            CcPushOpCode(pMachine, (pMachine->currentToken == Inc) ? SUB : ADD);
            CcNextToken(pMachine);
        }
        else if (pMachine->currentToken == Brak) {
            CcNextToken(pMachine); CcPushOpCode(pMachine, PSH); CcOnExpression(pMachine, Assign); ExitIfAnythingWrong(pMachine);
            if (pMachine->currentToken == ']') CcNextToken(pMachine); else { LogMsg("%d: close bracket expected", pMachine->lineNum); Except(pMachine, "Error in CcOnExpression"); }
            if (t > PTR) { CcPushOpCode(pMachine, PSH); CcPushOpCode(pMachine, IMM); CcPushImm(pMachine, sizeof(int)); CcPushOpCode(pMachine, MUL); }
            else if (t < PTR) { LogMsg("%d: pointer type expected", pMachine->lineNum); Except(pMachine, "Error in CcOnExpression"); }
            CcPushOpCode(pMachine, ADD);
            CcPushImm(pMachine, ((pMachine->currentExprType = t - PTR) == CHAR) ? LC : LI);
        }
        else { LogMsg("%d: compiler error pMachine->currentToken=%d", pMachine->lineNum, pMachine->currentToken); Except(pMachine, "Error in CcOnExpression"); }
    }
}
void CcStatement(CMachine *pMachine)
{
    int* a, * b;

    if (pMachine->currentToken == If) {
        CcNextToken(pMachine);
        if (pMachine->currentToken == '(') CcNextToken(pMachine); else { LogMsg("%d: open paren expected", pMachine->lineNum); Except(pMachine, "Error in CcStatement"); }
        CcOnExpression(pMachine, Assign);
        ExitIfAnythingWrong(pMachine);
        if (pMachine->currentToken == ')') CcNextToken(pMachine); else { LogMsg("%d: close paren expected", pMachine->lineNum); Except(pMachine, "Error in CcStatement"); }
        CcPushOpCode(pMachine, BZ);
        CcAdvanceOpc(pMachine);
        b = pMachine->pText;
        CcStatement(pMachine);
        ExitIfAnythingWrong(pMachine);
        if (pMachine->currentToken == Else) {
            *b = (int)(pMachine->pText + 3); CcPushOpCode(pMachine, JMP);  CcAdvanceOpc(pMachine);  b = pMachine->pText;
            CcNextToken(pMachine);
            CcStatement(pMachine);
            ExitIfAnythingWrong(pMachine);
        }
        *b = (int)(pMachine->pText + 1);
    }
    else if (pMachine->currentToken == While) {
        CcNextToken(pMachine);
        a = pMachine->pText + 1;
        if (pMachine->currentToken == '(') CcNextToken(pMachine); else { LogMsg("%d: open paren expected", pMachine->lineNum); Except(pMachine, "Error in CcStatement"); }
        CcOnExpression(pMachine, Assign);
        ExitIfAnythingWrong(pMachine);
        if (pMachine->currentToken == ')') CcNextToken(pMachine); else { LogMsg("%d: close paren expected", pMachine->lineNum); Except(pMachine, "Error in CcStatement"); }
        CcPushOpCode(pMachine, BZ); CcAdvanceOpc(pMachine);  b = pMachine->pText;
        CcStatement(pMachine);
        ExitIfAnythingWrong(pMachine);
        CcPushOpCode(pMachine, JMP); CcPushImm(pMachine, (int)a);
        *b = (int)(pMachine->pText + 1);
    }
    else if (pMachine->currentToken == Return) {
        CcNextToken(pMachine);
        if (pMachine->currentToken != ';') CcOnExpression(pMachine, Assign);
        ExitIfAnythingWrong(pMachine);
        CcPushOpCode(pMachine, LEV);
        if (pMachine->currentToken == ';') CcNextToken(pMachine); else { LogMsg("%d: semicolon expected", pMachine->lineNum); Except(pMachine, "Error in CcStatement"); }
    }
    else if (pMachine->currentToken == '{') {
        CcNextToken(pMachine);
        while (pMachine->currentToken != '}') CcStatement(pMachine);
        ExitIfAnythingWrong(pMachine);
        CcNextToken(pMachine);
    }
    else if (pMachine->currentToken == ';') {
        CcNextToken(pMachine);
    }
    else {
        CcOnExpression(pMachine, Assign);
        ExitIfAnythingWrong(pMachine);
        if (pMachine->currentToken == ';') CcNextToken(pMachine); else { LogMsg("%d: semicolon expected", pMachine->lineNum); Except(pMachine, "Error in CcStatement"); }
    }
}

bool CcBreakCheck(void)
{
	if (g_currentConsole == &g_debugConsole)
	{
		return(KbGetKeyState(KEY_CONTROL) == KEY_PRESSED && KbGetKeyState(KEY_C) == KEY_PRESSED);
	}
	
    return 0;
}

void CcOnAllocateSomething(CMachine* pMachine, void* pJustAllocated) {
    // find a free spot in the current allocations
    for (int i = 0; i < pMachine->g_memoryAllocCount; i++) {
        //! NOTE: Is this really necessary? g_memoryAllocCount would be 0 anyway :P
        if (pMachine->g_memoryAllocatedPointers[i] == NULL) {
            pMachine->g_memoryAllocatedPointers[i] = pJustAllocated;
            return;
        }
    }
    pMachine->g_memoryAllocatedPointers[pMachine->g_memoryAllocCount++] = pJustAllocated;
}
void CcOnDeallocateSomething(CMachine* pMachine, void* pJustAllocated) {
    for (int i = 0; i < pMachine->g_memoryAllocCount; i++) {
        //! NOTE: Is this really necessary? g_memoryAllocCount would be 0 anyway :P
        if (pMachine->g_memoryAllocatedPointers[i] == pJustAllocated) {
            pMachine->g_memoryAllocatedPointers[i] = NULL;
            if (i == pMachine->g_memoryAllocCount - 1) // is this the last element that we've got?
                pMachine->g_memoryAllocCount--;
            return;
        }
    }
}

CCSTATUS CcInitMachine(CMachine* pMachine)
{
    pMachine->g_memoryAllocCount = 0;

    pMachine->printAssembly = 0;
    pMachine->printCycles = 0;

    pMachine->g_pErrorExists = false;

    pMachine->main_poolSize = 256 * 1024; // arbitrary size
    if (!(pMachine->pStartOfSymbols = pMachine->pCurrSymbol = (int*)MmAllocate(pMachine->main_poolSize))) { 
        LogMsg("could not MmAllocate(%d) symbol area", pMachine->main_poolSize); 
        return CCSTATUS_NO_MALLOC_POOL_AREA; 
    }
    if (!(pMachine->pTextStart = pMachine->pLastText = pMachine->pText = (int*)MmAllocate(pMachine->main_poolSize))) { 
        LogMsg("could not MmAllocate(%d) text area", pMachine->main_poolSize); 
        return CCSTATUS_NO_MALLOC_TEXT_AREA;
    }
    if (!(pMachine->pDataStart = pMachine->pData = (char*)MmAllocate(pMachine->main_poolSize))) { 
        LogMsg("could not MmAllocate(%d) data area", pMachine->main_poolSize); 
        return CCSTATUS_NO_MALLOC_DATA_AREA;
    }
    if (!(pMachine->main_stackStart = pMachine->main_stackPtr = (int*)MmAllocate(pMachine->main_poolSize))) { 
        LogMsg("could not MmAllocate(%d) stack area", pMachine->main_poolSize); 
        return CCSTATUS_NO_MALLOC_STACK_AREA;
    }

    memset(pMachine->pCurrSymbol, 0, pMachine->main_poolSize);
    memset(pMachine->pText, 0, pMachine->main_poolSize);
    memset(pMachine->pData, 0, pMachine->main_poolSize);

    pMachine->pSource =
        "char else enum if int return sizeof while "//keywords

        //available function list
        "open read close LogMsg LogMsgNoCr malloc free memset memcmp Random DrawPixel ExecuteScript ReadChar ReadInt "
        "ClearScreen FillScreen FillRectangle DrawRectangle ShiftScreenY DrawString sprintf MoveCursor Sleep exit "

        //other keywords
        "void main"
        ;

    pMachine->main_tempI = Char;
    while (pMachine->main_tempI <= While) {
        CcNextToken(pMachine);
        pMachine->pCurrIdentifier[Tk] = pMachine->main_tempI++;
    } // add keywords to symbol table
    pMachine->main_tempI = OPEN;
    while (pMachine->main_tempI <= EXIT) {
        CcNextToken(pMachine);
        pMachine->pCurrIdentifier[Class] = Sys;
        pMachine->pCurrIdentifier[Type] = INT;
        pMachine->pCurrIdentifier[Val] = pMachine->main_tempI++;
    } // add library to symbol table
    CcNextToken(pMachine); pMachine->pCurrIdentifier[Tk] = Char; // handle void type
    CcNextToken(pMachine); pMachine->main_idMain = pMachine->pCurrIdentifier; // keep track of main

    return 0;
}
CCSTATUS CcCompileCode(CMachine* pMachine, const char* pCode, int length)
{
    pMachine->main_tempI = length;
	if (!length) pMachine->main_tempI = strlen(pCode);
    if (!(pMachine->pLastSource = pMachine->pSource = pMachine->pSourceStart = (char*)MmAllocate(pMachine->main_tempI+30))) {
        LogMsg("could not MmAllocate(%d) source area", pMachine->main_poolSize);
        return CCSTATUS_NO_MALLOC_SRC_AREA;
    }
    memcpy(pMachine->pSource, pCode, pMachine->main_tempI);
    pMachine->pSource[pMachine->main_tempI] = 0;
    pMachine->pSource[pMachine->main_tempI+1] = 0;
    pMachine->pSource[pMachine->main_tempI+2] = 0;
	//LogMsg("Source code:%s=====",pMachine->pSource);
	//LogMsg("MainTempI: %d",pMachine->main_tempI);

    // parse declarations
    pMachine->lineNum = 1;
    CcNextToken(pMachine);
    while (pMachine->currentToken) {
        pMachine->main_baseType = INT; // basetype
        if (pMachine->currentToken == Int) CcNextToken(pMachine);
        else if (pMachine->currentToken == Char) { CcNextToken(pMachine); pMachine->main_baseType = CHAR; }
        else if (pMachine->currentToken == Enum) {
            CcNextToken(pMachine);
            if (pMachine->currentToken != '{') CcNextToken(pMachine);
            if (pMachine->currentToken == '{') {
                CcNextToken(pMachine);
                pMachine->main_tempI = 0;
                while (pMachine->currentToken != '}') {
                    if (pMachine->currentToken != Id) { LogMsg("%d: bad enum identifier %d", pMachine->lineNum, pMachine->currentToken); return CCSTATUS_BAD_ENUM_IDENTIFIER; }
                    CcNextToken(pMachine);
                    if (pMachine->currentToken == Assign) {
                        CcNextToken(pMachine);
                        if (pMachine->currentToken != Num) { LogMsg("%d: bad enum initializer", pMachine->lineNum); return CCSTATUS_BAD_ENUM_INITIALIZER; }
                        pMachine->main_tempI = pMachine->currentTokenValue;
                        CcNextToken(pMachine);
                    }
                    pMachine->pCurrIdentifier[Class] = Num; pMachine->pCurrIdentifier[Type] = INT; pMachine->pCurrIdentifier[Val] = pMachine->main_tempI++;
                    if (pMachine->currentToken == ',') CcNextToken(pMachine);
                }
                CcNextToken(pMachine);
            }
        }
        while (pMachine->currentToken != ';' && pMachine->currentToken != '}') {
            pMachine->main_tokenType = pMachine->main_baseType;
            while (pMachine->currentToken == Mul) { CcNextToken(pMachine); pMachine->main_tokenType = pMachine->main_tokenType + PTR; }
            if (pMachine->currentToken != Id) { LogMsg("%d: bad global declaration", pMachine->lineNum); return CCSTATUS_BAD_GLOBAL_DECLARATION; }
            if (pMachine->pCurrIdentifier[Class]) {
                LogMsg("%d: duplicate global definition", pMachine->lineNum); return CCSTATUS_DUP_GLOBAL_DECLARATION;
            }
            CcNextToken(pMachine);
            pMachine->pCurrIdentifier[Type] = pMachine->main_tokenType;
            if (pMachine->currentToken == '(') { // function
                pMachine->pCurrIdentifier[Class] = Fun;
                pMachine->pCurrIdentifier[Val] = (int)(pMachine->pText + 1);
                CcNextToken(pMachine); pMachine->main_tempI = 0;
                while (pMachine->currentToken != ')') {
                    pMachine->main_tokenType = INT;
                    if (pMachine->currentToken == Int) CcNextToken(pMachine);
                    else if (pMachine->currentToken == Char) { CcNextToken(pMachine); pMachine->main_tokenType = CHAR; }
                    while (pMachine->currentToken == Mul) { CcNextToken(pMachine); pMachine->main_tokenType = pMachine->main_tokenType + PTR; }
                    if (pMachine->currentToken != Id) { LogMsg("%d: bad parameter declaration", pMachine->lineNum); return CCSTATUS_BAD_PARM_DECLARATION; }
                    if (pMachine->pCurrIdentifier[Class] == Loc) { LogMsg("%d: duplicate parameter definition", pMachine->lineNum); return CCSTATUS_DUP_PARM_DECLARATION; }
                    pMachine->pCurrIdentifier[HClass] = pMachine->pCurrIdentifier[Class]; pMachine->pCurrIdentifier[Class] = Loc;
                    pMachine->pCurrIdentifier[HType] = pMachine->pCurrIdentifier[Type];  pMachine->pCurrIdentifier[Type] = pMachine->main_tokenType;
                    pMachine->pCurrIdentifier[HVal] = pMachine->pCurrIdentifier[Val];   pMachine->pCurrIdentifier[Val] = pMachine->main_tempI++;
                    CcNextToken(pMachine);
                    if (pMachine->currentToken == ',') CcNextToken(pMachine);
                }
                CcNextToken(pMachine);
                if (pMachine->currentToken != '{') {
                    LogMsg("%d: bad function definition", pMachine->lineNum);
                    return CCSTATUS_BAD_FUNC_DECLARATION;
                }
                pMachine->localVariableOffset = ++pMachine->main_tempI;
                CcNextToken(pMachine);
                while (pMachine->currentToken == Int || pMachine->currentToken == Char) {
                    pMachine->main_baseType = (pMachine->currentToken == Int) ? INT : CHAR;
                    CcNextToken(pMachine);
                    while (pMachine->currentToken != ';') {
                        pMachine->main_tokenType = pMachine->main_baseType;
                        while (pMachine->currentToken == Mul) { CcNextToken(pMachine); pMachine->main_tokenType = pMachine->main_tokenType + PTR; }
                        if (pMachine->currentToken != Id) { LogMsg("%d: bad local declaration", pMachine->lineNum); return CCSTATUS_BAD_LOCAL_DECLARATION; }
                        if (pMachine->pCurrIdentifier[Class] == Loc) {
                            LogMsg("%d: duplicate local definition", pMachine->lineNum); return CCSTATUS_DUP_LOCAL_DECLARATION;
                        }
                        pMachine->pCurrIdentifier[HClass] = pMachine->pCurrIdentifier[Class]; pMachine->pCurrIdentifier[Class] = Loc;
                        pMachine->pCurrIdentifier[HType] = pMachine->pCurrIdentifier[Type];  pMachine->pCurrIdentifier[Type] = pMachine->main_tokenType;
                        pMachine->pCurrIdentifier[HVal] = pMachine->pCurrIdentifier[Val];   pMachine->pCurrIdentifier[Val] = ++pMachine->main_tempI;
                        CcNextToken(pMachine);
                        if (pMachine->currentToken == ',') CcNextToken(pMachine);
                    }
                    CcNextToken(pMachine);
                }
                CcPushOpCode(pMachine, ENT); CcPushImm(pMachine, pMachine->main_tempI - pMachine->localVariableOffset);
                while (pMachine->currentToken != '}') {
                    CcStatement(pMachine);

                    if (pMachine->g_pErrorExists) {
                        pMachine->m_halted = true;// goto _cleanup;
                        return CCSTATUS_ERROR_FOUND;
                    }
                }
                CcPushOpCode(pMachine, LEV);
                pMachine->pCurrIdentifier = pMachine->pCurrSymbol; // unwind symbol table locals
                while (pMachine->pCurrIdentifier[Tk]) {
                    if (pMachine->pCurrIdentifier[Class] == Loc) {
                        pMachine->pCurrIdentifier[Class] = pMachine->pCurrIdentifier[HClass];
                        pMachine->pCurrIdentifier[Type] = pMachine->pCurrIdentifier[HType];
                        pMachine->pCurrIdentifier[Val] = pMachine->pCurrIdentifier[HVal];
                    }
                    pMachine->pCurrIdentifier = pMachine->pCurrIdentifier + Idsz;
                }
            }
            else {
                pMachine->pCurrIdentifier[Class] = Glo;
                pMachine->pCurrIdentifier[Val] = (int)pMachine->pData;
                pMachine->pData += sizeof(int);
            }
            if (pMachine->currentToken == ',') CcNextToken(pMachine);
        }
        CcNextToken(pMachine);
    }
    if (!(pMachine->main_instPtr = (int*)pMachine->main_idMain[Val])) { LogMsg("main() not defined"); return CCSTATUS_MAIN_NOT_DEFINED; }
    //if (src) return 0;

    // setup stack
    pMachine->main_basePtr = pMachine->main_stackPtr = (int*)((int)pMachine->main_stackPtr + pMachine->main_poolSize);
    *--pMachine->main_stackPtr = EXIT; // call exit if main returns
    *--pMachine->main_stackPtr = PSH; pMachine->main_tempT = pMachine->main_stackPtr;
    *--pMachine->main_stackPtr = 0;
    *--pMachine->main_stackPtr = (int)NULL;
    *--pMachine->main_stackPtr = (int)pMachine->main_tempT;

    pMachine->retnVal = 0;

    // run...
    pMachine->main_cycle = 0;
    return CCSTATUS_SUCCESS;
}
void CcRunMachine(CMachine* pMachine, int cycs_per_run)
{
    while (cycs_per_run != 0) {
        cycs_per_run--;
        if (CcBreakCheck()) {
            LogMsg("Ctrl-C, exit at cycle %d", pMachine->main_cycle);
            pMachine->retnVal = CCSTATUS_CTRL_C;
            pMachine->m_halted = 1;
            return;
            //goto _cleanup;
        }

        pMachine->main_tempI = *pMachine->main_instPtr++; ++pMachine->main_cycle;
        if (pMachine->printCycles) {
            LogMsgNoCr("%d> %s", pMachine->main_cycle,
                &"LEA \0IMM \0JMP \0JSR \0BZ  \0BNZ \0ENT \0ADJ \0LEV \0LI  \0LC  \0SI  \0SC  \0PSH \0"
                "OR  \0XOR \0AND \0EQ  \0NE  \0LT  \0GT  \0LE  \0GE  \0SHL \0SHR \0ADD \0SUB \0MUL \0DIV \0MOD \0"
                "OPEN\0READ\0CLOS\0PRTF\0PRTN\0MALC\0FREE\0MSET\0MCMP\0RAND\0DRPX\0EXSC\0RDCH\0RDIN\0CLSC\0"
                "FLSC\0FLRC\0DRRC\0SSCY\0DRST\0SPTF\0MVCR\0SLEP\0EXIT\0"[pMachine->main_tempI * 5]);
            if (pMachine->main_tempI <= ADJ) {
                void* ptr = (void*)(*pMachine->main_instPtr);
                LogMsg(" %#x", (int)ptr);
            }
            else LogMsg("");
        }
        if (pMachine->main_tempI == LEA) pMachine->main_theAReg = (int)(pMachine->main_basePtr + *pMachine->main_instPtr++);                             // load local address
        else if (pMachine->main_tempI == IMM) pMachine->main_theAReg = *pMachine->main_instPtr++;                                         // load global address or immediate
        else if (pMachine->main_tempI == JMP) pMachine->main_instPtr = (int*)*pMachine->main_instPtr;                                   // jump
        else if (pMachine->main_tempI == JSR) { *--pMachine->main_stackPtr = (int)(pMachine->main_instPtr + 1); pMachine->main_instPtr = (int*)*pMachine->main_instPtr; }        // jump to subroutine
        else if (pMachine->main_tempI == BZ)  pMachine->main_instPtr = pMachine->main_theAReg ? pMachine->main_instPtr + 1 : (int*)*pMachine->main_instPtr;                      // branch if zero
        else if (pMachine->main_tempI == BNZ) pMachine->main_instPtr = pMachine->main_theAReg ? (int*)*pMachine->main_instPtr : pMachine->main_instPtr + 1;                      // branch if not zero
        else if (pMachine->main_tempI == ENT) { *--pMachine->main_stackPtr = (int)pMachine->main_basePtr; pMachine->main_basePtr = pMachine->main_stackPtr; pMachine->main_stackPtr = pMachine->main_stackPtr - *pMachine->main_instPtr++; }     // enter subroutine
        else if (pMachine->main_tempI == ADJ) pMachine->main_stackPtr = pMachine->main_stackPtr + *pMachine->main_instPtr++;                                   // stack adjust
        else if (pMachine->main_tempI == LEV) { pMachine->main_stackPtr = pMachine->main_basePtr; pMachine->main_basePtr = (int*)*pMachine->main_stackPtr++; pMachine->main_instPtr = (int*)*pMachine->main_stackPtr++; } // leave subroutine
        else if (pMachine->main_tempI == LI)  pMachine->main_theAReg = *(int*)pMachine->main_theAReg;                                     // load int
        else if (pMachine->main_tempI == LC)  pMachine->main_theAReg = *(char*)pMachine->main_theAReg;                                    // load char
        else if (pMachine->main_tempI == SI)  *(int*)*pMachine->main_stackPtr++ = pMachine->main_theAReg;                                 // store int
        else if (pMachine->main_tempI == SC)  pMachine->main_theAReg = *(char*)*pMachine->main_stackPtr++ = pMachine->main_theAReg;                            // store char
        else if (pMachine->main_tempI == PSH) *--pMachine->main_stackPtr = pMachine->main_theAReg;                                         // push

        else if (pMachine->main_tempI == OR)  pMachine->main_theAReg = *pMachine->main_stackPtr++ | pMachine->main_theAReg;
        else if (pMachine->main_tempI == XOR) pMachine->main_theAReg = *pMachine->main_stackPtr++ ^ pMachine->main_theAReg;
        else if (pMachine->main_tempI == AND) pMachine->main_theAReg = *pMachine->main_stackPtr++ & pMachine->main_theAReg;
        else if (pMachine->main_tempI == EQ)  pMachine->main_theAReg = *pMachine->main_stackPtr++ == pMachine->main_theAReg;
        else if (pMachine->main_tempI == NE)  pMachine->main_theAReg = *pMachine->main_stackPtr++ != pMachine->main_theAReg;
        else if (pMachine->main_tempI == LT)  pMachine->main_theAReg = *pMachine->main_stackPtr++ < pMachine->main_theAReg;
        else if (pMachine->main_tempI == GT)  pMachine->main_theAReg = *pMachine->main_stackPtr++ > pMachine->main_theAReg;
        else if (pMachine->main_tempI == LE)  pMachine->main_theAReg = *pMachine->main_stackPtr++ <= pMachine->main_theAReg;
        else if (pMachine->main_tempI == GE)  pMachine->main_theAReg = *pMachine->main_stackPtr++ >= pMachine->main_theAReg;
        else if (pMachine->main_tempI == SHL) pMachine->main_theAReg = *pMachine->main_stackPtr++ << pMachine->main_theAReg;
        else if (pMachine->main_tempI == SHR) pMachine->main_theAReg = *pMachine->main_stackPtr++ >> pMachine->main_theAReg;
        else if (pMachine->main_tempI == ADD) pMachine->main_theAReg = *pMachine->main_stackPtr++ + pMachine->main_theAReg;
        else if (pMachine->main_tempI == SUB) pMachine->main_theAReg = *pMachine->main_stackPtr++ - pMachine->main_theAReg;
        else if (pMachine->main_tempI == MUL) pMachine->main_theAReg = *pMachine->main_stackPtr++ * pMachine->main_theAReg;
        else if (pMachine->main_tempI == DIV) pMachine->main_theAReg = *pMachine->main_stackPtr++ / pMachine->main_theAReg;
        else if (pMachine->main_tempI == MOD) pMachine->main_theAReg = *pMachine->main_stackPtr++ % pMachine->main_theAReg;

        else if (pMachine->main_tempI == OPEN) LogMsg("Not supported!");
        else if (pMachine->main_tempI == READ) LogMsg("Not supported!");
        else if (pMachine->main_tempI == CLOS) LogMsg("Not supported!");
        else if (pMachine->main_tempI == PRTF) { pMachine->main_tempT = pMachine->main_stackPtr + pMachine->main_instPtr[1]; LogMsg    ((char*)pMachine->main_tempT[-1], pMachine->main_tempT[-2], pMachine->main_tempT[-3], pMachine->main_tempT[-4], pMachine->main_tempT[-5], pMachine->main_tempT[-6]); }
        else if (pMachine->main_tempI == PRTN) { pMachine->main_tempT = pMachine->main_stackPtr + pMachine->main_instPtr[1]; LogMsgNoCr((char*)pMachine->main_tempT[-1], pMachine->main_tempT[-2], pMachine->main_tempT[-3], pMachine->main_tempT[-4], pMachine->main_tempT[-5], pMachine->main_tempT[-6]); }
        else if (pMachine->main_tempI == MALC) {
            if (pMachine->g_memoryAllocCount < MAX_ALLOCS - 2) {
                pMachine->main_theAReg = (int)MmAllocate(*pMachine->main_stackPtr);
                CcOnAllocateSomething(pMachine, (void*)pMachine->main_theAReg);
            }
            else
            {
                LogMsg("Out of memory");
                pMachine->main_theAReg = (int)NULL;
            }
        }
        else if (pMachine->main_tempI == FREE) {
            CcOnDeallocateSomething(pMachine, (void*)*pMachine->main_stackPtr);
            MmFree((void*)*pMachine->main_stackPtr);
        }
        else if (pMachine->main_tempI == MSET) pMachine->main_theAReg = (int)memset((char*)pMachine->main_stackPtr[2], pMachine->main_stackPtr[1], *pMachine->main_stackPtr);
        else if (pMachine->main_tempI == MCMP) pMachine->main_theAReg = memcmp((char*)pMachine->main_stackPtr[2], (char*)pMachine->main_stackPtr[1], *pMachine->main_stackPtr);
        else if (pMachine->main_tempI == RAND) {
            int x = *pMachine->main_stackPtr;
            int rng = GetRandom();
            rng = rng & 0xFFFF;//make sure it is unsigned
            if (x) pMachine->main_theAReg = rng % x;
            else pMachine->main_theAReg = rng;
            //LogMsg("[DEBUG] Random(%d) returned %d", x,a);
        }
        else if (pMachine->main_tempI == RDCH) {
			while(CoInputBufferEmpty()) hlt;
			pMachine->main_theAReg = CoGetChar();
        }
        else if (pMachine->main_tempI == RDIN) {
			/*if (pMachine->m_bHookedConsole)  //Did we hook the console?
				pMachine->main_theAReg = KeReadIntDebug();
			else
				pMachine->main_theAReg = -1;*/
			
			char buffer [11];
			LogMsgNoCr("?");
			CoGetString(buffer, 11);
			
			int num = atoi (buffer);
			
			pMachine->main_theAReg = num;
        }
        else if (pMachine->main_tempI == CLSC) {
#ifndef _WIN32
            CoClearScreen(g_currentConsole);
            g_currentConsole->curX = 0;
            g_currentConsole->curY = 0;
#endif
        }
        else if (pMachine->main_tempI == MVCR) {
#ifndef _WIN32
            g_currentConsole->curX = (int)pMachine->main_stackPtr[1];
            g_currentConsole->curY = (int)pMachine->main_stackPtr[0];
#endif
        }
        else if (pMachine->main_tempI == SLEP) {
#ifndef _WIN32
            WaitMS((int)pMachine->main_stackPtr[0]);
#else
			Sleep ((int)pMachine->main_stackPtr[0]);
#endif
        }
        else if (pMachine->main_tempI == DRPX) VidPlotPixel((int)pMachine->main_stackPtr[2], (int)pMachine->main_stackPtr[1], (int)*pMachine->main_stackPtr);
        else if (pMachine->main_tempI == FLRC) {
#ifndef _WIN32
            Rectangle r;
            r.left = (int)pMachine->main_stackPtr[4]; r.top = (int)pMachine->main_stackPtr[3];
            r.right = (int)pMachine->main_stackPtr[2], r.bottom = (int)pMachine->main_stackPtr[1];
            uint32_t color = (uint32_t)pMachine->main_stackPtr[0];
            VidFillRectangle(color, r);
#endif
        }
        else if (pMachine->main_tempI == DRRC) {
#ifndef _WIN32
            Rectangle r;
            r.left = (int)pMachine->main_stackPtr[4]; r.top = (int)pMachine->main_stackPtr[3];
            r.right = (int)pMachine->main_stackPtr[2], r.bottom = (int)pMachine->main_stackPtr[1];
            uint32_t color = (uint32_t)pMachine->main_stackPtr[0];
            VidDrawRectangle(color, r);
#endif
        }
        else if (pMachine->main_tempI == FLSC) {
            uint32_t color = (int)pMachine->main_stackPtr[0];
            VidFillScreen(color);
        }
        else if (pMachine->main_tempI == SSCY) {
            //int howMuch = (int)pMachine->main_stackPtr[0];
            //VidShiftScreen(howMuch);
        }
        else if (pMachine->main_tempI == DRST) {
            int color = (int)pMachine->main_stackPtr[3], px = (int)pMachine->main_stackPtr[2], py = (int)pMachine->main_stackPtr[1]; const char* pText = (char*)pMachine->main_stackPtr[0];
            VidTextOut(pText, px, py, color, TRANSPARENT);
        }
        else if (pMachine->main_tempI == SPTF) {
            pMachine->main_tempT = pMachine->main_stackPtr + pMachine->main_instPtr[1];
            char* pBuf = (char*)pMachine->main_tempT[-1], * pFormat = (char*)pMachine->main_tempT[-2];

            sprintf(pBuf, pFormat, pMachine->main_tempT[-3], pMachine->main_tempT[-4], pMachine->main_tempT[-5], pMachine->main_tempT[-6]);
        }
        else if (pMachine->main_tempI == EXSC) {
            /*const char* pFileName = (const char*)*sp;

            uint8_t* data = NULL; int size = 0;
            int errorCode = FileLoad(g_curDir, pFileName, &size, &data);
            if (errorCode != Success) {
                LogMsg("%d, file operation failed: %s", cycle, GetErrorString(errorCode));
            } else {
                //CcRunCCode((const char*)data);
            }
            if (data) FreeMem(data);*/
            LogMsg("Not supported yet!");
        }
        else if (pMachine->main_tempI == EXIT) {
            if (pMachine->printCycles) LogMsg("exit(%d) cycle = %d", *pMachine->main_stackPtr, pMachine->main_cycle);
            pMachine->retnVal = *pMachine->main_stackPtr;
            pMachine->m_halted = 1;
            return;
            //goto _cleanup;
        }
        else {
            LogMsg("ERROR: Unknown instruction %d! cycle = %d", pMachine->main_tempI, pMachine->main_cycle); 
            pMachine->retnVal = CCSTATUS_UNKNOWN_INSTRUCTION;
            pMachine->m_halted = 1;
            return;//goto _cleanup; 
        }
    }
}
void CcKillMachine(CMachine* pMachine)
{
    bool printedMemleakAlert = true; int memLeakCount = 0;
    for (int i = 0; i < pMachine->g_memoryAllocCount; i++) {
        if (pMachine->g_memoryAllocatedPointers[i] != NULL) {
            if (printedMemleakAlert) {
                LogMsg("WARNING: Memory leak detected, automatically freed.");
                printedMemleakAlert = false;
            }
            memLeakCount++;
            MmFree(pMachine->g_memoryAllocatedPointers[i]);
            pMachine->g_memoryAllocatedPointers[i] = NULL;
        }
    }
    if (memLeakCount)
        LogMsg("Unfreed block count: %d", memLeakCount);

    if (pMachine->pTextStart)          MmFree(pMachine->pTextStart);
    if (pMachine->pDataStart)          MmFree(pMachine->pDataStart);
    if (pMachine->pSourceStart)        MmFree(pMachine->pSourceStart);
    if (pMachine->main_stackStart)     MmFree(pMachine->main_stackStart);
    if (pMachine->pStartOfIdentifiers) MmFree(pMachine->pStartOfIdentifiers);
    if (pMachine->pStartOfSymbols)     MmFree(pMachine->pStartOfSymbols);
}

// for backwards compatibility with CcRunCCode
CCSTATUS CcRunCCode(const char* pCode, int length)
{
    CMachine* pMachine;
    pMachine = (CMachine*)MmAllocate(sizeof(CMachine));
    if (!pMachine) {
        LogMsg("Wtf?! (16)");
        return 1;
    }
    memset(pMachine, 0, sizeof(CMachine));
	
	pMachine->m_bHookedConsole = true;

    //int c = CcRunCCode(pMachine, lol);
    int state;
    state = CcInitMachine(pMachine);
    if (state != 0) {
		CcKillMachine(pMachine);
		return state;
	}
    state = CcCompileCode(pMachine, pCode, length);
    if (state != 0) {
		CcKillMachine(pMachine);
		return state;
	}

    while (!pMachine->m_halted)
        CcRunMachine(pMachine, 1000);

    int rv = pMachine->retnVal;

    CcKillMachine(pMachine);
	MmFree(pMachine);
    return rv;
}