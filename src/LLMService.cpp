#include "LLMService.h"
#include "SimpleJson.h"
#include <sstream>
#include <fstream>
#include <wininet.h>
#include <mutex>
#pragma comment(lib, "wininet.lib")

LLMService* LLMService::instance = nullptr;

LLMService::LLMService() : apiUrl(L"https://api.openai.com/v1/chat/completions")
{
}

LLMService::~LLMService()
{
}

LLMService& LLMService::getInstance()
{
    // Thread-safe singleton using double-checked locking
    if (!instance) {
        static std::mutex instanceMutex;
        std::lock_guard<std::mutex> lock(instanceMutex);
        if (!instance) {
            instance = new LLMService();
        }
    }
    return *instance;
}

void LLMService::initialize()
{
    loadSettings();
}

void LLMService::cleanup()
{
    saveSettings();
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

void LLMService::setApiKey(const std::wstring& key)
{
    apiKey = key;
}

std::wstring LLMService::getApiKey() const
{
    return apiKey;
}

bool LLMService::isConfigured() const
{
    return !apiKey.empty();
}

std::wstring LLMService::callLLM(const std::wstring& prompt, const std::wstring& model)
{
    if (!isConfigured()) {
        MessageBox(NULL, L"OpenAI API key not configured. Please set it in the plugin settings.", L"LLM Assistant", MB_OK | MB_ICONWARNING);
        return L"";
    }

    try {
        // Prepare JSON payload using JsonBuilder
        JsonBuilder builder;
        builder.startObject();
        builder.addString("model", std::string(model.begin(), model.end()));
        builder.addNumber("max_tokens", 1000);
        builder.addNumber("temperature", 0.7);
        
        builder.startArray("messages");
        builder.addObjectToArray();
        builder.addString("role", "user");
        builder.addString("content", std::string(prompt.begin(), prompt.end()));
        builder.endObjectInArray();
        builder.endArray();
        builder.endObject();
        
        std::string jsonString = builder.toString();
        
        // Prepare headers
        std::string authHeader = "Authorization: Bearer " + std::string(apiKey.begin(), apiKey.end());
        std::string headers = "Content-Type: application/json\r\n" + authHeader + "\r\n";
        
        // Make HTTP request
        std::string response = makeHttpRequest(std::string(apiUrl.begin(), apiUrl.end()), headers, jsonString);
        
        if (response.empty()) {
            MessageBox(NULL, L"Failed to get response from OpenAI API", L"LLM Assistant", MB_OK | MB_ICONERROR);
            return L"";
        }
        
        // Parse response
        return extractContentFromResponse(response);
    }
    catch (const std::exception& e) {
        std::wstring errorMsg = L"Error calling LLM: " + std::wstring(e.what(), e.what() + strlen(e.what()));
        MessageBox(NULL, errorMsg.c_str(), L"LLM Assistant", MB_OK | MB_ICONERROR);
        return L"";
    }
}

std::string LLMService::makeHttpRequest(const std::string& url, const std::string& headers, const std::string& postData)
{
    HINTERNET hInternet = nullptr;
    HINTERNET hConnect = nullptr;
    HINTERNET hRequest = nullptr;
    
    try {
        hInternet = InternetOpenA("LLM Assistant", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) {
            return "";
        }
        
        // Parse URL to get components
        URL_COMPONENTSA urlComp = {0};
        urlComp.dwStructSize = sizeof(urlComp);
        char hostname[256] = {0};
        char path[1024] = {0};
        urlComp.lpszHostName = hostname;
        urlComp.dwHostNameLength = sizeof(hostname) - 1;
        urlComp.lpszUrlPath = path;
        urlComp.dwUrlPathLength = sizeof(path) - 1;
        
        if (!InternetCrackUrlA(url.c_str(), 0, 0, &urlComp)) {
            InternetCloseHandle(hInternet);
            return "";
        }
        
        hConnect = InternetConnectA(hInternet, hostname, urlComp.nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
        if (!hConnect) {
            InternetCloseHandle(hInternet);
            return "";
        }
        
        DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
        if (urlComp.nScheme == INTERNET_SCHEME_HTTPS) {
            flags |= INTERNET_FLAG_SECURE;
        }
        
        hRequest = HttpOpenRequestA(hConnect, "POST", path, NULL, NULL, NULL, flags, 0);
        if (!hRequest) {
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);
            return "";
        }
        
        // Send request with headers and data
        BOOL result = HttpSendRequestA(hRequest, 
                                      headers.empty() ? NULL : headers.c_str(), 
                                      static_cast<DWORD>(headers.length()), 
                                      postData.empty() ? NULL : (LPVOID)postData.c_str(), 
                                      static_cast<DWORD>(postData.length()));
        
        std::string response;
        if (result) {
            char buffer[4096];
            DWORD bytesRead = 0;
            
            while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                response.append(buffer, bytesRead);
            }
        }
        
        // Cleanup resources
        if (hRequest) InternetCloseHandle(hRequest);
        if (hConnect) InternetCloseHandle(hConnect);
        if (hInternet) InternetCloseHandle(hInternet);
        
        return response;
    }
    catch (...) {
        // Ensure cleanup in case of exceptions
        if (hRequest) InternetCloseHandle(hRequest);
        if (hConnect) InternetCloseHandle(hConnect);
        if (hInternet) InternetCloseHandle(hInternet);
        return "";
    }
}

std::wstring LLMService::extractContentFromResponse(const std::string& jsonResponse)
{
    try {
        SimpleJson parser(jsonResponse);
        
        if (parser.hasError()) {
            std::string errorMsg = parser.getErrorMessage();
            return L"API Error: " + std::wstring(errorMsg.begin(), errorMsg.end());
        }
        
        std::string content = parser.extractContent();
        if (!content.empty()) {
            return std::wstring(content.begin(), content.end());
        }
        
        return L"No content found in response";
    }
    catch (const std::exception& e) {
        return L"Error parsing response: " + std::wstring(e.what(), e.what() + strlen(e.what()));
    }
}

void LLMService::loadSettings()
{
    HKEY hKey = nullptr;
    try {
        // Load API key from registry
        if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\NotepadPlusPlus\\LLMAssistant", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            wchar_t buffer[512] = {0};
            DWORD bufferSize = sizeof(buffer) - sizeof(wchar_t); // Leave space for null terminator
            DWORD valueType = REG_SZ;
            
            if (RegQueryValueEx(hKey, L"ApiKey", NULL, &valueType, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
                if (valueType == REG_SZ && bufferSize > 0) {
                    buffer[bufferSize / sizeof(wchar_t)] = L'\0'; // Ensure null termination
                    apiKey = buffer;
                }
            }
            RegCloseKey(hKey);
            hKey = nullptr;
        }
    }
    catch (...) {
        if (hKey) {
            RegCloseKey(hKey);
        }
    }
}

void LLMService::saveSettings()
{
    HKEY hKey = nullptr;
    try {
        // Save API key to registry
        if (RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\NotepadPlusPlus\\LLMAssistant", 0, NULL, 
                          REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            
            DWORD dataSize = static_cast<DWORD>((apiKey.length() + 1) * sizeof(wchar_t));
            if (RegSetValueEx(hKey, L"ApiKey", 0, REG_SZ, (LPBYTE)apiKey.c_str(), dataSize) == ERROR_SUCCESS) {
                // Successfully saved
            }
            
            RegCloseKey(hKey);
            hKey = nullptr;
        }
    }
    catch (...) {
        if (hKey) {
            RegCloseKey(hKey);
        }
    }
}
