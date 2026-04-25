#include "kittypriv.h"

// ========== SYSTEM LAUNCHER ==========
void ShowLauncherDialog(HWND hParent) {
    char exePath[MAX_PATH] = "";
    if (!BrowseForFile(hParent, exePath, MAX_PATH, TRUE))
        return;
    
    if (RunAsSystem(exePath, NULL)) {
        MessageBox(hParent, "Process launched as SYSTEM!", "Success", MB_OK);
    } else {
        ShowError(hParent, "Failed to launch process as SYSTEM");
    }
}

// ========== PROCESS KILLER ==========
typedef struct {
    DWORD pid;
    char name[MAX_PATH];
} ProcEntry;

// Get process name from PID
BOOL GetProcessName(DWORD pid, char* name, DWORD size) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) return FALSE;
    
    BOOL result = GetModuleBaseNameA(hProcess, NULL, name, size);
    CloseHandle(hProcess);
    return result;
}

// Remove PPL protection if present (requires SYSTEM)
BOOL RemovePPLProtection(HANDLE hProcess) {
    // This requires calling RtlSetProcessIsProtected with FALSE
    // Needs ntdll.dll functions
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) return FALSE;
    
    typedef NTSTATUS (NTAPI *RtlSetProcessIsProtected_t)(HANDLE, BOOLEAN);
    RtlSetProcessIsProtected_t RtlSetProcessIsProtected = 
        (RtlSetProcessIsProtected_t)GetProcAddress(hNtdll, "RtlSetProcessIsProtected");
    
    if (!RtlSetProcessIsProtected) return FALSE;
    
    return RtlSetProcessIsProtected(hProcess, FALSE) == 0;
}

// Kill process (even protected)
BOOL KillProcess(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess) return FALSE;
    
    // Try to remove PPL if present (only works on certain versions)
    RemovePPLProtection(hProcess);
    
    BOOL result = TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);
    return result;
}

// Process list dialog procedure
INT_PTR CALLBACK ProcessDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hList;
    
    switch (msg) {
        case WM_INITDIALOG:
            hList = GetDlgItem(hDlg, IDC_LIST);
            // Enable debug privilege to open protected processes
            EnablePrivilege(SE_DEBUG_NAME, TRUE);
            
            // Populate process list
            HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (hSnapshot != INVALID_HANDLE_VALUE) {
                PROCESSENTRY32 pe = {sizeof(pe)};
                if (Process32First(hSnapshot, &pe)) {
                    do {
                        char item[256];
                        sprintf(item, "PID: %d - %s", pe.th32ProcessID, pe.szExeFile);
                        SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)item);
                        // Store PID in item data
                        int idx = SendMessageA(hList, LB_GETCOUNT, 0, 0) - 1;
                        SendMessageA(hList, LB_SETITEMDATA, idx, pe.th32ProcessID);
                    } while (Process32Next(hSnapshot, &pe));
                }
                CloseHandle(hSnapshot);
            }
            return TRUE;
            
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                int idx = SendMessageA(hList, LB_GETCURSEL, 0, 0);
                if (idx != LB_ERR) {
                    DWORD pid = SendMessageA(hList, LB_GETITEMDATA, idx, 0);
                    if (MessageBox(hDlg, "Terminate this process?", "Confirm", MB_YESNO | MB_ICONWARNING) == IDYES) {
                        if (KillProcess(pid)) {
                            MessageBox(hDlg, "Process terminated.", "Success", MB_OK);
                            EndDialog(hDlg, IDOK);
                        } else {
                            MessageBox(hDlg, "Failed to terminate process. May be critical system process.", "Error", MB_OK);
                        }
                    }
                }
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, IDCANCEL);
            }
            break;
    }
    return FALSE;
}

void ShowProcessDialog(HWND hParent) {
    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(100), hParent, ProcessDlgProc);
}

// ========== SERVICE MANAGER ==========
typedef struct {
    char name[256];
    char display[512];
    DWORD state;
} ServiceEntry;

