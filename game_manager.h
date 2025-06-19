#pragma once

namespace goldminer {

    class GameManager {
    public:
        void StartNewGame();
        void StartNextLevel();
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

        void UpdateTotalScores();
    };

}
