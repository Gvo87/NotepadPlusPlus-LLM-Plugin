#include "ModelParams.h"
#include "RequestTarget.h"
#include "SimpleJson.h"
#include "TextCodec.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("UTF-8 conversion round trips international text")
{
    const std::wstring original = L"Café 日本語 🙂";
    REQUIRE(utf8ToWide(wideToUtf8(original)) == original);
}

TEST_CASE("JSON builder escapes every JSON control character")
{
    JsonBuilder builder;
    builder.startObject();
    builder.addString("content", std::string("quote\"\n\t\r\b\f") + static_cast<char>(0x01));
    builder.endObject();

    const std::string json = builder.toString();
    REQUIRE(json.find("\\\"") != std::string::npos);
    REQUIRE(json.find("\\n") != std::string::npos);
    REQUIRE(json.find("\\t") != std::string::npos);
    REQUIRE(json.find("\\r") != std::string::npos);
    REQUIRE(json.find("\\b") != std::string::npos);
    REQUIRE(json.find("\\f") != std::string::npos);
    REQUIRE(json.find("\\u0001") != std::string::npos);
}

TEST_CASE("OpenAI content parses escaped Unicode")
{
    SimpleJson parser(R"({"choices":[{"message":{"content":"Caf\u00e9 \ud83d\ude42"}}]})");
    REQUIRE_FALSE(parser.hasError());
    REQUIRE(utf8ToWide(parser.extractContent()) == L"Café 🙂");
}

TEST_CASE("OpenAI errors are recognized separately from content")
{
    SimpleJson parser(R"({"error":{"message":"Incorrect API key provided"}})");
    REQUIRE(parser.hasError());
    REQUIRE(parser.getErrorMessage() == "Incorrect API key provided");
    REQUIRE(parser.extractContent().empty());
}

TEST_CASE("GPT-5 and newer models use reasoning parameters")
{
    REQUIRE(isReasoningModel(L"gpt-6-luna"));
    REQUIRE(isReasoningModel(L"gpt-6-astra"));
    REQUIRE(isReasoningModel(L"gpt-6.1-sol"));
    REQUIRE(isReasoningModel(L"gpt-5.6-terra"));
    REQUIRE(isReasoningModel(L"GPT-5-mini"));
    REQUIRE(isReasoningModel(L"o4-mini"));
}

TEST_CASE("Legacy and chat-tuned models keep classic parameters")
{
    REQUIRE_FALSE(isReasoningModel(L"gpt-4o"));
    REQUIRE_FALSE(isReasoningModel(L"gpt-4.1-mini"));
    REQUIRE_FALSE(isReasoningModel(L"gpt-3.5-turbo"));
    REQUIRE_FALSE(isReasoningModel(L"gpt-5-chat-latest"));
    REQUIRE_FALSE(isReasoningModel(L"chatgpt-4o-latest"));
}

TEST_CASE("Deprecated and shut-down models are retired")
{
    REQUIRE(isRetiredModel(L"gpt-4o"));
    REQUIRE(isRetiredModel(L"GPT-4o-mini"));
    REQUIRE(isRetiredModel(L"gpt-4.1-nano"));
    REQUIRE(isRetiredModel(L"gpt-3.5-turbo"));
    REQUIRE(isRetiredModel(L"gpt-5"));
    REQUIRE(isRetiredModel(L"gpt-5-mini"));
    REQUIRE(isRetiredModel(L"gpt-5-chat-latest"));
    REQUIRE(isRetiredModel(L"gpt-5.1"));
    REQUIRE(isRetiredModel(L"gpt-5.2-chat-latest"));
}

TEST_CASE("Current models are not retired")
{
    REQUIRE_FALSE(isRetiredModel(L"gpt-6-luna"));
    REQUIRE_FALSE(isRetiredModel(L"gpt-6.1-sol"));
    REQUIRE_FALSE(isRetiredModel(L"gpt-5.6-terra"));
    REQUIRE_FALSE(isRetiredModel(L"gpt-5.5"));
    REQUIRE_FALSE(isRetiredModel(L"gpt-5.2"));
    REQUIRE_FALSE(isRetiredModel(L"o4-mini"));
}

TEST_CASE("Selection fallback captures the complete document range")
{
    const TextRange range = resolveTextRange(12, 12, 100);
    REQUIRE(range.usesWholeDocument);
    REQUIRE(range.start == 0);
    REQUIRE(range.end == 100);
    REQUIRE(isTextRangeValid(range, 100));
}

TEST_CASE("Selected range remains targeted")
{
    const TextRange range = resolveTextRange(4, 12, 100);
    REQUIRE_FALSE(range.usesWholeDocument);
    REQUIRE(range.start == 4);
    REQUIRE(range.end == 12);
    REQUIRE(isTextRangeValid(range, 100));
}

TEST_CASE("Changed document revisions reject stale responses")
{
    REQUIRE(isRevisionCurrent(9, 9));
    REQUIRE_FALSE(isRevisionCurrent(9, 10));
}
