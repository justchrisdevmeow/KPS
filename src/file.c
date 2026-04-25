#include "kittypriv.h"

// File browser with delete/view capability
void ShowFileBrowser(HWND hParent) {
    OPENFILENAME ofn = {sizeof(ofn)};
    char filePath[MAX_PATH] = "";
    ofn.hwndOwner = hParent;
    ofn.lpstrFilter = "All files\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;
    
    if (GetOpenFileName(&ofn)) {
        int result = MessageBox(hParent, 
            "Delete this file?\nThis action cannot be undone.", 
            "Confirm Delete", MB_YESNO | MB_ICONWARNING);
        
        if (result == IDYES) {
            if (DeleteFile(filePath)) {
                char msg[256];
                sprintf(msg, "Deleted: %s", filePath);
                MessageBox(hParent, msg, "Success", MB_OK);
            } else {
                ShowError(hParent, "Failed to delete file (may be in use or access denied)");
            }
        }
    }
}

// Main entry for file module
void ShowFileDialog(HWND hParent) {
    ShowFileBrowser(hParent);
}
