#ifndef SIMPLEJSON_H
#define SIMPLEJSON_H

#include <string>
#include <map>
#include <vector>

// Simple JSON parser for basic OpenAI API response parsing
class SimpleJson {
private:
    std::string jsonText;
    size_t pos;
    
    void skipWhitespace();
    std::string parseString();
    std::string parseValue();
    
public:
    SimpleJson(const std::string& json);
    
    // Extract content from OpenAI API response
    std::string extractContent();
    
    // Check if response contains error
    bool hasError();
    std::string getErrorMessage();
};

// JSON builder for creating request payloads
class JsonBuilder {
private:
    std::string json;
    bool firstItem;
    
public:
    JsonBuilder();
    
    void startObject();
    void endObject();
    void startArray(const std::string& key);
    void endArray();
    void addString(const std::string& key, const std::string& value);
    void addNumber(const std::string& key, int value);
    void addNumber(const std::string& key, double value);
    void addObjectToArray();
    void endObjectInArray();
    
    std::string toString() const;
};

#endif // SIMPLEJSON_H
