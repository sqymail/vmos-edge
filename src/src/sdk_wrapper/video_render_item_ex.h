#pragma once

#include <QQuickItem>
#include <QMutex>
#include <QSGTexture>
#include <QSGSimpleTextureNode>
#include <memory>
#include "video_render_sink.h"
#include "video_frame.h"


class VideoRenderItemEx : public QQuickItem, public armcloud::VideoRenderSink {
    Q_OBJECT
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
    Q_PROPERTY(bool hasVideo READ hasVideo WRITE setHasVideo NOTIFY hasVideoChanged FINAL)
public:
    explicit VideoRenderItemEx(QQuickItem* parent = nullptr);
    ~VideoRenderItemEx() override;

    void onFrame(std::shared_ptr<armcloud::VideoFrame>& frame) override;

    qreal rotation() const { return m_angle; }
    void setRotation(qreal angle);

    bool hasVideo() const { return m_hasVideo; }
    void setHasVideo(bool value);
signals:
    void rotationChanged();
    void hasVideoChanged();
protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) override;

private:
    std::shared_ptr<armcloud::VideoFrame> m_frame;
    QMutex m_mutex;
    qreal m_angle = 0.0;
    bool m_hasVideo = false;
};
