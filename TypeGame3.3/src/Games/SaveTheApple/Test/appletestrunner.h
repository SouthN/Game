/* ------------------------------------------------------------------
// 文件名     : appletestrunner.h
// 创建者     : [您的姓名/邮箱]
// 创建时间   : 2024-XX-XX
// 功能描述   : 拯救苹果自动化命令行测试运行器
------------------------------------------------------------------ */
#pragma once
#include <QObject>
#include <QString>

class AppleTestRunner : public QObject
{
    Q_OBJECT
public:
    explicit AppleTestRunner(QObject* parent = nullptr) : QObject(parent) {}
    ~AppleTestRunner() override = default;

    // 禁用拷贝
    AppleTestRunner(const AppleTestRunner&) = delete;
    AppleTestRunner& operator=(const AppleTestRunner&) = delete;

    // 执行测试的主入口
    static int runTest(const QString& inputJsonPath, const QString& outputJsonPath);
};