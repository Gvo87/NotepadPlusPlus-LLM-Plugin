#include "PluginDefinition.h"
#include "menuCmdID.h"
#include "LLMService.h"
#include "SettingsDialog.h"
#include "resource.h"

// Scintilla message definitions
#ifndef SCI_GETCURRENTPOS
#define SCI_GETCURRENTPOS 2008
#define SCI_GETANCHOR 2009
#define SCI_SETCURRENTPOS 2141
#define SCI_SETANCHOR 2022
#define SCI_SCROLLCARET 2169
#endif
#include <string>
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <tchar.h>

// Forward declarations
std::wstring getSelectedText();
void replaceSelectedText(const std::wstring& newText);
std::wstring getCurrentFileName();
std::wstring getFileExtension(const std::wstring& filename);
std::wstring getLanguageContext(const std::wstring& extension);
std::wstring getContextualPrompt(const std::wstring& basePrompt, const std::wstring& operation);
void showStatusMessage(const wchar_t* message);
void clearStatusMessage();

// Global module handle for the plugin DLL
HMODULE g_hModule = NULL;

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE hModule)
{
    // Store the module handle for resource access
    g_hModule = (HMODULE)hModule;
    
    // Initialize LLM service
    LLMService::getInstance().initialize();
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
    LLMService::getInstance().cleanup();
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{
    //--------------------------------------------//
    //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
    //--------------------------------------------//
    // with function :
    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );
    // Create shortcut for Custom Prompt: Ctrl+Shift+Alt+P
    ShortcutKey* customShortcut = new ShortcutKey;
    customShortcut->_isCtrl = true;
    customShortcut->_isShift = true;
    customShortcut->_isAlt = true;
    customShortcut->_key = 'P';
    
    setCommand(0, (TCHAR*)TEXT("Custom Prompt"), customPrompt, customShortcut, false);
    
    // Create shortcut for Enhance Text: Ctrl+Shift+Alt+E
    ShortcutKey* enhanceShortcut = new ShortcutKey;
    enhanceShortcut->_isCtrl = true;
    enhanceShortcut->_isShift = true;
    enhanceShortcut->_isAlt = true;
    enhanceShortcut->_key = 'E';
    
    setCommand(1, (TCHAR*)TEXT("Enhance Text"), enhanceText, enhanceShortcut, false);
    
    // Create shortcut for Generate Code: Ctrl+Shift+Alt+G
    ShortcutKey* generateShortcut = new ShortcutKey;
    generateShortcut->_isCtrl = true;
    generateShortcut->_isShift = true;
    generateShortcut->_isAlt = true;
    generateShortcut->_key = 'G';
    
    setCommand(2, (TCHAR*)TEXT("Generate Code"), generateCode, generateShortcut, false);
    
    // Create shortcut for Complete Text: Ctrl+Shift+Alt+C
    ShortcutKey* completeShortcut = new ShortcutKey;
    completeShortcut->_isCtrl = true;
    completeShortcut->_isShift = true;
    completeShortcut->_isAlt = true;
    completeShortcut->_key = 'C';
    
    setCommand(3, (TCHAR*)TEXT("Complete Text"), completeText, completeShortcut, false);
    
    // Create shortcut for Check Grammar: Ctrl+Shift+Alt+R
    ShortcutKey* grammarShortcut = new ShortcutKey;
    grammarShortcut->_isCtrl = true;
    grammarShortcut->_isShift = true;
    grammarShortcut->_isAlt = true;
    grammarShortcut->_key = 'R';
    
    setCommand(4, (TCHAR*)TEXT("Check Grammar"), checkGrammar, grammarShortcut, false);
    
    // Create shortcut for Translate Text: Ctrl+Shift+Alt+T
    ShortcutKey* translateShortcut = new ShortcutKey;
    translateShortcut->_isCtrl = true;
    translateShortcut->_isShift = true;
    translateShortcut->_isAlt = true;
    translateShortcut->_key = 'T';
    
    setCommand(5, (TCHAR*)TEXT("Translate Text"), translateText, translateShortcut, false);
    
    // Create shortcut for Summarize Text: Ctrl+Shift+Alt+S
    ShortcutKey* summarizeShortcut = new ShortcutKey;
    summarizeShortcut->_isCtrl = true;
    summarizeShortcut->_isShift = true;
    summarizeShortcut->_isAlt = true;
    summarizeShortcut->_key = 'S';
    
    setCommand(6, (TCHAR*)TEXT("Summarize Text"), summarizeText, summarizeShortcut, false);
    setCommand(7, (TCHAR*)TEXT("Settings"), openSettings, NULL, false);
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
    // Clean up all allocated shortcuts
    for (int i = 0; i < 7; i++) {
        if (funcItem[i]._pShKey) {
            delete funcItem[i]._pShKey;
            funcItem[i]._pShKey = nullptr;
        }
    }
}

