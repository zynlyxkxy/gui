#include "ConfigPage.h"
#include "ui_ConfigPage.h"

#include "istar_ui_stylesheet.h"
#include "dialog/messagedialog.h"
#include "istar_language.h"
#include "istar_encryption.h"
#include "istar_filesystem.h"
#include "LicenseWatermark.h"
#include "Common/ConfigRoot.h"

#include <QDate>
#include <chrono>

#define TR_STR(msg)  QString::fromStdString(Tr(msg))

namespace
{
    /// 授权文件目录（相对配置根）。改目录只改这一处。
    const char *const kLicenseDir = "gui_config/license_data";
}

using namespace ISTAR;
using namespace ISTAR::GUI;
using namespace ISTAR::UIStyle;

ConfigPage::ConfigPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ConfigPage)
{
    ui->setupUi(this);

    Initialize();


    ui->gb_license->SetheaderFontSize(16);
    ui->lbl_LicenseDate_MCtrl->setStyleSheet(iStarUIStyleSheet->fontStyle_normal_12);
    ui->led_LicenseDate_MCtrl->setStyleSheet(iStarUIStyleSheet->LineEdit_Dark + iStarUIStyleSheet->fontStyle_normal_12);
    ui->lbl_LicenseDate_GUI->setStyleSheet(iStarUIStyleSheet->fontStyle_normal_12);
    ui->led_LicenseDate_GUI->setStyleSheet(iStarUIStyleSheet->LineEdit_Dark +iStarUIStyleSheet->fontStyle_normal_12);

    refreshTexts();
}

ConfigPage::~ConfigPage()
{
    delete ui;
}

void ConfigPage::Initialize()
{
    verifier = ISTARLicense::getInstance();
    // 主目录 + 备份目录（备份目录可为空 = 不启用备份）；目录固定为 <配置根>/<kLicenseDir>
    verifier->Initialize(ConfigRoot::Under(kLicenseDir).toStdString());
}

void ConfigPage::Authorize()
{

    // 1. 采集设备指纹
    DeviceID deviceID = verifier->GetDeviceID();

    // 2. 模拟供应商签发授权文件（正常流程由供应商工具完成）
    //    文件名用全数字时间戳（与客户授权文件命名一致），文件名第一段即解密密钥
    // "Permanent" / "yyyy-MM-dd"
    std::string plaintext = deviceID.toJson() + "#2026-09-01";
    std::string key =  std::to_string(
                       std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch()).count()); // 默认取当前 Unix 秒时间戳
    std::string encrypted = Encryption::EncodedText(plaintext, key);  // Base64 密文
    FileSystem::Write_CRC(ConfigRoot::Under(kLicenseDir).toStdString(), key + ".db", encrypted);  // 带 CRC 尾部

}

bool ConfigPage::CheckLicenseIsExpiration()
{
    if (verifier->IsFolderEmpty())
    {
        if (verifier->WriteDeviceInfoToFile())
        {
            // 请求文件已生成，等待供应商签发授权文件
            MessageDialog dlg(MessageDialog::Attention,this, TR_STR("没有授权文件，请求文件已生成"));
        }
        return true;
    }

    // 3. 校验授权
    if (verifier->CheckAuthorization())
    {

        // 返回"Permanent" / "yyyy-MM-dd" / "Not authorized"
        std::string GUIExpirationDate = verifier->GetLicenseExpirationDate();

        if(GUIExpirationDate == "Permanent")
        {
            ui->led_LicenseDate_MCtrl->setText(QString::fromStdString(GUIExpirationDate));
            ui->led_LicenseDate_GUI->setText(QString::fromStdString(GUIExpirationDate)); 
        }
        else
        {
            ui->led_LicenseDate_MCtrl->setText(QString::fromStdString(GUIExpirationDate));
            ui->led_LicenseDate_GUI->setText(QString::fromStdString(GUIExpirationDate));

        }
        return false;

    }
    else  //过期/未授权
    {
        std::string GUIExpirationDate = verifier->GetLicenseExpirationDate();

        if(GUIExpirationDate == "Not authorized")
        {
            //关闭
            ConfigPage::ApplyLicenseStatus(Expired, this);
            return true;
        }

        switch(CheckExpirationDate(GUIExpirationDate))
        {
            case GracePeriod:
            {
                if(ConfigPage::ApplyLicenseStatus(GracePeriod, this, GUIExpirationDate))
                {
                    ui->led_LicenseDate_MCtrl->setText(QString::fromStdString(GUIExpirationDate));
                    ui->led_LicenseDate_GUI->setText(QString::fromStdString(GUIExpirationDate));
                }
                return false;
            }
            break;

            case Expired:
            {
                ConfigPage::ApplyLicenseStatus(Expired, this);
                return true;
            }
            break;
        }

        ConfigPage::ApplyLicenseStatus(Expired, this);
        return true;
    }

}

