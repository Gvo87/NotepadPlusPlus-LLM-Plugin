#ifndef PLUGINDEFINITION_H
#define PLUGINDEFINITION_H

//
// All definitions of plugin interface
//
#include "PluginInterface.h"

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

// Get plugin module handle
HMODULE getPluginModuleHandle();

// Input prompt dialog procedure
INT_PTR CALLBACK InputPromptDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#endif //PLUGINDEFINITION_H
