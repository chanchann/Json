#ifndef WFREST_JSON_H_
#define WFREST_JSON_H_

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <functional>
#include <map>
#include <cassert>
#include "json_parser.h"
#include "JsonValue.h"

namespace wfrest
{

class Json
{
public: 
    // Constructors for the various types of JSON value.
    Json() : val_(nullptr) {}
    Json(const std::string& str) : val_(str) {}
    Json(const char* str) : val_(str) {}
    Json(std::nullptr_t null) : val_(null) {}
    Json(double val) : val_(val) {}
    Json(int val) : val_(val) {}
    Json(bool val) : val_(val) {}
    ~Json() = default;

    Json(const Json& json) = delete;
    Json& operator=(const Json& json) = delete;
    Json(Json&& other);
    Json& operator=(Json&& other);

    static Json parse(const std::string &str);
    static Json parse(const std::ifstream& stream);
    
    const std::string dump() const;
    const std::string dump(int spaces) const;

public:
    int type() const
    {
        return val_.type();
    }

    bool is_null() const
    {
        return type() == JSON_VALUE_NULL;
    }

    bool is_number() const
    {
        return type() == JSON_VALUE_NUMBER;
    }

    bool is_boolean() const
    {
        int type = this->type();
        return type == JSON_VALUE_TRUE || type == JSON_VALUE_FALSE;
    }

    bool is_object() const
    {
        return type() == JSON_VALUE_OBJECT;
    }

    bool is_array() const
    {
        return type() == JSON_VALUE_ARRAY;
    }

    bool is_string() const
    {
        return type() == JSON_VALUE_STRING;
    }

    int size() const;

    bool empty() const
    {
        return val_.empty();
    }

    void clear()
    {
        val_.to_object();
    }

public:
    // object
    Json& operator[](const std::string& key);
    
    template <typename T>
    Json& operator=(const T& val)
    {
        Json* json = this->parent_;
        assert(json->type() == JSON_VALUE_OBJECT);
        json->push_back(json->key_, val);
        return *this;
    }

    template <typename T>
    void push_back(const std::string& key, const T& val)
    {
        val_.push_back(key, val);
    }

    template <typename T>
    void push_back(const T& val)
    {
        val_.push_back(val);
    }

    // todo : template<typename T>
    bool has(const std::string& key) const
    {
        if (!is_object())
        {
            return false;
        }
        const auto it = object_.find(key);
        if(it != object_.end())
        {
            return true;
        }
        json_object_t* obj = json_value_object(val_.json());
        const json_value_t* val = json_object_find(key.c_str(), obj);
        return val == nullptr ? false : true;
    }
    
private:
    Json create_incomplete_json();

    friend inline std::ostream& operator << (std::ostream& os, const Json& json) { return (os << json.dump()); }
private:
    Json(JsonValue &&val) : val_(std::move(val)) {}

    Json(const json_value_t* val) : val_(val) {}

private:
    std::map<std::string, Json> object_;
    std::string key_;
    Json* parent_ = nullptr;  // watcher
    JsonValue val_;
};

}  // namespace wfrest

#endif  // WFREST_JSON_H_
