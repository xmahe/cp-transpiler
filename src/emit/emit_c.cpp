#include "emit_c.h"

#include <fstream>
#include <sstream>

namespace cplus::emit {

void CEmitter::append_line(std::string& out, const std::string& line) {
    out.append(line);
    out.push_back('\n');
}

std::string CEmitter::linkage_prefix(const cplus::lower::CLinkage linkage) {
    return linkage == cplus::lower::CLinkage::Internal ? "static " : "";
}

std::string CEmitter::guard_name(const std::string& stem) {
    std::string guard = "CPLUS_";
    for (const char ch : stem) {
        if (std::isalnum(static_cast<unsigned char>(ch))) {
            guard.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
        } else {
            guard.push_back('_');
        }
    }
    guard.append("_H");
    return guard;
}

void CEmitter::emit_maybe_type(std::string& out, const cplus::lower::CMaybeType& maybe_type) {
    append_line(out, "typedef struct {");
    append_line(out, "    bool has_value;");
    append_line(out, "    " + maybe_type.value_type.spelling + " value;");
    append_line(out, "} " + maybe_type.name + ";");
}

void CEmitter::emit_enum(std::string& out, const cplus::lower::CEnum& enumeration) {
    append_line(out, "typedef enum {");
    for (const auto& member : enumeration.members) {
        append_line(out, "    " + member + ",");
    }
    append_line(out, "} " + enumeration.name + ";");
}

void CEmitter::emit_struct(std::string& out, const cplus::lower::CStruct& structure) {
    append_line(out, "typedef struct " + structure.name + " {");
    for (const auto& field : structure.fields) {
        std::string line = "    " + field.type.spelling + " " + field.name + ";";
        if (field.is_private_intent) {
            line += " // private";
        }
        append_line(out, line);
    }
    append_line(out, "} " + structure.name + ";");
}

void CEmitter::emit_global(std::string& out, const cplus::lower::CGlobal& global) {
    std::string line = linkage_prefix(global.linkage) + global.type.spelling + " " + global.name;
    if (!global.initializer.empty()) {
        line += " = " + global.initializer;
    }
    line += ";";
    append_line(out, line);
}

void CEmitter::emit_function_decl(std::string& out, const cplus::lower::CFunction& function) {
    std::ostringstream line;
    line << linkage_prefix(function.linkage) << function.return_type.spelling << " " << function.name << "(";
    for (std::size_t i = 0; i < function.parameters.size(); ++i) {
        const auto& param = function.parameters[i];
        if (i != 0) {
            line << ", ";
        }
        line << param.type.spelling << " " << param.name;
    }
    line << ");";
    append_line(out, line.str());
}

void CEmitter::emit_function_def(std::string& out, const cplus::lower::CFunction& function) {
    std::ostringstream line;
    line << linkage_prefix(function.linkage) << function.return_type.spelling << " " << function.name << "(";
    for (std::size_t i = 0; i < function.parameters.size(); ++i) {
        const auto& param = function.parameters[i];
        if (i != 0) {
            line << ", ";
        }
        line << param.type.spelling << " " << param.name;
    }
    line << ") {";
    append_line(out, line.str());
    if (function.body_lines.empty()) {
        append_line(out, "    /* empty */");
    } else {
        for (const auto& body_line : function.body_lines) {
            append_line(out, "    " + body_line);
        }
    }
    append_line(out, "}");
}

std::string CEmitter::emit_header(const cplus::lower::CModule& module) const {
    std::string out;
    const auto guard = guard_name(module.header_name.empty() ? "generated" : module.header_name);
    append_line(out, "#ifndef " + guard);
    append_line(out, "#define " + guard);
    append_line(out, "");
    append_line(out, "#include <stdbool.h>");
    append_line(out, "#include <stdint.h>");
    append_line(out, "");

    for (const auto& maybe_type : module.maybe_types) {
        emit_maybe_type(out, maybe_type);
        append_line(out, "");
    }

    for (const auto& enumeration : module.enums) {
        emit_enum(out, enumeration);
        append_line(out, "");
        append_line(out, "const char* " + enumeration.name + "___ToString(" + enumeration.name + " value);");
        append_line(out, "");
    }

    for (const auto& structure : module.structs) {
        emit_struct(out, structure);
        append_line(out, "");
    }

    for (const auto& function : module.functions) {
        if (function.linkage == cplus::lower::CLinkage::Internal) {
            continue;
        }
        emit_function_decl(out, function);
    }

    append_line(out, "");
    append_line(out, "#endif");
    return out;
}

std::string CEmitter::emit_source(const cplus::lower::CModule& module) const {
    std::string out;
    append_line(out, "#include \"" + module.header_name + "\"");
    append_line(out, "");

    for (const auto& global : module.globals) {
        emit_global(out, global);
    }
    if (!module.globals.empty()) {
        append_line(out, "");
    }

    for (const auto& function : module.functions) {
        emit_function_def(out, function);
        append_line(out, "");
    }

    for (const auto& enumeration : module.enums) {
        append_line(out, "const char* " + enumeration.name + "___ToString(" + enumeration.name + " value) {");
        append_line(out, "    switch (value) {");
        for (const auto& member : enumeration.members) {
            append_line(out, "    case " + member + ": return \"" + member + "\";");
        }
        append_line(out, "    default: return \"<invalid>\";");
        append_line(out, "    }");
        append_line(out, "}");
        append_line(out, "");
    }

    return out;
}

bool CEmitter::write_header(const cplus::lower::CModule& module, const std::filesystem::path& path) const {
    std::ofstream out(path);
    if (!out) {
        return false;
    }
    out << emit_header(module);
    return static_cast<bool>(out);
}

bool CEmitter::write_source(const cplus::lower::CModule& module, const std::filesystem::path& path) const {
    std::ofstream out(path);
    if (!out) {
        return false;
    }
    out << emit_source(module);
    return static_cast<bool>(out);
}

} // namespace cplus::emit
