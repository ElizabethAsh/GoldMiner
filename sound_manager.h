#pragma once
#include <SFML/Audio.hpp>
#include <string>
#include <map>
#include <vector>
#include <memory>

class SoundManager {
public:
    SoundManager();
    ~SoundManager();

    bool loadSound(const std::string& name, const std::string& filePath);
    void playSound(const std::string& name);
    void playSoundLimited(const std::string& name, sf::Time duration);
    void stopAllSounds();
    void update();

private:
    std::map<std::string, sf::SoundBuffer> m_soundBuffers;
    std::unique_ptr<sf::Sound> m_currentSound;
    sf::Clock m_currentSoundClock;
    sf::Time m_currentSoundLimit;
};