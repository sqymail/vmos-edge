import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import FluentUI
import Utils

FluPopup {
    id: root
    implicitWidth: 500
    padding: 20
    spacing: 15
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    property var modelData: null
    property string deviceBrand: ""  // 品牌
    property string deviceModel: ""   // 机型

    onOpened: {
        // 重置品牌和机型
        root.deviceBrand = ""
        root.deviceModel = ""
        
        // 如果有 hostIp 和 dbId，获取品牌和机型
        if (modelData && modelData.hostIp && (modelData.dbId || modelData.id || modelData.name)) {
            var dbId = modelData.dbId || modelData.id || modelData.name
            reqGetDeviceBrandModel(modelData.hostIp, dbId)
        }
    }

    // 规范化 Android 版本
    function normalizeAndroidVersion(v) {
        var s = (v === undefined || v === null) ? "" : ("" + v)
        var m = s.match(/(\d{1,2})/)
        return m && m[1] ? m[1] : ""
    }

    // 从镜像名称获取 Android 版本
    function getAndroidVersionFromImage(imageName) {
        if (!imageName) return ""
        
        // 首先尝试从 imagesModel 中查找
        if (typeof imagesModel !== 'undefined') {
            for (var i = 0; i < imagesModel.rowCount(); i++) {
                var idx = imagesModel.index(i, 0)
                var n = imagesModel.data(idx, ImagesModel.NameRole).toString()
                var fn = imagesModel.data(idx, ImagesModel.FileNameRole).toString()
                var v = imagesModel.data(idx, ImagesModel.VersionRole).toString()
                if ((imageName && n === imageName) || (imageName && fn === imageName)) {
                    return normalizeAndroidVersion(v)
                }
            }
        }
        
        // 如果找不到，尝试从镜像名称中提取
        var m = imageName.match(/android\s*(\d{1,2})/i)
        if (m && m[1]) return normalizeAndroidVersion(m[1])
        
        return ""
    }

    // 获取镜像版本显示文本
    function getImageVersionText() {
        if (!modelData) return ""
        var image = modelData.image || ""
        if (image) {
            // 如果包含冒号，取冒号后的部分作为版本
            var parts = image.split(":")
            if (parts.length > 1) {
                return parts[0]
            }
            return image
        }
        return ""
    }

    // 获取 Android 版本显示文本
    function getAndroidVersionText() {
        if (!modelData) return ""
        // 优先使用 aospVersion
        if (modelData.aospVersion) {
            return normalizeAndroidVersion(modelData.aospVersion)
        }
        // 其次从镜像名称中提取
        var image = modelData.image || ""
        if (image) {
            // 如果 image 包含冒号，取冒号前的部分
            var imageName = image.split(":")[0]
            return getAndroidVersionFromImage(imageName)
        }
        return ""
    }

    // 调用 shell 接口获取云机品牌和机型
    function reqGetDeviceBrandModel(hostIp, dbId) {
        if (!hostIp || !dbId) {
            console.warn("[云机详情] 获取品牌机型: hostIp 或 dbId 为空")
            return
        }
        
        // 分别获取品牌和机型
        // 先获取品牌
        console.log("[云机详情] 请求获取品牌:", hostIp, dbId)
        Network.postJson(`http://${hostIp}:18182/android_api/v1/shell/${dbId}`)
        .add("cmd", "getprop ro.product.brand")
        .setUserData({hostIp: hostIp, dbId: dbId, type: "brand"})
        .bind(root)
        .go(getDeviceBrandModel)
        
        // 再获取机型
        console.log("[云机详情] 请求获取机型:", hostIp, dbId)
        Network.postJson(`http://${hostIp}:18182/android_api/v1/shell/${dbId}`)
        .add("cmd", "getprop ro.product.model")
        .setUserData({hostIp: hostIp, dbId: dbId, type: "model"})
        .bind(root)
        .go(getDeviceBrandModel)
    }

    // 获取品牌和机型回调
    NetworkCallable {
        id: getDeviceBrandModel
        onError:
            (status, errorString, result, userData) => {
                console.debug("[云机详情] 获取品牌机型失败:", status, errorString, result)
                // 失败不影响功能，静默处理
            }
        onSuccess:
            (result, userData) => {
                try {
                    var res = JSON.parse(result)
                    if(res.code === 200){
                        var type = userData && userData.type ? userData.type : ""  // "brand" 或 "model"
                        var value = ""
                        
                        // 根据实际返回格式，品牌和机型信息在 data.message 字段中
                        if (res.data && res.data.message) {
                            value = res.data.message.toString().trim()
                        } else if (res.data && res.data.output) {
                            value = res.data.output.toString().trim()
                        } else if (res.data && typeof res.data === 'string') {
                            value = res.data.toString().trim()
                        } else if (res.data && res.data.value) {
                            value = res.data.value.toString().trim()
                        } else if (res.msg) {
                            value = res.msg.toString().trim()
                        }
                        
                        // 清理值（移除可能的引号、方括号等）
                        value = value.replace(/^\[|\]$/g, "").replace(/^"|"$/g, "").trim()
                        
                        if (value) {
                            if (type === "brand") {
                                root.deviceBrand = value
                                console.log("[云机详情] 获取到品牌:", value)
                            } else if (type === "model") {
                                root.deviceModel = value
                                console.log("[云机详情] 获取到机型:", value)
                            }
                        }
                    }
                } catch (e) {
                    console.error("[云机详情] 解析品牌机型数据失败:", e)
                }
            }
    }

    ColumnLayout {
        width: parent.width

        // 标题栏
        RowLayout {
            Layout.fillWidth: true
            FluText {
                text: qsTr("云机详情")
                font.bold: true
                font.pixelSize: 16
            }

            Item { Layout.fillWidth: true }

            FluImageButton {
                implicitWidth: 24
                implicitHeight: 24
                normalImage: "qrc:/res/common/btn_close_normal.png"
                hoveredImage: "qrc:/res/common/btn_close_normal.png"
                pushedImage: "qrc:/res/common/btn_close_normal.png"
                onClicked: root.close()
            }
        }

        // 详情信息
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            // 云机ID
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                FluText {
                    text: qsTr("云机ID：")
                    color: "#666"
                    Layout.preferredWidth: 120
                    horizontalAlignment: Text.AlignRight
                }
                FluText {
                    text: modelData?.dbId ?? modelData?.id ?? ""
                    Layout.fillWidth: true
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            var textToCopy = modelData?.dbId ?? modelData?.id ?? ""
                            if (textToCopy) {
                                FluTools.clipText(textToCopy)
                                showSuccess(qsTr("复制成功"))
                            }
                        }
                    }
                }
            }

            // 云机名称
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                FluText {
                    text: qsTr("云机名称：")
                    color: "#666"
                    Layout.preferredWidth: 120
                    horizontalAlignment: Text.AlignRight
                }
                FluText {
                    text: modelData?.displayName ?? modelData?.name ?? ""
                    Layout.fillWidth: true
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            var textToCopy = modelData?.displayName ?? modelData?.name ?? ""
                            if (textToCopy) {
                                FluTools.clipText(textToCopy)
                                showSuccess(qsTr("复制成功"))
                            }
                        }
                    }
                }
            }

            // 镜像版本
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                FluText {
                    text: qsTr("镜像版本：")
                    color: "#666"
                    Layout.preferredWidth: 120
                    horizontalAlignment: Text.AlignRight
                }
                FluText {
                    text: root.getImageVersionText()
                    Layout.fillWidth: true
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            var textToCopy = root.getImageVersionText()
                            if (textToCopy) {
                                FluTools.clipText(textToCopy)
                                showSuccess(qsTr("复制成功"))
                            }
                        }
                    }
                }
            }

            // Android版本
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                FluText {
                    text: qsTr("Android版本：")
                    color: "#666"
                    Layout.preferredWidth: 120
                    horizontalAlignment: Text.AlignRight
                }
                FluText {
                    text: root.getAndroidVersionText() ? ("Android " + root.getAndroidVersionText()) : ""
                    Layout.fillWidth: true
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            var textToCopy = root.getAndroidVersionText()
                            if (textToCopy) {
                                FluTools.clipText(textToCopy)
                                showSuccess(qsTr("复制成功"))
                            }
                        }
                    }
                }
            }

            // 品牌
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                FluText {
                    text: qsTr("品牌：")
                    color: "#666"
                    Layout.preferredWidth: 120
                    horizontalAlignment: Text.AlignRight
                }
                FluText {
                    text: root.deviceBrand || ""
                    Layout.fillWidth: true
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (root.deviceBrand) {
                                FluTools.clipText(root.deviceBrand)
                                showSuccess(qsTr("复制成功"))
                            }
                        }
                    }
                }
            }

            // 机型
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                FluText {
                    text: qsTr("机型：")
                    color: "#666"
                    Layout.preferredWidth: 120
                    horizontalAlignment: Text.AlignRight
                }
                FluText {
                    text: root.deviceModel || ""
                    Layout.fillWidth: true
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (root.deviceModel) {
                                FluTools.clipText(root.deviceModel)
                                showSuccess(qsTr("复制成功"))
                            }
                        }
                    }
                }
            }

            // ADB
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                FluText {
                    text: qsTr("ADB：")
                    color: "#666"
                    Layout.preferredWidth: 120
                    horizontalAlignment: Text.AlignRight
                }
                FluText {
                    text: modelData?.adb ? `${modelData.hostIp ?? ""}:${modelData.adb}` : ""
                    Layout.fillWidth: true
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            var textToCopy = modelData?.adb ? `${modelData.hostIp ?? ""}:${modelData.adb}` : ""
                            if (textToCopy) {
                                FluTools.clipText(textToCopy)
                                showSuccess(qsTr("复制成功"))
                            }
                        }
                    }
                }
            }

            // 容器网络
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                FluText {
                    text: qsTr("容器网络：")
                    color: "#666"
                    Layout.preferredWidth: 120
                    horizontalAlignment: Text.AlignRight
                }
                FluText {
                    text: modelData?.ip ?? modelData?.containerNetwork ?? ""
                    Layout.fillWidth: true
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            var textToCopy = modelData?.ip ?? modelData?.containerNetwork ?? ""
                            if (textToCopy) {
                                FluTools.clipText(textToCopy)
                                showSuccess(qsTr("复制成功"))
                            }
                        }
                    }
                }
            }

            // 局域网络
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                FluText {
                    text: qsTr("局域网络：")
                    color: "#666"
                    Layout.preferredWidth: 120
                    horizontalAlignment: Text.AlignRight
                }
                FluText {
                    text: modelData?.macvlanIp || "-"
                    Layout.fillWidth: true
                    
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            var textToCopy = modelData?.macvlanIp ?? ""
                            if (textToCopy) {
                                FluTools.clipText(textToCopy)
                                showSuccess(qsTr("复制成功"))
                            }
                        }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        // 关闭按钮
        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            FluButton {
                text: qsTr("关闭")
                onClicked: root.close()
            }
        }
    }
}
