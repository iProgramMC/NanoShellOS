/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp

   Window Manager Keyboard Queue Module
******************************************/
#include "wi.h"

void WinAddToInputQueue (Window* this, char input)
{
	if (!input) return;
	
	this->m_inputBuffer[this->m_inputBufferEnd++] = input;
	while
	   (this->m_inputBufferEnd >= KB_BUF_SIZE)
		this->m_inputBufferEnd -= KB_BUF_SIZE;
}

bool WinAnythingOnInputQueue (Window* this)
{
	return this->m_inputBufferBeg != this->m_inputBufferEnd;
}

char WinReadFromInputQueue (Window* this)
{
	if (WinAnythingOnInputQueue(this))
	{
		char k = this->m_inputBuffer[this->m_inputBufferBeg++];
		while
		   (this->m_inputBufferBeg >= KB_BUF_SIZE)
			this->m_inputBufferBeg -= KB_BUF_SIZE;
		return k;
	}
	else return 0;
}
