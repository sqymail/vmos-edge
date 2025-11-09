#include "scrcpy_controller.h"
#include "../sdk_wrapper/video_frame.h"
#include <QDebug>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QVariantMap>
#include <libyuv.h>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QProcess>
#include <QTemporaryDir>
#include <QDateTime>
#include <QTimer>
#include <QRegularExpression>
#include <QThread>
#include <QCoreApplication>
#include <QFileInfo>
#include <QFile>
#include <archive.h>
#include <archive_entry.h>
#include "../../QtScrcpyCore/src/adb/adbprocessimpl.h"

ScrcpyController::ScrcpyController(QObject *parent)
    : QObject(parent)
    , m_isFirstFrame(false)
{
}

ScrcpyController::~ScrcpyController()
{
    if (m_device) {
        m_device->deRegisterDeviceObserver(this);
    }
}

void ScrcpyController::initialize(QPointer<qsc::IDevice> device, armcloud::VideoRenderSink* sink)
{
    if (!device) {
        qWarning() << "ScrcpyController::initialize - device is null";
        return;
    }

    if (!sink) {
        qWarning() << "ScrcpyController::initialize - sink is null";
        return;
    }
    m_device = device;
    m_sink = sink;
    m_device->registerDeviceObserver(this);

    connect(m_device, &qsc::IDevice::deviceConnected, this, [this](bool success, const QString& serial, const QString& deviceName, const QSize& size){
        if (success) {
            emit connectionEstablished();
        }
    });
    connect(m_device, &qsc::IDevice::deviceDisconnected, this, [this](const QString& serial){
        emit connectionLost();
    });
}

void ScrcpyController::onFrame(int width, int height, uint8_t* dataY, uint8_t* dataU, uint8_t* dataV, int linesizeY, int linesizeU, int linesizeV)
{
    if (!m_sink) {
        return;
    }

    if(!m_isFirstFrame){
        m_isFirstFrame = true;
        emit screenInfo(width, height);
    }

    m_frameSize = QSize(width, height);

    // Create an ARGB VideoFrame, which is what the existing rendering pipeline expects.
    auto videoFrame = std::make_shared<armcloud::VideoFrame>(width, height, armcloud::PixelFormat::ARGB);

    // Use libyuv to convert from I420 (YUV420P) to ARGB.
    libyuv::I420ToARGB(dataY, linesizeY,
                       dataU, linesizeU,
                       dataV, linesizeV,
                       videoFrame->buffer(0), videoFrame->stride(0),
                       width, height);

    // Call the sink's onFrame method (which is implemented by VideoRenderItem)
    m_sink->onFrame(videoFrame);
}

void ScrcpyController::updateFPS(quint32 fps)
{
    emit fpsUpdated(fps);
}

void ScrcpyController::grabCursor(bool grab)
{
    emit grabCursorChanged(grab);
}

// --- Input Event Handling --- //

void ScrcpyController::sendMouseEvent(const QVariant& event, int viewWidth, int viewHeight)
{
    if (!m_device || m_frameSize.isEmpty()) return;

    QVariantMap map = event.toMap();
    QPointF pos(map["x"].toReal(), map["y"].toReal());
    Qt::MouseButtons buttons = static_cast<Qt::MouseButtons>(map["buttons"].toInt());
    Qt::MouseButton button = static_cast<Qt::MouseButton>(map["button"].toInt());
    QEvent::Type type = static_cast<QEvent::Type>(map["type"].toInt());

    QMouseEvent mouseEvent(type, pos, button, buttons, Qt::NoModifier);

    m_device->mouseEvent(&mouseEvent, m_frameSize, QSize(viewWidth, viewHeight));
}

