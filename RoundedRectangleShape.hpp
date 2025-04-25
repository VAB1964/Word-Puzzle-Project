#ifndef ROUNDEDRECTANGLESHAPE_HPP
#define ROUNDEDRECTANGLESHAPE_HPP

#include <SFML/Graphics.hpp>
#include <cmath>

/*
    RoundedRectangleShape – SFML 3.0 helper
    ---------------------------------------
    • size   : total width / height
    • radius : corner radius (pixels)
    • points : how many points per corner arc (? 2)
*/
class RoundedRectangleShape : public sf::Shape
{
public:
    explicit RoundedRectangleShape(const sf::Vector2f& size = { 0.f, 0.f },
        float radius = 0.f,
        std::size_t cornerPts = 8)
        : m_size(size), m_radius(radius), m_cornerPts(cornerPts)
    {
        update();            // build the point array
    }

    // setters ----------------------------------------------------
    void setSize(const sf::Vector2f& size) { m_size = size;   update(); }
    void setCornersRadius(float radius) { m_radius = radius; update(); }
    void setCornerPointCount(std::size_t count) { m_cornerPts = count; update(); }

    // sf::Shape overrides ---------------------------------------
    virtual std::size_t getPointCount() const override
    {
        return m_cornerPts * 4;                  // 4 corners
    }

    // ------------------------------------------------------------------
    //  CCW?convex point generator – works for all four corners
    // ------------------------------------------------------------------
    virtual sf::Vector2f getPoint(std::size_t index) const override
    {
        const std::size_t cp = std::max<std::size_t>(2, m_cornerPts);  // ?2
        const std::size_t corner = index / cp;   // 0 TL, 1 TR, 2 BR, 3 BL
        const std::size_t i = index % cp;   // point on the 90° arc

        // Progress angle 0?90 deg, clockwise from the *corner’s* inside edge
        const float step = 90.f / (cp - 1);
        float deg;
        switch (corner)
        {
        case 0: deg = 180.f + i * step;             break; // TL: 180 ? 270
        case 1: deg = 270.f + i * step;             break; // TR: 270 ? 360
        case 2: deg = 0.f + i * step;             break; // BR:   0 ?  90
        default:deg = 90.f + i * step;             break; // BL:  90 ? 180
        }
        float rad = deg * 3.14159265f / 180.f;

        // Corner centre
        float cx = (corner == 0 || corner == 3) ? m_radius
            : m_size.x - m_radius;
        float cy = (corner <= 1) ? m_radius
            : m_size.y - m_radius;

        return { cx + std::cos(rad) * m_radius,
                 cy + std::sin(rad) * m_radius };
    }

private:
    sf::Vector2f m_size;
    float        m_radius;
    std::size_t  m_cornerPts;
};

#endif // ROUNDEDRECTANGLESHAPE_HPP
