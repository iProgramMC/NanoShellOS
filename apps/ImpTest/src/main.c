
#define UNUSED __attribute__((unused))
#define NSIMPORT  __attribute__((section(".nanoshell_imports")))

extern NSIMPORT void LogString(const char* str);
extern NSIMPORT int  GetVersionNumber();
extern NSIMPORT void FictionalImport();

char vn[] = "V0.00 Yo!";
int NsMain ()
{
	int ven = GetVersionNumber();
	vn[1] = '0' + (ven / 100 % 10);
	vn[3] = '0' + (ven / 10  % 10);
	vn[4] = '0' + (ven       % 10);
	LogString(vn);
	
	FictionalImport();
	
	return 0;
}
