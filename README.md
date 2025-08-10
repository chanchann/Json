## User Guide: C++ JSON Wrapper (`Json.h`)

This guide explains how to use the C++ JSON wrapper built on top of the bundled C JSON library. It covers common tasks, behaviors, edge cases, and best practices.

### What you get
- **RAII C++ API** over a C JSON engine
- **Chainable access** to nested values with auto-vivification
- **Type-safe getters** and clear error behavior
- **Deep-copy semantics** for safe composition
- **Safe serialization** with proper string escaping

---

## Getting started

- Include the header:
```cpp
#include "Json.h"
```
- Create values directly or via factories:
```cpp
Json j_null;                 // null
Json j_str("hello");        // string
Json j_num(42);              // number
Json j_bool(true);           // boolean
Json j_obj = Json::object(); // {}
Json j_arr = Json::array();  // []
```
- Parse from string:
```cpp
Json parsed = Json::parse("{\"a\":1,\"b\":[false,null]}");
```

---

## Creating and reading values

### Construction
- **Strings**: `Json(const char*)`, `Json(const std::string&)`
- **Numbers**: `Json(double)`, `Json(int)`, `Json(long)`
- **Booleans**: `Json(bool)`
- **Null**: `Json()`, `Json(std::nullptr_t)`
- **Object/Array**: `Json::object()`, `Json::array()`

### Reading typed values
Use `get<T>()` with the supported specializations:
- `std::string`, `double`, `int`, `long`, `bool`

Example:
```cpp
Json j("hello");
std::string s = j.get<std::string>(); // "hello"
```

### Type checks
```cpp
j.is_null(); j.is_string(); j.is_number(); j.is_boolean(); j.is_object(); j.is_array();
```

---

## Access and chaining

### Object access
```cpp
Json j = Json::object();
j["name"] = "Alice";
std::string name = j["name"].get<std::string>();
```

### Array access
```cpp
Json arr = Json::array();
arr.push_back(1).push_back(2);
int first = arr[0].get<int>(); // 1
```

### Chained access with auto-vivification
- Intermediate containers are created automatically during writes.
```cpp
Json j;
j["user"]["info"]["name"] = "Alice";
// j is now {"user":{"info":{"name":"Alice"}}}

Json a;
a[2][0] = 42; // auto-extends array with nulls, then creates nested array
// a is now [null,null,[42]]
```

### Reading missing values
- Reading a missing key or out-of-bounds index returns a `null` proxy.
- Calling `get<T>()` on it throws `std::bad_cast`.
```cpp
Json j = Json::object();
bool is_null = j["missing"].is_null(); // true

Json a = Json::array().push_back(1);
// a[5] is null; a[5].get<int>() throws std::bad_cast
```

---

## Mutation operations

### set(key, value)
- Ensures the root is an object (auto-converts null to object), then sets/replaces the key.
```cpp
Json j = Json::object();
j.set("age", 30).set("name", "Alice");
```

### push_back(value)
- Ensures the root is an array (auto-converts null to array), then appends.
```cpp
Json a = Json::array();
a.push_back(1).push_back("two").push_back(true);
```

### Index assignment for arrays
- Replaces an element at index; the array is auto-extended with `null`s as needed.
```cpp
Json a = Json::array();
a[3] = 7; // a becomes [null,null,null,7]
```

### erase
- From objects: `erase(const std::string&)`
- From arrays: `erase(size_t index)`
```cpp
Json j = Json::object();
j["k"] = 1; j.erase("k"); // j["k"] becomes null

Json a = Json::array().push_back(10).push_back(20).push_back(30);
a.erase(1); // remove middle element -> now [10,30]
```

---

## Parsing and serialization

### parse
- `static Json parse(const std::string&)`
- Throws `std::runtime_error` on invalid JSON.

### dump
- Returns a JSON string with proper escaping of quotes, backslashes, control characters, and common escapes (`\n`, `\t`, etc.).
```cpp
Json j = Json::object();
j["s"] = std::string("quote: \" backslash: \\ newline:\n");
std::string out = j.dump();
```

---

## Deep-copy semantics

- Assigning complex values (objects/arrays) via `set`, `push_back`, or `a[index] = value` performs a deep copy into the destination structure.
- Reading through a proxy (`Json j = root["k"];`) returns a detached copy; later modifications to the copy do not affect the original.

Examples:
```cpp
Json sub = Json::object();
sub["k"] = "v";
Json j = Json::object();
j["sub"] = sub;   // deep copy
sub["k"] = "x";   // j["sub"]["k"] stays "v"

Json copy = j["sub"]; // copy
copy["k"] = "y";     // j is unchanged
```

---

## Error handling and exceptions

- Writing object keys on a non-object (and non-null) root: throws `std::runtime_error`.
- Writing array elements on a non-array (and non-null) root: throws `std::runtime_error`.
- Reading missing values: returns `null` proxy; `get<T>()` throws `std::bad_cast`.
- Parsing invalid JSON: throws `std::runtime_error`.

---

## Performance tips

- **Prefer push_back** over frequent index-based replacements; index assignment currently rebuilds the array internally (O(n)).
- **Batch nested writes** by composing sub-objects/arrays and assigning once to minimize deep copies.
- **Avoid unnecessary copies**: hold references to the root `Json` and assign via chained proxies directly.

---

## Interoperability and integration

- Include `Json.h` in your C++ code; it uses the bundled C library for parsing and storage.
- Build with the provided `Makefile`:
  - Build: `make`
  - Run tests: `make test`
- The project includes `json_parser.c/.h` and links them automatically for the test target.

---

## Known limitations (current)

- **No const overload of operator[]**: read-only traversal via proxies is possible, but const-indexing isn’t provided yet.
- **No iteration helpers** (size, keys, iterators). If you need structured traversal, either:
  - Maintain your own schema-specific loops/indices, or
  - Extend the wrapper with iteration APIs, or
  - Use the underlying C API (`json_array_for_each`, `json_object_for_each`) carefully.
- **No direct numeric integer/float distinction**: numbers are stored as doubles.

---

## Frequently asked questions

- **Q: What happens when I do `j["a"][2][0] = 5;` on a null root?**
  - A: The root becomes an object, `j["a"]` becomes an array, it is extended with `null`s up to index 2, and `j["a"][2]` becomes an array, with element `0` set to 5.

- **Q: Why does `arr[10]` not throw?**
  - A: Reads of missing values return a `null` proxy for convenience. Call `get<T>()` to enforce type and existence, which will throw `std::bad_cast` for null/mismatch.

- **Q: Are writes deep-copied?**
  - A: Yes. Objects/arrays are deeply copied to avoid aliasing bugs and ownership issues across C/C++ boundaries.

- **Q: Is the API thread-safe?**
  - A: Not guaranteed. Don’t share a `Json` instance across threads without external synchronization.

---

## Example end-to-end
```cpp
#include "Json.h"
#include <iostream>

int main() {
  Json j;
  j["user"]["name"] = "Alice";
  j["user"]["age"] = 30;
  j["flags"][0] = true;

  // Deep-copy behavior
  Json user_copy = j["user"]; // detached copy
  user_copy["name"] = "Bob"; // j remains unchanged

  std::cout << j.dump() << "\n"; // {"user":{"name":"Alice","age":30},"flags":[true]}
}
```

---

## Testing
- Run the bundled test suite to validate behavior:
  - `make test`
