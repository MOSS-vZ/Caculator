#include "Button.h"

Button::Button(const sf::Texture& normalTex, float x, float y)
    : m_sprite(normalTex) {
    m_sprite.setPosition({ x, y });
}

void Button::setPosition(float x, float y) {
    m_sprite.setPosition({ x, y });
}

void Button::setScale(float scaleX, float scaleY) {
    m_sprite.setScale({ scaleX, scaleY });
}

void Button::setColor(const sf::Color& color) {
    m_sprite.setColor(color);   // SFML 3.x 使用 setColor
}

sf::Vector2f Button::getPosition() const {
    return m_sprite.getPosition();
}

bool Button::isHovered(const sf::Vector2f& mousePos) const {
    return m_sprite.getGlobalBounds().contains(mousePos);
}

bool Button::isClicked(const sf::Vector2f& mousePos, bool mousePressed) const {
    return mousePressed && isHovered(mousePos);
}

void Button::draw(sf::RenderWindow& window) const {
    window.draw(m_sprite);
}

sf::FloatRect Button::getGlobalBounds() const {
    return m_sprite.getGlobalBounds();
}