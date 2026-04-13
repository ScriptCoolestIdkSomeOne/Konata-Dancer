#pragma once
#include <windows.h>
#include <vector>

extern HHOOK g_hook;
extern HHOOK g_msgHook;
extern std::vector<HWND> g_hookedWindows;

void EnableBluehair();
void DisableBluehair();
void textinshit();
LRESULT CALLBACK bluehair(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK createhook(int, WPARAM, LPARAM);
LRESULT CALLBACK gtmsghook(int, WPARAM, LPARAM);
BOOL CALLBACK itslikechristmasmiracle(HWND, LPARAM);