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
            // Only count actual text inserts/deletes as document changes.
            // Scintilla also fires SCN_MODIFIED for styling, folding and markers,
            // which would otherwise cause the plugin to think the user edited the
            // document while an LLM request was running and discard the response.
            if (notifyCode->nmhdr.code == SCN_MODIFIED
                && (notifyCode->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT))) {
                noteDocumentModified(notifyCode->nmhdr.hwndFrom);
            }
            break;
    }
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
    return handlePluginMessage(Message, wParam, lParam);
}

extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
