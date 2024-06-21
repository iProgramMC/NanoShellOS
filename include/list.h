/***
	The NanoShell Operating System
	Copyright (C) 2024 iProgramInCpp

Module name:
	list.h
	
Abstract:
	This header file contains the definitions related
	to the linked list code.
	
	It is important to note that the list entries' memory
	is managed manually by the user of these functions.
	This is merely the implementation of the doubly circularly
	linked list data structure.
	
	Note that this is analogous to rtl/list from the
	Boron operating system.
	
Author:
	iProgramInCpp - 2 October 2023
***/
#ifndef BORON_RTL_LIST_H
#define BORON_RTL_LIST_H

#include <main.h>

typedef struct _LIST_ENTRY LIST_ENTRY, *PLIST_ENTRY;

struct _LIST_ENTRY
{
	PLIST_ENTRY Flink;
	PLIST_ENTRY Blink;
};

// void InitializeListHead(PLIST_ENTRY ListHead);
#define InitializeListHead(Head) \
	(Head)->Flink = (Head),      \
	(Head)->Blink = (Head)

// void InsertHeadList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry);
#define InsertHeadList(ListHead, Entry) \
    (Entry)->Flink = (ListHead)->Flink,   \
	(Entry)->Blink = (ListHead),          \
	(ListHead)->Flink->Blink = (Entry), \
	(ListHead)->Flink = (Entry)

// void InsertTailList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry);
#define InsertTailList(ListHead, Entry) \
    (Entry)->Blink = (ListHead)->Blink,   \
	(Entry)->Flink = (ListHead),          \
	(ListHead)->Blink->Flink = (Entry), \
	(ListHead)->Blink = (Entry)

// PLIST_ENTRY RemoveHeadList(PLIST_ENTRY ListHead);
static inline ALWAYS_INLINE UNUSED
PLIST_ENTRY RemoveHeadList(PLIST_ENTRY ListHead)
{
	PLIST_ENTRY Entry = ListHead->Flink;
	ListHead->Flink = Entry->Flink;
	ListHead->Flink->Blink = ListHead;
#ifdef DEBUG
	Entry->Flink = NULL;
	Entry->Blink = NULL;
#endif
	return Entry;
}

// PLIST_ENTRY RemoveTailList(PLIST_ENTRY ListHead);
static inline ALWAYS_INLINE UNUSED
PLIST_ENTRY RemoveTailList(PLIST_ENTRY ListHead)
{
	PLIST_ENTRY Entry = ListHead->Blink;
	ListHead->Blink = Entry->Blink;
	ListHead->Blink->Flink = ListHead;
#ifdef DEBUG
	Entry->Flink = NULL;
	Entry->Blink = NULL;
#endif
	return Entry;
}

// bool RemoveEntryList(PLIST_ENTRY Entry);
static inline ALWAYS_INLINE UNUSED
bool RemoveEntryList(PLIST_ENTRY Entry)
{
	PLIST_ENTRY OldFlink, OldBlink;
	OldFlink = Entry->Flink;
	OldBlink = Entry->Blink;
	OldBlink->Flink = OldFlink;
	OldFlink->Blink = OldBlink;
#ifdef DEBUG
	Entry->Flink = NULL;
	Entry->Blink = NULL;
#endif
	return OldFlink == OldBlink;
}

// bool IsListEmpty(PLIST_ENTRY ListHead);
#define IsListEmpty(ListHead) ((ListHead)->Flink == (ListHead))

#endif//BORON_RTL_LIST_H
