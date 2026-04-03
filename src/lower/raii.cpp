#include "raii.h"

#include <sstream>

namespace cplus::lower {

CleanupPlan RaiiPlanner::plan_for_locals(const std::vector<LocalObject>& locals) const {
    CleanupPlan plan;
    for (std::size_t index = 0; index < locals.size(); ++index) {
        const auto& local = locals[index];
        if (!local.needs_destruction) {
            continue;
        }

        CleanupStep step;
        step.label = "__cleanup_" + std::to_string(index);
        step.destruct_calls.push_back(local.type_name + "___Destruct(&" + local.name + ")");
        plan.steps.push_back(std::move(step));
    }

    return plan;
}

std::vector<std::string> RaiiPlanner::label_chain(const CleanupPlan& plan) const {
    std::vector<std::string> labels;
    labels.reserve(plan.steps.size());
    for (const auto& step : plan.steps) {
        labels.push_back(step.label);
    }
    return labels;
}

} // namespace cplus::lower
