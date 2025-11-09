#pragma once

#include <QObject>
#include <QPointer>
#include <QSize>
#include "QtScrcpyCore.h"
#include "../sdk_wrapper/video_render_sink.h"

// Forward declarations for input events
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;

class ScrcpyController : public QObject, public qsc::DeviceObserver
{
    Q_OBJECT
public:
    explicit ScrcpyController(QObject *parent = nullptr);
    ~ScrcpyController() override;

    /**
     * @brief Initializes the controller and connects it to the device and the sink.
     * @param device The device object from QtScrcpyCore, obtained via DeviceManager.
     * @param sink The UI render item (your VideoRenderItem) which implements armcloud::VideoRenderSink.
     */
    Q_INVOKABLE void initialize(QPointer<qsc::IDevice> device, armcloud::VideoRenderSink* sink);

    // --- Input Handling Slots --- //
    Q_INVOKABLE void sendMouseEvent(const QVariant& event, int viewWidth, int viewHeight);
    Q_INVOKABLE void sendWheelEvent(const QVariant& event, int viewWidth, int viewHeight);
    Q_INVOKABLE void sendKeyEvent(const QVariant& event);

    // --- Device Control Slots --- //
    Q_INVOKABLE void sendGoBack();
    Q_INVOKABLE void sendGoHome();
    Q_INVOKABLE void sendGoMenu();
    Q_INVOKABLE void sendAppSwitch();
    Q_INVOKABLE void sendPower();
    Q_INVOKABLE void sendVolumeUp();
    Q_INVOKABLE void sendVolumeDown();
    // Q_INVOKABLE void setDisplayPower(bool on);
    // Q_INVOKABLE void expandNotificationPanel();
    // Q_INVOKABLE void collapsePanel();
    Q_INVOKABLE void sendTextInput(const QString& text);
    // Q_INVOKABLE void clipboardPaste();
    Q_INVOKABLE void localScreenshot();
    Q_INVOKABLE void sendPushFileRequest(const QString& file, const QString& devicePath = "");
    Q_INVOKABLE void sendInstallApkRequest(const QString& apkFile);
    Q_INVOKABLE void sendInstallXapkRequest(const QString& xapkFile);
    // Q_INVOKABLE void showTouch(bool show);

signals:
    void newFrame(const QImage &frame);
    void fpsUpdated(int fps);
    void grabCursorChanged(bool grab);
    void screenInfo(int width, int height);
    void connectionEstablished();
    void connectionLost();
    void xapkInstallProgress(const QString& message);  // XAPK 安装进度通知
    void xapkInstallFinished(bool success, const QString& message);  // XAPK 安装完成通知

protected:
    // Override from qsc::DeviceObserver
    void onFrame(int width, int height, uint8_t* dataY, uint8_t* dataU, uint8_t* dataV, int linesizeY, int linesizeU, int linesizeV) override;
    void updateFPS(quint32 fps) override;
    void grabCursor(bool grab) override;

private slots:
    // 处理后台线程完成后的结果
    void onXapkInstallCompleted(bool success, const QString& message, const QString& extractDir,
                                 const QStringList& obbFiles, const QString& packageName);

private:
    // 辅助方法：从 APK 文件中提取包名
    QString extractPackageNameFromApk(const QString& apkFile);
    // 辅助方法：通过 adb 获取最近安装的包名
    QString getRecentlyInstalledPackageName(const QString& serial);
    // 辅助方法：使用 LibArchive 解压 ZIP/XAPK 文件
    bool extractZip(const QString& zipPath, const QString& outputDir);
    // 辅助方法：推送 OBB 文件到设备
    void pushObbFiles(const QStringList& obbFiles, const QString& packageName);

    QPointer<qsc::IDevice> m_device;
    armcloud::VideoRenderSink* m_sink = nullptr;
    QSize m_frameSize;
    bool m_isFirstFrame;
};
