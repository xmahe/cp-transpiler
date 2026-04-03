#pragma once

#include "../lower/ir.h"

#include <filesystem>
#include <string>

namespace cplus::emit {

class CEmitter {
public:
    std::string emit_header(const cplus::lower::CModule& module) const;
    std::string emit_source(const cplus::lower::CModule& module) const;
    bool write_header(const cplus::lower::CModule& module, const std::filesystem::path& path) const;
    bool write_source(const cplus::lower::CModule& module, const std::filesystem::path& path) const;

private:
    static void append_line(std::string& out, const std::string& line);
    static std::string guard_name(const std::string& stem);
    static std::string linkage_prefix(cplus::lower::CLinkage linkage);
    static void emit_maybe_type(std::string& out, const cplus::lower::CMaybeType& maybe_type);
    static void emit_enum(std::string& out, const cplus::lower::CEnum& enumeration);
    static void emit_struct(std::string& out, const cplus::lower::CStruct& structure);
    static void emit_global(std::string& out, const cplus::lower::CGlobal& global);
    static void emit_function_decl(std::string& out, const cplus::lower::CFunction& function);
    static void emit_function_def(std::string& out, const cplus::lower::CFunction& function);
};

} // namespace cplus::emit
