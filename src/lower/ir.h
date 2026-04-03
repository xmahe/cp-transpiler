#pragma once

#include <string>
#include <vector>

namespace cplus::lower {

enum class CLinkage {
    External,
    Internal,
};

struct CType {
    std::string spelling;
};

struct CParameter {
    std::string name;
    CType type;
};

struct CField {
    std::string name;
    CType type;
    bool is_private_intent = false;
    bool is_static = false;
};

struct CStruct {
    std::string name;
    std::vector<CField> fields;
};

struct CEnum {
    struct Member {
        std::string source_name;
        std::string c_name;
    };

    std::string name;
    std::vector<Member> members;
};

struct CMaybeType {
    std::string name;
    CType value_type;
};

struct CGlobal {
    std::string name;
    CType type;
    CLinkage linkage = CLinkage::Internal;
    std::string initializer;
};

struct CFunction {
    std::string name;
    CType return_type;
    std::vector<CParameter> parameters;
    std::vector<std::string> body_lines;
    CLinkage linkage = CLinkage::External;
};

struct CModule {
    std::string header_name;
    std::string source_name;
    std::vector<std::string> source_prelude_lines;
    std::vector<CStruct> structs;
    std::vector<CEnum> enums;
    std::vector<CMaybeType> maybe_types;
    std::vector<CGlobal> globals;
    std::vector<CFunction> functions;
};

} // namespace cplus::lower
