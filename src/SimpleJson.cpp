#include "SimpleJson.h"
#include <sstream>
#include <algorithm>

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
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case '\\': result += '\\'; break;
                case '"': result += '"'; break;
                default: result += jsonText[pos]; break;
            }
        } else {
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
    return jsonText.find("\"error\"") != std::string::npos;
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
            case '\n': escapedValue += "\\n"; break;
            case '\r': escapedValue += "\\r"; break;
            case '\t': escapedValue += "\\t"; break;
            default: escapedValue += c; break;
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
