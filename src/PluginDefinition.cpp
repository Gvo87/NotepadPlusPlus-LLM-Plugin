#include "PluginDefinition.h"
#include "LLMResult.h"
#include "LLMService.h"
#include "RequestTarget.h"
#include "SettingsDialog.h"
#include "TextCodec.h"
#include "menuCmdID.h"
#include "resource.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <cwctype>
#include <map>
#include <string>
#include <thread>

#define STATUSBAR_DOC_TYPE 0
#define NPPM_SETSTATUSBAR (NPPMSG + 24)
#define SCI_GETDOCPOINTER 2357
#define LLM_INTERNAL_RESULT_READY 1

std::wstring getContextualPrompt(const std::wstring& basePrompt, const std::wstring& operation);
NppData nppData;

namespace {
struct DocumentTarget
{
    HWND scintilla;
    uintptr_t document;
    TextRange range;
    unsigned long revision;
};

struct RequestCompletion
{
    DocumentTarget target;
    LLMResult result;
    bool isConnectionTest;
    HWND dialog;
    std::wstring originalText;
    bool appendOriginalText;
};

HMODULE g_module = nullptr;
std::thread g_worker;
std::atomic<bool> g_requestActive(false);
bool g_runtimeInitialized = false;
bool g_serviceInitialized = false;
std::atomic<bool> g_shuttingDown(false);
std::map<HWND, unsigned long> g_documentRevisions;
HWND g_messageWindow = nullptr;
const wchar_t* const kMessageWindowClass = L"LLMAssistantMessageWindow";

// Timer that clears a completion message from the status bar after a short delay.
constexpr UINT_PTR kStatusClearTimerId = 1;
constexpr UINT kStatusClearDelayMs = 4000;

void processCompletion(RequestCompletion* completion);
void showStatusMessage(const wchar_t* message);
void clearStatusMessage();

// Shows a status message and schedules it to be cleared automatically.
void showTransientStatusMessage(const wchar_t* message)
{
    showStatusMessage(message);
    if (g_messageWindow) {
        SetTimer(g_messageWindow, kStatusClearTimerId, kStatusClearDelayMs, nullptr);
    }
}

LRESULT CALLBACK messageWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_LLM_RESULT) {
        processCompletion(reinterpret_cast<RequestCompletion*>(lParam));
        return 0;
    }
    if (message == WM_TIMER && wParam == kStatusClearTimerId) {
        KillTimer(window, kStatusClearTimerId);
        clearStatusMessage();
        return 0;
    }
    return DefWindowProc(window, message, wParam, lParam);
}

void createMessageWindow()
{
    if (g_messageWindow) {
        return;
    }

    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = messageWindowProc;
    windowClass.hInstance = g_module;
    windowClass.lpszClassName = kMessageWindowClass;
    RegisterClassExW(&windowClass);

    g_messageWindow = CreateWindowExW(0, kMessageWindowClass, L"", 0, 0, 0, 0, 0,
        HWND_MESSAGE, nullptr, g_module, nullptr);
}

void destroyMessageWindow()
{
    if (g_messageWindow) {
        DestroyWindow(g_messageWindow);
        g_messageWindow = nullptr;
    }
    UnregisterClassW(kMessageWindowClass, g_module);
}

void showStatusMessage(const wchar_t* message)
{
    if (nppData._nppHandle) {
        SendMessage(nppData._nppHandle, NPPM_SETSTATUSBAR, STATUSBAR_DOC_TYPE,
            reinterpret_cast<LPARAM>(message));
    }
}

void clearStatusMessage()
{
    // Notepad++ ignores an empty status string, so send a single space to blank it.
    showStatusMessage(L" ");
}

HWND currentScintilla()
{
    int currentEdit = 0;
    if (!nppData._nppHandle) {
        return nullptr;
    }

    SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, reinterpret_cast<LPARAM>(&currentEdit));
    return currentEdit == 0 ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
}

