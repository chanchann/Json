#ifndef JSON_H
#define JSON_H

#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <utility>
#include <initializer_list>

// Include the C library headers
extern "C" {
#include "json_parser.h"
#include "list.h"
}

class Json;

// Forward declaration
class JsonProxy;

// Helper to convert any value to a Json object
template<typename T>
Json toJson(T value);


/**
 * @class Json
 * @brief A C++ wrapper for the C JSON library, providing a modern interface.
 */
class Json {
private:
    json_value_t* m_value;

    // Private constructor for internal use (e.g., from proxy)
    // It takes ownership of the pointer.
    explicit Json(json_value_t* value) : m_value(value) {}

    void swap(Json& other) noexcept {
        std::swap(m_value, other.m_value);
    }

public:
    // *******************************************************************
    // Constructors and Destructor (RAII)
    // *******************************************************************

    // Default constructor: creates a null value
    Json() : m_value(json_value_create(JSON_VALUE_NULL, NULL)) {}

    // Constructor for nullptr
    Json(std::nullptr_t) : Json() {}

    // Constructor for C-style strings
    Json(const char* value) : m_value(json_value_create(JSON_VALUE_STRING, value)) {}

    // Constructor for std::string
    Json(const std::string& value) : m_value(json_value_create(JSON_VALUE_STRING, value.c_str())) {}

    // Constructor for double
    Json(double value) : m_value(json_value_create(JSON_VALUE_NUMBER, value)) {}

    // Constructor for int
    Json(int value) : m_value(json_value_create(JSON_VALUE_NUMBER, static_cast<double>(value))) {}
    
    // Constructor for long
    Json(long value) : m_value(json_value_create(JSON_VALUE_NUMBER, static_cast<double>(value))) {}

    // Constructor for boolean
    Json(bool value) : m_value(json_value_create(value ? JSON_VALUE_TRUE : JSON_VALUE_FALSE, NULL)) {}

    // Copy constructor
    Json(const Json& other) : m_value(json_value_copy(other.m_value)) {}

    // Move constructor
    Json(Json&& other) noexcept : m_value(other.m_value) {
        other.m_value = nullptr;
    }

    // Destructor
    ~Json() {
        if (m_value) {
            json_value_destroy(m_value);
        }
    }

    // *******************************************************************
    // Assignment Operators
    // *******************************************************************

    // Copy assignment
    Json& operator=(const Json& other) {
        if (this != &other) {
            Json temp(other);
            swap(temp);
        }
        return *this;
    }

    // Move assignment
    Json& operator=(Json&& other) noexcept {
        if (this != &other) {
            swap(other);
            // Destroy the old value that was swapped into 'other'
            json_value_destroy(other.m_value);
            other.m_value = nullptr;
        }
        return *this;
    }

    // *******************************************************************
    // Static Factory Methods
    // *******************************************************************

    static Json object() {
        return Json(json_value_create(JSON_VALUE_OBJECT, NULL));
    }

    static Json array() {
        return Json(json_value_create(JSON_VALUE_ARRAY, NULL));
    }

    static Json parse(const std::string& json_string) {
        json_value_t* val = json_value_parse(json_string.c_str());
        if (!val) {
            throw std::runtime_error("Failed to parse JSON string");
        }
        return Json(val);
    }

    // *******************************************************************
    // Type Checking
    // *******************************************************************

    int type() const { return m_value ? json_value_type(m_value) : JSON_VALUE_NULL; }
    bool is_null() const { return type() == JSON_VALUE_NULL; }
    bool is_string() const { return type() == JSON_VALUE_STRING; }
    bool is_number() const { return type() == JSON_VALUE_NUMBER; }
    bool is_boolean() const { return type() == JSON_VALUE_TRUE || type() == JSON_VALUE_FALSE; }
    bool is_object() const { return type() == JSON_VALUE_OBJECT; }
    bool is_array() const { return type() == JSON_VALUE_ARRAY; }