void ScrcpyController::sendWheelEvent(const QVariant& event, int viewWidth, int viewHeight)
{
    if (!m_device || m_frameSize.isEmpty()) return;

    QVariantMap map = event.toMap();
    QPointF pos(map["x"].toReal(), map["y"].toReal());
    QPoint angleDelta = map["angleDelta"].toPoint();
    Qt::MouseButtons buttons = static_cast<Qt::MouseButtons>(map["buttons"].toInt());
    Qt::KeyboardModifiers modifiers = static_cast<Qt::KeyboardModifiers>(map["modifiers"].toInt());

    // Use the more complete Qt 6 constructor, filling unused arguments with defaults.
    QWheelEvent wheelEvent(pos, pos, QPoint(), angleDelta, buttons, modifiers, Qt::ScrollUpdate, false);

    m_device->wheelEvent(&wheelEvent, m_frameSize, QSize(viewWidth, viewHeight));
}

void ScrcpyController::sendKeyEvent(const QVariant& event)
{
    if (!m_device || m_frameSize.isEmpty()) return;

    QVariantMap map = event.toMap();
    QEvent::Type type = static_cast<QEvent::Type>(map["type"].toInt());
    int key = map["key"].toInt();
    Qt::KeyboardModifiers modifiers = static_cast<Qt::KeyboardModifiers>(map["modifiers"].toInt());
    QString text = map["text"].toString();

    QKeyEvent keyEvent(type, key, modifiers, text);

    m_device->keyEvent(&keyEvent, m_frameSize, m_frameSize);
}

// --- Device Control Implementations --- //

void ScrcpyController::sendGoBack() {
    if (m_device) {
        m_device->postGoBack();
    }
}

void ScrcpyController::sendGoHome() {
    if (m_device){
        m_device->postGoHome();
    }
}

void ScrcpyController::sendGoMenu() {
    if (m_device) {
        m_device->postGoMenu();
    }
}

void ScrcpyController::sendAppSwitch() {
    if (m_device) {
        m_device->postAppSwitch();
    }
}

void ScrcpyController::sendPower() {
    if (m_device) {
        m_device->postPower();
    }
}

void ScrcpyController::sendVolumeUp() {
    if (m_device) {
        m_device->postVolumeUp();
    }
}

void ScrcpyController::sendVolumeDown() {
    if (m_device) {
        m_device->postVolumeDown();
    }
}

// void ScrcpyController::setDisplayPower(bool on) {
//     if (m_device) {
//         m_device->setDisplayPower(on);
//     }
// }

// void ScrcpyController::expandNotificationPanel() {
//     if (m_device) {
//         m_device->expandNotificationPanel();
//     }
// }

// void ScrcpyController::collapsePanel() {
//     if (m_device) {
//         m_device->collapsePanel();
//     }
// }

// void ScrcpyController::clipboardPaste() {
//     if (m_device) {
//         m_device->clipboardPaste();
//     }
// }

// void ScrcpyController::showTouch(bool show) {
//     if (m_device){
//         m_device->showTouch(show);
//     }
// }

void ScrcpyController::sendTextInput(const QString& text)
{
    if (m_device) {
        QString nonConstText = text;
        m_device->postTextInput(nonConstText);
    }
}

void ScrcpyController::localScreenshot()
{
    if (m_device) {
        m_device->screenshot();
    }
}

void ScrcpyController::sendPushFileRequest(const QString& file, const QString& devicePath)
{
    if (m_device) {
        m_device->pushFileRequest(file, devicePath);
    }
}

void ScrcpyController::sendInstallApkRequest(const QString& apkFile)
{
    if (m_device) {
        m_device->installApkRequest(apkFile);
    }
}

