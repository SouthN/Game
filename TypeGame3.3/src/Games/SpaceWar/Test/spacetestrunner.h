/* ------------------------------------------------------------------
// 文件名     : spacetestrunner.h
// 功能描述   : 太空大战自动化命令行测试运行器
------------------------------------------------------------------ */
#pragma once
#include <QObject>
#include <QString>

class SpaceTestRunner : public QObject
{
    Q_OBJECT
public:
    explicit SpaceTestRunner(QObject* parent = nullptr) : QObject(parent) {}
    ~SpaceTestRunner() override = default;

    // 禁用拷贝
    SpaceTestRunner(const SpaceTestRunner&) = delete;
    SpaceTestRunner& operator=(const SpaceTestRunner&) = delete;

    // 执行测试的主入口
    static int runTest(const QString& inputJsonPath, const QString& outputJsonPath);
};