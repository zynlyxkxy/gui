#ifndef MCRECIPEMANAGER_H
#define MCRECIPEMANAGER_H

#include "MCRecipeModule/ReData/S_Recipe.h"            // S_ChipRecipe / S_RecipeContentWafer
#include "MCRecipeModule/McData/S_MachineConstant.h"   // S_MachineConstant（mcData() 的返回类型）

#include <QObject>
#include <QString>
#include <QStringList>

/**
 * MC + 处方数据管理单例：独占与 mctrl 的处方通讯，解析、缓存、变更判定、更新分发。
 * 纯数据服务，不含任何 UI（错误以字符串返回，由调用方弹窗）。
 *
 * 数据访问：chipRecipe() / waferRecipe() 返回成员的 const 引用，
 * ProcessBase 构造时持有这两个引用 —— 因此【契约】：
 *   下面两个数据成员只许原地赋值更新，禁止改成指针 / optional / 移动重建
 *   （否则所有流程手里的引用悬空）。
 *
 * 变更判定：selectRecipe 拉取的原始 body 与上次缓存比对，内容未变则
 * 不发 sigRecipeUpdated —— 同名重选不会触发流程界面刷新。
 *
 * 更新分发：处方变化时发一次 sigRecipeUpdated，各页收到后只刷自己的显示
 * （每页各刷一次，切页不再补刷）。【契约】订阅方不得在该信号的回调里发起
 * 任何查询或弹窗（如蓝膜快照）—— 那是"选中处方的那一页"自己的事，
 * 见各页 sig_recipeChosen 的处理。
 */
class MCRecipeManager : public QObject
{
    Q_OBJECT

public:
    static MCRecipeManager *instance();

    // 已放行处方名单（打开选择弹窗前拉取）
    bool releasedNameList(QStringList &out);

    // 选择处方：通知 mctrl → 拉取 → 解析 →（内容有变时）发 sigRecipeUpdated
    // 返回 false 时 err 为错误描述（英文 key，由调用方翻译展示）；内容未变返回 true 但不发信号
    bool selectRecipe(const QString &name, QString &err);

    // ---- 数据访问（只读引用，见类注释契约）----
    const S_ChipRecipe &chipRecipe() const;
    const S_RecipeContentWafer &waferRecipe() const;
    const QString &reName() const;        // 当前处方名字（预览 tab 用）
    const QString &reDescription() const;   // 当前处方描述（预览 tab 用）

    // ---- MC（机器常数）：结构体将来搬入，同款只读引用分发 ----
    const S_MachineConstant &mcData() const;

signals:
    void sigRecipeUpdated();   // 处方内容真正变化时（选同名/内容未变不发）
    void sigMcUpdated();

private:
    MCRecipeManager() = default;
    ~MCRecipeManager() override = default;
    MCRecipeManager(const MCRecipeManager &) = delete;
    MCRecipeManager &operator=(const MCRecipeManager &) = delete;

    // ⚠ 契约成员：流程类持有引用，只许原地赋值，禁止指针化/重建
    S_ChipRecipe m_chipRecipe;
    S_RecipeContentWafer m_waferRecipe;
    QString m_chipDescription;
    QString m_chipName;

    S_MachineConstant m_mc;

    // 变更判定：上次解析的原始 body（处方与机器常数各判各的）
    std::string m_lastWaferBody;
    std::string m_lastChipBody;
    std::string m_lastMcBody;
};

#endif // MCRECIPEMANAGER_H