    // *******************************************************************
    // Value Accessors (get<T>)
    // *******************************************************************

    template <typename T>
    T get() const;

    // *******************************************************************
    // Nested Access (operator[])
    // *******************************************************************
    
    JsonProxy operator[](const char* key);
    JsonProxy operator[](const std::string& key);
    JsonProxy operator[](size_t index);
    JsonProxy operator[](int index);

    // *******************************************************************
    // Modification & Chaining
    // *******************************************************************

    template<typename T>
    Json& set(const std::string& key, T value) {
        if (!is_object()) {
            if (is_null()) {
                // Auto-convert null to object
                json_value_destroy(m_value);
                m_value = json_value_create(JSON_VALUE_OBJECT, NULL);
            } else {
                throw std::runtime_error("Cannot set key on non-object type");
            }
        }
        
        Json j_value(value);
        json_object_t* obj = json_value_object(m_value);
        const json_value_t* existing = json_object_find(key.c_str(), obj);
        
        if (existing) {
            json_object_remove(existing, obj);
        }
        
        // Use the C API to add the value based on its type
        switch(j_value.type()) {
            case JSON_VALUE_STRING:
                json_object_append(obj, key.c_str(), JSON_VALUE_STRING, json_value_string(j_value.get_c_value()));
                break;
            case JSON_VALUE_NUMBER:
                json_object_append(obj, key.c_str(), JSON_VALUE_NUMBER, json_value_number(j_value.get_c_value()));
                break;
            case JSON_VALUE_TRUE:
                json_object_append(obj, key.c_str(), JSON_VALUE_TRUE);
                break;
            case JSON_VALUE_FALSE:
                json_object_append(obj, key.c_str(), JSON_VALUE_FALSE);
                break;
            case JSON_VALUE_NULL:
                json_object_append(obj, key.c_str(), JSON_VALUE_NULL);
                break;
            case JSON_VALUE_OBJECT:
            case JSON_VALUE_ARRAY:
                {
                    const json_value_t* new_val = json_object_append(obj, key.c_str(), j_value.type());
                    // Deep copy contents from j_value into new_val
                    if (j_value.is_array()) {
                        json_array_t* src_arr = json_value_array(j_value.get_c_value());
                        json_array_t* dest_arr = json_value_array(new_val);
                        const json_value_t* src_elem = NULL;
                        json_array_for_each(src_elem, src_arr) {
                            // Append deep copy of src_elem
                            append_deep_copy_to_array(dest_arr, src_elem);
                        }
                    } else {
                        json_object_t* src_obj = json_value_object(j_value.get_c_value());
                        json_object_t* dest_obj = json_value_object(new_val);
                        const char* name = NULL;
                        const json_value_t* src_val = NULL;
                        json_object_for_each(name, src_val, src_obj) {
                            // Append deep copy of src_val under name
                            append_deep_copy_to_object(dest_obj, name, src_val);
                        }
                    }
                }
                break;
        }

        return *this;
    }

