#include <mln/style/projection.hpp>

namespace mln {
namespace style {

std::optional<ProjectionType> projectionTypeFromString(const std::string& value) {
    if (value == "mercator") {
        return ProjectionType::Mercator;
    }
    if (value == "vertical-perspective") {
        return ProjectionType::VerticalPerspective;
    }
    if (value == "globe") {
        return ProjectionType::Globe;
    }
    return std::nullopt;
}

const char* projectionTypeToString(ProjectionType type) {
    switch (type) {
        case ProjectionType::VerticalPerspective:
            return "vertical-perspective";
        case ProjectionType::Globe:
            return "globe";
        case ProjectionType::Mercator:
        default:
            return "mercator";
    }
}

} // namespace style
} // namespace mln
