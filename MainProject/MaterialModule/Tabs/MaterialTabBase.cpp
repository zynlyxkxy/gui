#include "MaterialTabBase.h"

#include <QWidget>

MaterialTabBase::MaterialTabBase(int id, QWidget *tabPage, Ui::MaterialPage *ui, QWidget *dialogParent)
    : ui(ui)
    , m_tabPage(tabPage)
    , m_dialogParent(dialogParent)
    , m_id(id)
    , m_rcpChip(MCRecipeManager::instance()->chipRecipe())
    , m_rcpWafer(MCRecipeManager::instance()->waferRecipe())
    , m_mc(MCRecipeManager::instance()->mcData())
{
}

MaterialTabBase::~MaterialTabBase() = default;

void MaterialTabBase::SetMctrlReporter(const std::function<void(int, const std::string &)> &reporter)
{
    m_mctrlReporter = reporter;
}

void MaterialTabBase::setStatus(MctrlStatus status)
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

void MaterialTabBase::clearData()
{
}

void MaterialTabBase::updateRecipe()
{
}

void MaterialTabBase::setConnect()
{
}

void MaterialTabBase::ReceiveMctrlCommand(const ProcessMessage &data)
{

}

void MaterialTabBase::setTabEnabled(bool enabled)
{
    if (m_tabPage != nullptr)
    {
        m_tabPage->setEnabled(enabled);
    }
}