    template<typename T>
    Json& push_back(T value) {
        if (!is_array()) {
             if (is_null()) {
                // Auto-convert null to array
                json_value_destroy(m_value);
                m_value = json_value_create(JSON_VALUE_ARRAY, NULL);
            } else {
                throw std::runtime_error("Cannot push_back on non-array type");
            }
        }
        Json j_value(value);
        json_array_t* arr = json_value_array(m_value);
        
        // Use the C API to add the value based on its type
        switch(j_value.type()) {
            case JSON_VALUE_STRING:
                json_array_append(arr, JSON_VALUE_STRING, json_value_string(j_value.get_c_value()));
                break;
            case JSON_VALUE_NUMBER:
                json_array_append(arr, JSON_VALUE_NUMBER, json_value_number(j_value.get_c_value()));
                break;
            case JSON_VALUE_TRUE:
                json_array_append(arr, JSON_VALUE_TRUE);
                break;
            case JSON_VALUE_FALSE:
                json_array_append(arr, JSON_VALUE_FALSE);
                break;
            case JSON_VALUE_NULL:
                json_array_append(arr, JSON_VALUE_NULL);
                break;
            case JSON_VALUE_OBJECT:
            case JSON_VALUE_ARRAY:
                // For complex types, create empty and then deep-copy structure
                {
                    const json_value_t* new_val = json_array_append(arr, j_value.type());
                    if (j_value.is_array()) {
                        json_array_t* src_arr = json_value_array(j_value.get_c_value());
                        json_array_t* dest_arr = json_value_array(new_val);
                        const json_value_t* src_elem = NULL;
                        json_array_for_each(src_elem, src_arr) {
                            append_deep_copy_to_array(dest_arr, src_elem);
                        }
                    } else {
                        json_object_t* src_obj = json_value_object(j_value.get_c_value());
                        json_object_t* dest_obj = json_value_object(new_val);
                        const char* name = NULL;
                        const json_value_t* src_val = NULL;
                        json_object_for_each(name, src_val, src_obj) {
                            append_deep_copy_to_object(dest_obj, name, src_val);
                        }
                    }
                }
                break;
        }
        
        return *this;
    }

    Json& erase(const std::string& key) {
        if (!is_object()) return *this;
        json_object_t* obj = json_value_object(m_value);
        const json_value_t* val_to_remove = json_object_find(key.c_str(), obj);
        if (val_to_remove) {
            json_object_remove(val_to_remove, obj);
        }
        return *this;
    }

    Json& erase(size_t index) {
        if (!is_array()) return *this;
        json_array_t* arr = json_value_array(m_value);
        if (index >= json_array_size(arr)) return *this;
        
        const json_value_t* target = NULL;
        const json_value_t* it = NULL;
        size_t current_idx = 0;
        json_array_for_each(it, arr) {
            if (current_idx == index) { target = it; break; }
            ++current_idx;
        }
        if (target) {
            json_array_remove(target, arr);
        }
        return *this;
    }

    // *******************************************************************
    // Serialization
    // *******************************************************************

    std::string dump() const {
        std::stringstream ss;
        dump_recursive(ss, m_value);
        return ss.str();
    }

    // *******************************************************************
    // Internal Helpers
    // *******************************************************************
    

    json_value_t* get_c_value() { return m_value; }
    const json_value_t* get_c_value() const { return m_value; }


private:
    // Append a deep copy of src_val as a new element to dest_arr
    void append_deep_copy_to_array(json_array_t* dest_arr, const json_value_t* src_val) const {
        switch (json_value_type(src_val)) {
            case JSON_VALUE_STRING:
                json_array_append(dest_arr, JSON_VALUE_STRING, json_value_string(src_val));
                break;
            case JSON_VALUE_NUMBER:
                json_array_append(dest_arr, JSON_VALUE_NUMBER, json_value_number(src_val));
                break;
            case JSON_VALUE_TRUE:
                json_array_append(dest_arr, JSON_VALUE_TRUE);
                break;
            case JSON_VALUE_FALSE:
                json_array_append(dest_arr, JSON_VALUE_FALSE);
                break;
            case JSON_VALUE_NULL:
                json_array_append(dest_arr, JSON_VALUE_NULL);
                break;
            case JSON_VALUE_ARRAY: {
                const json_value_t* new_child = json_array_append(dest_arr, JSON_VALUE_ARRAY);
                json_array_t* child_arr = json_value_array(new_child);
                const json_value_t* elem = NULL;
                const json_array_t* src_arr = json_value_array(src_val);
                json_array_for_each(elem, src_arr) {
                    append_deep_copy_to_array(child_arr, elem);
                }
                break;
            }
            case JSON_VALUE_OBJECT: {
                const json_value_t* new_child = json_array_append(dest_arr, JSON_VALUE_OBJECT);
                json_object_t* child_obj = json_value_object(new_child);
                const char* name = NULL;
                const json_value_t* v = NULL;
                const json_object_t* src_obj = json_value_object(src_val);
                json_object_for_each(name, v, src_obj) {
                    append_deep_copy_to_object(child_obj, name, v);
                }
                break;
            }
        }
    }

