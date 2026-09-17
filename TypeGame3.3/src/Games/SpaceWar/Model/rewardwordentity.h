#pragma once
#include "gameentity.h"
#include <QString>
#include <QFont>

class RewardWordEntity : public GameEntity
{
    Q_OBJECT
public:
    RewardWordEntity();
    ~RewardWordEntity() override = default;

    void paint(QPainter* painter) override;
    void destroy() override;
    void updateLogic(float deltaTime = 0.0f) override;

	void setWord(const QString& word); // 设置奖励单词
    QString getWord() const { return m_word; }

    // 检查按键是否匹配下一个字母
    bool checkNextLetter(QChar c);
    bool isCompleted() const;

private:
    QString m_word;
    int m_typedIndex = 0; // 当前已正确输入的字母数量
    float m_speed = 100.0f; // 横向飞过屏幕的速度
};