bool captureDocumentTarget(DocumentTarget& target, std::wstring& text)
{
    HWND scintilla = currentScintilla();
    if (!scintilla || !IsWindow(scintilla)) {
        return false;
    }

    const int documentLength = static_cast<int>(SendMessage(scintilla, SCI_GETLENGTH, 0, 0));
    const int selectionStart = static_cast<int>(SendMessage(scintilla, SCI_GETSELECTIONSTART, 0, 0));
    const int selectionEnd = static_cast<int>(SendMessage(scintilla, SCI_GETSELECTIONEND, 0, 0));
    const TextRange range = resolveTextRange(selectionStart, selectionEnd, documentLength);
    if (!isTextRangeValid(range, documentLength)) {
        return false;
    }

    std::string utf8(static_cast<size_t>(range.end - range.start) + 1, '\0');
    if (range.usesWholeDocument) {
        SendMessageA(scintilla, SCI_GETTEXT, documentLength + 1, reinterpret_cast<LPARAM>(&utf8[0]));
    } else {
        SendMessageA(scintilla, SCI_GETSELTEXT, 0, reinterpret_cast<LPARAM>(&utf8[0]));
    }
    utf8.resize(strlen(utf8.c_str()));
    text = utf8ToWide(utf8);
    if (text.empty()) {
        return false;
    }

    target.scintilla = scintilla;
    target.document = static_cast<uintptr_t>(SendMessage(scintilla, SCI_GETDOCPOINTER, 0, 0));
    target.range = range;
    target.revision = g_documentRevisions[scintilla];
    return target.document != 0;
}

bool targetCanBeUpdated(const DocumentTarget& target)
{
    if (!target.scintilla || !IsWindow(target.scintilla)
        || static_cast<uintptr_t>(SendMessage(target.scintilla, SCI_GETDOCPOINTER, 0, 0)) != target.document
        || !isRevisionCurrent(target.revision, g_documentRevisions[target.scintilla])) {
        return false;
    }

    const int documentLength = static_cast<int>(SendMessage(target.scintilla, SCI_GETLENGTH, 0, 0));
    return isTextRangeValid(target.range, documentLength);
}

void replaceCapturedRange(const DocumentTarget& target, const std::wstring& text)
{
    const std::string utf8 = wideToUtf8(text);
    if (utf8.empty() && !text.empty()) {
        MessageBox(nppData._nppHandle, L"Unable to encode the LLM response as UTF-8.",
            L"LLM Assistant", MB_OK | MB_ICONERROR);
        return;
    }

    SendMessage(target.scintilla, SCI_SETSEL, target.range.start, target.range.end);
    SendMessageA(target.scintilla, SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(utf8.c_str()));
}

void joinCompletedWorker()
{
    if (g_worker.joinable()) {
        g_worker.join();
    }
}

void processCompletion(RequestCompletion* completion)
{
    g_requestActive = false;

    if (!completion) {
        clearStatusMessage();
        return;
    }

    if (completion->isConnectionTest) {
        clearStatusMessage();
        if (IsWindow(completion->dialog)) {
            setConnectionTestStatus(completion->dialog, completion->result.success, completion->result.error);
        }
    } else if (!completion->result.success) {
        showTransientStatusMessage(L"LLM request failed.");
        MessageBox(nppData._nppHandle, completion->result.error.c_str(), L"LLM Assistant", MB_OK | MB_ICONERROR);
    } else if (!targetCanBeUpdated(completion->target)) {
        showTransientStatusMessage(L"LLM response discarded - document changed.");
        MessageBox(nppData._nppHandle,
            L"The document changed while the LLM request was running. The response was discarded.",
            L"LLM Assistant", MB_OK | MB_ICONWARNING);
    } else {
        const std::wstring replacement = completion->appendOriginalText
            ? completion->originalText + completion->result.content
            : completion->result.content;
        replaceCapturedRange(completion->target, replacement);
        showTransientStatusMessage(L"LLM response applied.");
    }

    delete completion;
}

void dispatchCompletion(RequestCompletion* completion)
{
    // Deliver the result to the UI thread through the plugin's own hidden
    // message-only window. PostMessage is cross-thread safe and the window's
    // messages are pumped by Notepad++'s main UI thread, so processCompletion
    // always runs on the UI thread. This avoids the fragile NPPM_MSGTOPLUGIN
    // routing that previously dropped results and left the request "stuck".
    if (!g_messageWindow || !IsWindow(g_messageWindow)) {
        g_requestActive = false;
        clearStatusMessage();
        delete completion;
        return;
    }

    if (!PostMessage(g_messageWindow, WM_LLM_RESULT, 0, reinterpret_cast<LPARAM>(completion))) {
        g_requestActive = false;
        clearStatusMessage();
        delete completion;
    }
}

