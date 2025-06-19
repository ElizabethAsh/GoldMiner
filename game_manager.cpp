#include "game_manager.h"
#include "gold_miner_ecs.h"
#include "bagel.h"

namespace goldminer {

    void GameManager::StartNewGame() {
        currentLevel = 0;
        totalScoreP1 = 0;
        totalScoreP2 = 0;
    }

    void GameManager::StartNextLevel() {
        if (currentLevel >= totalLevels) return;

        switch (currentLevel) {
            case 0: LoadLayout1(); break;
            case 1: LoadLayout2(); break;
            case 2: LoadLayout3(); break;
        }

        currentLevel++;
    }

    void GameManager::EndCurrentLevel() {
        UpdateTotalScores();
    }

    void GameManager::UpdateTotalScores() {
        for (bagel::id_type i = 0; i <= bagel::World::maxId().id; ++i) {
            bagel::ent_type ent{i};
            if (bagel::World::mask(ent).test(bagel::Component<Score>::Bit) &&
                bagel::World::mask(ent).test(bagel::Component<PlayerInfo>::Bit)) {
                auto& score = bagel::World::getComponent<Score>(ent);
                auto& info = bagel::World::getComponent<PlayerInfo>(ent);
                if (info.playerID == 1) totalScoreP1 += score.points;
                else if (info.playerID == 2) totalScoreP2 += score.points;
            }
        }
    }

    int GameManager::GetTotalScore(int playerId) const {
        return (playerId == 1) ? totalScoreP1 : totalScoreP2;
    }

    int GameManager::GetCurrentLevel() const {
        return currentLevel;
    }

    bool GameManager::HasMoreLevels() const {
        return currentLevel < totalLevels;
    }

    int GameManager::GetWinnerOverall() const {
        if (totalScoreP1 > totalScoreP2) return 1;
        if (totalScoreP2 > totalScoreP1) return 2;
        return 0; // tie
    }

}
