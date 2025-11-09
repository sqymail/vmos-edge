#include "video_render_item_ex.h"

#include <QQuickWindow>
#include <QMutexLocker>
#include <cmath>
#include <QDebug>

#include <QSGMaterial>
#include <QSGGeometry>
#include <QSGMaterialShader>
#include <QSGTransformNode>
#include <QSGSimpleTextureNode>
#include <QSGRendererInterface>
#include <rhi/qshader.h>
#include <rhi/qshaderbaker.h>

// For YUV rendering, we need a custom scene graph node, material, and shader.
// This allows the YUV->RGB conversion to be done on the GPU, which is very efficient.

namespace {

// Unique type for our custom material
class YuvMaterialType : public QSGMaterialType {};

class YuvMaterial : public QSGMaterial
{
public:
    YuvMaterial() {
        m_textures[0] = m_textures[1] = m_textures[2] = nullptr;
    }

    ~YuvMaterial() override {
        // The material does not own the textures, the node does.
    }

    QSGMaterialType *type() const override {
        static YuvMaterialType type;
        return &type;
    }

    int compare(const QSGMaterial *other) const override {
        const auto* o = static_cast<const YuvMaterial*>(other);
        if (m_textures[0] != o->m_textures[0] ||
            m_textures[1] != o->m_textures[1] ||
            m_textures[2] != o->m_textures[2]) {
            return 1;
        }
        return 0;
    }

    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override;

