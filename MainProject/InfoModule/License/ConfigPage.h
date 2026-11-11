#ifndef CONFIGPAGE_H
#define CONFIGPAGE_H

#include <QWidget>
#include "istar_license.h"

using namespace ISTAR::BasicPlatform;

namespace Ui {
class ConfigPage;
}

enum Status
{
    Unexpired,
    GracePeriod,
    Expired
};

class ConfigPage : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigPage(QWidget *parent = nullptr);
    ~ConfigPage();

    void refreshTexts();

    void Authorize(); //授权

    bool CheckLicenseIsExpiration();

private:

    void Initialize();

    // 授权日期判断：date 为 "yyyy-MM-dd"（如 "2026-12-31"，与 ISTARLicense 返回的 ISO 日期一致）
    static Status CheckExpirationDate(const std::string &date);

    // 按授权状态切换界面行为；返回 false 表示拒绝启动（调用方需据此终止启动流程）
    //   GracePeriod: 在 window 上叠加斜向半透明水印「软件已到期，剩余X天，及时联系生产厂商」
    //   Expired    : 弹窗提示并拒绝启动
    // window  传主窗口（MainWindow::this，或本页所在顶层窗口 window()）
    // expireDate 为 "yyyy-MM-dd"（宽限期算“剩余X天”用）
    static bool ApplyLicenseStatus(Status status, QWidget *window = nullptr , std::string expireDate = "");

private:
    static constexpr int kGracePeriodDays = 30;  // 宽限期天数：过期后 30 天内可用

    Ui::ConfigPage *ui;

    ISTARLicense *verifier = nullptr;

    std::string guiExpirationDate;
    std::string mctrlExpirationDate;


};

#endif // CONFIGPAGE_H
