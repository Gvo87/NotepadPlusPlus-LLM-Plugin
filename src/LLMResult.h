#ifndef LLMRESULT_H
#define LLMRESULT_H

#include <string>
#include <utility>

struct LLMResult
{
    bool success;
    std::wstring content;
    std::wstring error;

    static LLMResult Success(std::wstring value)
    {
        return { true, std::move(value), L"" };
    }

    static LLMResult Failure(std::wstring message)
    {
        return { false, L"", std::move(message) };
    }
};

#endif
