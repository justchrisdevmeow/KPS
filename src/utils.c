#include "kittypriv.h"

BOOL IsAdmin(void) {
    BOOL isAdmin = FALSE;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    PSID adminGroup = NULL;
    
    if (AllocateAndInitializeSid(&ntAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0,0,0,0,0,0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin;
}

BOOL EnablePrivilege(LPCTSTR privilege, BOOL enable) {
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;
    
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return FALSE;
    
    if (!LookupPrivilegeValue(NULL, privilege, &luid)) {
        CloseHandle(hToken);
        return FALSE;
    }
    
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;
    
    BOOL result = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
    CloseHandle(hToken);
    return result && GetLastError() != ERROR_NOT_ALL_ASSIGNED;
}

BOOL GetSystemToken(HANDLE* hToken) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
    if (!hProcess) return FALSE;
    
    if (!OpenProcessToken(hProcess, TOKEN_DUPLICATE, hToken)) {
        CloseHandle(hProcess);
        return FALSE;
    }
    
    CloseHandle(hProcess);
    return TRUE;
}

BOOL RunAsSystem(LPCSTR exePath, LPSTR cmdLine) {
    HANDLE hToken;
    STARTUPINFO si = {sizeof(si)};
    PROCESS_INFORMATION pi = {0};
    
    if (!GetSystemToken(&hToken)) {
        // Fallback: try to get SYSTEM token from winlogon
        // Not implemented in this version
        return FALSE;
    }
    
    si.lpDesktop = "winsta0\\default";
    
    BOOL result = CreateProcessAsUser(
        hToken, exePath, cmdLine, NULL, NULL, FALSE,
        CREATE_NEW_CONSOLE | NORMAL_PRIORITY_CLASS,
        NULL, NULL, &si, &pi);
    
    CloseHandle(hToken);
    if (result) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    return result;
}

void ShowError(HWND hWnd, LPCSTR msg) {
    MessageBox(hWnd, msg, "KittyPrivSystem Error", MB_OK | MB_ICONERROR);
}

BOOL BrowseForFile(HWND hWnd, LPSTR path, DWORD pathSize, BOOL forOpen) {
    OPENFILENAME ofn = {sizeof(ofn)};
    char fileBuffer[MAX_PATH] = "";
    
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = "Executable files\0*.exe\0All files\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = pathSize;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    
    if (forOpen)
        return GetOpenFileName(&ofn);
    else
        return GetSaveFileName(&ofn);
}