//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//

void customPrompt()
{
    // Show input dialog for custom prompt first
    wchar_t promptBuffer[1024] = {0};
    
    // Use a simple input dialog with GetDlgItemText approach
    INT_PTR result = DialogBoxParam(getPluginModuleHandle(), 
                                   MAKEINTRESOURCE(IDD_INPUT_PROMPT), 
                                   nppData._nppHandle, 
                                   InputPromptDialogProc, 
                                   (LPARAM)promptBuffer);
    
    if (result == IDOK) {
        std::wstring userPrompt(promptBuffer);
        if (userPrompt.empty()) {
            MessageBox(nppData._nppHandle, TEXT("Please enter a prompt."), TEXT("LLM Assistant"), MB_OK);
            return;
        }
        
        // Get the selected text after the dialog is closed
        std::wstring selectedText = getSelectedText();
        if (selectedText.empty()) {
            MessageBox(nppData._nppHandle, TEXT("No text selected. Please select some text to process."), TEXT("LLM Assistant"), MB_OK);
            return;
        }
        
        // Combine user prompt with selected text
        std::wstring fullPrompt = userPrompt + L"\n\nText to process:\n\n" + selectedText;
        showStatusMessage(L"Processing LLM request...");
    std::wstring result = LLMService::getInstance().callLLM(fullPrompt);
    clearStatusMessage();
        
        if (!result.empty()) {
            replaceSelectedText(result);
        }
    }
}

void enhanceText()
{
    // Store selection positions FIRST before any other operations
    int currentEdit;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);
    HWND curScintilla = (currentEdit == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
    
    int originalSelStart = ::SendMessage(curScintilla, SCI_GETSELECTIONSTART, 0, 0);
    int originalSelEnd = ::SendMessage(curScintilla, SCI_GETSELECTIONEND, 0, 0);
    
    std::wstring selectedText = getSelectedText();
    if (selectedText.empty()) {
        MessageBox(nppData._nppHandle, TEXT("No text available to enhance. Please open a document with text."), TEXT("LLM Assistant"), MB_OK);
        return;
    }
    
    std::wstring basePrompt = L"please enhance and improve the following text while maintaining its original meaning:\n\n" + selectedText;
    std::wstring prompt = getContextualPrompt(basePrompt, L"enhance");
    showStatusMessage(L"Processing LLM request...");
    std::wstring result = LLMService::getInstance().callLLM(prompt);
    clearStatusMessage();
    
    if (!result.empty()) {
        // Restore selection before replacing
        ::SendMessage(curScintilla, SCI_SETSEL, originalSelStart, originalSelEnd);
        replaceSelectedText(result);
    }
}

void generateCode()
{
    // Store selection positions FIRST before any other operations
    int currentEdit;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);
    HWND curScintilla = (currentEdit == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
    
    int originalSelStart = ::SendMessage(curScintilla, SCI_GETSELECTIONSTART, 0, 0);
    int originalSelEnd = ::SendMessage(curScintilla, SCI_GETSELECTIONEND, 0, 0);
    
    std::wstring selectedText = getSelectedText();
    if (selectedText.empty()) {
        MessageBox(nppData._nppHandle, TEXT("No text available to generate code from. Please open a document with text."), TEXT("LLM Assistant"), MB_OK);
        return;
    }
    
    std::wstring basePrompt = L"generate code based on the following description or comment:\n\n" + selectedText;
    std::wstring prompt = getContextualPrompt(basePrompt, L"generate");
    showStatusMessage(L"Processing LLM request...");
    std::wstring result = LLMService::getInstance().callLLM(prompt);
    clearStatusMessage();
    
    if (!result.empty()) {
        // Restore selection before replacing
        ::SendMessage(curScintilla, SCI_SETSEL, originalSelStart, originalSelEnd);
        replaceSelectedText(result);
    }
}

