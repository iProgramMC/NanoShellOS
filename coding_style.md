
## NanoShell Coding Style

The coding style of NanoShell mostly follows a certain set of rules that ensures cleanliness:

#### Variables
Use `pascalCase`. For global variables, use `g_`, for statics use `s_`. If a pointer, denote it with a `p` prefix.

###### Right:
```c
int g_something;
void* g_pAnother;
static int s_dumbVariable;
```
###### Wrong:
```c
int something;
void* g_another;
static int g_some_dumb_variable;
```

#### Function names
For function names, use `CamelCase`. Preferably, add a short namespace string such as `Ke` or `Mm` to separate it from other code. Use `Ke` if it modifies the kernel's state significantly

Use descriptive verbs in function names, and use verbose names unless it's more appropriate to not do so.

###### Right:
```c
void KeStopSystem();
void* MmAllocateMemory(size_t size);
void AcpiSomething();
char ConvertToASCII(int c);
```
###### Wrong:
```c
void StopSystem();
void allocmem(size_t sz);
void AdvancedConfigurationAndPowerInterfaceSomething();
char ConvertToTheAmericanStandardCodeForInformationInterchagne(int c);
```

#### Structures
Use `typedef struct {...} StructName;`. I've never liked having to specify `struct Something` all the time, I'm used to the C++ way. Prefix member variables with `m_` and pointers with `m_p`
###### Right:
```c
typedef struct
{
	int m_crap;
	void* m_pSomethingElse;
}
SomeStruct;

//...
void MessWith(SomeStruct* pStruct);
```
###### Wrong:
```c
struct someStruct
{
	int crap;
	void* somethingElse;
}

void mess_with(struct someStruct* strct);
```

#### Enum Members and Defines

Prefix constant defines with `C_`. Enum members should use `ALL_CAPS_AND_UNDERSCORES` and have their family at the beginning (they can be referenced even without the enum's name!)
In macros, surround the parameters with parentheses. If macros require more lines, surround the code with a `do { } while (0)` statement. Of course, don't overdo macros. :-)

###### Right:
```c
enum /* The name is optional */
{
	SOMETHING_UNK0,
	SOMETHING_UNK1,
	SOMETHING_WHATEVER,
	//...
};
enum
{
	SOMETHING2_WHATEVER,
	SOMETHING2_BLABLA,
	//...
};

#define C_PLAYER_SPEED (100.0f)

#define SOME_MACRO(a, b)  DoSomethingHere((a), (b))
#define ANOTHER_MACRO(a, b) do { DoSomething((a), (b)); DoMore((a)); } while (0)
```
###### Wrong:
```c
enum /* The name is optional */
{
	UNK0,
	UNK1,
	WHATEVER
};
enum
{
	WHATEVER, // !!! Avoid this.
	BLABLA,
};

#define PLAYER_SPEED 100.0f

#define SOME_MACRO(a, b)  DoSomethingHere((a), (b));
#define ANOTHER_MACRO(a, b) DoSomething((a), (b)); DoMore((a))
```

#### Punctuation

Always put `{` and `}` on their separate lines. Split statements to be easy on the eyes, don't group them too much.

###### Right:
```c
void MyFunctionHere(int* a)
{
	*a += GetRandom(2000);
	
	if (*a == 1996)
	{
		LogMsg("Yowza! You've triggered an easter egg!");
		*a += GetRandom(1000);
	}
	
	DoSomethingWith(a);
	DoSomethingElseWith(a);
	
	while (*a > 10000)
		*a -= 10000;
}
```
###### Wrong:
```c
void MyFunctionHere(int*a) {
	*a += GetRandom(2000);
	
	if(*a == 1996)    {
		LogMsg("Yowza! You've triggered an easter egg!");
		*a += GetRandom(1000);  }
	
	DoSomethingWith(a);
	
	
	
	DoSomethingElseWith(a);
	while (*a > 10000) *a -= 10000;
}
```

#### Comments
NanoShell uses many different types of comments. Don't imitate this pattern though:
```c
//comment here
```
This should be a reminder that all I'm doing is hacking together this thing, and it's not serious :-)

#### Casts
I'm still a little frustrated that C doesn't support function-style casts (i.e. `int(something)` rather than `(int)something`), I think that way is better. But you do you, C...

#### File Structure
File names are all lowercase, preferably less than 8 characters long (except the extension). If part of any of the predefined categories, such as window applications (`wapp`),
widgets (`widget`), the memory manager (`mm`) etc, place your source there.