void ScrcpyController::sendInstallXapkRequest(const QString& xapkFile)
{
    // 使用后台线程执行阻塞操作，避免阻塞 UI 线程
    
    if (!m_device) {
        qWarning() << "ScrcpyController::sendInstallXapkRequest - device is null";
        return;
    }

    QFileInfo fileInfo(xapkFile);
    if (!fileInfo.exists()) {
        qWarning() << "ScrcpyController::sendInstallXapkRequest - file does not exist:" << xapkFile;
        return;
    }

    if (!xapkFile.toLower().endsWith(".xapk")) {
        qWarning() << "ScrcpyController::sendInstallXapkRequest - file is not a XAPK file:" << xapkFile;
        return;
    }

    emit xapkInstallProgress("开始解压 XAPK 文件...");

    // 创建临时目录用于解压 XAPK
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir baseDir(tempDir);
    QString extractDir = baseDir.absoluteFilePath("xapk_extract_" + QString::number(QDateTime::currentMSecsSinceEpoch()));
    
    if (!QDir().mkpath(extractDir)) {
        qWarning() << "ScrcpyController::sendInstallXapkRequest - failed to create temp directory:" << extractDir;
        emit xapkInstallFinished(false, "无法创建临时目录");
        return;
    }

    qDebug() << "Extracting XAPK file:" << xapkFile << "to:" << extractDir;

    // 在主线程中获取设备信息（避免线程安全问题）
    QString adbPath = AdbProcessImpl::getAdbPath();
    QString serial = m_device->getSerial();
    
    if (adbPath.isEmpty() || serial.isEmpty()) {
        qWarning() << "ScrcpyController::sendInstallXapkRequest - ADB path or serial is empty";
        QDir(extractDir).removeRecursively();
        emit xapkInstallFinished(false, "ADB 路径或设备序列号为空");
        return;
    }

    // 在后台线程执行阻塞操作
    QThread* thread = new QThread(this);
    
    // 使用 lambda 在后台线程中执行所有阻塞操作
    connect(thread, &QThread::started, [=]() {
        // 第一步：解压 XAPK 文件
        qDebug() << "Background thread: Extracting XAPK file";
        if (!extractZip(xapkFile, extractDir)) {
            qWarning() << "Background thread: Failed to extract XAPK file";
            QMetaObject::invokeMethod(this, "onXapkInstallCompleted", 
                                    Qt::QueuedConnection,
                                    Q_ARG(bool, false),
                                    Q_ARG(QString, "解压 XAPK 文件失败"),
                                    Q_ARG(QString, extractDir),
                                    Q_ARG(QStringList, QStringList()),
                                    Q_ARG(QString, QString()));
            thread->quit();
            return;
        }
        
        QMetaObject::invokeMethod(this, "xapkInstallProgress", 
                                Qt::QueuedConnection,
                                Q_ARG(QString, "解压完成，正在查找 APK 文件..."));
        
        // 第二步：查找解压后的文件
        QDir extractDirObj(extractDir);
        QStringList entries = extractDirObj.entryList(QDir::Files | QDir::NoDotAndDotDot);
        
        QStringList allApkFiles;
        QString mainApkFile;
        QStringList obbFiles;
        qint64 maxApkSize = 0;

        for (const QString& entry : entries) {
            QString filePath = extractDirObj.absoluteFilePath(entry);
            QFileInfo entryInfo(filePath);
            
            if (entry.toLower().endsWith(".apk")) {
                allApkFiles.append(filePath);
                qint64 fileSize = entryInfo.size();
                if (fileSize > maxApkSize) {
                    maxApkSize = fileSize;
                    mainApkFile = filePath;
                }
            } else if (entry.toLower().endsWith(".obb")) {
                obbFiles.append(filePath);
            }
        }

        if (allApkFiles.isEmpty()) {
            qWarning() << "Background thread: No APK file found in XAPK";
            QMetaObject::invokeMethod(this, "onXapkInstallCompleted", 
                                    Qt::QueuedConnection,
                                    Q_ARG(bool, false),
                                    Q_ARG(QString, "未找到 APK 文件"),
                                    Q_ARG(QString, extractDir),
                                    Q_ARG(QStringList, QStringList()),
                                    Q_ARG(QString, QString()));
            thread->quit();
            return;
        }

        qDebug() << "Background thread: Found APK files:" << allApkFiles.size();
        qDebug() << "Background thread: Found main APK:" << mainApkFile;
        qDebug() << "Background thread: Found OBB files:" << obbFiles;

        QMetaObject::invokeMethod(this, "xapkInstallProgress", 
                                Qt::QueuedConnection,
                                Q_ARG(QString, "正在提取包名..."));
        
        // 第三步：提取包名
        QString packageName = extractPackageNameFromApk(mainApkFile);
        if (packageName.isEmpty()) {
            qDebug() << "Background thread: Could not extract package name from APK";
        } else {
            qDebug() << "Background thread: Extracted package name:" << packageName;
        }

        // 第五步：安装 APK（阻塞操作）
        if (allApkFiles.size() > 1) {
            // 多个 APK，使用 install-multiple
            QMetaObject::invokeMethod(this, "xapkInstallProgress", 
                                    Qt::QueuedConnection,
                                    Q_ARG(QString, QString("正在安装 %1 个 APK 文件...").arg(allApkFiles.size())));
            
            // 重新排序 APK 文件
            QStringList sortedApkFiles;
            if (!mainApkFile.isEmpty()) {
                sortedApkFiles.append(mainApkFile);
            }
            for (const QString& apkFile : allApkFiles) {
                if (apkFile != mainApkFile) {
                    sortedApkFiles.append(apkFile);
                }
            }
            
            // 构建命令
            QStringList adbArgs;
            adbArgs << "-s" << serial;
            adbArgs << "install-multiple";
            adbArgs << "-r";
            for (const QString& apkFile : sortedApkFiles) {
                adbArgs << apkFile;
            }
            
            qDebug() << "Background thread: Executing install-multiple";
            
            // 执行安装（阻塞操作，但已在后台线程）
            QProcess installProcess;
            installProcess.setProgram(adbPath);
            installProcess.setArguments(adbArgs);
            installProcess.start();
            
            if (!installProcess.waitForFinished(60000)) {
                qWarning() << "Background thread: install-multiple timeout";
                QMetaObject::invokeMethod(this, "onXapkInstallCompleted", 
                                        Qt::QueuedConnection,
                                        Q_ARG(bool, false),
                                        Q_ARG(QString, "安装超时（60秒）"),
                                        Q_ARG(QString, extractDir),
                                        Q_ARG(QStringList, QStringList()),
                                        Q_ARG(QString, QString()));
                thread->quit();
                return;
            }
            
            if (installProcess.exitCode() != 0) {
                QString errorOutput = installProcess.readAllStandardError();
                QString standardOutput = installProcess.readAllStandardOutput();
                qWarning() << "Background thread: install-multiple failed:"
                           << "Exit code:" << installProcess.exitCode()
                           << "Error:" << errorOutput;
                QMetaObject::invokeMethod(this, "onXapkInstallCompleted", 
                                        Qt::QueuedConnection,
                                        Q_ARG(bool, false),
                                        Q_ARG(QString, QString("安装失败: %1").arg(errorOutput)),
                                        Q_ARG(QString, extractDir),
                                        Q_ARG(QStringList, QStringList()),
                                        Q_ARG(QString, QString()));
                thread->quit();
                return;
            }
            
            qDebug() << "Background thread: Install-multiple completed successfully";
            QString output = installProcess.readAllStandardOutput();
            if (!output.isEmpty()) {
                qDebug() << "Background thread: Install output:" << output;
            }
            
            // 安装成功，通知主线程
            QMetaObject::invokeMethod(this, "onXapkInstallCompleted", 
                                    Qt::QueuedConnection,
                                    Q_ARG(bool, true),
                                    Q_ARG(QString, "XAPK 安装成功"),
                                    Q_ARG(QString, extractDir),
                                    Q_ARG(QStringList, obbFiles),
                                    Q_ARG(QString, packageName));
        } else {
            // 单个 APK，使用异步安装（不阻塞）
            QMetaObject::invokeMethod(this, "xapkInstallProgress", 
                                    Qt::QueuedConnection,
                                    Q_ARG(QString, "正在安装 APK 文件..."));
            
            // 在主线程中执行 installApkRequest（需要访问 Qt 对象）
            QMetaObject::invokeMethod(this, [this, allApkFiles]() {
                m_device->installApkRequest(allApkFiles.first());
            }, Qt::QueuedConnection);
            
            // 单个 APK 安装是异步的
            QMetaObject::invokeMethod(this, "onXapkInstallCompleted", 
                                    Qt::QueuedConnection,
                                    Q_ARG(bool, true),
                                    Q_ARG(QString, "APK 安装已启动"),
                                    Q_ARG(QString, extractDir),
                                    Q_ARG(QStringList, obbFiles),
                                    Q_ARG(QString, packageName));
        }
        
        thread->quit();
    });
    
    // 线程结束时自动清理
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    
    // 启动后台线程
    thread->start();
}

