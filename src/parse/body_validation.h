#pragma once

#include "ast/ast.h"
#include "support/diagnostics.h"

#include <vector>

namespace cplus::parse {

void validate_body_ast(const std::vector<ast::Module>& modules, support::Diagnostics& diagnostics);

} // namespace cplus::parse