    // Append a deep copy of src_val into dest_obj under key
    void append_deep_copy_to_object(json_object_t* dest_obj, const char* key, const json_value_t* src_val) const {
        switch (json_value_type(src_val)) {
            case JSON_VALUE_STRING:
                json_object_append(dest_obj, key, JSON_VALUE_STRING, json_value_string(src_val));
                break;
            case JSON_VALUE_NUMBER:
                json_object_append(dest_obj, key, JSON_VALUE_NUMBER, json_value_number(src_val));
                break;
            case JSON_VALUE_TRUE:
                json_object_append(dest_obj, key, JSON_VALUE_TRUE);
                break;
            case JSON_VALUE_FALSE:
                json_object_append(dest_obj, key, JSON_VALUE_FALSE);
                break;
            case JSON_VALUE_NULL:
                json_object_append(dest_obj, key, JSON_VALUE_NULL);
                break;
            case JSON_VALUE_ARRAY: {
                const json_value_t* new_child = json_object_append(dest_obj, key, JSON_VALUE_ARRAY);
                json_array_t* child_arr = json_value_array(new_child);
                const json_value_t* elem = NULL;
                const json_array_t* src_arr = json_value_array(src_val);
                json_array_for_each(elem, src_arr) {
                    append_deep_copy_to_array(child_arr, elem);
                }
                break;
            }
            case JSON_VALUE_OBJECT: {
                const json_value_t* new_child = json_object_append(dest_obj, key, JSON_VALUE_OBJECT);
                json_object_t* child_obj = json_value_object(new_child);
                const char* name = NULL;
                const json_value_t* v = NULL;
                const json_object_t* src_obj = json_value_object(src_val);
                json_object_for_each(name, v, src_obj) {
                    append_deep_copy_to_object(child_obj, name, v);
                }
                break;
            }
        }
    }

    // Append a deep copy of a Json wrapper value to a C array
    void append_deep_copy_json_to_array(json_array_t* dest_arr, const Json& elem) const {
        switch (elem.type()) {
            case JSON_VALUE_STRING:
                json_array_append(dest_arr, JSON_VALUE_STRING, json_value_string(elem.get_c_value()));
                break;
            case JSON_VALUE_NUMBER:
                json_array_append(dest_arr, JSON_VALUE_NUMBER, json_value_number(elem.get_c_value()));
                break;
            case JSON_VALUE_TRUE:
                json_array_append(dest_arr, JSON_VALUE_TRUE);
                break;
            case JSON_VALUE_FALSE:
                json_array_append(dest_arr, JSON_VALUE_FALSE);
                break;
            case JSON_VALUE_NULL:
                json_array_append(dest_arr, JSON_VALUE_NULL);
                break;
            case JSON_VALUE_OBJECT: {
                const json_value_t* new_val = json_array_append(dest_arr, JSON_VALUE_OBJECT);
                json_object_t* dest_obj = json_value_object(new_val);
                const char* name = NULL;
                const json_value_t* src_val = NULL;
                json_object_t* src_obj = json_value_object(elem.get_c_value());
                json_object_for_each(name, src_val, src_obj) {
                    append_deep_copy_to_object(dest_obj, name, src_val);
                }
                break;
            }
            case JSON_VALUE_ARRAY: {
                const json_value_t* new_val = json_array_append(dest_arr, JSON_VALUE_ARRAY);
                json_array_t* dest_child_arr = json_value_array(new_val);
                const json_value_t* src_elem = NULL;
                json_array_t* src_arr = json_value_array(elem.get_c_value());
                json_array_for_each(src_elem, src_arr) {
                    append_deep_copy_to_array(dest_child_arr, src_elem);
                }
                break;
            }
        }
    }

