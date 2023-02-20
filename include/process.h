//  ***************************************************************
//  process.h - Creation date: 10/04/2022
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2022-2023 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************
#ifndef _PROCESS_H
#define _PROCESS_H

#include<main.h>
#include<memory.h>
#include<task.h>
#include<string.h>
#include<rt.h>

/**************************************************************

	A Process is a task or set of tasks that are part of
	a running program, the "Process". All tasks that inherit
	from the process have the same memory space (i.e. the
	Heap). When a task inside a process crashes, ALL tasks
	are taken down, and the tasks' heap is deleted.
	The Heap APIs are thusly not recommended for use outside
	of here anymore.

	The Process structures will not be used for any scheduling/
	multitasking, only the Task ones will, but these group the
	tasks. 
	
***************************************************************/

struct Proc;

typedef void (*DeathProc) (struct Proc*);

struct Proc
{
	bool  bActive;
	bool  bWillDie;
	
	char  sIdentifier[250];
	
	int   nTasks;
	Task* sTasks [4];
	void* pDetail;
	
	DeathProc OnDeath;
	
	UserHeap* pHeap;
	
	uint64_t nIdentifier;
	
	void* pSymTab, *pStrTab;
	int   nSymTabEntries;
	bool  bWaitingForCrashAck;
	
	ProgramInfo   pProgInfo;
	ResourceTable sResourceTable;
};
typedef struct Proc Process;

enum
{
	EX_PROC_SUCCESS,
	EX_PROC_TOO_MANY_PROCESSES=0x50000000,
	EX_PROC_CANT_MAKE_HEAP,
};

void ExKillProcess(Process *pProc);
Process* ExGetRunningProc();
Process* ExGetProcessByRID(uint64_t rid);
Process* ExCreateProcess (TaskedFunction pTaskedFunc, int nParameter, const char *pIdent, int nHeapSize, int *pErrCode, void* pDetail);

void ExSetProgramInfo(const ProgramInfo* pProgInfo);
void ExLoadResourceTable(void *pResourceTableData);

#endif//_PROCESS_H
