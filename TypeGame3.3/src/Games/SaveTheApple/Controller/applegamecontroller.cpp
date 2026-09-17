#include "applegamecontroller.h"
#include "entityfactory.h"
#include "audiomanager.h"

#include "backpackdialog.h" // 引入背包界面头文件

AppleGameController::AppleGameController(AppleGameData* data, AppleGameView* view, QObject* parent)
    : GameControllerBase(data, view, parent)
{
    initConnection(data, view);
}

void AppleGameController::initConnection(AppleGameData* data, AppleGameView* view)
{
    if (view) {
        connect(view, &AppleGameView::keyPressed, this, &GameControllerBase::handleKeyPress);
        connect(view, &AppleGameView::nextLevelClicked, this, &GameControllerBase::onNextLevelClicked);
        connect(view, &AppleGameView::restartGameClicked, this, &GameControllerBase::onRestartGameClicked);
        connect(view, &AppleGameView::exitGameClicked, this, &GameControllerBase::onExitGameClicked);
    }
}

// ---------------------------------------------------------
// 具体业务实现
// ---------------------------------------------------------

GameEntity* AppleGameController::createNewEntity() {
    return EntityFactory::instance()->createEntity<AppleEntity>();
}

void AppleGameController::updateViewEntities() {
    if (getGameView()) getGameView()->updateAllApples();
}

void AppleGameController::removeEntityFromView(GameEntity* entity) {
    if (getGameView()) getGameView()->removeAppleFromScene(static_cast<AppleEntity*>(entity));
}

void AppleGameController::clearSuccessIconsFromView() {
    if (getGameView()) getGameView()->clearSuccessAppleIcons();
}

void AppleGameController::playHitSound() {
    AudioManager::instance()->playEffect(AUDIO_EFFECT_APPLE_HIT);
}

void AppleGameController::playMissSound() {
    AudioManager::instance()->playEffect(AUDIO_EFFECT_APPLE_MISS);
}

int AppleGameController::getEntityWidth() const {
    return AppleEntity::APPLE_WIDTH;
}

int AppleGameController::getEntityHeight() const {
    return AppleEntity::APPLE_HEIGHT;
}