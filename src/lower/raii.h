#pragma once

#include <string>
#include <vector>

namespace cplus::lower {

struct LocalObject {
    std::string name;
    std::string type_name;
    bool needs_destruction = true;
};

struct CleanupStep {
    std::string label;
    std::vector<std::string> destroy_calls;
};

struct CleanupPlan {
    std::vector<CleanupStep> steps;
};

class RaiiPlanner {
public:
    CleanupPlan plan_for_locals(const std::vector<LocalObject>& locals) const;
    std::vector<std::string> label_chain(const CleanupPlan& plan) const;
};

} // namespace cplus::lower
