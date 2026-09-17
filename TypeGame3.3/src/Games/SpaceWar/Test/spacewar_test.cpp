#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QApplication>
#include "planegamedata.h"
#include "planegamecontroller.h"
#include "planegameconfig.h"
#include "planeentity.h"
#include "playerentity.h"
#include "machinegunbulletentity.h"
#include "entityfactory.h"

class SpaceWarTest : public testing::Test {
protected:
    void SetUp() override {
        config = PlaneGameConfig::instance();
        config->resetStats();
        config->setLevel(1);
        config->setMaxEntityCount(5);
        config->setPassTarget(10);

        gameData = new PlaneGameData(config);
    }

    void TearDown() override {
        delete gameData;
        PlaneGameConfig::destroyInstance();
        EntityFactory::destroyInstance();
    }

    PlaneGameConfig* config;
    PlaneGameData* gameData;
};

// 验证用例 1：玩家生命值扣减与恢复系统
TEST_F(SpaceWarTest, PlayerHealthSystem) {
    PlayerEntity* player = new PlayerEntity();
    player->initHealth(player->attributes()->maxHp());

    EXPECT_EQ(player->getHp(), player->attributes()->maxHp());

    // 模拟受到伤害
    bool isDead = player->takeDamage(5);
    EXPECT_FALSE(isDead);
    EXPECT_EQ(player->getHp(), player->attributes()->maxHp() - 5);
    // 模拟治疗 (超出上限应截断)
    player->heal(10);
    EXPECT_EQ(player->getHp(), player->attributes()->maxHp());

    // 模拟致命伤害
    isDead = player->takeDamage(20);
    EXPECT_TRUE(isDead);
    EXPECT_EQ(player->getHp(), 0);

    delete player;
}

// 验证用例 2：敌机状态与内存池管理
TEST_F(SpaceWarTest, EnemyPlaneLifecycle) {
    PlaneEntity* plane = EntityFactory::instance()->createEntity<PlaneEntity>();
    plane->setLetter('A');
    gameData->addPlane(plane);

    EXPECT_EQ(gameData->getAlivePlanes().size(), 1);
    EXPECT_TRUE(gameData->getExistLetters().contains('A'));

    // 模拟击中敌机逻辑
    bool destroyed = plane->takeDamage(3); // 默认 3 HP
    EXPECT_TRUE(destroyed);

    // 从数据层剥离
    gameData->removePlaneWithoutDestroy(plane);
    EXPECT_EQ(gameData->getAlivePlanes().size(), 0);

    // 释放字母并归还内存池
    gameData->removeExistLetter(plane->getLetter());
    EntityFactory::instance()->destroyEntity<PlaneEntity>(plane);

    EXPECT_FALSE(gameData->getExistLetters().contains('A'));
}

// 验证用例 3：机炮子弹伤害来自玩家属性组件
TEST_F(SpaceWarTest, MachineGunBulletUsesPlayerAttributeDamage) {
    PlayerEntity player;
    player.attributes()->setCritRate(0.0f);
    player.attributes()->setMachineGunDamage(3.0f);

    MachineGunBulletEntity bullet;
    bool isCrit = false;
    float damage = player.attributes()->calculateOutputDamage(
        player.attributes()->machineGunDamage(),
        isCrit
    );
    bullet.setDamage(damage);

    PlaneEntity plane;
    EXPECT_EQ(bullet.getDamage(), 3.0f);
    EXPECT_TRUE(plane.takeDamage(bullet.getDamage(), &player));
    EXPECT_EQ(plane.getHp(), 0);
}

// 验证用例 4：分数统计系统
TEST_F(SpaceWarTest, ScoreStatistics) {
    config->addScore(100);
    config->addScore(250);
    EXPECT_EQ(config->getScore(), 350);

    config->addSuccessCount();
    config->addSuccessCount();
    EXPECT_EQ(config->getSuccessCount(), 2);

    // 1. 重置基础统计信息 (击毁数/逃脱数等)
    config->resetStats();
    // 2. 重置太空大战专属的分数统计
    config->resetScore();

    // 重新断言，此时应该双双归零
    EXPECT_EQ(config->getScore(), 0);
    EXPECT_EQ(config->getSuccessCount(), 0);
}

// 验证用例 5：跨关卡时清理 Boss/动画残留字母，避免字母池卡住下一关刷怪
TEST_F(SpaceWarTest, NextLevelClearsResidualLetters) {
    config->setLevel(2);
    gameData->addExistLetter('A');
    gameData->addExistLetter('B');
    gameData->addExistLetter('C');

    PlaneGameController controller(gameData, nullptr);
    controller.setTestMode(true);
    controller.onNextLevelClicked();

    EXPECT_EQ(config->getLevel(), 3);
    EXPECT_TRUE(gameData->getExistLetters().isEmpty());
    EXPECT_EQ(gameData->getGameState(), GameDataBase::Playing);
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
