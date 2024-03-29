//  ***************************************************************
//  fs/tmp/file.c - Creation date: 14/05/2023
//  -------------------------------------------------------------
//  NanoShell Copyright (C) 2023 - Licensed under GPL V3
//
//  ***************************************************************
//  Programmer(s):  iProgramInCpp (iprogramincpp@gmail.com)
//  ***************************************************************
#include <tmpfs.h>
#include <string.h>
#include <memory.h>
#include <misc.h>

void FsTempDirSetup(FileNode* pFileNode, FileNode* pParentNode)
{
	TempFSNode* pTFNode = (TempFSNode*)pFileNode->m_implData;
	
	// we shouldn't be able to setup a directory if we can't
	ASSERT(pTFNode->m_nFileSizePages == 0);
	
	// add a '.' entry.
	TempFSDirEntry dotEntry, dotDotEntry;
	memset(&dotEntry,    0, sizeof dotEntry);
	memset(&dotDotEntry, 0, sizeof dotDotEntry);
	
	strcpy(dotEntry   .m_filename, ".");
	strcpy(dotDotEntry.m_filename, "..");
	
	// set the node of the dot entry to ourselves
	dotEntry.m_pNode = pTFNode;
	// add a reference to our own node. Don't worry, when we delete a directory,
	// we'll remove it and its references anyways.
	FsAddReference(pFileNode);
	
	int offset = 0;
	
	// write.
	FsTempFileWrite(pFileNode, offset, sizeof(TempFSDirEntry), &dotEntry, true);
	offset += sizeof(TempFSDirEntry);
	
	// if we have a parent...
	if (pParentNode)
	{
		TempFSNode* pParentTFNode = (TempFSNode*)pParentNode->m_implData;
		
		// also add a reference to the parent
		FsAddReference(pParentNode);
		// the parent has already added a reference to us.
		
		// set up the dotdot entry
		dotDotEntry.m_pNode = pParentTFNode;
		
		// write it next to the dot entry:
		FsTempFileWrite(pFileNode, offset, sizeof(TempFSDirEntry), &dotDotEntry, true);
		offset += sizeof(TempFSDirEntry);
	}
}

int FsTempDirOpen(UNUSED FileNode* pFileNode)
{
	return ERR_SUCCESS;
}

int FsTempDirClose(UNUSED FileNode* pFileNode)
{
	return ERR_SUCCESS;
}

int FsTempDirReadInternal(FileNode* pFileNode, uint32_t* pOffset, DirEnt* pDirEnt, bool bSkipDotAndDotDot, TempFSNode** pOutNode)
{
	TempFSDirEntry dirEntry;
	memset(&dirEntry, 0, sizeof dirEntry);
	
go_again:;
	int read = FsTempFileRead(pFileNode, *pOffset, sizeof dirEntry, &dirEntry, true);
	if (read < (int)sizeof dirEntry)
	{
		// we couldn't read enough data. we're probably at the end.
		return 1;
	}
	
	*pOffset += sizeof dirEntry;
	
	if (dirEntry.m_filename[0] == 0)
	{
		// this is an empty spot. I say we try again
		goto go_again;
	}
	
	if (bSkipDotAndDotDot && (strcmp(dirEntry.m_filename, ".") == 0 || strcmp(dirEntry.m_filename, "..") == 0))
	{
		goto go_again;
	}
	
	// fill out the direntry structure
	strncpy(pDirEnt->m_name, dirEntry.m_filename, sizeof pDirEnt->m_name);
	pDirEnt->m_name[sizeof pDirEnt->m_name - 1] = 0;
	pDirEnt->m_inode = (int)dirEntry.m_pNode->m_node.m_pParentSpecific;
	pDirEnt->m_type  = dirEntry.m_pNode->m_node.m_type;
	
	if (pOutNode)
		*pOutNode = dirEntry.m_pNode;
	
	return 0;
}

int FsTempDirRead(FileNode* pFileNode, uint32_t* pOffset, DirEnt* pDirEnt)
{
	return FsTempDirReadInternal(pFileNode, pOffset, pDirEnt, true, NULL);
}

