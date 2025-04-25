#pragma once

// RoundedRectangleShape.h

#ifndef ROUNDEDRECTANGLESHAPE_H
#define ROUNDEDRECTANGLESHAPE_H

#include <SFML/Graphics/Shape.hpp>
#include <SFML/System/Vector2.hpp>
#include <cmath>


class RoundedRectangleShape : public sf::Shape
{
public:
    RoundedRectangleShape(const sf::Vector2f& size = { 0, 0 }, float radius = 0, unsigned int cornerPointCount = 8)
    {
        m_size = size; m_cornerPointCount = std::max(2u, cornerPointCount); setRadius(radius); update();
    }
    void setSize(const sf::Vector2f& size) { m_size = size; setRadius(m_radius); update(); }
    const sf::Vector2f& getSize() const { return m_size; }
    void setRadius(float radius) { m_radius = std::max(0.f, std::min(radius, std::min(m_size.x / 2.f, m_size.y / 2.f))); update(); }
    float getRadius() const { return m_radius; }
    void setCornerPointCount(unsigned int count) { m_cornerPointCount = std::max(2u, count); update(); }
    virtual std::size_t getPointCount() const override { return m_cornerPointCount * 4; }
    virtual sf::Vector2f getPoint(std::size_t index) const override {
        if (getPointCount() == 0 || m_cornerPointCount < 2) return { 0, 0 };
        std::size_t cornerIndex = index / m_cornerPointCount; std::size_t pointInCornerIndex = index % m_cornerPointCount;
        float angle = 90.f * static_cast<float>(cornerIndex) + 90.f * (static_cast<float>(pointInCornerIndex) / (static_cast<float>(m_cornerPointCount) - 1.f));
        static const float pi = 3.141592654f; float radAngle = angle * pi / 180.f;
        sf::Vector2f cornerCenter;
        switch (cornerIndex) {
        case 0: cornerCenter = { m_size.x - m_radius, m_radius }; break; case 1: cornerCenter = { m_radius, m_radius }; break;
        case 2: cornerCenter = { m_radius, m_size.y - m_radius }; break; case 3: cornerCenter = { m_size.x - m_radius, m_size.y - m_radius }; break;
        default: return { 0, 0 };
        }
        return { cornerCenter.x + std::cos(radAngle) * m_radius, cornerCenter.y - std::sin(radAngle) * m_radius };
    }
private:
    sf::Vector2f m_size = { 0.f, 0.f };
    float m_radius = 0.f;
    unsigned int m_cornerPointCount = 8;

    void update() { Shape::update(); } 
};

#endif // ROUNDEDRECTANGLESHAPE_H