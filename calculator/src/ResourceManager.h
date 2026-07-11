#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <string>

class ResourceManager {
public:
    static ResourceManager& getInstance();
    const sf::Texture& getTexture(const std::string& filename);
    void clear();

    // Try loading a texture from multiple fallback paths.
    // Returns true on success, false if all paths failed.
    static bool loadTextureWithFallback(sf::Texture& tex, const std::string& relPath);

private:
    ResourceManager() = default;
    ~ResourceManager() = default;
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    std::unordered_map<std::string, sf::Texture> m_textures;
};