#ifndef TEXTCODEC_H
#define TEXTCODEC_H

#include <string>

std::string wideToUtf8(const std::wstring& value);
std::wstring utf8ToWide(const std::string& value);

#endif
