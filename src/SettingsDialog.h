#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <windows.h>
#include <string>
#include "resource.h"

void setConnectionTestStatus(HWND dialog, bool succeeded, const std::wstring& details);
void setConnectionTestPending(HWND dialog);

class SettingsDialog
{
private:
    HWND hDlg;
    static SettingsDialog* instance;
    
    // Dialog procedure
    static INT_PTR CALLBACK dialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    
    // Event handlers
    void onInitDialog();
    void onOK();
    void onCancel();
    void onTestConnection();
    
public:
    SettingsDialog();
    ~SettingsDialog();
    
    void show(HWND parent);
    void hide();
};

#endif // SETTINGSDIALOG_H
