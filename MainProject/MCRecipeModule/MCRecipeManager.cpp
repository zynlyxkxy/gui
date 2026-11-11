#include "MCRecipeManager.h"

#include "RPCModule/MCTRLClientManager.h"
#include "istar_serializer.hpp"

MCRecipeManager *MCRecipeManager::instance()
{
    static MCRecipeManager s_instance;
    return &s_instance;
}

bool MCRecipeManager::releasedNameList(QStringList &out)
{
    MctrlModule::StringList recipesList;
    if (MCTRLClientManager::getInstance()->getClient()->GetWaferReleasedNameList(recipesList)
            != ISTAR_RESULT_SUCCESS)
    {
        return false;
    }

    out.reserve(int(recipesList.items.size()));
    for (const std::string &s : recipesList.items)
    {
        out.append(QString::fromStdString(s));
    }
    return true;
}

bool MCRecipeManager::selectRecipe(const QString &name, QString &err)
{
    err.clear();

    // 通知 mctrl 当前选择的处方
    // 注意：SetWaferReleasedName 的形参是非 const 引用（std::string &），
    // 不能直接传 name.toStdString() 这样的临时对象，必须落到具名变量上
    std::string recipeName = name.toStdString();
    if (MCTRLClientManager::getInstance()->getClient()->SetWaferReleasedName(recipeName)
            != ISTAR_RESULT_SUCCESS)
    {
        err = QStringLiteral("Wafer recipe load failed!");
        return false;
    }

    // 拉取结构化处方
    MctrlModule::RcpInfoList r;
    if (MCTRLClientManager::getInstance()->getClient()->GetCurRecipeInfo(r) != ISTAR_RESULT_SUCCESS)
    {
        err = QStringLiteral("Chip recipe load failed!");
        return false;
    }

    if (r.waferInfo.items.empty())
    {
        err = QStringLiteral("Wafer recipe info is empty!");
        return false;
    }
    if (r.chipInfo.items.empty())
    {
        err = QStringLiteral("Chip recipe info is empty!");
        return false;
    }
    m_chipName = QString::fromStdString(recipeName);

    MctrlModule::MCConstant mc;
    if (MCTRLClientManager::getInstance()->getClient()->GetCurMCInfo(mc) != ISTAR_RESULT_SUCCESS)
    {
        err = QStringLiteral("MC load failed!");
        return false;
    }

    const std::string &waferBody = r.waferInfo.items[0].body;
    const std::string &chipBody = r.chipInfo.items[0].body;
    const std::string &mcBody = mc.body;

    // 处方：内容真正变化才原地更新并广播一次（同名重选 / mctrl 未换版本 → 不刷）。
    // 广播只表示"处方数据变了"，各页收到后只许刷自己的显示，不得借此发起查询/弹窗。
    if (waferBody != m_lastWaferBody || chipBody != m_lastChipBody)
    {
        // 解析并原地更新（契约：只许原地赋值，引用持有者自动看到新数据）
        m_waferRecipe = ISTAR::Serializer::from_rawJson<S_RecipeContentWafer>(waferBody);
        m_chipRecipe = ISTAR::Serializer::from_rawJson<S_ChipRecipe>(chipBody);
        m_chipDescription = QString::fromStdString(r.waferInfo.items[0].header.description);
        m_lastWaferBody = waferBody;
        m_lastChipBody = chipBody;

        emit sigRecipeUpdated();
    }

    // 机器常数：与处方无关，保持"内容变化才更新并通知"
    if (mcBody != m_lastMcBody)
    {
        m_mc = ISTAR::Serializer::from_rawJson<S_MachineConstant>(mcBody);
        m_lastMcBody = mcBody;

        emit sigMcUpdated();
    }
    return true;
}

const S_ChipRecipe &MCRecipeManager::chipRecipe() const
{
    return m_chipRecipe;
}

const S_RecipeContentWafer &MCRecipeManager::waferRecipe() const
{
    return m_waferRecipe;
}

const QString &MCRecipeManager::reName() const
{
    return m_chipName;
}

const QString &MCRecipeManager::reDescription() const
{
    return m_chipDescription;
}

const S_MachineConstant &MCRecipeManager::mcData() const
{
    return m_mc;
}
