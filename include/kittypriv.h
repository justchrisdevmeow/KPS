#ifndef KITTYPRIV_H
#define KITTYPRIV_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tlhelp32.h>
#include <winsvc.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

// UI controls IDs
#define ID_BTN_LAUNCHER    1001
#define ID_BTN_PROCESS     1002
#define ID_BTN_FILE        1003
#define ID_BTN_SERVICE     1004

// Function prototypes
BOOL IsAdmin(void);
BOOL EnablePrivilege(LPCTSTR privilege, BOOL enable);
BOOL GetSystemToken(HANDLE* hToken);
BOOL RunAsSystem(LPCSTR exePath, LPSTR cmdLine);

// Module functions
void ShowLauncherDialog(HWND hParent);
void ShowProcessDialog(HWND hParent);
void ShowFileDialog(HWND hParent);
void ShowServiceDialog(HWND hParent);

// Utils
void ShowError(HWND hWnd, LPCSTR msg);
BOOL BrowseForFile(HWND hWnd, LPSTR path, DWORD pathSize, BOOL forOpen);

#endif