void completeText()
{
    std::wstring selectedText = getSelectedText();
    if (selectedText.empty()) {
        MessageBox(nppData._nppHandle, TEXT("No text available to complete. Please open a document with text."), TEXT("LLM Assistant"), MB_OK);
        return;
    }
    
    std::wstring basePrompt = L"complete the following text naturally:\n\n" + selectedText;
    std::wstring prompt = getContextualPrompt(basePrompt, L"complete");
    showStatusMessage(L"Processing LLM request...");
    std::wstring result = LLMService::getInstance().callLLM(prompt);
    clearStatusMessage();
    
    if (!result.empty()) {
        replaceSelectedText(selectedText + result);
    }
}

void checkGrammar()
{
    std::wstring selectedText = getSelectedText();
    if (selectedText.empty()) {
        MessageBox(nppData._nppHandle, TEXT("No text available to check grammar. Please open a document with text."), TEXT("LLM Assistant"), MB_OK);
        return;
    }
    
    std::wstring basePrompt = L"check and correct the grammar in the following text:\n\n" + selectedText;
    std::wstring prompt = getContextualPrompt(basePrompt, L"grammar");
    showStatusMessage(L"Processing LLM request...");
    std::wstring result = LLMService::getInstance().callLLM(prompt);
    clearStatusMessage();
    
    if (!result.empty()) {
        replaceSelectedText(result);
    }
}

void translateText()
{
    std::wstring selectedText = getSelectedText();
    if (selectedText.empty()) {
        MessageBox(nppData._nppHandle, TEXT("No text available to translate. Please open a document with text."), TEXT("LLM Assistant"), MB_OK);
        return;
    }
    
    std::wstring basePrompt = L"translate the following text to English (or to the most appropriate language if already in English):\n\n" + selectedText;
    std::wstring prompt = getContextualPrompt(basePrompt, L"translate");
    showStatusMessage(L"Processing LLM request...");
    std::wstring result = LLMService::getInstance().callLLM(prompt);
    clearStatusMessage();
    
    if (!result.empty()) {
        replaceSelectedText(result);
    }
}

void summarizeText()
{
    std::wstring selectedText = getSelectedText();
    if (selectedText.empty()) {
        MessageBox(nppData._nppHandle, TEXT("No text available to summarize. Please open a document with text."), TEXT("LLM Assistant"), MB_OK);
        return;
    }
    
    std::wstring basePrompt = L"summarize the following text concisely:\n\n" + selectedText;
    std::wstring prompt = getContextualPrompt(basePrompt, L"summarize");
    showStatusMessage(L"Processing LLM request...");
    std::wstring result = LLMService::getInstance().callLLM(prompt);
    clearStatusMessage();
    
    if (!result.empty()) {
        replaceSelectedText(result);
    }
}

void openSettings()
{
    SettingsDialog dialog;
    dialog.show(nppData._nppHandle);
}

// Context-aware helper functions
std::wstring getCurrentFileName()
{
    wchar_t filename[MAX_PATH];
    ::SendMessage(nppData._nppHandle, NPPM_GETFILENAME, MAX_PATH, (LPARAM)filename);
    return std::wstring(filename);
}

std::wstring getFileExtension(const std::wstring& filename)
{
    size_t dotPos = filename.find_last_of(L'.');
    if (dotPos != std::wstring::npos && dotPos < filename.length() - 1) {
        std::wstring ext = filename.substr(dotPos);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
        return ext;
    }
    return L"";
}

std::wstring getLanguageContext(const std::wstring& extension)
{
    if (extension == L".py") {
        return L"Python";
    } else if (extension == L".js" || extension == L".jsx") {
        return L"JavaScript";
    } else if (extension == L".ts" || extension == L".tsx") {
        return L"TypeScript";
    } else if (extension == L".cpp" || extension == L".cc" || extension == L".cxx") {
        return L"C++";
    } else if (extension == L".c") {
        return L"C";
    } else if (extension == L".cs") {
        return L"C#";
    } else if (extension == L".java") {
        return L"Java";
    } else if (extension == L".php") {
        return L"PHP";
    } else if (extension == L".rb") {
        return L"Ruby";
    } else if (extension == L".go") {
        return L"Go";
    } else if (extension == L".rs") {
        return L"Rust";
    } else if (extension == L".html" || extension == L".htm") {
        return L"HTML";
    } else if (extension == L".css") {
        return L"CSS";
    } else if (extension == L".sql") {
        return L"SQL";
    } else if (extension == L".md") {
        return L"Markdown";
    } else if (extension == L".json") {
        return L"JSON";
    } else if (extension == L".xml") {
        return L"XML";
    } else if (extension == L".yaml" || extension == L".yml") {
        return L"YAML";
    }
    return L"";
}

