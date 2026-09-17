#pragma once

#include <string>

namespace mln {
namespace style {

class Terrain {
public:
    Terrain() = default;
    Terrain(std::string source_, float exaggeration_)
        : source(std::move(source_)),
          exaggeration(exaggeration_) {}

    const std::string& getSource() const { return source; }
    void setSource(std::string source_) { source = std::move(source_); }

    float getExaggeration() const { return exaggeration; }
    void setExaggeration(float exaggeration_) { exaggeration = exaggeration_; }

    bool valid() const { return !source.empty(); }

    friend bool operator==(const Terrain& a, const Terrain& b) {
        return a.source == b.source && a.exaggeration == b.exaggeration;
    }
    friend bool operator!=(const Terrain& a, const Terrain& b) { return !(a == b); }

private:
    std::string source;
    float exaggeration = 1.0f;
};

} // namespace style
} // namespace mln
