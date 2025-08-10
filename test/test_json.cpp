#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <cmath> // For std::abs
#include <utility>

#include "../Json.h"

// A simple testing utility
void test(const std::string& test_name, void (*test_func)()) {
    try {
        test_func();
        std::cout << "[PASS] " << test_name << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[FAIL] " << test_name << " - Exception: " << e.what() << std::endl;
        exit(1);
    } catch (...) {
        std::cerr << "[FAIL] " << test_name << " - Unknown exception" << std::endl;
        exit(1);
    }
}

// Test functions declarations
void test_construction();
void test_type_checking();
void test_getters();
void test_parsing();
void test_object_manipulation();
void test_array_manipulation();
void test_serialization_dump();
void test_chained_operations();
void test_auto_vivification();
void test_edge_cases();
void test_nested_chaining_and_autoviv();
void test_deep_copy_semantics();
void test_dump_escaping();
void test_move_semantics();
void test_missing_reads();
void test_proxy_conversion_copy();
void test_nonconst_operator_no_autovivify();

int main() {
    std::cout << "Running C++ JSON Wrapper Tests..." << std::endl;

    test("Construction", test_construction);
    test("Type Checking", test_type_checking);
    test("Value Getters", test_getters);
    test("JSON Parsing", test_parsing);
    test("Object Manipulation", test_object_manipulation);
    test("Array Manipulation", test_array_manipulation);
    test("Serialization (dump)", test_serialization_dump);
    test("Chained Operations", test_chained_operations);
    test("Auto-Vivification (on-the-fly creation)", test_auto_vivification);
    test("Nested Chaining and Auto-Vivification", test_nested_chaining_and_autoviv);
    test("Deep-Copy Semantics", test_deep_copy_semantics);
    test("Dump Escaping", test_dump_escaping);
    test("Move Semantics", test_move_semantics);
    test("Missing Reads Behavior", test_missing_reads);
    test("Proxy Conversion Yields Copy", test_proxy_conversion_copy);
    test("Non-const operator[] no auto-vivify on read-only", test_nonconst_operator_no_autovivify);
    test("Edge Cases", test_edge_cases);

    std::cout << "\nAll tests passed successfully!" << std::endl;

    return 0;
}

// Test function implementations

void test_construction() {
    Json j_null;
    assert(j_null.is_null());

    Json j_nullptr = nullptr;
    assert(j_nullptr.is_null());

    Json j_string("hello");
    assert(j_string.is_string());
    assert(j_string.get<std::string>() == "hello");

    Json j_std_string(std::string("world"));
    assert(j_std_string.is_string());
    assert(j_std_string.get<std::string>() == "world");

    Json j_int(42);
    assert(j_int.is_number());
    assert(j_int.get<int>() == 42);

    Json j_double(3.14);
    assert(j_double.is_number());
    assert(std::abs(j_double.get<double>() - 3.14) < 1e-9);

    Json j_true(true);
    assert(j_true.is_boolean());
    assert(j_true.get<bool>() == true);

    Json j_false(false);
    assert(j_false.is_boolean());
    assert(j_false.get<bool>() == false);

    Json j_obj = Json::object();
    assert(j_obj.is_object());

    Json j_arr = Json::array();
    assert(j_arr.is_array());
}

void test_type_checking() {
    assert(Json().is_null());
    assert(Json("text").is_string());
    assert(Json(123).is_number());
    assert(Json(true).is_boolean());
    assert(Json(false).is_boolean());
    assert(Json::object().is_object());
    assert(Json::array().is_array());

    Json j = 123;
    assert(!j.is_string());
}

void test_getters() {
    Json j_str("test");
    assert(j_str.get<std::string>() == "test");

    Json j_num(123.45);
    assert(std::abs(j_num.get<double>() - 123.45) < 1e-9);
    assert(j_num.get<int>() == 123);
    assert(j_num.get<long>() == 123L);

    Json j_bool(true);
    assert(j_bool.get<bool>() == true);

    bool thrown = false;
    try {
        Json j_num_err(123);
        j_num_err.get<std::string>();
    } catch (const std::bad_cast&) {
        thrown = true;
    }
    assert(thrown);
}

