#include "grid_observer.h"
#include "../sdk_wrapper/video_render_sink.h"
#include "../sdk_wrapper/video_frame.h"
#include "../sdk_wrapper/video_render_item.h"
#include "../sdk_wrapper/video_render_item_ex.h"
#include <QMetaObject>
#include <libyuv.h>

GridObserver::GridObserver(QObject *parent)
    : QObject(parent)
    , m_renderItem(nullptr)
    , m_renderSink(nullptr)
    , m_isFirstFrame(false)
{
}

void GridObserver::onFrame(int width, int height, uint8_t* dataY, uint8_t* dataU, uint8_t* dataV, 
                            int linesizeY, int linesizeU, int linesizeV)
{
    if (!m_renderSink || !m_renderItem) return;

    // 创建 VideoFrame 并转换 YUV 到 ARGB
    auto videoFrame = std::make_shared<armcloud::VideoFrame>(width, height, armcloud::PixelFormat::ARGB);
    libyuv::I420ToARGB(dataY, linesizeY,
                       dataU, linesizeU,
                       dataV, linesizeV,
                       videoFrame->buffer(0), videoFrame->stride(0),
                       width, height);
    
    // 调用渲染目标的 onFrame（VideoRenderSink 实现会处理线程安全）
    m_renderSink->onFrame(videoFrame);

    // 发射首次帧信号
    if (!m_isFirstFrame) {
        m_isFirstFrame = true;
        QMetaObject::invokeMethod(this, [this, width, height]() {
            emit frameReceived(width, height);
        }, Qt::QueuedConnection);
    }
}

void GridObserver::updateFPS(quint32 fps)
{
    QMetaObject::invokeMethod(this, [this, fps]() {
        emit fpsUpdated(static_cast<int>(fps));
    }, Qt::QueuedConnection);
}

void GridObserver::grabCursor(bool grab)
{
    Q_UNUSED(grab);
    // GridObserver 不需要处理 grabCursor
}

void GridObserver::setRenderSink(QObject *sink)
{
    if (!sink) {
        m_renderItem = nullptr;
        m_renderSink = nullptr;
        return;
    }
    
    // 尝试将 QObject* 转换为 VideoRenderSink*
    // VideoRenderItem 和 VideoRenderItemEx 都实现了 VideoRenderSink 接口
    armcloud::VideoRenderSink* renderSink = nullptr;
    
    // 尝试转换为 VideoRenderItem
    VideoRenderItem* renderItem = qobject_cast<VideoRenderItem*>(sink);
    if (renderItem) {
        renderSink = renderItem;
    } else {
        // 尝试转换为 VideoRenderItemEx
        VideoRenderItemEx* renderItemEx = qobject_cast<VideoRenderItemEx*>(sink);
        if (renderItemEx) {
            renderSink = renderItemEx;
        }
    }
    
    if (!renderSink) {
        qWarning() << "GridObserver::setRenderSink - sink is not a valid VideoRenderSink";
        m_renderItem = nullptr;
        m_renderSink = nullptr;
        return;
    }
    
    // 存储 QObject 引用（用于生命周期管理）和接口指针（用于调用）
    m_renderItem = sink;
    m_renderSink = renderSink;
}

void GridObserver::setSerial(const QString &serial)
{
    if (m_serial != serial) {
        m_serial = serial;
        emit serialChanged();
    }
}

// 手动包含 MOC 生成的文件（确保链接器能找到元对象代码）
// CMake AUTOMOC 会自动生成此文件，但需要显式包含以确保链接
#include "moc_grid_observer.cpp"

