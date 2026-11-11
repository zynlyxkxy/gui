#include "ProcessBase.h"

#include <QtGlobal>
#include <QWidget>   // Q_UNUSED

namespace {

} // namespace

ProcessBase::ProcessBase(int id, QWidget *tabPage, Ui::ProcessPage *ui, QWidget *dialogParent)
    : ui(ui)
    , m_tabPage(tabPage)
    , m_dialogParent(dialogParent)
    , m_rcpChip(MCRecipeManager::instance()->chipRecipe())
    , m_rcpWafer(MCRecipeManager::instance()->waferRecipe())
    , m_mc(MCRecipeManager::instance()->mcData())
    , m_id(id)
{
}

ProcessBase::~ProcessBase() = default;

// 点表逐项比较（位级：同一段 JSON 文本解析出的值必然相同）
bool ProcessBase::samePoints(const std::vector<S_Point2D> &a, const std::vector<S_Point2D> &b)
{
    if (a.size() != b.size())
    {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (a[i].x_ != b[i].x_ || a[i].y_ != b[i].y_)
        {
            return false;
        }
    }
    return true;
}

void ProcessBase::setStatus(MctrlStatus status)
{
    switch (status)
    {
        case MCTRL_STATUS_IDLE:
        case MCTRL_STATUS_BUSY:
            setTabEnabled(true);
            break;

        default:
            setTabEnabled(false);
            break;
    }
}

void ProcessBase::clearData()
{
}

void ProcessBase::updateRecipe()
{
}

void ProcessBase::updateMc()
{
}

void ProcessBase::setConnect()
{
}

void ProcessBase::setTabEnabled(bool enabled)
{
    if (m_tabPage != nullptr)
    {
        m_tabPage->setEnabled(enabled);
    }
}

void ProcessBase::SetMctrlReporter(const std::function<void(int, const std::string &)> &reporter)
{
    m_mctrlReporter = reporter;
}

void ProcessBase::ReceiveMctrlCommand(const ProcessMessage &data)
{
}

