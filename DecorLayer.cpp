#include "DecorLayer.h"
#include <random>     // For std::mt19937, distributions
#include <cmath>      // For std::cos, std::sin
#include <ctime>      // For std::time used in Rng seed
#include "Utils.h"




//--------------------------------------------------------------------
//  DecorLayer Implementation
//--------------------------------------------------------------------

// Constructor
DecorLayer::DecorLayer(std::size_t target) : m_targetCount(target) {}

// Update method
void DecorLayer::update(float dt, sf::Vector2u winSize, const ColorTheme& theme) {
    while (m_shapes.size() < m_targetCount) {
        m_spawn(winSize, theme.decorBase, theme.decorAccent1, theme.decorAccent2);
    }

    for (auto& s : m_shapes) {
        s.pos += s.vel * dt;
        // Simplified wrapping logic
        if (s.vel.x > 0 && s.pos.x - s.size > static_cast<float>(winSize.x)) {
            s.pos.x = -s.size - randRange<float>(50.f, 150.f); // Start further left
            s.pos.y = randRange<float>(0.f, static_cast<float>(winSize.y));
        }
        else if (s.vel.x < 0 && s.pos.x + s.size < 0.f) {
            s.pos.x = static_cast<float>(winSize.x) + s.size + randRange<float>(50.f, 150.f); // Start further right
            s.pos.y = randRange<float>(0.f, static_cast<float>(winSize.y));
        }
        // Simple vertical wrap (adjust if needed)
        if (s.pos.y - s.size > static_cast<float>(winSize.y)) s.pos.y = -s.size;
        else if (s.pos.y + s.size < 0.f) s.pos.y = static_cast<float>(winSize.y) + s.size;
    }
}

// Draw method
void DecorLayer::draw(sf::RenderTarget& rt) const {
    for (const auto& s : m_shapes) {
        switch (s.kind) {
        case Kind::Circle: {
            sf::CircleShape c(s.size);
            c.setOrigin({ s.size, s.size });
            c.setPosition(s.pos);
            c.setFillColor(s.col);
            rt.draw(c);
            break;
        }
        case Kind::Triangle: {
            sf::ConvexShape tri(3);
            sf::Vector2f centerSum(0, 0);
            for (int i = 0; i < 3; ++i) {
                float ang = DEG2RAD(s.rot + i * 120.f); // Use renamed helper
                sf::Vector2f point = { s.size * std::cos(ang), s.size * std::sin(ang) };
                tri.setPoint(i, point);
                centerSum += point;
            }
            tri.setOrigin(centerSum / 3.f);
            tri.setPosition(s.pos);
            tri.setFillColor(s.col);
            rt.draw(tri);
            break;
        }
        case Kind::Line: {
            sf::RectangleShape line({ s.size, 2.f }); // Slightly thicker line
            line.setOrigin({ s.size / 2.f, 1.f }); // Center origin
            line.setPosition(s.pos);
            line.setRotation(sf::degrees(s.rot)); // Use sf::degrees
            line.setFillColor(s.col);
            rt.draw(line);
            break;
        }
        }
    }
}

// Spawn method (made private)
void DecorLayer::m_spawn(sf::Vector2u win, const sf::Color& base, const sf::Color& accent1, const sf::Color& accent2) {
    Shape s;
    bool spawnLeft = randRange(0, 1) == 0;

    s.kind = static_cast<Kind>(randRange<int>(0, 2));
    float yPos = randRange<float>(0.f, static_cast<float>(win.y));
    float xPos = spawnLeft ? -randRange<float>(50.f, 200.f) : static_cast<float>(win.x) + randRange<float>(50.f, 200.f);
    s.pos = { xPos, yPos };
    s.vel = { (spawnLeft ? 1.f : -1.f) * randRange<float>(15.f, 45.f), 0.f };
    s.rot = randRange<float>(0.f, 360.f);

    switch (s.kind) {
    case Kind::Circle: // Use Base color
        s.size = randRange<float>(50.f, 120.f);
        s.col = sf::Color(base.r, base.g, base.b, randRange<int>(60, 100));
        break;
    case Kind::Triangle: // Use Accent1 color
        s.size = randRange<float>(40.f, 150.f);
        s.col = sf::Color(accent1.r, accent1.g, accent1.b, randRange<int>(30, 60));
        break;
    case Kind::Line: // Use Accent2 color
        s.size = randRange<float>(80.f, 180.f);
        s.col = sf::Color(accent2.r, accent2.g, accent2.b, randRange<int>(80, 140));
        break;
    }
    m_shapes.push_back(s); // Add to member vector
}