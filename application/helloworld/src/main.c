#include <nanoshell.h>

int addtodata=3;
int addtobss;



int main ()
{
	addtodata++;
	addtobss = 1;
	
	LogMsg("HELLO WORLD!!! Formatting works? %d %x %c %s %d %d", 1337, 0xcafebabe, 'D', "%steststring", 
		addtodata,//should print 4
		addtobss  //should print 1
	);
	
	for (int i = 0; i < 500; i++)
	{
		for (int j = 0; j < 500; j++)
			__asm__ volatile ("hlt\n\t");
		LogMsgNoCr("HI! %d", i);
	}
	
	return 42;//The answer to everything
}