void ScrcpyController::onXapkInstallCompleted(bool success, const QString& message, 
                                                const QString& extractDir,
                                                const QStringList& obbFiles, 
                                                const QString& packageName)
{
    // 这个方法在主线程中执行，可以安全访问 Qt 对象
    
    if (success) {
        emit xapkInstallProgress("APK 安装完成");
        
        // 处理 OBB 文件
        if (!obbFiles.isEmpty()) {
            if (!packageName.isEmpty()) {
                emit xapkInstallProgress("正在推送 OBB 文件...");
                pushObbFiles(obbFiles, packageName);
                emit xapkInstallProgress("OBB 文件推送已启动");
            } else {
                // 如果单个 APK 且没有包名，延迟获取
                QString serial = m_device->getSerial();
                QTimer::singleShot(2000, this, [this, serial, obbFiles, extractDir]() {
                    QString packageName = getRecentlyInstalledPackageName(serial);
                    if (!packageName.isEmpty()) {
                        pushObbFiles(obbFiles, packageName);
                        QTimer::singleShot(10000, [extractDir]() {
                            QDir(extractDir).removeRecursively();
                        });
                    } else {
                        qWarning() << "Could not determine package name for OBB files";
                        QTimer::singleShot(10000, [extractDir]() {
                            QDir(extractDir).removeRecursively();
                        });
                    }
                });
                emit xapkInstallFinished(true, message);
                return;
            }
        }
        
        // 清理临时目录
        QTimer::singleShot(10000, [extractDir]() {
            QDir(extractDir).removeRecursively();
        });
    } else {
        // 安装失败，延迟清理临时目录
        QTimer::singleShot(30000, [extractDir]() {
            QDir(extractDir).removeRecursively();
        });
    }
    
    emit xapkInstallFinished(success, message);
}

