#include <nanoshell.h>
#include <window.h>

void CALLBACK WndProc (Window* pWindow, int messageType, int parm1, int parm2)
{
	DefaultWindowProc(pWindow, messageType, parm1, parm2);
}

int main ()
{
	Window* pWindow = CreateWindow ("Hello, NanoShell world!", 100, 100, 300, 300, WndProc, 0);
	
	if (!pWindow)
		return 1;
	
	while (HandleMessages (pWindow));
	
	return 0;
}
