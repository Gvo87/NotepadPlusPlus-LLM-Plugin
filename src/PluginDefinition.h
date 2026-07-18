#ifndef PLUGINDEFINITION_H
#define PLUGINDEFINITION_H

//
// All definitions of plugin interface
//
#include "PluginInterface.h"

// Posted directly to the Settings dialog window when a connection test finishes.
// Delivering the result to the dialog's own modal message loop is reliable, unlike
// relaying it back through Notepad++ while the plugin's modal dialog is running.
#define WM_LLM_TEST_COMPLETE (WM_APP + 110)

//-------------------------------------//
//-- STEP 1. DEFINE YOUR PLUGIN NAME --//
//-------------------------------------//
// Here define your plugin name
//
const TCHAR NPP_PLUGIN_NAME[] = TEXT("LLM Assistant");

//-----------------------------------------------//
//-- STEP 2. DEFINE YOUR PLUGIN COMMAND NUMBER --//
//-----------------------------------------------//
//
// Here define the number of your plugin commands
//
const int nbFunc = 8;

//
// Initialization of your plugin data here
// It will be called while plugin loading
void pluginInit(HANDLE hModule);
void pluginRuntimeInit();

//
// Cleanup of your plugin
// It will be called while plugin unloading
void pluginCleanUp();
void noteDocumentModified(HWND scintilla);
LRESULT handlePluginMessage(UINT message, WPARAM wParam, LPARAM lParam);

//
//Initialization of your plugin commands
//
void commandMenuInit();

//
//Clean up your plugin commands allocation (if any)
//
void commandMenuCleanUp();

//
// Function which sets your command 
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk = NULL, bool check0nInit = false);

//
// Your plugin command functions
//
void customPrompt();
void enhanceText();
void generateCode();
void completeText();
void checkGrammar();
void translateText();
void summarizeText();
void openSettings();
void testConnection(HWND dialog, const std::wstring& apiKey);

// Finalizes a connection test on the UI thread. Called by the Settings dialog when it
// receives WM_LLM_TEST_COMPLETE. Resets request state, clears the status bar, updates
// the dialog, and frees the completion payload passed as lParam.
void completeConnectionTest(HWND dialog, LPARAM lParam);

// Get plugin module handle
HMODULE getPluginModuleHandle();

// Input prompt dialog procedure
INT_PTR CALLBACK InputPromptDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#endif //PLUGINDEFINITION_H
