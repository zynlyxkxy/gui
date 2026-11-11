#include "UpdatePage.h"
#include "PreviewPage.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QWebChannel>
#include "istar_log.h"
#include "istar_appconfig.h"

UpdatePage::UpdatePage(QString name, QWidget *parent) :
    FrameworkWidget(name, parent)
{
    const auto &config = ISTAR::BasicPlatform::ISTARAppConfigManager::getInstance()->GetAppConfig();
    QString dataPath = QString::fromStdString(config.appDataPath);

    m_filePath  = dataPath + "UpdateContentMarkDown/update.md";

    QHBoxLayout *MainLayout = new QHBoxLayout(this);

    QWebEngineView * preview = addUpdateContent(m_filePath, m_content);

    MainLayout->addWidget(preview);
}


UpdatePage::~UpdatePage()
{

}

QWebEngineView *UpdatePage::addUpdateContent(QString updateContentFilePath, Document& m_content)
{
    QWebEngineView *preview = new QWebEngineView(this);
    preview->setContextMenuPolicy(Qt::NoContextMenu);

    PreviewPage *page = new PreviewPage(this);
    preview->setPage(page);

    QWebChannel *channel = new QWebChannel(this);
    channel->registerObject(QStringLiteral("content"), &m_content);
    page->setWebChannel(channel);

    preview->setUrl(QUrl("qrc:/new/index.html"));

    // 1. 确保目标目录存在
    QString dirPath = QFileInfo(updateContentFilePath).path();
    QDir dir(dirPath);
    if (!dir.exists())
    {
        if (!dir.mkdir(dirPath))
        {
            // 处理无法创建目录的情况，例如显示错误信息或记录日志

            ISTARLogDebug("%s", QStringLiteral("创建配置文件失败").toUtf8().constData());
        }
    }

    // 2. 读取markdown文件
    //QFile defaultTextFile(":/default.md");
    if (QFileInfo::exists(updateContentFilePath))
    {
        QFile defaultTextFile(updateContentFilePath);
        defaultTextFile.open(QIODevice::ReadOnly);
        m_content.setText(defaultTextFile.readAll());
    }else
    {
        ISTARLogDebug("The MarkDown file %s does not exist", updateContentFilePath.toUtf8().constData());
    }

    return preview;
}
