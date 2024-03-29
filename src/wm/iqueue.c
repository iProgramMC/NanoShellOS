/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

   Window Manager Keyboard Queue Module
******************************************/
#include "wi.h"

void WinAddToInputQueue (Window* this, char input)
{
	if (!input) return;
	if (!this)  return;
	if (!this->m_inputBuffer) return;
	
	this->m_inputBuffer[this->m_inputBufferEnd++] = input;
	while
	   (this->m_inputBufferEnd >= WIN_KB_BUF_SIZE)
		this->m_inputBufferEnd -= WIN_KB_BUF_SIZE;
	
	KeUnsuspendTasksWaitingForObject(this);
}

bool WinAnythingOnInputQueue (Window* this)
{
	if (!this) return false;
	return this->m_inputBufferBeg != this->m_inputBufferEnd;
}

char WinReadFromInputQueue (Window* this)
{
	if (!this) return 0;
	if (WinAnythingOnInputQueue(this))
	{
		char k = this->m_inputBuffer[this->m_inputBufferBeg++];
		while
		   (this->m_inputBufferBeg >= WIN_KB_BUF_SIZE)
			this->m_inputBufferBeg -= WIN_KB_BUF_SIZE;
		return k;
	}
	else return 0;
}
