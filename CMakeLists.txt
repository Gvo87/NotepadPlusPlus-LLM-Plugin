cmake_minimum_required(VERSION 3.21)
project(LLMAssistantTests LANGUAGES CXX)

include(FetchContent)
include(CTest)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Catch2)

add_executable(LLMAssistantTests
    src/RequestTarget.cpp
    src/SimpleJson.cpp
    src/TextCodec.cpp
    tests/CoreTests.cpp
)

target_include_directories(LLMAssistantTests PRIVATE src)
target_compile_features(LLMAssistantTests PRIVATE cxx_std_17)
target_link_libraries(LLMAssistantTests PRIVATE Catch2::Catch2WithMain)

# The source files are UTF-8. Without /utf-8, MSVC decodes narrow/wide string
# literals using the system ANSI code page (e.g. CP1252 on CI runners), which
# corrupts non-ASCII literals like L"Café 🙂" and breaks the Unicode tests.
if(MSVC)
    target_compile_options(LLMAssistantTests PRIVATE /utf-8)
endif()

include(Catch)
catch_discover_tests(LLMAssistantTests)
