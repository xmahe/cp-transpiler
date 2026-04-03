#include "body_validation.h"

#include "support/strings.h"

#include <optional>
#include <string>
#include <unordered_set>
#include <utility>

namespace cplus::parse {
namespace {

bool is_maybe_type(std::string_view spelling) {
    return spelling.rfind("maybe<", 0) == 0;
}

bool is_block_statement(const ast::Statement* stmt) {
    return stmt != nullptr && std::holds_alternative<ast::BlockStmt>(stmt->node);
}

const ast::IdentifierExpr* as_identifier(const ast::Expression* expr) {
    if (expr == nullptr) {
        return nullptr;
    }
    return std::get_if<ast::IdentifierExpr>(&expr->node);
}

const ast::MemberExpr* as_member(const ast::Expression* expr) {
    if (expr == nullptr) {
        return nullptr;
    }
    return std::get_if<ast::MemberExpr>(&expr->node);
}

const ast::CallExpr* as_call(const ast::Expression* expr) {
    if (expr == nullptr) {
        return nullptr;
    }
    return std::get_if<ast::CallExpr>(&expr->node);
}

std::optional<std::string> maybe_guard_name(const ast::Expression* expr) {
    const auto* call = as_call(expr);
    if (call == nullptr || !call->arguments.empty()) {
        return std::nullopt;
    }
    const auto* member = as_member(call->callee.get());
    if (member == nullptr || member->member != "has_value") {
        return std::nullopt;
    }
    const auto* ident = as_identifier(member->object.get());
    if (ident == nullptr || ident->name.parts.size() != 1) {
        return std::nullopt;
    }
    return ident->name.parts.front();
}

bool contains_assignment(const ast::Expression* expr) {
    if (expr == nullptr) {
        return false;
    }

    return std::visit(
        [&](const auto& node) -> bool {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, ast::AssignmentExpr>) {
                return true;
            } else if constexpr (std::is_same_v<T, ast::BinaryExpr>) {
                return contains_assignment(node.lhs.get()) || contains_assignment(node.rhs.get());
            } else if constexpr (std::is_same_v<T, ast::UnaryExpr>) {
                return contains_assignment(node.operand.get());
            } else if constexpr (std::is_same_v<T, ast::MemberExpr>) {
                return contains_assignment(node.object.get());
            } else if constexpr (std::is_same_v<T, ast::CallExpr>) {
                if (contains_assignment(node.callee.get())) {
                    return true;
                }
                for (const auto& arg : node.arguments) {
                    if (contains_assignment(arg.get())) {
                        return true;
                    }
                }
                return false;
            }
            return false;
        },
        expr->node);
}

void validate_expression(
    const ast::Expression* expr,
    const std::unordered_set<std::string>& maybe_names,
    const std::unordered_set<std::string>& guarded_names,
    const std::filesystem::path& file,
    support::Diagnostics& diagnostics);

void validate_statement(
    const ast::Statement* stmt,
    std::unordered_set<std::string> maybe_names,
    std::unordered_set<std::string> guarded_names,
    int if_depth,
    const std::filesystem::path& file,
    support::Diagnostics& diagnostics);

void validate_block(
    const ast::BlockStmt& block,
    std::unordered_set<std::string> maybe_names,
    std::unordered_set<std::string> guarded_names,
    int if_depth,
    const std::filesystem::path& file,
    support::Diagnostics& diagnostics) {
    bool saw_non_decl = false;

    for (const auto& stmt : block.statements) {
        if (stmt == nullptr) {
            continue;
        }

        if (const auto* decl = std::get_if<ast::DeclStmt>(&stmt->node)) {
            if (saw_non_decl) {
                diagnostics.error("declarations may not be interleaved with statements", stmt->span, file.string());
            }
            if (is_maybe_type(decl->type.spelling)) {
                maybe_names.insert(decl->name);
            }
            if (decl->initializer != nullptr) {
                validate_expression(decl->initializer.get(), maybe_names, guarded_names, file, diagnostics);
            }
            continue;
        }

        saw_non_decl = true;
        validate_statement(stmt.get(), maybe_names, guarded_names, if_depth, file, diagnostics);
    }
}

void validate_expression(
    const ast::Expression* expr,
    const std::unordered_set<std::string>& maybe_names,
    const std::unordered_set<std::string>& guarded_names,
    const std::filesystem::path& file,
    support::Diagnostics& diagnostics) {
    if (expr == nullptr) {
        return;
    }

    std::visit(
        [&](const auto& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, ast::MemberExpr>) {
                validate_expression(node.object.get(), maybe_names, guarded_names, file, diagnostics);
            } else if constexpr (std::is_same_v<T, ast::CallExpr>) {
                if (const auto* member = as_member(node.callee.get()); member != nullptr && node.arguments.empty() &&
                    member->member == "value") {
                    if (const auto* ident = as_identifier(member->object.get());
                        ident != nullptr && ident->name.parts.size() == 1) {
                        const auto& name = ident->name.parts.front();
                        if (maybe_names.contains(name) && !guarded_names.contains(name)) {
                            diagnostics.error(
                                "unsafe maybe<T>.value() access without proven has_value(): " + name,
                                expr->span,
                                file.string());
                        }
                    }
                }

                validate_expression(node.callee.get(), maybe_names, guarded_names, file, diagnostics);
                for (const auto& arg : node.arguments) {
                    validate_expression(arg.get(), maybe_names, guarded_names, file, diagnostics);
                }
            } else if constexpr (std::is_same_v<T, ast::AssignmentExpr>) {
                validate_expression(node.target.get(), maybe_names, guarded_names, file, diagnostics);
                validate_expression(node.value.get(), maybe_names, guarded_names, file, diagnostics);
            } else if constexpr (std::is_same_v<T, ast::BinaryExpr>) {
                validate_expression(node.lhs.get(), maybe_names, guarded_names, file, diagnostics);
                validate_expression(node.rhs.get(), maybe_names, guarded_names, file, diagnostics);
            } else if constexpr (std::is_same_v<T, ast::UnaryExpr>) {
                validate_expression(node.operand.get(), maybe_names, guarded_names, file, diagnostics);
            }
        },
        expr->node);
}