    static void append_escaped_string(std::stringstream& ss, const char* s) {
        if (!s) {
            return;
        }
        for (const unsigned char c : std::string(s)) {
            switch (c) {
                case '"': ss << "\\\""; break;
                case '\\': ss << "\\\\"; break;
                case '\b': ss << "\\b"; break;
                case '\f': ss << "\\f"; break;
                case '\n': ss << "\\n"; break;
                case '\r': ss << "\\r"; break;
                case '\t': ss << "\\t"; break;
                default:
                    if (c < 0x20) {
                        // Control characters as \u00XX
                        const char hex[] = "0123456789abcdef";
                        ss << "\\u00" << hex[(c >> 4) & 0xF] << hex[c & 0xF];
                    } else {
                        ss << c;
                    }
            }
        }
    }

    void dump_recursive(std::stringstream& ss, const json_value_t* val) const {
        if (!val) {
            ss << "null";
            return;
        }
        switch (json_value_type(val)) {
            case JSON_VALUE_NULL:
                ss << "null";
                break;
            case JSON_VALUE_FALSE:
                ss << "false";
                break;
            case JSON_VALUE_TRUE:
                ss << "true";
                break;
            case JSON_VALUE_NUMBER:
                ss << json_value_number(val);
                break;
            case JSON_VALUE_STRING:
                ss << "\""; append_escaped_string(ss, json_value_string(val)); ss << "\"";
                break;
            case JSON_VALUE_ARRAY: {
                ss << "[";
                const json_array_t* arr = json_value_array(val);
                const json_value_t* elem = NULL;
                bool first = true;
                json_array_for_each(elem, arr) {
                    if (!first) ss << ",";
                    dump_recursive(ss, elem);
                    first = false;
                }
                ss << "]";
                break;
            }
            case JSON_VALUE_OBJECT: {
                ss << "{";
                const json_object_t* obj = json_value_object(val);
                const char* name = NULL;
                const json_value_t* value = NULL;
                bool first = true;
                json_object_for_each(name, value, obj) {
                    if (!first) ss << ",";
                    ss << "\""; append_escaped_string(ss, name); ss << "\":";
                    dump_recursive(ss, value);
                    first = false;
                }
                ss << "}";
                break;
            }
        }
    }

    friend class JsonProxy;
};

// *******************************************************************
// Template specializations for get<T>
// *******************************************************************

template<> inline std::string Json::get<std::string>() const {
    if (!is_string()) throw std::bad_cast();
    return json_value_string(m_value);
}

template<> inline double Json::get<double>() const {
    if (!is_number()) throw std::bad_cast();
    return json_value_number(m_value);
}

template<> inline int Json::get<int>() const {
    if (!is_number()) throw std::bad_cast();
    return static_cast<int>(json_value_number(m_value));
}

template<> inline long Json::get<long>() const {
    if (!is_number()) throw std::bad_cast();
    return static_cast<long>(json_value_number(m_value));
}

template<> inline bool Json::get<bool>() const {
    if (!is_boolean()) throw std::bad_cast();
    return type() == JSON_VALUE_TRUE;
}


/**
 * @class JsonProxy
 * @brief A proxy class to enable chained operator[] access.
 */
class JsonProxy {
private:
    struct PathSegment {
        bool is_index;
        std::string key;
        size_t index;
    };

    Json& m_parent;
    std::vector<PathSegment> m_path;

public:
    JsonProxy(Json& parent, const char* key) : m_parent(parent) {
        PathSegment seg{false, std::string(key), 0};
        m_path.push_back(std::move(seg));
    }
    JsonProxy(Json& parent, size_t index) : m_parent(parent) {
        PathSegment seg{true, std::string(), index};
        m_path.push_back(std::move(seg));
    }

    // Assignment for writing: obj["key1"]["key2"] = value;
    template<typename T>
    JsonProxy& operator=(T value) {
        Json j_value(value);
        set_value_at_path(j_value);
        return *this;
    }
    