int FsTempDirLookup(FileNode* pFileNode, const char* pName, FileNode** pFNOut)
{
	*pFNOut = NULL;
	
	DirEnt ent;
	memset(&ent, 0, sizeof ent);
	
	uint32_t offset = 0;
	int err = 0;
	
	while ((err = FsTempDirReadInternal(pFileNode, &offset, &ent, false, NULL)) == 0)
	{
		if (strcmp(ent.m_name, pName) == 0)
		{
			FileNode* pFN = (FileNode*)ent.m_inode;
			
			FsAddReference(pFN);
			
			*pFNOut = pFN;
			return ERR_SUCCESS;
		}
	}
	
	return ERR_NO_FILE;
}

int FsTempDirAddEntry(FileNode* pFileNode, FileNode* pChildNode, const char* pName)
{
	if (!*pName)
		return -EINVAL;
	
	TempFSDirEntry dent;
	
	if (strlen(pName) > sizeof dent.m_filename - 2)
		return -ENAMETOOLONG;
	
	int offset = 0;
	memset(&dent, 0, sizeof dent);
	
	int offset_spot = -1;
	
	while (true)
	{
		int readIn = FsTempFileRead(pFileNode, offset, sizeof dent, &dent, false);
		
		// we've read enough...
		if (readIn < (int)sizeof dent)
			break;
		
		if (dent.m_filename[0] == 0 && offset_spot < 0)
			offset_spot = offset;
		
		// the file exists already!!
		if (strcmp(dent.m_filename, pName) == 0)
			return -EEXIST;
		
		offset += sizeof dent;
	}
	
	if (offset_spot < 0)
		offset_spot = offset;
	
	offset = offset_spot;
	// this is an empty directory entry. Replace it.
	strcpy(dent.m_filename, pName);
	
	dent.m_pNode = (TempFSNode*)pChildNode->m_implData;
	
	// write it.
	int written = FsTempFileWrite(pFileNode, offset, sizeof dent, &dent, false);
	if (written < (int)sizeof dent)
	{
		// who knows why the write failed.. probably due to a lack of memory
		return -ENOSPC;
	}
	
	if (pChildNode->m_bHasDirCallbacks)
	{
		pChildNode->m_parentDirIndex = offset;
		pChildNode->m_pParent = pFileNode;
		FsAddReference(pFileNode);
	}
	
	// add a reference to our child.
	FsAddReference(pChildNode);
	
	return 0;
}

int FsTempDirCreate(FileNode* pFileNode, const char* pName)
{
	UNUSED TempFSDirEntry dent;
	if (strlen(pName) >= sizeof(dent.m_filename) - 2)
		return -ENAMETOOLONG;
	
	// try to create a new file.
	TempFSNode* pNewChildTFNode = FsTempCreateNode(NULL, false);
	if (!pNewChildTFNode)
		return -ENOSPC;
	
	FileNode* pNewChild = &pNewChildTFNode->m_node;
	
	// okay, the file was created. Add it to the directory;
	int result = FsTempDirAddEntry(pFileNode, pNewChild, pName);
	
	if (result != 0)
	{
		// couldn't add, get rid of us.
		FsTempFreeNode(pNewChildTFNode);
	}
	
	return -result;
}

int FsTempDirCreateDir(FileNode* pFileNode, const char* pName)
{
	UNUSED TempFSDirEntry dent;
	if (strlen(pName) >= sizeof(dent.m_filename) - 2)
		return -ENAMETOOLONG;
	
	int refCountOld = pFileNode->m_refCount;
	
	// try to create a new file.
	TempFSNode* pNewChildTFNode = FsTempCreateNode(pFileNode, true);
	if (!pNewChildTFNode)
		return -ENOSPC;
	
	FileNode* pNewChild = &pNewChildTFNode->m_node;
	
	// okay, the file was created. Add it to the directory;
	int result = FsTempDirAddEntry(pFileNode, pNewChild, pName);
	
	if (result != 0)
	{
		// couldn't add, get rid of us.
		FsTempFreeNode(pNewChildTFNode);
		
		// and get rid of the side effects of this.
		pFileNode->m_refCount = refCountOld;
	}
	
	return -result;
}

void FsTempDirMaybeShrink(FileNode* pFileNode)
{
	uint32_t offset = pFileNode->m_length;
	
	// the last offset which had a zeroed out entry
	uint32_t offset_last = offset;
	
	TempFSDirEntry dent;
	while (offset > 0)
	{
		offset -= sizeof dent;
		
		FsTempFileRead(pFileNode, offset, sizeof dent, &dent, true);
		
		if (dent.m_filename[0] == 0)
			offset_last = offset;
		else
			break;
	}
	
	FsTempFileShrink(pFileNode, offset_last);
}