void ScrcpyController::pushObbFiles(const QStringList& obbFiles, const QString& packageName)
{
    if (packageName.isEmpty() || obbFiles.isEmpty()) {
        return;
    }
    
    qDebug() << "Pushing OBB files for package:" << packageName;
    
    // OBB 文件需要推送到 /sdcard/Android/obb/<package_name>/
    QString obbBasePath = QString("/sdcard/Android/obb/%1").arg(packageName);
    
    for (const QString& obbFile : obbFiles) {
        QFileInfo obbInfo(obbFile);
        QString obbFileName = obbInfo.fileName();
        QString deviceObbPath = QString("%1/%2").arg(obbBasePath, obbFileName);
        
        qDebug() << "Pushing OBB file:" << obbFile << "to" << deviceObbPath;
        m_device->pushFileRequest(obbFile, deviceObbPath);
    }
    
    qDebug() << "OBB files pushed successfully";
}

QString ScrcpyController::extractPackageNameFromApk(const QString& apkFile)
{
    // 获取应用程序目录路径
    QString appDir = QCoreApplication::applicationDirPath();
    
    // 辅助函数：尝试使用指定的 aapt 路径提取包名
    auto tryExtractPackageName = [&apkFile](const QString& aaptPath) -> QString {
        QFileInfo aaptInfo(aaptPath);
        // 如果是相对路径（系统 PATH），直接尝试；如果是绝对路径，检查文件是否存在
        if (aaptPath.contains("/") || aaptPath.contains("\\")) {
            // 绝对路径，检查文件是否存在
            if (!aaptInfo.exists() || !aaptInfo.isFile()) {
                return QString(); // 文件不存在
            }
        }
        
        QProcess aaptProcess;
        aaptProcess.setProgram(aaptPath);
        aaptProcess.setArguments(QStringList() << "dump" << "badging" << apkFile);
        aaptProcess.start();
        
        if (aaptProcess.waitForFinished(5000) && aaptProcess.exitCode() == 0) {
            QByteArray output = aaptProcess.readAllStandardOutput();
            QString outputStr = QString::fromUtf8(output);
            
            // 解析 aapt 输出，查找 package: name='...'
            QRegularExpression regex(R"(package:\s*name='([^']+)')");
            QRegularExpressionMatch match = regex.match(outputStr);
            if (match.hasMatch()) {
                QString packageName = match.captured(1);
                qDebug() << "Extracted package name using" << aaptPath << "package:" << packageName;
                return packageName;
            }
        }
        return QString();
    };
    
    // 方法1: 优先尝试应用程序目录下的 aapt
#ifdef Q_OS_WIN
    QString localAapt = appDir + "/aapt.exe";
    QString localAapt2 = appDir + "/aapt2.exe";
#else
    QString localAapt = appDir + "/aapt";
    QString localAapt2 = appDir + "/aapt2";
#endif
    
    QString packageName = tryExtractPackageName(localAapt);
    if (!packageName.isEmpty()) {
        return packageName;
    }
    
    // 方法2: 尝试应用程序目录下的 aapt2
    packageName = tryExtractPackageName(localAapt2);
    if (!packageName.isEmpty()) {
        return packageName;
    }
    
    // 方法3: 回退到系统 PATH 中的 aapt
    packageName = tryExtractPackageName("aapt");
    if (!packageName.isEmpty()) {
        return packageName;
    }
    
    // 方法4: 回退到系统 PATH 中的 aapt2
    packageName = tryExtractPackageName("aapt2");
    if (!packageName.isEmpty()) {
        return packageName;
    }
    
    qDebug() << "Could not extract package name using aapt/aapt2. "
             << "Checked application directory:" << appDir
             << "and system PATH.";
    return QString();
}