std::wstring getContextualPrompt(const std::wstring& basePrompt, const std::wstring& operation)
{
    std::wstring filename = getCurrentFileName();
    std::wstring extension = getFileExtension(filename);
    std::wstring language = getLanguageContext(extension);
    
    if (language.empty()) {
        return basePrompt;
    }
    
    std::wstring contextualPrompt = L"As a " + language + L" expert, " + basePrompt;
    
    // Add language-specific guidelines based on operation
    if (operation == L"enhance") {
        if (language == L"Python") {
            contextualPrompt += L" Follow PEP 8 standards, use type hints, and include docstrings.";
        } else if (language == L"JavaScript" || language == L"TypeScript") {
            contextualPrompt += L" Use modern ES6+ syntax, proper async/await patterns, and JSDoc comments.";
        } else if (language == L"C++") {
            contextualPrompt += L" Follow modern C++ best practices, use RAII, and include proper comments.";
        } else if (language == L"Java") {
            contextualPrompt += L" Follow Java conventions, use proper naming, and include Javadoc comments.";
        } else if (language == L"C#") {
            contextualPrompt += L" Follow C# conventions, use proper naming, and include XML documentation.";
        }
    } else if (operation == L"generate") {
        if (language == L"Python") {
            contextualPrompt += L" Write clean, Pythonic code with proper error handling and type hints.";
        } else if (language == L"JavaScript" || language == L"TypeScript") {
            contextualPrompt += L" Write modern, efficient code with proper error handling and clear structure.";
        } else if (language == L"C++") {
            contextualPrompt += L" Write efficient, memory-safe code with proper resource management.";
        }
    } else if (operation == L"grammar") {
        if (language == L"Markdown") {
            contextualPrompt += L" Maintain proper Markdown formatting while fixing grammar.";
        } else if (language == L"HTML") {
            contextualPrompt += L" Fix grammar in text content while preserving HTML structure.";
        }
    }
    
    return contextualPrompt;
}

// Status bar message IDs (from Notepad++ source)
#define STATUSBAR_DOC_TYPE 0
#define NPPM_SETSTATUSBAR (NPPMSG + 30)

// Helper functions
void showStatusMessage(const wchar_t* message) {
    if (!nppData._nppHandle) return;
    
    // Set status bar text (part 0 is the main status bar)
    ::SendMessage(nppData._nppHandle, NPPM_SETSTATUSBAR, STATUSBAR_DOC_TYPE, (LPARAM)message);
    
    // Force the UI to update
    ::UpdateWindow(nppData._nppHandle);
}

void clearStatusMessage() {
    if (!nppData._nppHandle) return;
    
    // Clear the status bar
    ::SendMessage(nppData._nppHandle, NPPM_SETSTATUSBAR, STATUSBAR_DOC_TYPE, (LPARAM)L"");
    
    // Force the UI to update
    ::UpdateWindow(nppData._nppHandle);
}

// Structure to hold selection state
struct SelectionState {
    int start;
    int end;
    int currentPos;
    int anchor;
};

// Helper function to get and save the current selection state
SelectionState saveSelectionState(HWND hScintilla) {
    SelectionState state;
    state.start = (int)::SendMessage(hScintilla, SCI_GETSELECTIONSTART, 0, 0);
    state.end = (int)::SendMessage(hScintilla, SCI_GETSELECTIONEND, 0, 0);
    state.currentPos = (int)::SendMessage(hScintilla, SCI_GETCURRENTPOS, 0, 0);
    state.anchor = (int)::SendMessage(hScintilla, SCI_GETANCHOR, 0, 0);
    return state;
}

// Helper function to restore the selection state
void restoreSelectionState(HWND hScintilla, const SelectionState& state) {
    // First set the selection range
    ::SendMessage(hScintilla, SCI_SETSEL, state.start, state.end);
    
    // Then set the anchor and current position to preserve the selection direction
    ::SendMessage(hScintilla, SCI_SETCURRENTPOS, state.currentPos, 0);
    ::SendMessage(hScintilla, SCI_SETANCHOR, state.anchor, 0);
    
    // Ensure the caret is visible
    ::SendMessage(hScintilla, SCI_SCROLLCARET, 0, 0);
}