void startRequest(const DocumentTarget& target, const std::wstring& text, const std::wstring& prompt,
    bool appendOriginalText)
{
    if (g_requestActive) {
        MessageBox(nppData._nppHandle, L"An LLM request is already in progress.", L"LLM Assistant",
            MB_OK | MB_ICONINFORMATION);
        return;
    }

    joinCompletedWorker();
    g_requestActive = true;
    if (g_messageWindow) {
        KillTimer(g_messageWindow, kStatusClearTimerId);
    }
    showStatusMessage(L"Processing LLM request...");
    g_worker = std::thread([target, text, prompt, appendOriginalText]() {
        RequestCompletion* completion = new RequestCompletion {
            target,
            LLMService::getInstance().callLLM(prompt),
            false,
            nullptr,
            text,
            appendOriginalText
        };
        if (g_shuttingDown) {
            delete completion;
            return;
        }
        dispatchCompletion(completion);
    });
}

void runCommand(const std::wstring& basePrompt, const std::wstring& operation, bool appendOriginalText = false)
{
    DocumentTarget target = {};
    std::wstring text;
    if (!captureDocumentTarget(target, text)) {
        MessageBox(nppData._nppHandle, L"No text is available. Open a document or select text to process.",
            L"LLM Assistant", MB_OK | MB_ICONWARNING);
        return;
    }

    const std::wstring prompt = getContextualPrompt(basePrompt + text, operation);
    startRequest(target, text, prompt, appendOriginalText);
}
}

HMODULE g_hModule = nullptr;
FuncItem funcItem[nbFunc];

LRESULT handlePluginMessage(UINT message, WPARAM, LPARAM lParam)
{
    if (message == LLM_INTERNAL_RESULT_READY) {
        processCompletion(reinterpret_cast<RequestCompletion*>(lParam));
        return TRUE;
    }

    if (message != NPPM_MSGTOPLUGIN) {
        return FALSE;
    }
    CommunicationInfo* info = reinterpret_cast<CommunicationInfo*>(lParam);
    if (!info || info->internalMsg != LLM_INTERNAL_RESULT_READY || !info->info) {
        return FALSE;
    }

    processCompletion(reinterpret_cast<RequestCompletion*>(info->info));
    return TRUE;
}

void pluginInit(HANDLE hModule)
{
    g_module = static_cast<HMODULE>(hModule);
    g_hModule = g_module;
}

void pluginRuntimeInit()
{
    if (g_runtimeInitialized) {
        return;
    }

    LLMService::getInstance().initialize();
    createMessageWindow();
    g_serviceInitialized = true;
    g_runtimeInitialized = true;
    g_shuttingDown = false;
}

void pluginCleanUp()
{
    if (!g_serviceInitialized) {
        return;
    }

    g_shuttingDown = true;
    if (g_worker.joinable()) {
        g_worker.join();
    }
    destroyMessageWindow();
    LLMService::getInstance().cleanup();
    g_runtimeInitialized = false;
    g_serviceInitialized = false;
    g_requestActive = false;
}

void noteDocumentModified(HWND scintilla)
{
    if (scintilla) {
        ++g_documentRevisions[scintilla];
    }
}

