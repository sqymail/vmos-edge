#include "utils.h"
#include <QProcess>
#include <QUuid>
#include <chrono>
#include <QDir>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtWebEngine/QtWebEngine>
#else
#include <QtWebEngineQuick/QtWebEngineQuick>
#endif


Utils::Utils(QObject *parent)
    : QObject(parent){

}


QString Utils::getMachineId(){
    return QSysInfo::machineUniqueId();
}

void Utils::setCookie(const QString& domain, const QString& name, const QString& value){
    QQuickWebEngineProfile *profile = QQuickWebEngineProfile::defaultProfile();
    QWebEngineCookieStore *cookieStore = profile->cookieStore();
    QNetworkCookie cookie;
    cookie.setDomain(domain);
    cookie.setName(name.toUtf8());
    cookie.setValue(value.toUtf8());
    cookieStore->setCookie(cookie);
}

void Utils::openApp(const QString& filePath){

#ifdef Q_OS_MAC
    QProcess::startDetached("open", QStringList() << filePath);
#elif defined(Q_OS_LINUX)
    QProcess::startDetached("bash", QStringList() << "-c" << QString("chmod +x \"%1\" && \"%1\"").arg(filePath));
#elif defined(Q_OS_WIN)
    QProcess::startDetached(filePath);
#endif
}

QString Utils::uuid() {
    return QUuid::createUuid().toString().remove('-').remove('{').remove('}');
}

int64_t Utils::milliseconds() {
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

bool Utils::createDirectory(const QString &path)
{
    QDir dir;
    return dir.mkdir(path);
}

QString Utils::getClipboradText()
{
    return QGuiApplication::clipboard()->text();
}

void Utils::executeCommandInTerminal(const QString& command)
{
#ifdef Q_OS_WIN
    // 在Windows上，使用cmd /k来打开cmd窗口并执行命令
    QProcess::startDetached("cmd", QStringList() << "/k" << command);
#elif defined(Q_OS_MAC)
    // 在macOS上，使用Terminal.app来执行命令
    QProcess::startDetached("osascript", QStringList() << "-e" << QString("tell application \"Terminal\" to do script \"%1\"").arg(command));
#elif defined(Q_OS_LINUX)
    // 在Linux上，使用gnome-terminal或xterm
    QString terminal = "gnome-terminal";
    if (QProcess::execute("which", QStringList() << "gnome-terminal") != 0) {
        terminal = "xterm";
    }
    QProcess::startDetached(terminal, QStringList() << "-e" << "bash" << "-c" << QString("%1; exec bash").arg(command));
#endif
}