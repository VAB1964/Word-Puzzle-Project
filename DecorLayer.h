#pragma once
#ifndef DECORLAYER_H
#define DECORLAYER_H

#include "theme.h" 
#include <SFML/Graphics.hpp> // Includes Vector2f, Color, RenderTarget, Drawable, Transformable etc.
#include <vector>
#include <string> // Included by SFML/Graphics, but good practice
// Needs ColorTheme for the update method


//--------------------------------------------------------------------
//  DecorLayer Declaration
//--------------------------------------------------------------------
class DecorLayer {
public:
    // Nested types
    enum class Kind { Circle, Triangle, Line };
    struct Shape {
        Kind kind;
        sf::Vector2f pos;
        sf::Vector2f vel;
        float size = 0.f;
        float rot = 0.f;
        sf::Color col = sf::Color::Transparent; // Default initialize
    };

    // Constructor
    explicit DecorLayer(std::size_t target = 25);

    // Public Methods
    void update(float dt, sf::Vector2u winSize, const ColorTheme& theme);
    void draw(sf::RenderTarget& rt) const;

private:
    // Member Variables
    std::vector<Shape> m_shapes;
    std::size_t m_targetCount;

    // Private Helper Methods
    void m_spawn(sf::Vector2u win, const sf::Color& base, const sf::Color& accent1, const sf::Color& accent2);

    // Needs random utilities, declare static or include header here?
    // Including <random> here is okay for this small utility.
    // Or better, move randRange to its own utility header.
    // For now, assume randRange is available globally or via another include.
    // We'll call the global one defined in Source.cpp/main.cpp later.
};

#endif // DECORLAYER_H