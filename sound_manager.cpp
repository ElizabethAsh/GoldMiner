#include "sound_manager.h"
#include <iostream>
#include <algorithm> // For std::remove_if
#include <memory>
   // For std::unique_ptr and std::make_unique

SoundManager::SoundManager() {

    loadSound("coin", "res/sounds/coin.wav");
    loadSound("collision", "res/sounds/collision.wav");
    loadSound("stretch", "res/sounds/stretch.wav");
    loadSound("stretch", "res/sounds/stretch.wav");
    loadSound("ticking-clock", "res/sounds/ticking-clock.wav");
    loadSound("small_coins", "res/sounds/small_coins.wav");


}

SoundManager::~SoundManager() {
}

bool SoundManager::loadSound(const std::string& name, const std::string& filePath) {
    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile(filePath)) {
        std::cerr << "שגיאה: קובץ סאונד לא נטען: " << filePath << std::endl;
        return false;
    }
    m_soundBuffers[name] = std::move(buffer);
    return true;
}

void SoundManager::playSoundLimited(const std::string& name, sf::Time duration) {
    auto it = m_soundBuffers.find(name);
    if (it != m_soundBuffers.end()) {
        if (m_currentSound && m_currentSound->getStatus() == sf::SoundSource::Status::Playing) {
            m_currentSound->stop();
        }

        m_currentSound = std::make_unique<sf::Sound>(it->second);
        m_currentSound->play();
        m_currentSoundClock.restart(); // Start timing
        m_currentSoundLimit = duration; // Set the limit
    } else {
        std::cerr << "שגיאה: סאונד '" << name << "' לא נמצא במנהל הסאונדים.\n";
    }
}


void SoundManager::playSound(const std::string& name) {
    auto it = m_soundBuffers.find(name);
    if (it != m_soundBuffers.end()) {
        // Stop any currently playing sound first
        if (m_currentSound && m_currentSound->getStatus() == sf::SoundSource::Status::Playing) {
            m_currentSound->stop();
        }

        // Create the new sound and assign it to m_currentSound
        m_currentSound = std::make_unique<sf::Sound>(it->second);
        m_currentSound->play();
    } else {
        std::cerr << "שגיאה: סאונד '" << name << "' לא נמצא במנהל הסאונדים.\n";
    }
}

void SoundManager::stopAllSounds() {
    if (m_currentSound) { // Check if there's a sound object
        m_currentSound->stop();
        m_currentSound.reset(); // Release the unique_ptr, effectively deleting the sound
    }
}

void SoundManager::update() {
    if (m_currentSound && m_currentSoundLimit != sf::Time::Zero) { // If a sound is playing and has a limit
        if (m_currentSoundClock.getElapsedTime() >= m_currentSoundLimit) {
            m_currentSound->stop();
            m_currentSound.reset(); // Stop and release the sound
            m_currentSoundLimit = sf::Time::Zero; // Reset limit
            std::cout << "DEBUG: Sound stopped due to time limit.\n";
        }
    }
}