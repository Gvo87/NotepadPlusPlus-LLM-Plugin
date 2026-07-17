#include "SettingsDialog.h"
#include "LLMService.h"
#include "PluginDefinition.h"
#include <commctrl.h>

SettingsDialog* SettingsDialog::instance = nullptr;

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
    
    // Set focus to API key field
    SetFocus(GetDlgItem(hDlg, IDC_API_KEY));
}

void SettingsDialog::onOK()
{
    // Get API key from text field
    wchar_t buffer[512];
    GetDlgItemText(hDlg, IDC_API_KEY, buffer, sizeof(buffer) / sizeof(wchar_t));
    
    // Save to LLM service
    LLMService::getInstance().setApiKey(buffer);
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
    testConnection(hDlg, buffer);
}