QString ScrcpyController::getRecentlyInstalledPackageName(const QString& serial)
{
    if (serial.isEmpty()) {
        return QString();
    }
    
    // 获取 adb 路径 - 使用 AdbProcessImpl 的静态方法
    // 注意：这需要包含 AdbProcessImpl 的头文件，但为了保持封装性，我们使用其他方法
    // 实际上，由于 getRecentlyInstalledPackageName 方法不太可靠，这里暂时返回空
    // 主要依赖 extractPackageNameFromApk 在安装前获取包名
    
    // 方法：通过 adb shell dumpsys package 获取最近安装的包
    // 查找 firstInstallTime 最近的包（但这需要遍历所有包，较慢）
    // 更实用的方法：使用 adb shell pm list packages -3 获取第三方包列表
    // 然后检查每个包的安装时间，但这会很慢
    
    // 由于无法可靠地获取最近安装的包名，这个方法主要用于备用
    // 主要依赖 extractPackageNameFromApk 方法在安装前获取包名
    
    qDebug() << "getRecentlyInstalledPackageName: This method is not fully implemented. "
             << "Please use aapt/aapt2 to extract package name before installation.";
    return QString();
}

bool ScrcpyController::extractZip(const QString& zipPath, const QString& outputDir)
{
    struct archive *a;
    struct archive *ext;
    struct archive_entry *entry;
    int flags;
    int r;
    
    // 设置解压标志
    flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS;
    
    // 创建读取和解压上下文
    a = archive_read_new();
    archive_read_support_format_zip(a);  // 只支持 ZIP 格式
    archive_read_support_filter_all(a);
    
    ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, flags);
    archive_write_disk_set_standard_lookup(ext);
    
#ifdef Q_OS_WIN
    // Windows上：设置工作目录，让libarchive使用相对路径
    QString originalCurrentDir = QDir::currentPath();
    QDir::setCurrent(outputDir);
#endif
    
    // 打开 ZIP 文件
    QFile zipFile(zipPath);
    if (!zipFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open ZIP file:" << zipPath << "Error:" << zipFile.errorString();
        archive_read_free(a);
        archive_write_free(ext);
#ifdef Q_OS_WIN
        QDir::setCurrent(originalCurrentDir);
#endif
        return false;
    }
    
