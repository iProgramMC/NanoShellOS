#include <nanoshell.h>

// https://github.com/jefftasticgames/school-projects/blob/main/oregon.py

//#define Random(x) ((x)==0?rand():(rand()%(x)))

// simple crappy RNG
int g_randGen = 0x9521af17;

void GetTimeStampCounter(int* high, int* low)
{
	if (!high && !low) return; //! What's the point?
	int edx, eax;
	__asm__ volatile ("rdtsc":"=a"(eax),"=d"(edx));
	if (high) *high = edx;
	if (low ) *low  = eax;
}
int Random1 (void)
{
	g_randGen += (int)0xe120fc15;
	uint64_t tmp = (uint64_t)g_randGen * 0x4a39b70d;
	uint32_t m1 = (tmp >> 32) ^ tmp;
	tmp = (uint64_t)m1 * 0x12fad5c9;
	uint32_t m2 = (tmp >> 32) ^ tmp;
	return m2 & 0x7FFFFFFF;//make it always positive.
}
int Random(int num)
{
	if (num <= 0) return 0;
	return Random1 () % num;
}

void RandInit()
{
	int h, l;
	GetTimeStampCounter(&h, &l);
	g_randGen ^= l;
}

#define ClearScreen() LogMsg("Clear screen attempt")

// local vars - game state
int g_milesTraveled,g_foodRemaining,g_healthLevel,g_appetite,g_month,g_day,g_sicknessesThisMonth,
	g_randomEventsThisMonth,g_playing;
char* g_playerName;

// constants
int g_minMilesPerTravel,g_maxMilesPerTravel,g_minDaysPerTravel,g_maxDaysPerTravel,g_minDaysPerRest,g_maxDaysPerRest,
    g_healthChangePerRest,g_maxHealth,g_minDaysPerRandomEvent,g_maxDaysPerRandomEvent,g_minFoodPerRandomEvent,
    g_maxFoodPerRandomEvent,g_foodPerHunt,g_minDaysPerHunt,g_maxDaysPerHunt,g_milesBetweenNYandOR;