void test_parsing() {
    std::string valid_json = R"({"name":"John","age":30,"city":"New York"})";
    Json j = Json::parse(valid_json);
    assert(j.is_object());
    assert(j["name"].get<std::string>() == "John");
    assert(j["age"].get<int>() == 30);

    bool thrown = false;
    try {
        Json::parse("{'invalid': 'json'}"); // single quotes are invalid
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    assert(thrown);
}

void test_object_manipulation() {
    Json j = Json::object();
    j["key1"] = "value1";
    assert(j["key1"].get<std::string>() == "value1");

    j["key1"] = "value1_modified";
    assert(j["key1"].get<std::string>() == "value1_modified");

    j["key2"] = 123;
    assert(j["key2"].get<int>() == 123);

    j.erase("key1");
    assert(j["key1"].is_null());
}

void test_array_manipulation() {
    Json arr = Json::array();
    arr.push_back(10);
    arr.push_back("twenty");
    arr.push_back(true);

    assert(arr[0].get<int>() == 10);
    assert(arr[1].get<std::string>() == "twenty");
    assert(arr[2].get<bool>() == true);

    arr[0] = 100;
    assert(arr[0].get<int>() == 100);
    
    arr.erase(1); // Erase "twenty"
    assert(arr[1].get<bool>() == true);
}

void test_serialization_dump() {
    Json j = Json::object();
    j["a"] = 1;
    j["b"] = "two";
    Json c_array = Json::array();
    c_array.push_back(nullptr);
    c_array.push_back(false);
    j["c"] = c_array;
    
    // Note: C library doesn't guarantee key order, so we check for content
    std::string dumped = j.dump();
    assert(dumped.find("\"a\":1") != std::string::npos);
    assert(dumped.find("\"b\":\"two\"") != std::string::npos);
    assert(dumped.find("\"c\":[null,false]") != std::string::npos);
    
    Json arr = Json::array().push_back(1).push_back(2);
    assert(arr.dump() == "[1,2]");
}

void test_chained_operations() {
    Json obj = Json::object();
    obj.set("name", "Chain")
       .set("version", 1.0)
       .set("active", true);

    assert(obj["name"].get<std::string>() == "Chain");
    assert(obj["version"].get<double>() == 1.0);
    assert(obj["active"].get<bool>() == true);

    obj.erase("version").erase("active");
    assert(obj["version"].is_null());
    assert(obj["active"].is_null());

    Json arr = Json::array();
    arr.push_back(100)
       .push_back(200)
       .push_back(300);
    
    assert(arr[0].get<int>() == 100);
    assert(arr[1].get<int>() == 200);
    assert(arr[2].get<int>() == 300);
}

void test_auto_vivification() {
    Json j; // Starts as null
    
    // Test basic object auto-vivification
    j["name"] = "John"; // This should auto-convert j from null to object
    
    assert(j.is_object());
    assert(j["name"].get<std::string>() == "John");
    
    // Test nested object creation using explicit objects
    Json user_obj = Json::object();
    user_obj["name"] = "Alice";
    user_obj["age"] = 30;
    j["user"] = user_obj;
    
    assert(j["user"].is_object());
    Json retrieved_user = j["user"].operator Json();
    assert(retrieved_user["name"].get<std::string>() == "Alice");
    assert(retrieved_user["age"].get<int>() == 30);
    
    // Test array auto-vivification
    Json arr; // Starts as null
    arr[0] = "first"; // This should auto-convert arr from null to array
    
    assert(arr.is_array());
    assert(arr[0].get<std::string>() == "first");
}

void test_nested_chaining_and_autoviv() {
    Json j; // null
    j["a"]["b"]["c"] = 1;
    assert(j.is_object());
    assert(j["a"]["b"]["c"].get<int>() == 1);

    j["arr"][2][0] = "x";
    assert(j["arr"].is_array());
    assert(j["arr"][2].is_array());
    assert(j["arr"][2][0].get<std::string>() == "x");

    j["mix"][0]["k"] = true;
    assert(j["mix"].is_array());
    assert(j["mix"][0].is_object());
    assert(j["mix"][0]["k"].get<bool>() == true);
}

void test_deep_copy_semantics() {
    // Object assigned into object should be deep-copied
    Json sub = Json::object();
    sub["k"] = "v";
    Json j = Json::object();
    j["sub"] = sub;
    sub["k"] = "changed";
    assert(j["sub"]["k"].get<std::string>() == "v");

    // push_back with nested object is deep copy
    Json arr = Json::array();
    Json elem = Json::object();
    elem["x"] = 1;
    arr.push_back(elem);
    elem["x"] = 2;
    assert(arr[0]["x"].get<int>() == 1);

    // Array element replacement with nested array is deep copy
    Json arr2 = Json::array();
    arr2.push_back(Json::array().push_back(1));
    Json inner = Json::array().push_back(9);
    arr2[0] = inner;
    inner[0] = 42;
    assert(arr2[0][0].get<int>() == 9);
}

void test_dump_escaping() {
    Json j = Json::object();
    j["s"] = std::string("quote: \" backslash: \\ newline:\n tab:\t control:\x01");
    std::string d = j.dump();
    // Check escapes present
    assert(d.find("\\\"") != std::string::npos); // \"
    assert(d.find("\\\\") != std::string::npos); // \\
    assert(d.find("\\n") != std::string::npos);
    assert(d.find("\\t") != std::string::npos);
    assert(d.find("\\u0001") != std::string::npos);
}

void test_move_semantics() {
    Json j = Json::object();
    j["a"] = 1;
    Json m(std::move(j));
    assert(m["a"].get<int>() == 1);
    // moved-from should be safe and treated as null
    assert(j.is_null());

    Json j2 = Json::array();
    j2.push_back(1);
    Json m2 = Json::object();
    m2 = std::move(j2);
    assert(m2.is_array());
    assert(m2[0].get<int>() == 1);
    assert(j2.is_null());
}

void test_missing_reads() {
    Json obj = Json::object();
    assert(obj["missing"].is_null());

    Json arr = Json::array().push_back(1);
    assert(arr[5].is_null());

    bool bad_cast = false;
    try {
        (void)arr[5].get<int>();
    } catch (const std::bad_cast&) {
        bad_cast = true;
    }
    assert(bad_cast);
}

void test_proxy_conversion_copy() {
    Json j = Json::object();
    j["x"]["y"] = 1;
    Json copy = j["x"]; // copy should be detached
    copy["y"] = 2;
    assert(j["x"]["y"].get<int>() == 1);
}

void test_nonconst_operator_no_autovivify() {
    // Case 1: root is null, accessing by key should not mutate
    Json j;
    (void)j["a"]; // create proxy only
    assert(j.is_null());
    (void)static_cast<Json>(j["a"]);
    assert(j.is_null());
    (void)j["a"].is_null();
    assert(j.is_null());

    // Case 2: object without key should not get a new null member on read-only access
    Json obj = Json::object();
    (void)obj["missing"]; // read-like
    assert(obj.dump() == "{}");
    (void)static_cast<Json>(obj["missing"]);
    assert(obj.dump() == "{}");

    // Case 3: array read of out-of-bounds index should not extend array
    Json arr = Json::array();
    (void)arr[3];
    assert(arr.dump() == "[]");
    (void)static_cast<Json>(arr[3]);
    assert(arr.dump() == "[]");
}

void test_edge_cases() {
    // Empty object and array
    Json empty_obj = Json::object();
    assert(empty_obj.dump() == "{}");
    Json empty_arr = Json::array();
    assert(empty_arr.dump() == "[]");

    // Assigning to a non-object
    bool thrown = false;
    try {
        Json j = 123;
        j["key"] = "value";
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    assert(thrown);

    // push_back to a non-array
    thrown = false;
    try {
        Json j = 123;
        j.push_back("value");
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    assert(thrown);
    
    // Accessing out of bounds (reading) returns null; get<T>() on it throws bad_cast
    Json arr = Json::array().push_back(1);
    assert(arr[5].is_null());
    bool bad_cast = false;
    try {
        (void)arr[5].get<int>();
    } catch (const std::bad_cast&) {
        bad_cast = true;
    }
    assert(bad_cast);
}