void validate_statement(
    const ast::Statement* stmt,
    std::unordered_set<std::string> maybe_names,
    std::unordered_set<std::string> guarded_names,
    int if_depth,
    const std::filesystem::path& file,
    support::Diagnostics& diagnostics) {
    if (stmt == nullptr) {
        return;
    }

    std::visit(
        [&](const auto& node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, ast::BlockStmt>) {
                validate_block(node, std::move(maybe_names), std::move(guarded_names), if_depth, file, diagnostics);
            } else if constexpr (std::is_same_v<T, ast::ReturnStmt>) {
                validate_expression(node.value.get(), maybe_names, guarded_names, file, diagnostics);
            } else if constexpr (std::is_same_v<T, ast::ExprStmt>) {
                validate_expression(node.expression.get(), maybe_names, guarded_names, file, diagnostics);
            } else if constexpr (std::is_same_v<T, ast::IfStmt>) {
                if (if_depth >= 2) {
                    diagnostics.error("if nesting depth may not exceed 2", stmt->span, file.string());
                }
                if (contains_assignment(node.condition.get())) {
                    diagnostics.error("assignment inside conditions is forbidden", node.condition->span, file.string());
                }
                validate_expression(node.condition.get(), maybe_names, guarded_names, file, diagnostics);
                if (!is_block_statement(node.then_branch.get())) {
                    diagnostics.error("control-flow bodies must use braces", node.then_branch != nullptr ? node.then_branch->span : stmt->span, file.string());
                }
                std::unordered_set<std::string> then_guarded = guarded_names;
                if (const auto guard = maybe_guard_name(node.condition.get()); guard.has_value() && maybe_names.contains(*guard)) {
                    then_guarded.insert(*guard);
                }
                validate_statement(node.then_branch.get(), maybe_names, std::move(then_guarded), if_depth + 1, file, diagnostics);
                if (node.else_branch != nullptr) {
                    if (!is_block_statement(node.else_branch.get())) {
                        diagnostics.error("control-flow bodies must use braces", node.else_branch->span, file.string());
                    }
                    validate_statement(node.else_branch.get(), maybe_names, guarded_names, if_depth + 1, file, diagnostics);
                }
            } else if constexpr (std::is_same_v<T, ast::WhileStmt>) {
                if (contains_assignment(node.condition.get())) {
                    diagnostics.error("assignment inside conditions is forbidden", node.condition->span, file.string());
                }
                validate_expression(node.condition.get(), maybe_names, guarded_names, file, diagnostics);
                if (!is_block_statement(node.body.get())) {
                    diagnostics.error("control-flow bodies must use braces", node.body != nullptr ? node.body->span : stmt->span, file.string());
                }
                validate_statement(node.body.get(), maybe_names, guarded_names, if_depth, file, diagnostics);
            } else if constexpr (std::is_same_v<T, ast::ForStmt>) {
                if (node.init != nullptr) {
                    if (!std::holds_alternative<ast::ExprStmt>(node.init->node)) {
                        diagnostics.error("for initializer must be an expression", node.init->span, file.string());
                    } else if (const auto* init_expr = std::get_if<ast::ExprStmt>(&node.init->node)) {
                        validate_expression(init_expr->expression.get(), maybe_names, guarded_names, file, diagnostics);
                    }
                }
                if (node.condition != nullptr) {
                    if (contains_assignment(node.condition.get())) {
                        diagnostics.error("assignment inside conditions is forbidden", node.condition->span, file.string());
                    }
                    validate_expression(node.condition.get(), maybe_names, guarded_names, file, diagnostics);
                }
                if (node.step != nullptr) {
                    validate_expression(node.step.get(), maybe_names, guarded_names, file, diagnostics);
                }
                if (!is_block_statement(node.body.get())) {
                    diagnostics.error("control-flow bodies must use braces", node.body != nullptr ? node.body->span : stmt->span, file.string());
                }
                validate_statement(node.body.get(), maybe_names, guarded_names, if_depth, file, diagnostics);
            } else if constexpr (std::is_same_v<T, ast::ContinueStmt>) {
                diagnostics.error("continue is forbidden", stmt->span, file.string());
            }
        },
        stmt->node);
}

