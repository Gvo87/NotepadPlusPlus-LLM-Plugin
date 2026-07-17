#ifndef LLMSERVICE_H
#define LLMSERVICE_H

#include <string>
#include <mutex>
#include <windows.h>
#include <wininet.h>
#include "LLMResult.h"

class LLMService
{
private:
    static LLMService* instance;
    std::wstring apiKey;
    std::wstring apiUrl;
    mutable std::mutex settingsMutex;
    
    LLMService();
    ~LLMService();
    
    struct HttpResponse
    {
        bool succeeded;
        DWORD statusCode;
        std::string body;
        std::wstring error;
    };

    HttpResponse makeHttpRequest(const std::string& url, const std::string& headers, const std::string& postData);
    LLMResult extractContentFromResponse(const std::string& jsonResponse);

public:
    static LLMService& getInstance();
    
    void initialize();
    void cleanup();
    
    // Configuration
    void setApiKey(const std::wstring& key);
    std::wstring getApiKey() const;
    
    // Main LLM call function. This method is safe to call from a background thread.
    LLMResult callLLM(const std::wstring& prompt, const std::wstring& model = L"gpt-3.5-turbo");
    LLMResult callLLMWithApiKey(const std::wstring& prompt, const std::wstring& apiKey,
        const std::wstring& model = L"gpt-3.5-turbo");
    
    // Utility functions
    bool isConfigured() const;
    void loadSettings();
    void saveSettings();
};

#endif // LLMSERVICE_H
