#include "SettingsDialog.h"
#include "LLMService.h"
#include "PluginDefinition.h"
#include <commctrl.h>
#include <algorithm>
#include <cwctype>

SettingsDialog* SettingsDialog::instance = nullptr;

enum class ConnectionStatus { None, Pending, Success, Failure };
static ConnectionStatus g_connectionStatus = ConnectionStatus::None;

static void refreshConnectionStatusText(HWND dialog, const wchar_t* text)
{
    SetDlgItemText(dialog, IDC_CONNECTION_STATUS, text);
    InvalidateRect(GetDlgItem(dialog, IDC_CONNECTION_STATUS), nullptr, TRUE);
}

// Maps a verbose error into a short, meaningful label for the status line.
static const wchar_t* shortFailureReason(const std::wstring& details)
{
    std::wstring d = details;
    std::transform(d.begin(), d.end(), d.begin(), towlower);
    const auto has = [&](const wchar_t* needle) { return d.find(needle) != std::wstring::npos; };

    if (has(L"quota") || has(L"billing"))       return L"No quota";
    if (has(L"rate limit"))                      return L"Rate limited";
    if (has(L"enter an api key"))                return L"Enter a key";
    if (has(L"already in progress"))             return L"Busy";
    if (has(L"timed out") || has(L"timeout"))    return L"Timed out";
    if (has(L"incorrect api key") || has(L"invalid") || has(L"no api key") || has(L"api key"))
        return L"Invalid key";
    return L"Failed";
}

void setConnectionTestStatus(HWND dialog, bool succeeded, const std::wstring& details)
{
    if (!IsWindow(dialog)) {
        return;
    }

    g_connectionStatus = succeeded ? ConnectionStatus::Success : ConnectionStatus::Failure;
    const std::wstring text = succeeded
        ? std::wstring(L"Connected")
        : L"Failed: " + std::wstring(shortFailureReason(details));
    refreshConnectionStatusText(dialog, text.c_str());
    EnableWindow(GetDlgItem(dialog, IDC_TEST_CONNECTION), TRUE);
}

void setConnectionTestPending(HWND dialog)
{
    if (!IsWindow(dialog)) {
        return;
    }

    g_connectionStatus = ConnectionStatus::Pending;
    refreshConnectionStatusText(dialog, L"Testing...");
    EnableWindow(GetDlgItem(dialog, IDC_TEST_CONNECTION), FALSE);
}

SettingsDialog::SettingsDialog() : hDlg(NULL)
{
    instance = this;
}

SettingsDialog::~SettingsDialog()
{
    instance = nullptr;
}

void SettingsDialog::show(HWND parent)
{
    DialogBox(getPluginModuleHandle(), MAKEINTRESOURCE(IDD_SETTINGS), parent, dialogProc);
}

void SettingsDialog::hide()
{
    if (hDlg) {
        EndDialog(hDlg, 0);
    }
}

INT_PTR CALLBACK SettingsDialog::dialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (instance) {
        instance->hDlg = hDlg;
    }

    if (message == WM_LLM_TEST_COMPLETE) {
        completeConnectionTest(hDlg, lParam);
        return TRUE;
    }

    if (message == WM_CTLCOLORSTATIC
        && reinterpret_cast<HWND>(lParam) == GetDlgItem(hDlg, IDC_CONNECTION_STATUS)) {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        COLORREF color;
        switch (g_connectionStatus) {
            case ConnectionStatus::Success: color = RGB(0, 140, 0); break;
            case ConnectionStatus::Failure: color = RGB(200, 0, 0); break;
            case ConnectionStatus::Pending: color = RGB(110, 110, 110); break;
            default:                        color = GetSysColor(COLOR_WINDOWTEXT); break;
        }
        SetTextColor(hdc, color);
        SetBkMode(hdc, TRANSPARENT);
        return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_3DFACE));
    }

    switch (message) {
        case WM_INITDIALOG:
            if (instance) {
                instance->onInitDialog();
            }
            return TRUE;
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    if (instance) {
                        instance->onOK();
                    }
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
                    
                case IDCANCEL:
                    if (instance) {
                        instance->onCancel();
                    }
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
                    
                case IDC_TEST_CONNECTION:
                    if (instance) {
                        instance->onTestConnection();
                    }
                    return TRUE;
            }
            break;
    }
    
    return FALSE;
}

void SettingsDialog::onInitDialog()
{
    // Set current API key in the text field
    std::wstring currentKey = LLMService::getInstance().getApiKey();
    SetDlgItemText(hDlg, IDC_API_KEY, currentKey.c_str());

    // Populate the model selector with newer OpenAI models (gpt-4o and above).
    // The combo is editable, so a brand-new model name can also be typed in
    // without needing a plugin rebuild.
    HWND combo = GetDlgItem(hDlg, IDC_MODEL_COMBO);
    static const wchar_t* const kModels[] = {
        L"gpt-5",
        L"gpt-5-mini",
        L"gpt-5-nano",
        L"gpt-5-chat-latest",
        L"gpt-4.1",
        L"gpt-4.1-mini",
        L"gpt-4.1-nano",
        L"gpt-4o"
    };
    for (const wchar_t* model : kModels) {
        SendMessage(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(model));
    }

    const std::wstring currentModel = LLMService::getInstance().getModel();
    const LRESULT index = SendMessage(combo, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1),
        reinterpret_cast<LPARAM>(currentModel.c_str()));
    if (index != CB_ERR) {
        SendMessage(combo, CB_SETCURSEL, static_cast<WPARAM>(index), 0);
    } else {
        SetWindowText(combo, currentModel.c_str());
    }

    // Set focus to API key field
    SetFocus(GetDlgItem(hDlg, IDC_API_KEY));
}

void SettingsDialog::onOK()
{
    // Get API key from text field
    wchar_t buffer[512] = {};
    GetDlgItemText(hDlg, IDC_API_KEY, buffer, sizeof(buffer) / sizeof(wchar_t));

    // Get selected/typed model from the combo box
    wchar_t modelBuffer[256] = {};
    GetDlgItemText(hDlg, IDC_MODEL_COMBO, modelBuffer, sizeof(modelBuffer) / sizeof(wchar_t));

    // Save to LLM service
    LLMService::getInstance().setApiKey(buffer);
    LLMService::getInstance().setModel(modelBuffer);
    LLMService::getInstance().saveSettings();
    
    MessageBox(hDlg, L"Settings saved successfully!", L"LLM Assistant", MB_OK | MB_ICONINFORMATION);
}

void SettingsDialog::onCancel()
{
    // Nothing special to do on cancel
}

void SettingsDialog::onTestConnection()
{
    wchar_t buffer[512] = {};
    GetDlgItemText(hDlg, IDC_API_KEY, buffer, sizeof(buffer) / sizeof(wchar_t));
    wchar_t modelBuffer[256] = {};
    GetDlgItemText(hDlg, IDC_MODEL_COMBO, modelBuffer, sizeof(modelBuffer) / sizeof(wchar_t));
    setConnectionTestPending(hDlg);
    testConnection(hDlg, buffer, modelBuffer);
}