void validate_function(const ast::FunctionDecl& decl, const std::filesystem::path& file, support::Diagnostics& diagnostics) {
    auto reject_if_contains = [&](std::string_view needle, std::string_view message) {
        if (decl.body_source.find(needle) != std::string::npos) {
            diagnostics.error(std::string(message), decl.span, file.string());
        }
    };

    reject_if_contains("goto", "goto is forbidden");
    reject_if_contains("++", "++ is forbidden");
    reject_if_contains("--", "-- is forbidden");
    reject_if_contains("?", "ternary operator is forbidden");
    if (decl.body_source.find("switch") != std::string::npos && decl.body_source.find("default:") == std::string::npos) {
        diagnostics.error("switch must include default", decl.span, file.string());
    }

    if (decl.body == nullptr) {
        return;
    }

    std::unordered_set<std::string> maybe_names;
    for (const auto& param : decl.signature.parameters) {
        if (is_maybe_type(param.type.spelling)) {
            maybe_names.insert(param.name);
        }
    }

    validate_block(*decl.body, std::move(maybe_names), {}, 0, file, diagnostics);
}

void validate_declarations(
    const std::vector<ast::Declaration>& declarations,
    const std::filesystem::path& file,
    support::Diagnostics& diagnostics) {
    for (const auto& decl : declarations) {
        std::visit(
            [&](const auto& node) {
                using T = std::decay_t<decltype(node)>;
                if constexpr (std::is_same_v<T, ast::NamespaceDecl>) {
                    validate_declarations(node.declarations, file, diagnostics);
                } else if constexpr (std::is_same_v<T, ast::FunctionDecl>) {
                    validate_function(node, file, diagnostics);
                }
            },
            decl);
    }
}

} // namespace

void validate_body_ast(const std::vector<ast::Module>& modules, support::Diagnostics& diagnostics) {
    for (const auto& module : modules) {
        validate_declarations(module.declarations, module.source_path, diagnostics);
    }
}

} // namespace cplus::parse
