#include "game_manager.h"
#include "gold_miner_ecs.h"
#include "bagel.h"

namespace goldminer {

    void GameManager::StartFullGame(SpriteID sprite1, SpriteID sprite2, float timePerPlayer) {
        currentLevel = 0;
        totalScoreP1 = 0;
        totalScoreP2 = 0;

        InitPlayersAndRopes(sprite1, sprite2, timePerPlayer);
        StartNextLevel(timePerPlayer);
    }

    void GameManager::InitPlayersAndRopes(SpriteID sprite1, SpriteID sprite2, float timePerPlayer)
    {
        goldminer::CreatePlayer(1, sprite1, timePerPlayer);
        goldminer::CreatePlayer(2, sprite2, timePerPlayer);
        goldminer::CreateRope(1);
        goldminer::CreateRope(2);
        goldminer::CreateUIEntity(1);
        goldminer::CreateUIEntity(2);
    }

    void GameManager::MarkAllItemsForDestruction() {
        using namespace bagel;
        Mask mask = MaskBuilder{}.set<ItemType>().build();

        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type ent{id};
            if (World::mask(ent).test(mask)) {
                World::addComponent<DestroyTag>(ent, {});
            }
        }
    }

    void GameManager::ResetGameState(float timePerPlayer) {
        using namespace bagel;

        Mask scoreMask = MaskBuilder{}.set<Score>().set<PlayerInfo>().build();
        Mask timerMask = MaskBuilder{}.set<GameTimer>().set<PlayerInfo>().build();
        Mask ropeMask = MaskBuilder{}.set<RopeControl>().set<Length>().build();

        for (id_type id = 0; id <= World::maxId().id; ++id) {
            ent_type ent{id};

            if (World::mask(ent).test(scoreMask)) {
                World::getComponent<Score>(ent).points = 0;
            }
            if (World::mask(ent).test(timerMask)) {
                World::getComponent<GameTimer>(ent).timeLeft = timePerPlayer;
            }
            if (World::mask(ent).test(ropeMask)) {
                World::getComponent<RopeControl>(ent).state = RopeControl::State::AtRest;
                World::getComponent<Length>(ent).value = 0.0f;
            }
        }
    }

    void GameManager::StartNextLevel(float timePerPlayer) {
        if (currentLevel >= totalLevels) return;

        MarkAllItemsForDestruction();
        DestructionSystem();
        ResetGameState(timePerPlayer);

        switch (currentLevel) {
            case 0: LoadLayout1(); break;
            case 1: LoadLayout2(); break;
            case 2: LoadLayout3(); break;
        }

        bagel::Entity e = bagel::Entity::create();
        SpriteID bgID = static_cast<SpriteID>(SPRITE_BACKGROUND_LEVEL1 + currentLevel);
        e.add(LevelInfo{bgID});

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


    void GameManager::LoadLayout1() {
        CreateGold(100.0f, 500.0f);
        CreateDiamond(500.0f, 520.0f);
        CreateDiamond(650.0f, 400.0f);
        CreateRock(900.0f, 530.0f);
        CreateGold(1000.0f, 350.0f);
        CreateTreasureChest(300.0f, 510.0f);
        CreateGold(300.0f, 350.0f);
    }

    void GameManager::LoadLayout2() {
        CreateDiamond(100.0f, 500.0f);
        CreateRock(500.0f, 520.0f);
        CreateTreasureChest(650.0f, 400.0f);
        CreateGold(900.0f, 530.0f);
        CreateGold(300.0f, 350.0f);
        CreateRock(1000.0f, 350.0f);
        CreateTreasureChest(300.0f, 400.0f);
    }

    void GameManager::LoadLayout3() {
        CreateGold(150.0f, 500.0f);
        CreateRock(300.0f, 520.0f);
        CreateDiamond(750.0f, 540.0f);
        CreateTreasureChest(1000.0f, 550.0f);
        CreateGold(300.0f, 350.0f);
        CreateGold(1000.0f, 350.0f);
        CreateRock(200.0f, 400.0f);
        CreateTreasureChest(500.0f, 550.0f);
        CreateDiamond(600.0f, 350.0f);
    }


}
