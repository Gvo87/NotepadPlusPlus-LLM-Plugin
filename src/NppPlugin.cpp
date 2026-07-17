#include "PluginDefinition.h"

extern FuncItem funcItem[nbFunc];
extern NppData nppData;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reasonForCall, LPVOID lpReserved)
{
    switch (reasonForCall)
    {
        case DLL_PROCESS_ATTACH:
            pluginInit(hModule);
            break;

        case DLL_PROCESS_DETACH:
            // Notepad++ sends NPPN_SHUTDOWN before unloading plugins. Avoid waiting
            // for a worker thread while the loader lock is held.
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
    nppData = notpadPlusData;
    pluginRuntimeInit();
    commandMenuInit();
}

extern "C" __declspec(dllexport) const TCHAR * getName()
{
    return NPP_PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
    *nbF = nbFunc;
    return funcItem;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
    // Handle Scintilla notifications if needed
    switch (notifyCode->nmhdr.code) 
    {
        case NPPN_SHUTDOWN:
            commandMenuCleanUp();
            pluginCleanUp();
            break;

        default:
            if (notifyCode->nmhdr.code == 2008) { // SCN_MODIFIED
                noteDocumentModified(notifyCode->nmhdr.hwndFrom);
            }
            break;
    }
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
    return TRUE;
}

extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
