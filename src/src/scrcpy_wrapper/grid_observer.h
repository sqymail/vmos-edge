#pragma once

#include <QObject>
#include <QString>
#include <QPointer>
#include "QtScrcpyCore.h"

namespace armcloud {
class VideoRenderSink;
}

class GridObserver : public QObject, public qsc::DeviceObserver
{
    Q_OBJECT
    Q_PROPERTY(QString serial READ serial WRITE setSerial NOTIFY serialChanged)
public:
    explicit GridObserver(QObject *parent = nullptr);
    ~GridObserver() override = default;

    void onFrame(int width, int height, uint8_t* dataY, uint8_t* dataU, uint8_t* dataV, 
                 int linesizeY, int linesizeU, int linesizeV) override;
    void updateFPS(quint32 fps) override;
    void grabCursor(bool grab) override;

    // 设置渲染目标（Q_INVOKABLE 让 QML 可以调用）
    Q_INVOKABLE void setRenderSink(QObject *sink);
    armcloud::VideoRenderSink* renderSink() const { return m_renderSink; }

    QString serial() const { return m_serial; }
    void setSerial(const QString &serial);

signals:
    void serialChanged();
    void frameReceived(int width, int height);
    void fpsUpdated(int fps);

private:
    QPointer<QObject> m_renderItem;  // 存储 QObject 引用（VideoRenderItem 继承自 QObject）
    armcloud::VideoRenderSink* m_renderSink;  // 原始指针，指向 m_renderItem 实现的接口
    QString m_serial;
    bool m_isFirstFrame;
};