int FsTempDirUnlinkInternal(FileNode* pFileNode, const char* pName, uint32_t inodeNo, bool bByInode, bool bCheckDirs)
{
	DirEnt ent;
	memset(&ent, 0, sizeof ent);
	
	DirEnt* pDirEnt = &ent;
	int err = 0;
	uint32_t offset = 0;
	TempFSNode* pTFSNode = NULL;
	
	while ((err = FsTempDirReadInternal(pFileNode, &offset, &ent, true, &pTFSNode)) == 0)
	{
		if ((!bByInode && strcmp(pDirEnt->m_name, pName) == 0) || (bByInode && pDirEnt->m_inode == inodeNo))
		{
			FileNode* pFN = (FileNode*)pDirEnt->m_inode;
			
			if (pFN->m_type != FILE_TYPE_FILE && bCheckDirs)
				return -EISDIR;
			
			// check if the node is actually immutable first
			if (!pTFSNode->m_bMutable)
				return -EACCES;
			
			// remove a reference to the file node, that reference being of our directory
			FsReleaseReference(pFN);
			
			// set the directory entry to nothing
			TempFSDirEntry dent;
			memset(&dent, 0, sizeof dent);
			
			// we need to subtract dent since the offset points to the dent after the current one.
			int offs = offset - sizeof dent;
			FsTempFileWrite(pFileNode, offs, sizeof dent, &dent, true);
			
			// we've removed it. Also shrink the file
			FsTempDirMaybeShrink(pFileNode);
			return 0;
		}
	}
	
	return -ENOENT;
}

int FsTempDirUnlink(FileNode* pFileNode, const char* pName)
{
	return FsTempDirUnlinkInternal(pFileNode, pName, 0, false, true);
}

int FsTempDirRemoveDir(FileNode* pFileNode)
{
	// look up our parent.
	FileNode* pParentNode;
	
	FsTempDirLookup(pFileNode, "..", &pParentNode);
	
	// we can't delete a directory with no parent, you know
	if (!pParentNode)
		return -ENOTSUP;
	
	int dotDirFlags = 0; // 1 - ., 2 - ..
	int nEntries    = 0;
	
	DirEnt ent;
	memset(&ent, 0, sizeof ent);
	
	DirEnt* pDirEnt = NULL;
	uint32_t offset = 0;
	
	int err = 0;
	while ((err = FsTempDirRead(pFileNode, &offset, &ent)) == 0)
	{
		if (strcmp(pDirEnt->m_name, ".") == 0)
			dotDirFlags |= 1;
		if (strcmp(pDirEnt->m_name, "..") == 0)
			dotDirFlags |= 2;
		nEntries++;
		
		// even if we would've eventually found . or .. entries, we've already got bad files in
		if (nEntries >= 3)
			break;
	}
	
	// same explanation as just right above.
	if (nEntries >= 3)
		return -ENOTEMPTY;
	
	// if there are two entries, but they aren't a '.' and '..' entry respectively.
	if (nEntries == 2 && dotDirFlags != 3)
		return -ENOTEMPTY;
	
	// if there's one entry and it isn't a '.' or '..' entry:
	if (nEntries == 1 && dotDirFlags == 0)
		return -ENOTEMPTY;
	
	// remove one reference from ourselves for the '.' entry:
	FsReleaseReference(pFileNode);
	
	// we should remove another reference, which this does.
	// It scans for any directory entries whose "inode number" is the dir we're trying to remove.
	
	// Eventually this should end up getting rid of ALL references to our directory, so
	// don't use pFileNode after this.
	FsTempDirUnlinkInternal(pParentNode, NULL, (int)pFileNode, true, false);
	
	// remove one reference from the parent, for the '..' dentry
	FsReleaseReference(pParentNode);
	
	return 0;
}

int FsTempDirRenameOp (FileNode* pFileNodeSrc, FileNode* pFileNodeDst, const char* pFileNameSrc, const char* pFileNameDst)
{
	FileNode* pManipulatedFile = NULL;
	
	int err = FsTempDirLookup(pFileNodeSrc, pFileNameSrc, &pManipulatedFile);
	
	if (FAILED(err))
		return err;
	
	// add a link to it in the destination
	FsTempDirAddEntry(pFileNodeDst, pManipulatedFile, pFileNameDst);
	
	// remove said link from the source.
	FsTempDirUnlinkInternal(pFileNodeDst, 0, (int)pManipulatedFile, true, false);
	
	return 0;
}