namespace
{
    // 解析 "yyyy-MM-dd"（兼容 "yyyy-MM-ddTHH:mm:ss" 这类 ISO 串，只取日期部分）
    // 解析失败（空串、格式不对、不存在的日期如 2026-02-30）返回无效 QDate
    QDate ParseYmdDate(const std::string &date)
    {
        const QString text = QString::fromStdString(date).trimmed();
        if (text.isEmpty())
        {
            return QDate();
        }

        QDate result = QDate::fromString(text, QStringLiteral("yyyy-MM-dd"));
        if (!result.isValid())
        {
            result = QDate::fromString(text.left(10), Qt::ISODate);
        }
        return result;
    }
}  // namespace


bool ConfigPage::ApplyLicenseStatus(Status status, QWidget *window, std::string expireDate)
{
    // 水印与弹窗都作用在顶层窗口上：调用方传 this（本页）或 window()（主窗口）结果一致
    QWidget *host = window ? window->window() : nullptr;

    switch (status)
    {
        case Unexpired:
            // 未到期：正常显示，不做任何附加操作
            return true;

        case GracePeriod:
        {
            int daysLeft = 0;
            const QDate expire = ParseYmdDate(expireDate);
            if (expire.isValid())
            {
                // daysTo() 返回 qint64，与 int 混用会让 qMax 模板推导失败(int vs long long)，
                // 故先用 qint64 统一计算，再收敛回 int
                const qint64 daysSinceExpire = expire.daysTo(QDate::currentDate());
                const qint64 remain = static_cast<qint64>(kGracePeriodDays) - daysSinceExpire;
                daysLeft = static_cast<int>(qMax<qint64>(0, remain));
            }

            // 宽限期：在窗口上叠加斜向半透明水印
            // AttachTo 内部会复用已存在的水印并更新文字，重复调用不会叠成多层
            LicenseWatermark::AttachTo(host, TR_STR("软件已到期，剩余%1天，及时联系生产厂商").arg(daysLeft));
            return true;
        }

        case Expired:   // 传入异常状态时也按彻底过期处理，避免漏掉分支
        {
            // 彻底过期：提示 + 拒绝启动
            if (host)
            {
                MessageDialog dlg(MessageDialog::Error, host, TR_STR("请联系生产厂商获取授权"));
                dlg.exec();
            }
            return false;
        }
    }
}


Status ConfigPage::CheckExpirationDate(const std::string &date)
{

    const QDate expireDate = ParseYmdDate(date);

    // 日期非法时按彻底过期处理，避免错误数据反而放行授权
    if (!expireDate.isValid())
    {
        return Expired;
    }

    const QDate today = QDate::currentDate();  // 当前系统日期（本地时区）

    // 1) 未超过当前日期（含当天到期）→ 未过期
    if (expireDate >= today)
    {
        return Unexpired;
    }

    // 2) 已过期：距今天数在 30 天以内（含第 30 天）为宽限期，超过 30 天彻底过期
    const int overdueDays = expireDate.daysTo(today);
    return (overdueDays <= kGracePeriodDays) ? GracePeriod : Expired;
}

void ConfigPage::refreshTexts()
{
    ui->gb_license->SetHeaderText(TR_STR("License"));
    ui->lbl_LicenseDate_MCtrl->setText(TR_STR("MCtrl Expiration Date"));
    ui->lbl_LicenseDate_GUI->setText(TR_STR("GUI Expiration Date"));
}
