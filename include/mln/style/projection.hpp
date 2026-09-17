#pragma once

#include <mln/math/clamp.hpp>

#include <cstdint>
#include <optional>
#include <string>

namespace mln {
namespace style {

enum class ProjectionType : uint8_t {
    Mercator,
    VerticalPerspective,
    Globe
};

std::optional<ProjectionType> projectionTypeFromString(const std::string&);
const char* projectionTypeToString(ProjectionType);

constexpr double GLOBE_TRANSITION_START_ZOOM = 11.0;
constexpr double GLOBE_TRANSITION_END_ZOOM = 12.0;

class ProjectionDefinition {
public:
    ProjectionDefinition() = default;
    explicit ProjectionDefinition(ProjectionType type_)
        : type(type_) {}

    ProjectionType getType() const { return type; }
    void setType(ProjectionType type_) { type = type_; }

    double transitionState(double zoom) const {
        switch (type) {
            case ProjectionType::Mercator:
                return 0.0;
            case ProjectionType::VerticalPerspective:
                return 1.0;
            case ProjectionType::Globe:
                break;
        }
        const double t = (zoom - GLOBE_TRANSITION_START_ZOOM) /
                         (GLOBE_TRANSITION_END_ZOOM - GLOBE_TRANSITION_START_ZOOM);
        return 1.0 - util::clamp(t, 0.0, 1.0);
    }

    bool usesGlobeRendering(double zoom) const { return transitionState(zoom) > 0.0; }

    friend bool operator==(const ProjectionDefinition& a, const ProjectionDefinition& b) { return a.type == b.type; }
    friend bool operator!=(const ProjectionDefinition& a, const ProjectionDefinition& b) { return !(a == b); }

private:
    ProjectionType type = ProjectionType::Mercator;
};

} // namespace style
} // namespace mln
