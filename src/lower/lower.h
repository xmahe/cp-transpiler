#pragma once

#include "ir.h"
#include "../sema/analyze.h"

#include <string_view>
#include <string>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cplus::lower {

class Lowerer {
public:
    CModule lower(const cplus::sema::AnalysisResult& analysis) const;

private:
    CStruct lower_class(
        const cplus::model::ClassDecl& decl,
        const std::unordered_map<std::string, std::string>& class_name_map,
        const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& inject_bindings) const;
    std::vector<CGlobal> lower_static_fields(
        const cplus::model::ClassDecl& decl,
        const std::unordered_map<std::string, std::string>& class_name_map,
        const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& inject_bindings) const;
    CEnum lower_enum(const cplus::model::EnumDecl& decl) const;
    CFunction lower_function(
        const cplus::model::FunctionDecl& decl,
        const std::unordered_map<std::string, std::string>& class_name_map,
        const std::unordered_set<std::string>& enum_names,
        const std::unordered_set<std::string>& default_constructible_class_names,
        const std::unordered_set<std::string>& destructible_class_names) const;
    CFunction lower_method(
        const cplus::model::FunctionSignature& sig,
        std::string_view class_name,
        std::string_view source_class_name,
        const std::vector<cplus::model::FieldDecl>& instance_fields,
        const std::unordered_set<std::string>& method_names,
        const std::unordered_map<std::string, std::string>& class_name_map,
        const std::unordered_set<std::string>& enum_names,
        const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& inject_bindings,
        const std::unordered_set<std::string>& default_constructible_class_names,
        const std::unordered_set<std::string>& destructible_class_names) const;
    CMaybeType lower_maybe(std::string_view spelling, const std::unordered_map<std::string, std::string>& class_name_map) const;
    static CType to_c_type(const cplus::model::TypeRef& type, const std::unordered_map<std::string, std::string>& class_name_map);
    static bool is_maybe_type(std::string_view spelling);
    static std::string maybe_type_name(std::string_view spelling);
    static std::string maybe_inner_type(std::string_view spelling);
    static std::string join_path(const std::vector<std::string>& path);
    static std::vector<std::string> lower_body_lines(std::string_view body_source);
    static std::vector<std::string> lower_method_body_lines(
        std::string_view body_source,
        const cplus::model::FunctionSignature& sig,
        const std::vector<cplus::model::FieldDecl>& instance_fields,
        std::string_view source_class_name,
        std::string_view class_name,
        const std::unordered_set<std::string>& method_names,
        const std::unordered_map<std::string, std::string>& class_name_map,
        const std::unordered_set<std::string>& enum_names,
        const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& inject_bindings);
    static std::string rewrite_method_body(
        std::string_view body_source,
        std::string_view class_name,
        const std::unordered_set<std::string>& field_names,
        const std::unordered_set<std::string>& local_names,
        const std::unordered_set<std::string>& method_names,
        const std::unordered_set<std::string>& enum_names,
        const std::unordered_map<std::string, std::string>& field_object_types,
        const std::unordered_map<std::string, std::string>& local_object_types);
    static std::unordered_set<std::string> collect_local_names(std::string_view body_source);
    static std::unordered_map<std::string, std::string> collect_local_object_types(
        std::string_view body_source,
        const std::unordered_map<std::string, std::string>& class_name_map);
    static std::string rewrite_local_class_constructors(
        std::string_view body_source,
        const std::unordered_map<std::string, std::string>& class_name_map,
        const std::unordered_set<std::string>& default_constructible_class_names);
    static std::string prepend_lines_to_body(
        std::vector<std::string> lines,
        std::string_view body_source);
    static std::string resolve_field_class_name(
        const cplus::model::FieldDecl& field,
        std::string_view source_class_name,
        const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& inject_bindings);
    static std::vector<std::string> member_construct_calls(
        const std::vector<cplus::model::FieldDecl>& instance_fields,
        std::string_view source_class_name,
        const std::unordered_map<std::string, std::string>& class_name_map,
        const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& inject_bindings,
        const std::unordered_set<std::string>& default_constructible_class_names);
    static std::vector<std::string> member_destruct_calls(
        const std::vector<cplus::model::FieldDecl>& instance_fields,
        std::string_view source_class_name,
        const std::unordered_map<std::string, std::string>& class_name_map,
        const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& inject_bindings,
        const std::unordered_set<std::string>& destructible_class_names);
    static std::string rewrite_returns_with_raii(
        std::string_view body_source,
        const CType& return_type,
        const std::unordered_map<std::string, std::string>& class_name_map,
        const std::unordered_set<std::string>& destructible_class_names,
        std::vector<std::string> extra_cleanup_lines = {});
};

} // namespace cplus::lower