    // Chained access: obj["key1"]["key2"]
    JsonProxy operator[](const char* key) {
        PathSegment seg{false, std::string(key), 0};
        JsonProxy next(m_parent, "");
        next.m_path = m_path;
        next.m_path.push_back(std::move(seg));
        return next;
    }

    JsonProxy operator[](size_t index) {
        PathSegment seg{true, std::string(), index};
        JsonProxy next(m_parent, "");
        next.m_path = m_path;
        next.m_path.push_back(std::move(seg));
        return next;
    }
    
    JsonProxy operator[](int index) {
        return (*this)[static_cast<size_t>(index)];
    }

    // Implicit conversion for reading: std::string s = obj["key"];
    operator Json() const {
        return get_value_at_path_copy();
    }
    
    template <typename T>
    T get() const {
        return static_cast<Json>(*this).template get<T>();
    }
    
    // Type checking methods for JsonProxy
    bool is_null() const { return static_cast<Json>(*this).is_null(); }
    bool is_string() const { return static_cast<Json>(*this).is_string(); }
    bool is_number() const { return static_cast<Json>(*this).is_number(); }
    bool is_boolean() const { return static_cast<Json>(*this).is_boolean(); }
    bool is_object() const { return static_cast<Json>(*this).is_object(); }
    bool is_array() const { return static_cast<Json>(*this).is_array(); }

private:
    // Traverse m_parent by m_path and return a deep copy of the value if found; otherwise null Json
    Json get_value_at_path_copy() const {
        const json_value_t* cur = m_parent.get_c_value();
        for (const PathSegment& seg : m_path) {
            if (!cur) return Json();
            if (seg.is_index) {
                if (json_value_type(cur) != JSON_VALUE_ARRAY) return Json();
                const json_array_t* arr = json_value_array(cur);
                const json_value_t* val = NULL;
                size_t idx = 0;
                bool found = false;
                json_array_for_each(val, arr) {
                    if (idx == seg.index) { cur = val; found = true; break; }
                    ++idx;
                }
                if (!found) return Json();
            } else {
                if (json_value_type(cur) != JSON_VALUE_OBJECT) return Json();
                const json_object_t* obj = json_value_object(cur);
                const json_value_t* val = json_object_find(seg.key.c_str(), obj);
                if (!val) return Json();
                cur = val;
            }
        }
        return cur ? Json(json_value_copy(cur)) : Json();
    }

    // Recursively set value into m_parent at m_path
    void set_value_at_path(const Json& newValue) {
        set_value_rec(m_parent, 0, newValue);
    }

    void set_value_rec(Json& current, size_t depth, const Json& newValue) {
        if (depth >= m_path.size()) {
            // Replace current entirely
            current = newValue;
            return;
        }
        const PathSegment& seg = m_path[depth];
        const bool is_last = (depth + 1 == m_path.size());
        if (is_last) {
            if (seg.is_index) {
                set_array_index(current, seg.index, newValue);
            } else {
                current.set(seg.key, newValue);
            }
            return;
        }
        // Not last: ensure intermediate container and recurse
        const PathSegment& nextSeg = m_path[depth + 1];
        Json child = get_immediate_child_copy(current, seg);
        // Ensure correct container type
        if (nextSeg.is_index) {
            if (!child.is_array()) {
                child = Json::array();
            }
        } else {
            if (!child.is_object()) {
                child = Json::object();
            }
        }
        set_value_rec(child, depth + 1, newValue);
        if (seg.is_index) {
            set_array_index(current, seg.index, child);
        } else {
            current.set(seg.key, child);
        }
    }

