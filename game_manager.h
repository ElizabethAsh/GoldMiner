#pragma once
#include "sprite_manager.h"

namespace goldminer {

    class GameManager {
    public:
        void StartFullGame(SpriteID sprite1, SpriteID sprite2, float timePerPlayer);
        void InitPlayersAndRopes(SpriteID sprite1, SpriteID sprite2, float timePerPlayer);
        void MarkAllItemsForDestruction();
        void ResetGameState(float timePerPlayer);
        void StartNextLevel(float timePerPlayer);
        void EndCurrentLevel();

        int GetTotalScore(int playerId) const;
        int GetCurrentLevel() const;
        bool HasMoreLevels() const;
        int GetWinnerOverall() const;

    private:
        int currentLevel = 0;
        int totalScoreP1 = 0;
        int totalScoreP2 = 0;
        static constexpr int totalLevels = 3;

        void LoadLayout1();
        void LoadLayout2();
        void LoadLayout3();
        void UpdateTotalScores();
    };

}
