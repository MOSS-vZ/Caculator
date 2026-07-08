#include "ResourceManager.h"
#include <iostream>

ResourceManager& ResourceManager::getInstance() {
    static ResourceManager instance;
    return instance;
}

const sf::Texture& ResourceManager::getTexture(const std::string& filename) {
    auto it = m_textures.find(filename);
    if (it != m_textures.end())
        return it->second;

    sf::Texture texture;
    std::string fullPath = "images/mainscreen/" + filename;
    if (!texture.loadFromFile(fullPath))
        std::cerr << "Failed to load: " << fullPath << std::endl;

    auto result = m_textures.emplace(filename, std::move(texture));
    return result.first->second;
}

void ResourceManager::clear() {
    m_textures.clear();
}