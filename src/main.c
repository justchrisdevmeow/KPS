#include "kittypriv.h"

// Global window handle
HWND g_hMainWnd = NULL;

// Message handler for main window
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            // Create buttons
            CreateWindow("BUTTON", "System Launcher", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                20, 20, 180, 40, hWnd, (HMENU)ID_BTN_LAUNCHER, GetModuleHandle(NULL), NULL);
            CreateWindow("BUTTON", "Process Killer", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                20, 70, 180, 40, hWnd, (HMENU)ID_BTN_PROCESS, GetModuleHandle(NULL), NULL);
            CreateWindow("BUTTON", "File Access Tool", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                20, 120, 180, 40, hWnd, (HMENU)ID_BTN_FILE, GetModuleHandle(NULL), NULL);
            CreateWindow("BUTTON", "Service Manager", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                20, 170, 180, 40, hWnd, (HMENU)ID_BTN_SERVICE, GetModuleHandle(NULL), NULL);
            break;
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_BTN_LAUNCHER:
                    ShowLauncherDialog(hWnd);
                    break;
                case ID_BTN_PROCESS:
                    ShowProcessDialog(hWnd);
                    break;
                case ID_BTN_FILE:
                    ShowFileDialog(hWnd);
                    break;
                case ID_BTN_SERVICE:
                    ShowServiceDialog(hWnd);
                    break;
            }
            break;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void RunAsSystemMain() {
    char exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);
    
    // Run the same exe with SYSTEM token
    RunAsSystem(exePath, NULL);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nShow) {
    // Check if running as admin, if not relaunch with UAC
    if (!IsAdmin()) {
        char exePath[MAX_PATH];
        GetModuleFileName(NULL, exePath, MAX_PATH);
        ShellExecute(NULL, "runas", exePath, lpCmdLine, NULL, SW_SHOWNORMAL);
        return 0;
    }
    
    // Check if running as SYSTEM (by checking token user)
    HANDLE hToken;
    OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
    char user[256];
    DWORD size = sizeof(user);
    GetTokenInformation(hToken, TokenUser, user, size, &size);
    CloseHandle(hToken);
    
    // If not SYSTEM, elevate to SYSTEM
    if (user[0] != 0) {
        // Simple check: if user isn't SYSTEM, relaunch as SYSTEM
        // For brevity, just use RunAsSystemMain
        RunAsSystemMain();
        return 0;
    }
    
    // Register main window class
    WNDCLASSEX wc = {sizeof(wc)};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "KittyPrivSystemClass";
    RegisterClassEx(&wc);
    
    g_hMainWnd = CreateWindowEx(0, "KittyPrivSystemClass", "KittyPrivSystem - SYSTEM Privilege Toolkit",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 240, 280,
        NULL, NULL, hInst, NULL);
    
    ShowWindow(g_hMainWnd, nShow);
    UpdateWindow(g_hMainWnd);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return msg.wParam;
}