int GetMilesRemaining() {
	return g_milesBetweenNYandOR - g_milesTraveled;
}
int RandomRange (int min, int max) {
    return Random(max - min) + min;
}
int GetDaysInMonth(int monthID) {
    if (monthID==1||monthID==3||monthID==5||monthID==7||monthID==8||monthID==10||monthID==12) return 31;
    else if(monthID==2) return 28;
    else return 30;
}
char* GetMonthName (int monthID) {
    if (monthID== 1) return "January";
    if (monthID== 2) return "February";
    if (monthID== 3) return "March";
    if (monthID== 4) return "April";
    if (monthID== 5) return "May";
    if (monthID== 6) return "June";
    if (monthID== 7) return "July";
    if (monthID== 8) return "August";
    if (monthID== 9) return "September";
    if (monthID==10) return "October";
    if (monthID==11) return "November";
    if (monthID==12) return "December";
    return "fake";
}
void InitializeVariables() {
    g_milesTraveled = 0;
    g_foodRemaining = 500;
    g_healthLevel = 5;
    g_appetite = 5;
    g_month = 3;
    g_day = 1;
    g_sicknessesThisMonth = 0;
    g_randomEventsThisMonth = 0;
    g_playerName = 0;//NULL
    g_playing = 1;//true
}
void InitializeConstants() {
    g_minMilesPerTravel = 30;
    g_maxMilesPerTravel = 90;
    g_minDaysPerTravel = 3;
    g_maxDaysPerTravel = 7;
    g_minDaysPerRest = 2;
    g_maxDaysPerRest = 5;
    g_healthChangePerRest = 1;
    g_maxHealth = 5;
    g_minDaysPerRandomEvent = 1;
    g_maxDaysPerRandomEvent = 10;
    g_minFoodPerRandomEvent = 1;
    g_maxFoodPerRandomEvent = 100;
    g_foodPerHunt = 100;
    g_minDaysPerHunt = 2;
    g_maxDaysPerHunt = 5;
    g_milesBetweenNYandOR = 2000;
}
int MaybeRolloverMonth() {
	return g_day > GetDaysInMonth(g_month);
}
int DoesPlayerWin() {
	return(g_milesBetweenNYandOR <= g_milesTraveled);
}
int IsGameOver() {
	if (g_month == 12 && g_day == 31) return 1;
	if (g_healthLevel <= 0 || g_foodRemaining <= 0) return 1;
	if (DoesPlayerWin()) return 1;
	return 0;
}
void OnSickness() {
    g_healthLevel = g_healthLevel - 1;
    g_sicknessesThisMonth++;
    LogMsg("YOW! You got sick and lost a healthpoint.");
}
void RandomSicknessOccurs() {
    int daysLeft;
    int randSick;
    daysLeft = GetDaysInMonth(g_month) - g_day;
	randSick = 1;
    if (daysLeft >= 1) {
        randSick = Random(daysLeft);
    }
    if (g_sicknessesThisMonth == 0)
        if (randSick < 2) OnSickness();
    else if (g_sicknessesThisMonth == 1)
        if (randSick < 1) OnSickness();
}
void OnConsumeFood() {
    g_foodRemaining = g_foodRemaining - g_appetite;
}
//void AdvanceGameClock(int randDays);
void RandomEventOccurs() {
	int randomEvent, randDays, randFood, randHealth, randDefense, numDays;
	char muggingPrompt;
	if (g_randomEventsThisMonth == 0)
		if (RandomRange(1, 25) == 1) {
			randomEvent = Random(3) + 1;
			if (randomEvent == 1) // flood
			{
				randDays = RandomRange(g_minDaysPerRandomEvent, g_maxDaysPerRandomEvent);
				randFood = RandomRange(g_minFoodPerRandomEvent, g_maxFoodPerRandomEvent);
				g_foodRemaining = g_foodRemaining - randFood;
				
				//AdvanceGameClock
				numDays = randDays;
				while (numDays > 0) {
					if (IsGameOver()) numDays = 0;
					g_day = g_day + 1;
					if (MaybeRolloverMonth())
					{
						g_sicknessesThisMonth = 0;
						g_randomEventsThisMonth = 0;
						g_day = 1;
						g_month++;
					}
					OnConsumeFood();
					numDays--;
				}
				RandomSicknessOccurs();
				RandomEventOccurs();
				
				
				LogMsg("A rain shower came through and flooded your wagon. You lost %d lbs. of food and took", randFood);
				LogMsg("%d days to recover.", randDays);
			}
			else if (randomEvent == 2) // dysentery
			{
				randDays = RandomRange(g_minDaysPerRandomEvent, g_maxDaysPerRandomEvent);
				randHealth = Random(2);
				g_healthLevel = g_healthLevel - randHealth;
				
				//AdvanceGameClock
				numDays = randDays;
				while (numDays > 0) {
					if (IsGameOver()) numDays = 0;
					g_day = g_day + 1;
					if (MaybeRolloverMonth())
					{
						g_sicknessesThisMonth = 0;
						g_randomEventsThisMonth = 0;
						g_day = 1;
						g_month++;
					}
					OnConsumeFood();
					numDays--;
				}
				RandomSicknessOccurs();
				RandomEventOccurs();
				
				
				LogMsg("You fell sick to dysentery and lost %d HP and took", randHealth);
				LogMsg("%d days to recover.", randDays);
			}
			else if (randomEvent == 3) // mugging
			{
				randFood = RandomRange(g_minFoodPerRandomEvent, g_maxFoodPerRandomEvent);
				LogMsg("Outlaws approach your wagon. Do you let them take your food? (Y/N)");
				muggingPrompt = ReadChar();
				if (muggingPrompt == 'N' || muggingPrompt == 'n') {
					LogMsg("You pull out your hunting rifle to defend yourself.");
					randDefense = Random(1);
					if (randDefense) {
						g_foodRemaining = g_foodRemaining + randFood;
						LogMsg("You shot the outlaws and gained %d lbs. of food.", randFood);
					} else {
						g_healthLevel = g_healthLevel - 3;
						LogMsg("You got shot, and lost 3 HP!");
					}
				} else {
					g_foodRemaining = g_foodRemaining - randFood;
					LogMsg("You let the outlaws take your food. You lost %d lbs. of food.", randFood);
				}
			}
			else if (randomEvent == 4) // stash
			{
				randFood = RandomRange(g_minFoodPerRandomEvent, g_maxFoodPerRandomEvent);
				g_foodRemaining = g_foodRemaining + randFood;
				LogMsg("Lucky day! You found a stash containing %d pounds of food! What a steal!", randFood);
			}
			g_randomEventsThisMonth++;
		}
}
//int IsGameOver();
//void CheckPlayerWin();
void AdvanceGameClock(int numDays2) {
	int numDays; 
	numDays = numDays2;
	while (numDays > 0) {
		if (IsGameOver()) numDays = 0;
		g_day = g_day + 1;
		if (MaybeRolloverMonth())
		{
			g_sicknessesThisMonth = 0;
			g_randomEventsThisMonth = 0;
			g_day = 1;
			g_month++;
		}
		OnConsumeFood();
		numDays--;
	}
	RandomSicknessOccurs();
	RandomEventOccurs();
}
void OnTravel() {
	int randMiles, randDays;
	randMiles = RandomRange(g_minMilesPerTravel, g_maxMilesPerTravel);
	randDays  = RandomRange(g_minDaysPerTravel,  g_maxDaysPerTravel);
	g_milesTraveled = g_milesTraveled + randMiles;
	g_appetite = 5;
	AdvanceGameClock(randDays);
	//CheckPlayerWin();
	LogMsg("You traveled %d miles!", randMiles);
	LogMsg("It took you %d days.", randDays);
	LogMsg("Only %d miles remain.", GetMilesRemaining());
}
void OnHunt() {
	int randDays;
	randDays = RandomRange(g_minDaysPerHunt, g_maxDaysPerHunt);
	g_foodRemaining = g_foodRemaining + 100;
	g_appetite = 8;
	LogMsg("You hunt all day, gathering 100 pounds of food in %d days.", randDays);
	AdvanceGameClock(randDays);
}
void OnRest() {
	int randDays;
	if (g_healthLevel < g_maxHealth) {
		randDays = RandomRange(g_minDaysPerRest, g_maxDaysPerRest);
		g_appetite = 3;
		g_healthLevel = g_healthLevel + g_healthChangePerRest;
		LogMsg("You rest for %d days, gaining 1 HP.", randDays);
		AdvanceGameClock(randDays);
	}
	else LogMsg("No need to rest, you feel just fine.");
}
void OnStatus() {
	LogMsg("FOOD: %d", g_foodRemaining);
	LogMsg("HEALTH: %d", g_healthLevel);
	LogMsg("DIST TRAVELED: %d", g_milesTraveled);
	LogMsg("MONTH: %s", GetMonthName(g_month));
	LogMsg("DAY: %d", g_day);
}
void OnHelp() {
	LogMsg("Each turn you can take one of 3 actions:\n");
	LogMsg("  [t]ravel - moves you randomly between 30-60 miles and takes 3-7 days (random).");
	LogMsg("  [r]est   - increases health 1 level (up to 5 maximum) and takes 2-5 days (random).");
	LogMsg("  [h]unt   - adds 100 lbs of food and takes 2-5 days (random).\n");
	LogMsg("When prompted for an action, you can also enter one of these commands without using up your turn:\n");
	LogMsg("  [s]tatus - lists food, health, distance traveled, and day.");
	LogMsg("  [c]lear  - clears the screen.");
	LogMsg("  [?]      - lists all the commands.");
	LogMsg("  [q]uit   - will end the game.\n");
	LogMsg("You have to use the shortcuts marked with brackets.\n");
}
void OnQuit() {
	g_playing = 0;
}
void OnInvalidInput (char c) {
	LogMsg("'%c' is an invalid command, try '?'.", c);
}
//int DoesPlayerWin();
void LossReport() {
	LogMsg("FOOD: %d", g_foodRemaining);
	LogMsg("HEALTH: %d", g_healthLevel);
	LogMsg("DIST TRAVELED: %d/%d", g_milesTraveled, g_milesBetweenNYandOR);
}
void WinReport() {
	LogMsg("FOOD: %d", g_foodRemaining);
	LogMsg("HEALTH: %d", g_healthLevel);
	LogMsg("DATE OF ARRIVAL:");
	LogMsg("MONTH: %s", GetMonthName(g_month));
	LogMsg("DAY: %d", g_day);
}
void OnClear() {
	ClearScreen();
}
int main() {
	RandInit();
	char action;
    LogMsg("The Oregon Trail - NanoShell Port (C) 2021 iProgramInCpp");
    LogMsg("Original version (C) 2021 jefftasticgames");
	
	LogMsg("\n\nWelcome to the Oregon Trail! The year is 1850 and Americans are headed out West to populate the frontier. Your goal is to travel by wagon train from Independence, MO to Oregon (2000 miles). You start on March 1st, and your goal is to reach Oregon by December 31st. The trail is arduous. Each day costs you food and health. You can hunt and rest, but you have to get there before winter!\n\n");
	OnHelp();
	
	InitializeConstants();
	InitializeVariables();
	
	OnStatus();
	while (g_playing) {
		LogMsg("Choose an action, or '?' for help...");
		action = ReadChar();
		     if (action == 't' || action == 'T') OnTravel();
		else if (action == 'r' || action == 'r') OnRest();
		else if (action == 'h' || action == 'H') OnHunt();
		else if (action == 'q' || action == 'Q') OnQuit();
		else if (action == 's' || action == 'S') OnStatus();
		else if (action == 'c' || action == 'C') OnClear();
		else if (action == '\n');//ignore
		else if (action == '?') OnHelp();
		else OnInvalidInput(action);
		
		if (IsGameOver()) g_playing = 0;
	}
	if (DoesPlayerWin()) {
		LogMsg("Congratulations, you made it to Oregon alive!");
		WinReport();
	} else {
		LogMsg("Alas, you lost.");
		LossReport();
	}
	
    return 0;
}
