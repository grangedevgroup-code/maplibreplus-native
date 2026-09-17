#include <mln/util/globe.hpp>

#include <mln/math/wrap.hpp>
#include <mln/util/constants.hpp>

#include <algorithm>
#include <cmath>

namespace mln {
namespace util {
namespace globe {

vec3 add(const vec3& a, const vec3& b) {
    return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

vec3 subtract(const vec3& a, const vec3& b) {
    return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

vec3 scale(const vec3& a, double s) {
    return {a[0] * s, a[1] * s, a[2] * s};
}

double dot(const vec3& a, const vec3& b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

vec3 cross(const vec3& a, const vec3& b) {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

double length(const vec3& a) {
    return std::sqrt(dot(a, a));
}

vec3 normalize(const vec3& a) {
    const double len = length(a);
    if (len == 0.0) {
        return {0.0, 0.0, 0.0};
    }
    return scale(a, 1.0 / len);
}

vec3 rotateX(const vec3& a, double rad) {
    const double c = std::cos(rad);
    const double s = std::sin(rad);
    return {a[0], a[1] * c - a[2] * s, a[1] * s + a[2] * c};
}

vec3 rotateY(const vec3& a, double rad) {
    const double c = std::cos(rad);
    const double s = std::sin(rad);
    return {a[2] * s + a[0] * c, a[1], a[2] * c - a[0] * s};
}

vec3 rotateZ(const vec3& a, double rad) {
    const double c = std::cos(rad);
    const double s = std::sin(rad);
    return {a[0] * c - a[1] * s, a[0] * s + a[1] * c, a[2]};
}

vec3 angularCoordinatesRadiansToVector(double lngRadians, double latRadians) {
    const double len = std::cos(latRadians);
    return {std::sin(lngRadians) * len, std::sin(latRadians), std::cos(lngRadians) * len};
}

vec3 latLngToSurfaceVector(const LatLng& latLng) {
    return angularCoordinatesRadiansToVector(latLng.longitude() * M_PI / 180.0, latLng.latitude() * M_PI / 180.0);
}

LatLng surfaceVectorToLatLng(const vec3& surface) {
    const double latRadians = std::asin(std::clamp(surface[1], -1.0, 1.0));
    const double latDegrees = latRadians / M_PI * 180.0;
    const double lengthXZ = std::sqrt(surface[0] * surface[0] + surface[2] * surface[2]);
    if (lengthXZ > 1e-6) {
        const double projX = surface[0] / lengthXZ;
        const double projZ = surface[2] / lengthXZ;
        const double acosZ = std::acos(std::clamp(projZ, -1.0, 1.0));
        const double lngRadians = (projX > 0) ? acosZ : -acosZ;
        const double lngDegrees = lngRadians / M_PI * 180.0;
        return LatLng{latDegrees, wrap(lngDegrees, -180.0, 180.0)};
    }
    return LatLng{latDegrees, 0.0};
}

vec2 mercatorCoordinatesToAngularCoordinatesRadians(double mercatorX, double mercatorY) {
    const double sphericalX = wrap(mercatorX * M_PI * 2.0 + M_PI, 0.0, M_PI * 2.0);
    const double sphericalY = 2.0 * std::atan(std::exp(M_PI - (mercatorY * M_PI * 2.0))) - M_PI * 0.5;
    return {sphericalX, sphericalY};
}

vec3 mercatorCoordinatesToSurfaceVector(double mercatorX, double mercatorY) {
    const vec2 angular = mercatorCoordinatesToAngularCoordinatesRadians(mercatorX, mercatorY);
    return angularCoordinatesRadiansToVector(angular[0], angular[1]);
}

vec3 projectTileCoordinatesToSphere(double inTileX, double inTileY, int32_t tileX, int32_t tileY, uint8_t tileZ) {
    const double scale_ = 1.0 / static_cast<double>(1 << tileZ);
    const double mercatorX = inTileX / static_cast<double>(util::EXTENT) * scale_ + tileX * scale_;
    const double mercatorY = inTileY / static_cast<double>(util::EXTENT) * scale_ + tileY * scale_;
    return mercatorCoordinatesToSurfaceVector(mercatorX, mercatorY);
}

double getGlobeRadiusPixels(double worldSize, double latitudeDegrees) {
    return worldSize / (2.0 * M_PI) / std::cos(latitudeDegrees * M_PI / 180.0);
}

double getGlobeCircumferencePixels(double worldSize, double latitudeDegrees) {
    return 2.0 * M_PI * getGlobeRadiusPixels(worldSize, latitudeDegrees);
}

double distanceOfLocationsPixels(double worldSize, double centerLatitude, const LatLng& a, const LatLng& b) {
    const vec3 vecA = latLngToSurfaceVector(a);
    const vec3 vecB = latLngToSurfaceVector(b);
    const double radians = std::acos(std::clamp(dot(vecA, vecB), -1.0, 1.0));
    return radians / (2.0 * M_PI) * getGlobeCircumferencePixels(worldSize, centerLatitude);
}

double getZoomAdjustment(double oldLatitude, double newLatitude) {
    const double oldScale = std::cos(oldLatitude * M_PI / 180.0);
    const double newScale = std::cos(newLatitude * M_PI / 180.0);
    return std::log2(newScale / oldScale);
}

bool raySphereIntersection(const vec3& origin, const vec3& direction, double radius, double& tOut) {
    const double originDotDirection = dot(origin, direction);
    const double radiusSquared = radius * radius;
    const vec3 inner = subtract(origin, scale(direction, originDotDirection));
    const double discriminant = radiusSquared - dot(inner, inner);
    if (discriminant < 0.0) {
        return false;
    }
    const double c = dot(origin, origin) - radiusSquared;
    const double q = -originDotDirection + (originDotDirection < 0.0 ? 1.0 : -1.0) * std::sqrt(discriminant);
    if (q == 0.0) {
        return false;
    }
    const double t0 = c / q;
    const double t1 = q;
    tOut = std::min(t0, t1);
    return true;
}

double pointPlaneSignedDistance(const vec4& plane, const vec3& point) {
    return plane[0] * point[0] + plane[1] * point[1] + plane[2] * point[2] + plane[3];
}

} // namespace globe
} // namespace util
} // namespace mln