#ifdef Q_OS_WIN
    // Windows上：读取整个文件到内存，然后使用内存方式打开archive
    QByteArray fileData = zipFile.readAll();
    zipFile.close();
    
    if (fileData.isEmpty()) {
        qDebug() << "Failed to read ZIP file or file is empty:" << zipPath;
        archive_read_free(a);
        archive_write_free(ext);
        QDir::setCurrent(originalCurrentDir);
        return false;
    }
    
    r = archive_read_open_memory(a, fileData.data(), fileData.size());
    if (r != ARCHIVE_OK) {
        qDebug() << "Failed to open archive from memory:" << archive_error_string(a);
        archive_read_free(a);
        archive_write_free(ext);
        QDir::setCurrent(originalCurrentDir);
        return false;
    }
#else
    // Unix系统上直接使用文件描述符
    int fd = zipFile.handle();
    if (fd == -1) {
        qDebug() << "Failed to get file descriptor for:" << zipPath;
        zipFile.close();
        archive_read_free(a);
        archive_write_free(ext);
        return false;
    }
    
    r = archive_read_open_fd(a, fd, 10240);
    if (r != ARCHIVE_OK) {
        qDebug() << "Failed to open ZIP file via fd:" << archive_error_string(a);
        zipFile.close();
        archive_read_free(a);
        archive_write_free(ext);
        return false;
    }
#endif
    
    // 解压所有文件
    while (true) {
        r = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF) {
            break;
        }
        if (r != ARCHIVE_OK) {
            qDebug() << "Failed to read header:" << archive_error_string(a);
            break;
        }
        
        // 设置输出路径
        QString entryPath = QString::fromUtf8(archive_entry_pathname(entry));
        
#ifdef Q_OS_WIN
        // Windows上：使用相对路径（相对于outputDir）
        QFileInfo entryInfo(entryPath);
        QString parentDir = entryInfo.path();
        if (!parentDir.isEmpty() && parentDir != ".") {
            QDir outputDirObj(outputDir);
            QString fullParentDir = outputDirObj.absoluteFilePath(parentDir);
            if (!QDir().mkpath(fullParentDir)) {
                qDebug() << "Failed to create parent directory:" << fullParentDir;
            }
        }
        
        QByteArray entryPathUtf8 = entryPath.toUtf8();
        archive_entry_set_pathname(entry, entryPathUtf8.constData());
#else
        // Unix系统上：使用完整路径
        QDir outputDirObj(outputDir);
        QString fullPath = outputDirObj.absoluteFilePath(entryPath);
        QByteArray fullPathUtf8 = fullPath.toUtf8();
        archive_entry_set_pathname(entry, fullPathUtf8.constData());
#endif
        
        // 写入文件
        r = archive_write_header(ext, entry);
        if (r != ARCHIVE_OK) {
            qDebug() << "Failed to write header:" << archive_error_string(ext) << "Entry path:" << entryPath;
            break;
        }
        
        // 复制文件内容
        if (archive_entry_size(entry) > 0) {
            const void *buff;
            size_t size;
            la_int64_t offset;
            
            for (;;) {
                r = archive_read_data_block(a, &buff, &size, &offset);
                if (r == ARCHIVE_EOF) {
                    break;
                }
                if (r != ARCHIVE_OK) {
                    qDebug() << "Failed to read data:" << archive_error_string(a);
                    break;
                }
                r = archive_write_data_block(ext, buff, size, offset);
                if (r != ARCHIVE_OK) {
                    qDebug() << "Failed to write data:" << archive_error_string(ext);
                    break;
                }
            }
        }
        
        r = archive_write_finish_entry(ext);
        if (r != ARCHIVE_OK) {
            qDebug() << "Failed to finish entry:" << archive_error_string(ext);
            break;
        }
    }
    
    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);
    
#ifdef Q_OS_WIN
    // 恢复原始工作目录
    QDir::setCurrent(originalCurrentDir);
    // 关闭QFile（Windows上已经在内存读取后关闭了）
#else
    zipFile.close();
#endif
    
    return (r == ARCHIVE_OK || r == ARCHIVE_EOF);
}
