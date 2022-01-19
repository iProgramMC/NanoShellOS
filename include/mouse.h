#ifndef _MOUSE_H
#define _MOUSE_H

#include <main.h>
#include <video.h>

#define MOUSE_FLAG_L_BUTTON 0x1
#define MOUSE_FLAG_R_BUTTON 0x2
#define MOUSE_FLAG_M_BUTTON 0x4

typedef struct
{
	uint8_t flags, xMov, yMov, zMov;
}
MousePacket;

/**
 * Returns whether the mouse has been found and
 * initted or not.
 */
bool IsMouseAvailable();

/**
 * Initializes the mouse device, if available.
 */
void MouseInit();


#endif//_MOUSE_H