#ifndef LLMSERVICE_H
#define LLMSERVICE_H

#include <string>
#include <mutex>
#include <windows.h>
#include <wininet.h>
#include "LLMResult.h"

class LLMService
{
public:
    // Default model used when none has been configured yet.
    static const wchar_t* defaultModel();

private:
    static LLMService* instance;
    std::wstring apiKey;
    std::wstring model;
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
    void setModel(const std::wstring& modelName);
    std::wstring getModel() const;
    
    // Main LLM call function. This method is safe to call from a background thread.
    // callLLM uses the configured API key and model.
    LLMResult callLLM(const std::wstring& prompt);
    LLMResult callLLMWithApiKey(const std::wstring& prompt, const std::wstring& apiKey,
        const std::wstring& model);
    
    // Utility functions
    bool isConfigured() const;
    void loadSettings();
    void saveSettings();
};

#endif // LLMSERVICE_H