// Get service state string
const char* GetStateString(DWORD state) {
    switch (state) {
        case SERVICE_STOPPED: return "STOPPED";
        case SERVICE_START_PENDING: return "STARTING";
        case SERVICE_STOP_PENDING: return "STOPPING";
        case SERVICE_RUNNING: return "RUNNING";
        case SERVICE_CONTINUE_PENDING: return "CONTINUING";
        case SERVICE_PAUSE_PENDING: return "PAUSING";
        case SERVICE_PAUSED: return "PAUSED";
        default: return "UNKNOWN";
    }
}

// Service manager dialog
INT_PTR CALLBACK ServiceDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hList;
    static SC_HANDLE hSCM = NULL;
    
    switch (msg) {
        case WM_INITDIALOG:
            hList = GetDlgItem(hDlg, IDC_LIST);
            hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_CONNECT);
            if (!hSCM) {
                MessageBox(hDlg, "Failed to open Service Control Manager", "Error", MB_OK);
                EndDialog(hDlg, IDCANCEL);
                return TRUE;
            }
            
            // Enumerate services
            DWORD bytesNeeded, servicesReturned, resumeHandle = 0;
            EnumServicesStatusEx(hSCM, SC_ENUM_PROCESS_INFO, SERVICE_WIN32,
                SERVICE_STATE_ALL, NULL, 0, &bytesNeeded, &servicesReturned, &resumeHandle, NULL);
            
            LPENUM_SERVICE_STATUS_PROCESS services = malloc(bytesNeeded);
            if (EnumServicesStatusEx(hSCM, SC_ENUM_PROCESS_INFO, SERVICE_WIN32,
                SERVICE_STATE_ALL, (LPBYTE)services, bytesNeeded, &bytesNeeded,
                &servicesReturned, &resumeHandle, NULL)) {
                
                for (DWORD i = 0; i < servicesReturned; i++) {
                    char item[512];
                    sprintf(item, "%s - %s [%s]",
                        services[i].lpServiceName,
                        services[i].lpDisplayName,
                        GetStateString(services[i].ServiceStatusProcess.dwCurrentState));
                    int idx = SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)item);
                    SendMessageA(hList, LB_SETITEMDATA, idx, (LPARAM)&services[i]);
                }
            }
            free(services);
            return TRUE;
            
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                int idx = SendMessageA(hList, LB_GETCURSEL, 0, 0);
                if (idx == LB_ERR) break;
                
                LPENUM_SERVICE_STATUS_PROCESS svc = (LPENUM_SERVICE_STATUS_PROCESS)SendMessageA(hList, LB_GETITEMDATA, idx, 0);
                if (!svc) break;
                
                // Show service control menu
                HMENU hMenu = CreatePopupMenu();
                AppendMenuA(hMenu, MF_STRING, 1, "Start");
                AppendMenuA(hMenu, MF_STRING, 2, "Stop");
                AppendMenuA(hMenu, MF_STRING, 3, "Restart");
                
                POINT pt;
                GetCursorPos(&pt);
                int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hDlg, NULL);
                DestroyMenu(hMenu);
                
                SC_HANDLE hService = OpenService(hSCM, svc->lpServiceName, SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS);
                if (!hService) {
                    MessageBox(hDlg, "Failed to open service. Run as SYSTEM for full control.", "Error", MB_OK);
                    break;
                }
                
                SERVICE_STATUS_PROCESS status;
                DWORD bytes;
                QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&status, sizeof(status), &bytes);
                
                if (cmd == 1 && status.dwCurrentState != SERVICE_RUNNING) {
                    StartService(hService, 0, NULL);
                } else if (cmd == 2 && status.dwCurrentState == SERVICE_RUNNING) {
                    ControlService(hService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&status);
                } else if (cmd == 3) {
                    ControlService(hService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&status);
                    Sleep(2000);
                    StartService(hService, 0, NULL);
                }
                
                CloseServiceHandle(hService);
                EndDialog(hDlg, IDOK); // Refresh
            } else if (LOWORD(wParam) == IDCANCEL) {
                if (hSCM) CloseServiceHandle(hSCM);
                EndDialog(hDlg, IDCANCEL);
            }
            break;
    }
    return FALSE;
}

void ShowServiceDialog(HWND hParent) {
    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(101), hParent, ServiceDlgProc);
}
