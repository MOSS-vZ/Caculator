#pragma once
#include <SFML/Graphics.hpp>

class Button {
public:
    Button(const sf::Texture& normalTex, float x, float y);

    void setPosition(float x, float y);
    void setScale(float scaleX, float scaleY);
    void setColor(const sf::Color& color);   // 新增：控制透明度/颜色
    sf::Vector2f getPosition() const;        // 新增：获取位置
    bool isHovered(const sf::Vector2f& mousePos) const;
    bool isClicked(const sf::Vector2f& mousePos, bool mousePressed) const;
    void draw(sf::RenderWindow& window) const;
    sf::FloatRect getGlobalBounds() const;
    const sf::Sprite& getSprite() const { return m_sprite; }
private:
    sf::Sprite m_sprite;
};