void commandMenuInit()
{
    ShortcutKey* customShortcut = new ShortcutKey { true, true, true, 'P' };
    ShortcutKey* enhanceShortcut = new ShortcutKey { true, true, true, 'E' };
    ShortcutKey* generateShortcut = new ShortcutKey { true, true, true, 'G' };
    ShortcutKey* completeShortcut = new ShortcutKey { true, true, true, 'C' };
    ShortcutKey* grammarShortcut = new ShortcutKey { true, true, true, 'R' };
    ShortcutKey* translateShortcut = new ShortcutKey { true, true, true, 'T' };
    ShortcutKey* summarizeShortcut = new ShortcutKey { true, true, true, 'S' };

    setCommand(0, const_cast<TCHAR*>(TEXT("Custom Prompt")), customPrompt, customShortcut, false);
    setCommand(1, const_cast<TCHAR*>(TEXT("Enhance Text")), enhanceText, enhanceShortcut, false);
    setCommand(2, const_cast<TCHAR*>(TEXT("Generate Code")), generateCode, generateShortcut, false);
    setCommand(3, const_cast<TCHAR*>(TEXT("Complete Text")), completeText, completeShortcut, false);
    setCommand(4, const_cast<TCHAR*>(TEXT("Check Grammar")), checkGrammar, grammarShortcut, false);
    setCommand(5, const_cast<TCHAR*>(TEXT("Translate Text")), translateText, translateShortcut, false);
    setCommand(6, const_cast<TCHAR*>(TEXT("Summarize Text")), summarizeText, summarizeShortcut, false);
    setCommand(7, const_cast<TCHAR*>(TEXT("Settings")), openSettings, nullptr, false);
}

void commandMenuCleanUp()
{
    for (int index = 0; index < nbFunc; ++index) {
        delete funcItem[index]._pShKey;
        funcItem[index]._pShKey = nullptr;
    }
}

bool setCommand(size_t index, TCHAR* commandName, PFUNCPLUGINCMD function, ShortcutKey* shortcut, bool checkedOnInit)
{
    if (index >= nbFunc || !function) {
        return false;
    }

    lstrcpy(funcItem[index]._itemName, commandName);
    funcItem[index]._pFunc = function;
    funcItem[index]._init2Check = checkedOnInit;
    funcItem[index]._pShKey = shortcut;
    return true;
}

void customPrompt()
{
    wchar_t promptBuffer[1024] = {};
    if (DialogBoxParam(getPluginModuleHandle(), MAKEINTRESOURCE(IDD_INPUT_PROMPT), nppData._nppHandle,
            InputPromptDialogProc, reinterpret_cast<LPARAM>(promptBuffer)) != IDOK) {
        return;
    }
    if (promptBuffer[0] == L'\0') {
        MessageBox(nppData._nppHandle, L"Please enter a prompt.", L"LLM Assistant", MB_OK | MB_ICONWARNING);
        return;
    }

    runCommand(std::wstring(promptBuffer) + L"\n\nText to process:\n\n", L"custom");
}

void enhanceText()
{
    runCommand(L"Please enhance and improve the following text while maintaining its original meaning:\n\n", L"enhance");
}

void generateCode()
{
    runCommand(L"Generate code based on the following description or comment:\n\n", L"generate");
}

void completeText()
{
    runCommand(L"Complete the following text naturally. Return only the continuation:\n\n", L"complete", true);
}

void checkGrammar()
{
    runCommand(L"Check and correct the grammar in the following text:\n\n", L"grammar");
}

void translateText()
{
    runCommand(L"Translate the following text to English. Return only the translation:\n\n", L"translate");
}

void summarizeText()
{
    runCommand(L"Summarize the following text concisely:\n\n", L"summarize");
}

void openSettings()
{
    SettingsDialog dialog;
    dialog.show(nppData._nppHandle);
}

void testConnection(HWND dialog, const std::wstring& apiKey, const std::wstring& model)
{
    if (g_requestActive) {
        setConnectionTestStatus(dialog, false, L"Another LLM request is already in progress.");
        return;
    }
    if (apiKey.empty()) {
        setConnectionTestStatus(dialog, false, L"Enter an API key first.");
        return;
    }

    const std::wstring testModel = model.empty() ? std::wstring(LLMService::defaultModel()) : model;
    joinCompletedWorker();
    g_requestActive = true;
    if (g_messageWindow) {
        KillTimer(g_messageWindow, kStatusClearTimerId);
    }
    showStatusMessage(L"Testing OpenAI connection...");
    g_worker = std::thread([dialog, apiKey, testModel]() {
        RequestCompletion* completion = new RequestCompletion {
            {},
            LLMService::getInstance().callLLMWithApiKey(
                L"Reply with the single word: connected.", apiKey, testModel),
            true,
            dialog,
            L"",
            false
        };
        // Post the result straight to the dialog window. The dialog's own modal loop
        // always pumps its messages, so this reliably reaches the UI thread even while
        // the modal Settings dialog is up.
        if (g_shuttingDown
            || !PostMessage(dialog, WM_LLM_TEST_COMPLETE, 0, reinterpret_cast<LPARAM>(completion))) {
            delete completion;
            g_requestActive = false;
        }
    });
}

