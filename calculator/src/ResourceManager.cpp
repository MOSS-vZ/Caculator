#include "ResourceManager.h"
#include <iostream>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

ResourceManager& ResourceManager::getInstance() {
    static ResourceManager instance;
    return instance;
}

const sf::Texture& ResourceManager::getTexture(const std::string& filename) {
    auto it = m_textures.find(filename);
    if (it != m_textures.end())
        return it->second;

    sf::Texture texture;
    std::string fullPath = "assets/images/mainscreen/" + filename;
    if (!texture.loadFromFile(fullPath))
        std::cerr << "Failed to load: " << fullPath << std::endl;

    auto result = m_textures.emplace(filename, std::move(texture));
    return result.first->second;
}

void ResourceManager::clear() {
    m_textures.clear();
}

bool ResourceManager::loadTextureWithFallback(sf::Texture& tex, const std::string& relPath) {
    std::vector<std::string> paths = {
        "assets/images/" + relPath,
        "../assets/images/" + relPath,
        "images/" + relPath,
        relPath,
        fs::current_path().string() + "/assets/images/" + relPath
    };
    for (const auto& p : paths) {
        if (tex.loadFromFile(p)) return true;
    }
    std::cerr << "Failed to load: " << relPath << std::endl;
    return false;
}