    QSGTexture* m_textures[3];
};


class YuvShader : public QSGMaterialShader
{
public:
    YuvShader() {
        // --- Vertex Shader (GLSL 430 for binding support) ---
        QShaderBaker vsBaker;
        vsBaker.setSourceString(QByteArrayLiteral(
                        R"(#version 430 core
                        precision highp float;
                        layout(location = 0) in vec4 vertex;
                        layout(location = 1) in vec2 texCoord;
                        layout(std140, binding = 0) uniform buf {
                            mat4 qt_Matrix;
                        };
                        layout(location = 0) out vec2 texc;
                        void main() {
                            gl_Position = qt_Matrix * vertex;
                            texc = texCoord;
                        })"), QShader::VertexStage);
        const QShader vs = vsBaker.bake();
        if (!vs.isValid()) {
            qWarning() << "Failed to bake vertex shader:" << vsBaker.errorMessage();
        }

        // --- Fragment Shader (GLSL 430 for binding support) ---
        QShaderBaker fsBaker;
        fsBaker.setSourceString(QByteArrayLiteral(
                        R"(#version 430 core
                        precision highp float;
                        layout(binding = 0) uniform sampler2D y_tex;
                        layout(binding = 1) uniform sampler2D u_tex;
                        layout(binding = 2) uniform sampler2D v_tex;
                        layout(location = 0) in vec2 texc;
                        layout(location = 0) out vec4 fragColor;
                        void main() {
                            float y = texture(y_tex, texc).r;
                            float u = texture(u_tex, texc).r;
                            float v = texture(v_tex, texc).r;

                            // BT.601 video range to RGB conversion
                            y = 1.164 * (y - 0.0625);
                            u = u - 0.5;
                            v = v - 0.5;

                            float r = y + 1.596 * v;
                            float g = y - 0.391 * u - 0.813 * v;
                            float b = y + 2.018 * u;

                            fragColor = vec4(r, g, b, 1.0);
                        })"), QShader::FragmentStage);
        const QShader fs = fsBaker.bake();
        if (!fs.isValid()) {
            qWarning() << "Failed to bake fragment shader:" << fsBaker.errorMessage();
        }

        setShader(QSGMaterialShader::VertexStage, vs);
        setShader(QSGMaterialShader::FragmentStage, fs);
    }

    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override
    {
        auto* material = static_cast<YuvMaterial*>(newMaterial);
        if (binding < 3) {
            *texture = material->m_textures[binding];
        }
    }
};

QSGMaterialShader *YuvMaterial::createShader(QSGRendererInterface::RenderMode) const {
    return new YuvShader();
}


class YuvRenderNode : public QSGGeometryNode
{
public:
    YuvRenderNode(QQuickWindow* window)
        : m_window(window)
        , m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4)
    {
        setGeometry(&m_geometry);

        auto* material = new YuvMaterial();
        setMaterial(material);
        setFlag(OwnsMaterial);
    }

    ~YuvRenderNode() override {
        delete m_textures[0];
        delete m_textures[1];
        delete m_textures[2];
    }

    void updateFrame(const std::shared_ptr<armcloud::VideoFrame>& frame) {
        if (!frame || !m_window) return;

        const int width = static_cast<int>(frame->width());
        const int height = static_cast<int>(frame->height());

        const bool sizeChanged = (m_size.width() != width || m_size.height() != height);
        if (sizeChanged) {
            m_size = {width, height};
        }

        const uchar* plane_data[] = { frame->buffer(0), frame->buffer(1), frame->buffer(2) };
        const uint32_t plane_strides[] = { frame->stride(0), frame->stride(1), frame->stride(2) };
        const QSize plane_sizes[] = { {width, height}, {width / 2, height / 2}, {width / 2, height / 2} };

        auto* mat = static_cast<YuvMaterial*>(material());

        for (int i = 0; i < 3; ++i) {
            QImage wrapper(plane_data[i], plane_sizes[i].width(), plane_sizes[i].height(), plane_strides[i], QImage::Format_Grayscale8);

            delete m_textures[i];
            m_textures[i] = m_window->createTextureFromImage(wrapper);
            
            if (m_textures[i]) {
                m_textures[i]->setFiltering(QSGTexture::Linear);
                m_textures[i]->setHorizontalWrapMode(QSGTexture::ClampToEdge);
                m_textures[i]->setVerticalWrapMode(QSGTexture::ClampToEdge);
            }
        }
        
        mat->m_textures[0] = m_textures[0];
        mat->m_textures[1] = m_textures[1];
        mat->m_textures[2] = m_textures[2];

        markDirty(QSGNode::DirtyMaterial);
    }

    void setRect(const QRectF &rect) {
        QSGGeometry::updateTexturedRectGeometry(&m_geometry, rect, QRectF(0, 0, 1, 1));
        markDirty(QSGNode::DirtyGeometry);
    }

private:
    QQuickWindow* m_window = nullptr;
    QSGGeometry m_geometry;
    QSGTexture* m_textures[3] = {nullptr, nullptr, nullptr};
    QSize m_size;
};

} // anonymous namespace


// --- VideoRenderItemEx Implementation ---

VideoRenderItemEx::VideoRenderItemEx(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

VideoRenderItemEx::~VideoRenderItemEx() = default;

void VideoRenderItemEx::onFrame(std::shared_ptr<armcloud::VideoFrame>& frame) {
    if (!frame) return;

    setHasVideo(true);
    {
        QMutexLocker locker(&m_mutex);
        m_frame = frame;
    }
    QMetaObject::invokeMethod(this, &QQuickItem::update, Qt::QueuedConnection);
}

void VideoRenderItemEx::setRotation(qreal angle) {
    if (qFuzzyCompare(m_angle, angle))
        return;

    m_angle = angle;
    emit rotationChanged();
    update();
}

QSGNode* VideoRenderItemEx::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
    auto* rootNode = static_cast<QSGTransformNode*>(oldNode);

    if (!window()) {
        return nullptr;
    }

    std::shared_ptr<armcloud::VideoFrame> frame;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_frame)
            return rootNode;
        frame = m_frame;
    }

    const auto frameFormat = frame->format();
    const QSize frameSize(static_cast<int>(frame->width()), static_cast<int>(frame->height()));
    QSGNode* contentNode = rootNode ? rootNode->firstChild() : nullptr;

    bool isYuv = (frameFormat == armcloud::PixelFormat::YUV420P);

    if (isYuv) {
        auto* yuvNode = dynamic_cast<YuvRenderNode*>(contentNode);
        if (!yuvNode) {
            delete rootNode;
            yuvNode = new YuvRenderNode(window());
            rootNode = new QSGTransformNode();
            rootNode->appendChildNode(yuvNode);
        }
        yuvNode->updateFrame(frame);
    } else { // ARGB Path
        auto* textureNode = dynamic_cast<QSGSimpleTextureNode*>(contentNode);
        if (!textureNode) {
            delete rootNode;
            textureNode = new QSGSimpleTextureNode();
            rootNode = new QSGTransformNode();
            rootNode->appendChildNode(textureNode);
        }
        QImage image(frame->buffer(0), frameSize.width(), frameSize.height(), frame->stride(0), QImage::Format_ARGB32);
        QSGTexture* texture = window()->createTextureFromImage(image);
        if (texture) {
            texture->setFiltering(QSGTexture::Linear);
        }
        delete textureNode->texture();
        textureNode->setTexture(texture);
    }

    contentNode = rootNode->firstChild();
    QRectF rect = boundingRect();
    qreal scale = qMin(rect.width() / frameSize.width(), rect.height() / frameSize.height());
    QSizeF scaledSize = frameSize * scale;
    QPointF topLeft((rect.width() - scaledSize.width()) / 2, (rect.height() - scaledSize.height()) / 2);
    QRectF centeredRect(topLeft, scaledSize);

    if (isYuv) {
        static_cast<YuvRenderNode*>(contentNode)->setRect(centeredRect);
    } else {
        static_cast<QSGSimpleTextureNode*>(contentNode)->setRect(centeredRect);
    }

    if (!qFuzzyIsNull(m_angle)) {
        QTransform transform;
        QPointF center = centeredRect.center();
        transform.translate(center.x(), center.y());
        transform.rotate(m_angle);
        transform.translate(-center.x(), -center.y());
        rootNode->setMatrix(transform);
    } else {
        rootNode->setMatrix(QTransform());
    }

    return rootNode;
}

void VideoRenderItemEx::setHasVideo(bool value) {
    if (m_hasVideo == value)
        return;
    m_hasVideo = value;
    emit hasVideoChanged();
}