    Json get_immediate_child_copy(Json& current, const PathSegment& seg) const {
        if (seg.is_index) {
            // Read array element as copy; if out of bounds or not array -> null
            const json_value_t* cur = current.get_c_value();
            if (!cur || json_value_type(cur) != JSON_VALUE_ARRAY) return Json();
            const json_array_t* arr = json_value_array(cur);
            const json_value_t* val = NULL;
            size_t idx = 0;
            json_array_for_each(val, arr) {
                if (idx == seg.index) {
                    return Json(json_value_copy(val));
                }
                ++idx;
            }
            return Json();
        } else {
            const json_value_t* cur = current.get_c_value();
            if (!cur || json_value_type(cur) != JSON_VALUE_OBJECT) return Json();
            const json_object_t* obj = json_value_object(cur);
            const json_value_t* val = json_object_find(seg.key.c_str(), obj);
            if (!val) return Json();
            return Json(json_value_copy(val));
        }
    }

    void set_array_index(Json& arrayOwner, size_t targetIndex, const Json& newElem) {
        if (!arrayOwner.is_array()) {
            if (arrayOwner.is_null()) {
                arrayOwner = Json::array();
            } else {
                throw std::runtime_error("Cannot index-assign on non-array type");
            }
        }
        json_array_t* arr = json_value_array(arrayOwner.get_c_value());

        size_t current_size = json_array_size(arr);
        if (targetIndex >= current_size) {
            while (json_array_size(arr) < targetIndex) {
                json_array_append(arr, JSON_VALUE_NULL);
            }
            m_parent.append_deep_copy_json_to_array(arr, newElem);
            return;
        }

        // In-bounds replacement: rebuild only the tail
        std::vector<const json_value_t*> element_ptrs;
        element_ptrs.reserve(current_size);
        const json_value_t* it = NULL;
        json_array_for_each(it, arr) {
            element_ptrs.push_back(it);
        }

        // Deep copy the tail elements (after targetIndex)
        std::vector<Json> tail_copies;
        tail_copies.reserve(current_size - targetIndex - 1);
        for (size_t i = targetIndex + 1; i < element_ptrs.size(); ++i) {
            tail_copies.emplace_back(Json(json_value_copy(element_ptrs[i])));
        }

        // Remove the tail from end to beginning
        for (size_t i = element_ptrs.size(); i-- > targetIndex + 1; ) {
            json_array_remove(element_ptrs[i], arr);
        }
        // Remove the target element
        json_array_remove(element_ptrs[targetIndex], arr);

        // Append the new element
        m_parent.append_deep_copy_json_to_array(arr, newElem);

        // Re-append the saved tail elements in original order
        for (const auto& elem_json : tail_copies) {
            m_parent.append_deep_copy_json_to_array(arr, elem_json);
        }
    }
};


// *******************************************************************
// Json method implementations that depend on JsonProxy
// *******************************************************************

inline JsonProxy Json::operator[](const char* key) {
    if (is_null()) {
        // Auto-vivify: if we are null, become an object
        json_value_destroy(m_value);
        m_value = json_value_create(JSON_VALUE_OBJECT, NULL);
    }
    if (!is_object()) throw std::runtime_error("operator[] called on non-object type");
    
    // Auto-vivify the key if it doesn't exist
    json_object_t* obj = json_value_object(m_value);
    const json_value_t* existing = json_object_find(key, obj);
    if (!existing) {
        // Create a null value at this key, which can be auto-vivified later
        json_object_append(obj, key, JSON_VALUE_NULL);
    }
    
    return JsonProxy(*this, key);
}

inline JsonProxy Json::operator[](const std::string& key) {
    return (*this)[key.c_str()];
}

inline JsonProxy Json::operator[](size_t index) {
    if (is_null()) {
        // Auto-vivify: if we are null, become an array
        json_value_destroy(m_value);
        m_value = json_value_create(JSON_VALUE_ARRAY, NULL);
    }
    if (!is_array()) throw std::runtime_error("operator[] called on non-array type");
    return JsonProxy(*this, index);
}

inline JsonProxy Json::operator[](int index) {
    return (*this)[static_cast<size_t>(index)];
}


#endif // JSON_H
