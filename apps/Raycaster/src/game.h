#ifndef GAME_H
#define GAME_H

#define PI 3.14159265358979323846264338327950

extern int ScreenWidth;
extern int ScreenHeight;

// from game engine:
double sin(double x);
double cos(double x);

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