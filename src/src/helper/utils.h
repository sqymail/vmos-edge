#pragma once

#include <QObject>
#include "../singleton.h"

class Utils : public QObject{
    Q_OBJECT
private:
    explicit Utils(QObject *parent = nullptr);
public:
    SINGLETON(Utils)

    Q_INVOKABLE QString getMachineId();
    Q_INVOKABLE void setCookie(const QString& domain, const QString& name, const QString& value);
    Q_INVOKABLE void openApp(const QString& filePath);
    Q_INVOKABLE QString uuid();
    Q_INVOKABLE int64_t milliseconds();
    Q_INVOKABLE bool createDirectory(const QString& path);
    Q_INVOKABLE QString getClipboradText();
    Q_INVOKABLE void executeCommandInTerminal(const QString& command);
};
