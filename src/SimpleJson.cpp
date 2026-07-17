#include "SimpleJson.h"
#include <sstream>
#include <algorithm>
#include <iomanip>

namespace {
bool appendUtf8CodePoint(std::string& output, unsigned int codePoint)
{
    if (codePoint <= 0x7F) {
        output += static_cast<char>(codePoint);
    } else if (codePoint <= 0x7FF) {
        output += static_cast<char>(0xC0 | (codePoint >> 6));
        output += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else if (codePoint <= 0xFFFF) {
        output += static_cast<char>(0xE0 | (codePoint >> 12));
        output += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        output += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else if (codePoint <= 0x10FFFF) {
        output += static_cast<char>(0xF0 | (codePoint >> 18));
        output += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
        output += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        output += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else {
        return false;
    }
    return true;
}

bool readHexCodePoint(const std::string& text, size_t& position, unsigned int& codePoint)
{
    if (position + 4 > text.size()) {
        return false;
    }

    codePoint = 0;
    for (size_t index = 0; index < 4; ++index) {
        const char character = text[position++];
        codePoint <<= 4;
        if (character >= '0' && character <= '9') {
            codePoint |= character - '0';
        } else if (character >= 'a' && character <= 'f') {
            codePoint |= character - 'a' + 10;
        } else if (character >= 'A' && character <= 'F') {
            codePoint |= character - 'A' + 10;
        } else {
            return false;
        }
    }
    return true;
}
}

SimpleJson::SimpleJson(const std::string& json) : jsonText(json), pos(0) {}

void SimpleJson::skipWhitespace() {
    while (pos < jsonText.length() && (jsonText[pos] == ' ' || jsonText[pos] == '\t' || 
           jsonText[pos] == '\n' || jsonText[pos] == '\r')) {
        pos++;
    }
}

std::string SimpleJson::parseString() {
    if (pos >= jsonText.length() || jsonText[pos] != '"') {
        return "";
    }
    
    pos++; // Skip opening quote
    std::string result;
    
    while (pos < jsonText.length() && jsonText[pos] != '"') {
        if (jsonText[pos] == '\\' && pos + 1 < jsonText.length()) {
            pos++; // Skip backslash
            switch (jsonText[pos]) {
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case '\\': result += '\\'; break;
                case '"': result += '"'; break;
                case '/': result += '/'; break;
                case 'u': {
                    ++pos;
                    unsigned int codePoint = 0;
                    if (!readHexCodePoint(jsonText, pos, codePoint)) {
                        return "";
                    }

                    if (codePoint >= 0xD800 && codePoint <= 0xDBFF) {
                        if (pos + 2 > jsonText.size() || jsonText[pos] != '\\' || jsonText[pos + 1] != 'u') {
                            return "";
                        }
                        pos += 2;
                        unsigned int lowSurrogate = 0;
                        if (!readHexCodePoint(jsonText, pos, lowSurrogate)
                            || lowSurrogate < 0xDC00 || lowSurrogate > 0xDFFF) {
                            return "";
                        }
                        codePoint = 0x10000 + ((codePoint - 0xD800) << 10) + (lowSurrogate - 0xDC00);
                    } else if (codePoint >= 0xDC00 && codePoint <= 0xDFFF) {
                        return "";
                    }

                    if (!appendUtf8CodePoint(result, codePoint)) {
                        return "";
                    }
                    --pos;
                    break;
                }
                default: return "";
            }
        } else {
            if (static_cast<unsigned char>(jsonText[pos]) < 0x20) {
                return "";
            }
            result += jsonText[pos];
        }
        pos++;
    }
    
    if (pos < jsonText.length()) {
        pos++; // Skip closing quote
    }
    
    return result;
}

std::string SimpleJson::extractContent() {
    // Look for "choices":[{"message":{"content":"..."}}]
    size_t choicesPos = jsonText.find("\"choices\"");
    if (choicesPos == std::string::npos) {
        return "";
    }
    
    size_t messagePos = jsonText.find("\"message\"", choicesPos);
    if (messagePos == std::string::npos) {
        return "";
    }
    
    size_t contentPos = jsonText.find("\"content\"", messagePos);
    if (contentPos == std::string::npos) {
        return "";
    }
    
    size_t colonPos = jsonText.find(":", contentPos);
    if (colonPos == std::string::npos) {
        return "";
    }
    
    pos = colonPos + 1;
    skipWhitespace();
    
    return parseString();
}

bool SimpleJson::hasError() {
    size_t errorPos = jsonText.find("\"error\"");
    if (errorPos == std::string::npos) {
        return false;
    }

    size_t colonPos = jsonText.find(':', errorPos + 7);
    if (colonPos == std::string::npos) {
        return false;
    }
    size_t valuePos = colonPos + 1;
    while (valuePos < jsonText.size() && (jsonText[valuePos] == ' ' || jsonText[valuePos] == '\t'
        || jsonText[valuePos] == '\n' || jsonText[valuePos] == '\r')) {
        ++valuePos;
    }
    return valuePos < jsonText.size() && jsonText[valuePos] == '{';
}

std::string SimpleJson::getErrorMessage() {
    size_t errorPos = jsonText.find("\"error\"");
    if (errorPos == std::string::npos) {
        return "";
    }
    
    size_t messagePos = jsonText.find("\"message\"", errorPos);
    if (messagePos == std::string::npos) {
        return "Unknown error";
    }
    
    size_t colonPos = jsonText.find(":", messagePos);
    if (colonPos == std::string::npos) {
        return "Unknown error";
    }
    
    pos = colonPos + 1;
    skipWhitespace();
    
    return parseString();
}

// JsonBuilder implementation
JsonBuilder::JsonBuilder() : firstItem(true) {}

void JsonBuilder::startObject() {
    json += "{";
    firstItem = true;
}

void JsonBuilder::endObject() {
    json += "}";
}

void JsonBuilder::startArray(const std::string& key) {
    if (!firstItem) json += ",";
    json += "\"" + key + "\":[";
    firstItem = true;
}

void JsonBuilder::endArray() {
    json += "]";
    firstItem = false;
}

void JsonBuilder::addString(const std::string& key, const std::string& value) {
    if (!firstItem) json += ",";
    
    // Escape special characters in value
    std::string escapedValue;
    for (char c : value) {
        switch (c) {
            case '"': escapedValue += "\\\""; break;
            case '\\': escapedValue += "\\\\"; break;
            case '\b': escapedValue += "\\b"; break;
            case '\f': escapedValue += "\\f"; break;
            case '\n': escapedValue += "\\n"; break;
            case '\r': escapedValue += "\\r"; break;
            case '\t': escapedValue += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    std::ostringstream escape;
                    escape << "\\u" << std::hex << std::uppercase << std::setw(4)
                           << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
                    escapedValue += escape.str();
                } else {
                    escapedValue += c;
                }
                break;
        }
    }
    
    json += "\"" + key + "\":\"" + escapedValue + "\"";
    firstItem = false;
}

void JsonBuilder::addNumber(const std::string& key, int value) {
    if (!firstItem) json += ",";
    json += "\"" + key + "\":" + std::to_string(value);
    firstItem = false;
}

void JsonBuilder::addNumber(const std::string& key, double value) {
    if (!firstItem) json += ",";
    json += "\"" + key + "\":" + std::to_string(value);
    firstItem = false;
}

void JsonBuilder::addObjectToArray() {
    if (!firstItem) json += ",";
    json += "{";
    firstItem = true;
}

void JsonBuilder::endObjectInArray() {
    json += "}";
    firstItem = false;
}

std::string JsonBuilder::toString() const {
    return json;
}
