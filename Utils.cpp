#include "Utils.h"

// Definition for the Rng function declared in Utils.h
std::mt19937& Rng() {
    static std::mt19937 rng(static_cast<unsigned>(std::time(nullptr)));
    return rng;
}