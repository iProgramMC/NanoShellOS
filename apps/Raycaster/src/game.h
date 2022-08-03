/*****************************************
		NanoShell Operating System
	      (C) 2022 iProgramInCpp
           Raycaster application

          Game logic header file
******************************************/
#ifndef GAME_H
#define GAME_H

#define PI 3.14159265358979323846264338327950

extern int ScreenWidth;
extern int ScreenHeight;

// from game engine:
int Classify(double x);
#define FP_INFINITE 1
#define FP_NAN 2
#define FP_NORMAL 4
#define FP_SUBNORMAL 8
#define FP_ZERO 16
#define isnan(x) (Classify(x) == FP_NAN)


double sin(double x);
double cos(double x);
double sqrt(double x);
double Abs(double x);
double Trim(double x);

double fmod(double x, double y);
double atan2l(double y, double x);
float  atan2f(float  y, float  x);

bool IsKeyDown (int keyCode);

// from game:
const char* GetGameName();
void Update (int deltaTime);
void Render (int deltaTime);
void Init   ();
void OnSize (int width, int height);

#endif//GAME_H