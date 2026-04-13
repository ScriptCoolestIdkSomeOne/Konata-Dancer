#ifndef PRIZE_H
#define PRIZE_H

#include <windows.h>

#define ID_TRAY_PRIZE 1007

bool ActivatePrize(HWND parent = NULL);
bool IsPrizeActive();
void AddPrizeToMenu(HMENU hMenu, UINT id);
void CleanupPrize();

#endif