#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include <windows.h>
#include <string>

// Scintilla messages
#define SCI_GETSELECTIONSTART 2143
#define SCI_GETSELECTIONEND 2144
#define SCI_GETSELTEXT 2161
#define SCI_REPLACESEL 2170
#define SCI_SELECTALL 2013
#define SCI_SETSEL 2160
#define SCI_GETTEXT 2182
#define SCI_GETLENGTH 2006
#define SCI_SETTEXT 2181
#define SCI_CLEARALL 2004
#define SCI_ADDTEXT 2001
#define SCI_PASTE 2302

// Notepad++ messages
#define NPPMSG (WM_USER + 1000)
#define NPPM_GETCURRENTSCINTILLA (NPPMSG + 4)
#define NPPM_GETFILENAME (NPPMSG + 29)
#define NPPM_MSGTOPLUGIN (NPPMSG + 47)

#define NPPN_FIRST 1000
#define NPPN_SHUTDOWN (NPPN_FIRST + 9)

// Forward declarations
struct SCNotification;

// Plugin interface structures
typedef struct {
    HWND _nppHandle;
    HWND _scintillaMainHandle;
    HWND _scintillaSecondHandle;
} NppData;

typedef void (*PFUNCPLUGINCMD)();

typedef struct {
    bool _isCtrl;
    bool _isAlt;
    bool _isShift;
    UCHAR _key;
} ShortcutKey;

typedef struct {
    TCHAR _itemName[64];
    PFUNCPLUGINCMD _pFunc;
    int _cmdID;
    bool _init2Check;
    ShortcutKey* _pShKey;
} FuncItem;

// SCNotification structure (simplified)
struct SCNotification {
    NMHDR nmhdr;
    int position;
    int ch;
    int modifiers;
    int modificationType;
    const char* text;
    int length;
    int linesAdded;
    int message;
    uintptr_t wParam;
    intptr_t lParam;
    int line;
    int foldLevelNow;
    int foldLevelPrev;
    int margin;
    int listType;
    int x;
    int y;
    int token;
    int annotationLinesAdded;
    int updated;
};

struct CommunicationInfo {
    long internalMsg;
    const wchar_t* srcModuleName;
    void* info;
};

// Plugin interface functions
extern "C" __declspec(dllexport) void setInfo(NppData);
extern "C" __declspec(dllexport) const TCHAR * getName();
extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *);
extern "C" __declspec(dllexport) void beNotified(SCNotification *);
extern "C" __declspec(dllexport) LRESULT messageProc(UINT, WPARAM, LPARAM);
extern "C" __declspec(dllexport) BOOL isUnicode();

// Helper function declarations
std::wstring getSelectedText();
void replaceSelectedText(const std::wstring& newText);

#endif // PLUGININTERFACE_H
