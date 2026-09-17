#pragma once

#include <mln/util/geo.hpp>
#include <mln/util/vectors.hpp>

#include <cstdint>

namespace mln {
namespace util {
namespace globe {

constexpr double GLOBE_RADIUS_M = 6371008.8;

vec3 add(const vec3& a, const vec3& b);
vec3 subtract(const vec3& a, const vec3& b);
vec3 scale(const vec3& a, double s);
double dot(const vec3& a, const vec3& b);
vec3 cross(const vec3& a, const vec3& b);
double length(const vec3& a);
vec3 normalize(const vec3& a);
vec3 rotateX(const vec3& a, double rad);
vec3 rotateY(const vec3& a, double rad);
vec3 rotateZ(const vec3& a, double rad);

vec3 angularCoordinatesRadiansToVector(double lngRadians, double latRadians);
vec3 latLngToSurfaceVector(const LatLng& latLng);
LatLng surfaceVectorToLatLng(const vec3& surface);

vec2 mercatorCoordinatesToAngularCoordinatesRadians(double mercatorX, double mercatorY);
vec3 mercatorCoordinatesToSurfaceVector(double mercatorX, double mercatorY);
vec3 projectTileCoordinatesToSphere(double inTileX, double inTileY, int32_t tileX, int32_t tileY, uint8_t tileZ);

double getGlobeRadiusPixels(double worldSize, double latitudeDegrees);
double getGlobeCircumferencePixels(double worldSize, double latitudeDegrees);
double distanceOfLocationsPixels(double worldSize, double centerLatitude, const LatLng& a, const LatLng& b);

double getZoomAdjustment(double oldLatitude, double newLatitude);

bool raySphereIntersection(const vec3& origin, const vec3& direction, double radius, double& tOut);

double pointPlaneSignedDistance(const vec4& plane, const vec3& point);

} // namespace globe
} // namespace util
} // namespace mln
