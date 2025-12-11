import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import FluentUI
import Qt.labs.platform
import Utils


FluPopup {
    id: root
    implicitWidth: 600
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    property var selectedDeviceList: [] // 用于存储批量操作时选中的云机列表
    property string installStatus: "idle" // 空闲
    property var selectedApkList: [] // 选中的文件

    Timer {
        id: closeTimer
        interval: 2000
        onTriggered: root.close()
    }

    Connections {
        target: fileCopyManager

        function onCopySucceeded() {
            console.log("BatchInstall: Copy succeeded. Adding to model.");
            hideLoading()
            closeTimer.start()
        }

        function onCopyFailed(reason) {
            console.log("BatchInstall: Copy failed: " + reason);
            hideLoading()
        }

        function onCopyFinished(success, message) {
            console.log("BatchInstall: Copy finished: " + message);
            //  更新显示进度
        }

        function onCopyProgress(copiedSize, totalSize) {
            console.log("BatchInstall: Copy progress: " + copiedSize / totalSize);
            //  更新显示进度
        }
    }

    FileDialog {
        id: fileDialog
        title: qsTr("选择安装包")
        fileMode: FileDialog.OpenFile
        nameFilters: [ "APK/XAPK (*.apk *.xapk)" ]
        onAccepted: {
            fileDialog.files.forEach(
                        item => {
                            const localPath = FluTools.toLocalPath(item)
                            const lowerPath = localPath.toLowerCase()
                            console.log("select file ", lowerPath)
                            selectedApkList.push(lowerPath)
                        })
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        //  标题栏
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            Layout.leftMargin: 20

            FluText {
                text: qsTr("批量安装（云机数量：%1）").arg(selectedDeviceList.length)
                font.bold: true
                font.pixelSize: 16
            }
        }

        //  选择文件
        RowLayout {
            Layout.leftMargin: 20
            spacing: 20
            FluText {
                text: qsTr("选择安装包")
                font.pixelSize: 16
            }

            Rectangle {
                Layout.preferredWidth: 400
                Layout.preferredHeight: 180
                border.color: "#409EFF"
                border.width: 1
                radius: 4

                FluFilledButton {
                    anchors.centerIn: parent
                    text: qsTr("拖拽文件到此处或点击上传")
                    Layout.preferredWidth: 300
                    normalColor: ThemeUI.blueColor
                    onClicked: fileDialog.open()
                }

                DropArea {
                    anchors.fill: parent
                    onDropped: (drop) => {
                                   if (drop.hasUrls && drop.urls.length > 0) {
                                        drop.urls.forEach(url => {
                                            var localPath = FluTools.toLocalPath(url)
                                            const lowerPath = localPath.toLowerCase()
                                            console.log("Dropped file path:", lowerPath)
                                            selectedApkList.push(lowerPath)
                                        })
                                    }
                    }
                }
            }
        }

        //  将选中的文件放置此处
        ColumnLayout {
            id: filelist
            Layout.fillHeight: true
        }

        //  最下方按钮
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            spacing: 60

            Item { Layout.fillWidth: true }

            FluButton {
                text: qsTr("取消")
                Layout.preferredWidth: 180
                normalColor: ThemeUI.grayColor
                onClicked: {
                    btnOk.enabled = true
                    root.selectedApkList = []
                    root.selectedDeviceList = []
                    root.close()
                }
            }

            Item { Layout.fillWidth: true }

            FluFilledButton {
                id: btnOk
                text: qsTr("确定")
                Layout.preferredWidth: 180
                normalColor: ThemeUI.blueColor
                enabled: root.selectedApkList.length && root.installStatus !== "ing"
                onClicked: {
                    if (root.selectedApkList.length > 0) {
                        console.log("===============file list :", root.selectedApkList);
                        root.installStatus = "ing"
                        //  todo
                    }
                }
            }

            Item { Layout.fillWidth: true }
        }
    }
}
