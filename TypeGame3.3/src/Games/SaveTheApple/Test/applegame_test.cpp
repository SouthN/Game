#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QApplication>   // 【修复1：新增 QApplication 头文件】
#include "applegamedata.h"
#include "applegameconfig.h"
#include "appleentity.h"
#include "entityfactory.h"

class SaveTheAppleTest : public testing::Test {
protected:
    void SetUp() override {
        config = AppleGameConfig::instance();
        config->resetStats();
        config->setLevel(1);
        config->setMaxEntityCount(3);
        config->setPassTarget(5);
        
        gameData = new AppleGameData(config);
    }

    void TearDown() override {
        delete gameData;
        AppleGameConfig::destroyInstance();
        EntityFactory::destroyInstance();
    }

    AppleGameConfig* config;
    AppleGameData* gameData;
};

// 验证用例 1：初始状态验证
TEST_F(SaveTheAppleTest, InitialStateCheck) {
    EXPECT_EQ(gameData->getGameState(), GameDataBase::Idle);
    EXPECT_EQ(gameData->getAliveApples().size(), 0);
    EXPECT_EQ(config->getSuccessCount(), 0);
}

// 验证内存池与实体状态流转
TEST_F(SaveTheAppleTest, EntityLifecycleAndPool) {
    AppleEntity* apple = EntityFactory::instance()->createEntity<AppleEntity>();
    apple->setLetter('X');
    gameData->addApple(apple);
    
    EXPECT_EQ(gameData->getAliveApples().size(), 1);
    EXPECT_TRUE(gameData->getExistLetters().contains('X'));
    
    // 模拟苹果越界标记死亡
    apple->setAlive(false);
    gameData->removeAppleWithoutDestroy(apple);
    
    EXPECT_EQ(gameData->getAliveApples().size(), 0);
    // 字母应该仍然被占用，直到动画彻底结束
    EXPECT_TRUE(gameData->getExistLetters().contains('X'));
    
    // 【修复2：正确模拟真实的回收链路】
    // 因为越界苹果已被移出 m_aliveApples，不能再调用 removeApple。
    // 我们必须像 AppleGameView::onAppleFadeOutFinished 那样，直接释放字母和实体
    gameData->removeExistLetter(apple->getLetter());
    EntityFactory::instance()->destroyEntity<AppleEntity>(apple);
    
    EXPECT_FALSE(gameData->getExistLetters().contains('X'));
}

// 【修复1核心：自定义 main 函数，丢弃 gtest_main】
int main(int argc, char **argv) {
    // 必须在所有测试用例执行之前，初始化 Qt GUI 环境
    QApplication app(argc, argv);
    
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}