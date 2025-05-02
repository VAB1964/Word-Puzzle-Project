#pragma once
#ifndef UTILS_H
#define UTILS_H

#include <random> // For std::mt19937, std::uniform_int_distribution, etc.
#include <ctime>  // For std::time
#include <SFML/System/Vector2.hpp>
#include <cmath>  // For M_PI if available, or define PI
#include <type_traits> // For std::is_integral_v, std::conditional_t

// Define PI if not provided by <cmath> (which sometimes happens)
#ifndef M_PI
constexpr float PI = 3.14159265359f;
#else
constexpr float PI = (float)M_PI;
#endif

constexpr float DEG2RAD(float d) { return d * PI / 180.f; }


// Function to get the random number generator
std::mt19937& Rng();

// Template function for random range (needs to be in header for template instantiation)
template <typename T>
T randRange(T a, T b) {
    // Use std::conditional_t for selecting distribution type based on T
    using dist = std::conditional_t<std::is_integral_v<T>,
        std::uniform_int_distribution<T>,
        std::uniform_real_distribution<T>>;
    return dist(a, b)(Rng()); // Call the Rng() function to get the generator
}

inline float distSq(const sf::Vector2f& p1, const sf::Vector2f& p2) {
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;
    return dx * dx + dy * dy;
    // Or using std::pow, though multiplication is often faster:
    // return std::pow(p1.x - p2.x, 2) + std::pow(p1.y - p2.y, 2);
}
#endif // UTILS_H