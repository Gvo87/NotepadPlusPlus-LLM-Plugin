#ifndef LLMSERVICE_H
#define LLMSERVICE_H

#include <string>
#include <windows.h>
#include <wininet.h>

class LLMService
{
private:
    static LLMService* instance;
    std::wstring apiKey;
    std::wstring apiUrl;
    
    LLMService();
    ~LLMService();
    
    // HTTP request helper
    std::string makeHttpRequest(const std::string& url, const std::string& headers, const std::string& postData);
    
    // JSON helper functions
    std::string escapeJson(const std::wstring& text);
    std::wstring extractContentFromResponse(const std::string& jsonResponse);

public:
    static LLMService& getInstance();
    
    void initialize();
    void cleanup();
    
    // Configuration
    void setApiKey(const std::wstring& key);
    std::wstring getApiKey() const;
    
    // Main LLM call function
    std::wstring callLLM(const std::wstring& prompt, const std::wstring& model = L"gpt-3.5-turbo");
    
    // Utility functions
    bool isConfigured() const;
    void loadSettings();
    void saveSettings();
};

#endif // LLMSERVICE_H