std::wstring getSelectedText()
{
    // Get current Scintilla handle
    int currentEdit;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);
    HWND curScintilla = (currentEdit == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;

    if (!curScintilla) return L"";

    // Get selection length
    int selLength = ::SendMessage(curScintilla, SCI_GETSELTEXT, 0, 0);
    if (selLength <= 1) return L""; // No text selected

    // Allocate buffer for UTF-8 text
    char* buffer = new char[selLength];
    if (!buffer) return L"";

    // Get the selected text
    ::SendMessage(curScintilla, SCI_GETSELTEXT, 0, (LPARAM)buffer);

    // Convert UTF-8 to wide string
    int wideLength = MultiByteToWideChar(CP_UTF8, 0, buffer, -1, NULL, 0);
    if (wideLength <= 0) {
        delete[] buffer;
        return L"";
    }

    wchar_t* wideBuffer = new wchar_t[wideLength];
    if (!wideBuffer) {
        delete[] buffer;
        return L"";
    }

    MultiByteToWideChar(CP_UTF8, 0, buffer, -1, wideBuffer, wideLength);

    std::wstring result(wideBuffer);

    delete[] buffer;
    delete[] wideBuffer;

    return result;
}

void replaceSelectedText(const std::wstring& newText)
{
    HWND curScintilla = nullptr;
    SelectionState savedState;
    
    // Debug: Show the new text we're trying to replace
    char debugMsg[512];
    sprintf_s(debugMsg, "[LLM] Replacing text with: %.*S (length=%zu)\n", 200, newText.c_str(), newText.length());
    OutputDebugStringA(debugMsg);
    
    int currentEdit;
    HWND nppHandle = nppData._nppHandle;
    if (!nppHandle || !IsWindow(nppHandle)) {
        OutputDebugStringA("[LLM] ERROR: Invalid NPP handle or window\n");
        return;
    }
    
    // Get current Scintilla view
    ::SendMessage(nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);
    curScintilla = (currentEdit == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
    
    if (!curScintilla || !IsWindow(curScintilla)) {
        OutputDebugStringA("[LLM] ERROR: curScintilla is null or invalid in replaceSelectedText\n");
        return;
    }
    
    // Save the current selection state before any operation
    savedState = saveSelectionState(curScintilla);
    
    // Debug: Show current selection info
    sprintf_s(debugMsg, "[LLM] Current selection - start=%d, end=%d, length=%d\n", 
             savedState.start, savedState.end, savedState.end - savedState.start);
    OutputDebugStringA(debugMsg);
    
    if (newText.empty()) {
        OutputDebugStringA("[LLM] newText is empty, clearing selection\n");
        // Restore selection before clearing
        restoreSelectionState(curScintilla, savedState);
        ::SendMessage(curScintilla, SCI_REPLACESEL, 0, (LPARAM)"");
        return;
    }
    
    // Restore the selection before replacing text
    restoreSelectionState(curScintilla, savedState);
    
    // Get the current selection after restoring it
    int currentSelStart = (int)::SendMessage(curScintilla, SCI_GETSELECTIONSTART, 0, 0);
    int currentSelEnd = (int)::SendMessage(curScintilla, SCI_GETSELECTIONEND, 0, 0);
    
    sprintf_s(debugMsg, "[LLM] Current selection before replace - start=%d, end=%d, length=%d\n", 
             currentSelStart, currentSelEnd, currentSelEnd - currentSelStart);
    OutputDebugStringA(debugMsg);
    
    // Add newlines for better formatting
    std::wstring textWithNewlines = newText + L"\n\n";
    
    // Declare variables for new selection
    int newSelStart = currentSelStart;
    int newSelEnd = currentSelStart;
    bool replacementSucceeded = false;
    
    // First, ensure the selection is set correctly
    ::SendMessage(curScintilla, SCI_SETSEL, currentSelStart, currentSelEnd);
    
    // Try multiple approaches for text replacement
    
    // Attempt 1: Direct wide character replacement (works for most cases)
    // if (!replacementSucceeded) {
    //     try {
    //         ::SendMessageW(curScintilla, SCI_REPLACESEL, 1, (LPARAM)textWithNewlines.c_str());
    //         OutputDebugStringA("[LLM] Text replacement completed using direct wide character method\n");
    //         newSelEnd = newSelStart + (int)textWithNewlines.length();
    //         replacementSucceeded = true;
    //     } catch (...) {
    //         OutputDebugStringA("[LLM] Direct wide character replacement failed, trying UTF-8 conversion\n");
    //     }
    // }
    
    // Attempt 2: UTF-8 conversion and replacement
    if (!replacementSucceeded) {
        int utf8Length = WideCharToMultiByte(CP_UTF8, 0, textWithNewlines.c_str(), -1, NULL, 0, NULL, NULL);
        if (utf8Length > 0) {
            std::string utf8Text(utf8Length, 0);
            WideCharToMultiByte(CP_UTF8, 0, textWithNewlines.c_str(), -1, &utf8Text[0], utf8Length, NULL, NULL);
            
            try {
                ::SendMessageA(curScintilla, SCI_REPLACESEL, 0, (LPARAM)utf8Text.c_str());
                OutputDebugStringA("[LLM] Text replacement completed using UTF-8 conversion\n");
                newSelEnd = newSelStart + (int)textWithNewlines.length();
                replacementSucceeded = true;
            } catch (...) {
                OutputDebugStringA("[LLM] UTF-8 conversion replacement failed, trying chunked replacement\n");
            }
        } else {
            OutputDebugStringA("[LLM] UTF-8 conversion failed, trying chunked replacement\n");
        }
    }
    
    // Attempt 3: Chunked replacement (for very large texts or encoding issues)
    if (!replacementSucceeded && !textWithNewlines.empty()) {
        try {
            const size_t chunkSize = 4096; // Reasonable chunk size
            size_t totalLength = textWithNewlines.length();
            size_t offset = 0;
            
            while (offset < totalLength) {
                size_t chunkLength = min(chunkSize, totalLength - offset);
                std::wstring chunk = textWithNewlines.substr(offset, chunkLength);
                
                // Try wide character first
                ::SendMessageW(curScintilla, SCI_REPLACESEL, 1, (LPARAM)chunk.c_str());
                
                offset += chunkLength;
                newSelEnd += (int)chunk.length();
                
                // Update selection for next chunk
                if (offset < totalLength) {
                    ::SendMessage(curScintilla, SCI_SETSEL, newSelEnd, newSelEnd);
                }
            }
            
            OutputDebugStringA("[LLM] Text replacement completed using chunked method\n");
            replacementSucceeded = true;
        } catch (...) {
            OutputDebugStringA("[LLM] Chunked replacement failed, trying single character fallback\n");
        }
    }
    
    // Final fallback: Single character replacement if all else fails
    if (!replacementSucceeded && !textWithNewlines.empty()) {
        try {
            // Try to replace with at least the first character
            std::wstring firstChar(1, textWithNewlines[0]);
            ::SendMessageW(curScintilla, SCI_REPLACESEL, 1, (LPARAM)firstChar.c_str());
            newSelEnd = newSelStart + 1;
            OutputDebugStringA("[LLM] Using single character fallback replacement\n");
        } catch (...) {
            OutputDebugStringA("[LLM] All replacement methods failed!\n");
            return; // Give up if even single character replacement fails
        }
    }
        
    // Create a new selection state for the replaced text
    SelectionState newState;
    newState.start = newSelStart;
    newState.end = newSelEnd;
    newState.currentPos = newSelEnd;  // Place cursor at the end
    newState.anchor = newSelStart;    // Anchor at the start
    
    // Apply the new selection
    restoreSelectionState(curScintilla, newState);
}

// Get plugin module handle
HMODULE getPluginModuleHandle()
{
    return g_hModule;
}

// Input prompt dialog procedure
INT_PTR CALLBACK InputPromptDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static wchar_t* promptBuffer = nullptr;
    
    switch (message)
    {
    case WM_INITDIALOG:
        {
            // Store the buffer pointer passed via lParam
            promptBuffer = (wchar_t*)lParam;
            
            // Set focus to the text input field
            SetFocus(GetDlgItem(hDlg, IDC_PROMPT_TEXT));
            
            // Set some default helpful text
            SetDlgItemText(hDlg, IDC_PROMPT_TEXT, L"Summarize this text");
            
            return FALSE; // We set the focus manually
        }
        
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            {
                // Get the text from the input field
                int textLength = GetWindowTextLength(GetDlgItem(hDlg, IDC_PROMPT_TEXT));
                if (textLength > 0 && textLength < 1024 && promptBuffer)
                {
                    GetDlgItemText(hDlg, IDC_PROMPT_TEXT, promptBuffer, 1024);
                }
                else if (promptBuffer)
                {
                    promptBuffer[0] = L'\0'; // Empty string
                }
                
                EndDialog(hDlg, IDOK);
                return TRUE;
            }
            
        case IDCANCEL:
            if (promptBuffer)
            {
                promptBuffer[0] = L'\0'; // Empty string
            }
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
        
    case WM_CLOSE:
        if (promptBuffer)
        {
            promptBuffer[0] = L'\0'; // Empty string
        }
        EndDialog(hDlg, IDCANCEL);
        return TRUE;
    }
    
    return FALSE;
}