void completeConnectionTest(HWND dialog, LPARAM lParam)
{
    RequestCompletion* completion = reinterpret_cast<RequestCompletion*>(lParam);
    if (!completion) {
        g_requestActive = false;
        return;
    }

    g_requestActive = false;
    clearStatusMessage();
    if (IsWindow(dialog)) {
        setConnectionTestStatus(dialog, completion->result.success, completion->result.error);
    }
    delete completion;
}

std::wstring getCurrentFileName()
{
    wchar_t filename[MAX_PATH] = {};
    SendMessage(nppData._nppHandle, NPPM_GETFILENAME, MAX_PATH, reinterpret_cast<LPARAM>(filename));
    return filename;
}

std::wstring getFileExtension(const std::wstring& filename)
{
    const size_t dotPosition = filename.find_last_of(L'.');
    if (dotPosition == std::wstring::npos || dotPosition == filename.length() - 1) {
        return L"";
    }

    std::wstring extension = filename.substr(dotPosition);
    std::transform(extension.begin(), extension.end(), extension.begin(), towlower);
    return extension;
}

std::wstring getLanguageContext(const std::wstring& extension)
{
    if (extension == L".py") return L"Python";
    if (extension == L".js" || extension == L".jsx") return L"JavaScript";
    if (extension == L".ts" || extension == L".tsx") return L"TypeScript";
    if (extension == L".cpp" || extension == L".cc" || extension == L".cxx") return L"C++";
    if (extension == L".c") return L"C";
    if (extension == L".cs") return L"C#";
    if (extension == L".java") return L"Java";
    if (extension == L".php") return L"PHP";
    if (extension == L".rb") return L"Ruby";
    if (extension == L".go") return L"Go";
    if (extension == L".rs") return L"Rust";
    if (extension == L".html" || extension == L".htm") return L"HTML";
    if (extension == L".css") return L"CSS";
    if (extension == L".sql") return L"SQL";
    if (extension == L".md") return L"Markdown";
    if (extension == L".json") return L"JSON";
    if (extension == L".xml") return L"XML";
    if (extension == L".yaml" || extension == L".yml") return L"YAML";
    return L"";
}

std::wstring getContextualPrompt(const std::wstring& basePrompt, const std::wstring& operation)
{
    const std::wstring language = getLanguageContext(getFileExtension(getCurrentFileName()));
    if (language.empty()) {
        return basePrompt;
    }

    std::wstring contextualPrompt = L"As a " + language + L" expert, " + basePrompt;
    if (operation == L"enhance") {
        if (language == L"Python") contextualPrompt += L" Follow PEP 8 standards, use type hints, and include docstrings.";
        else if (language == L"JavaScript" || language == L"TypeScript") contextualPrompt += L" Use modern ES6+ syntax and proper async/await patterns.";
        else if (language == L"C++") contextualPrompt += L" Follow modern C++ best practices and use RAII.";
    } else if (operation == L"generate") {
        if (language == L"Python") contextualPrompt += L" Write clean Pythonic code with error handling and type hints.";
        else if (language == L"JavaScript" || language == L"TypeScript") contextualPrompt += L" Write modern, efficient code with clear structure.";
        else if (language == L"C++") contextualPrompt += L" Write efficient, memory-safe code with proper resource management.";
    }
    return contextualPrompt;
}

HMODULE getPluginModuleHandle()
{
    return g_hModule;
}

INT_PTR CALLBACK InputPromptDialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    static wchar_t* promptBuffer = nullptr;
    switch (message) {
    case WM_INITDIALOG:
        promptBuffer = reinterpret_cast<wchar_t*>(lParam);
        SetFocus(GetDlgItem(dialog, IDC_PROMPT_TEXT));
        return FALSE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            GetDlgItemText(dialog, IDC_PROMPT_TEXT, promptBuffer, 1024);
            EndDialog(dialog, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(dialog, IDCANCEL);
            return TRUE;
        }
        break;
    case WM_CLOSE:
        EndDialog(dialog, IDCANCEL);
        return TRUE;
    }
    return FALSE;
}
