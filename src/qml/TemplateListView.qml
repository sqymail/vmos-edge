import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import FluentUI
import Utils
import Qt5Compat.GraphicalEffects
import Qt.labs.platform

Item {
    id: root
    implicitWidth: 1000
    implicitHeight: 800

    readonly property var itemWidth: [160.00, 160.00, 140.00, 100.00, 120.00, 150.00]
    readonly property int itemTotalWidth: itemWidth.reduce((acc, cur) => acc + cur, 0)
    property string currentImagePath: ""
    property var filteredModel: []
    property string filterText: ""
    property string filterVersion: ""
    property int sortType: 0 // 0: 导入时间（最晚优先）
    property var availableVersions: []

    Component.onCompleted: {
        root.currentImagePath = SettingsHelper.get("imagesPath", "")
        if(!root.currentImagePath){
            root.currentImagePath = FluTools.toLocalPath(StandardPaths.writableLocation(StandardPaths.AppLocalDataLocation) + "/images")
            SettingsHelper.save("imagesPath", root.currentImagePath)
        }
        // 指定模板数据文件路径并加载
        try {
            var tplPath = FluTools.toLocalPath(root.currentImagePath + "/template.json")
            tempLateModel.setFilePath(tplPath)
            tempLateModel.reloadConfig()
        } catch(e) {
            console.warn("tempLateModel setFilePath failed", e)
        }
        updateAvailableVersions()
        updateFilteredModel()
        
    }

    function updateAvailableVersions() {
        var versions = new Set()
        for (var i = 0; i < imagesModel.rowCount(); i++) {
            var index = imagesModel.index(i, 0)
            var version = imagesModel.data(index, ImagesModel.VersionRole).toString()
            if (version) {
                versions.add(version)
            }
        }
        
        // 转换为数组并排序
        var versionArray = Array.from(versions)
        versionArray.sort()
        
        // 添加"全部"选项
        root.availableVersions = [qsTr("全部Android版本")].concat(versionArray)
    }

    function updateFilteredModel() {
        var result = []
        for (var i = 0; i < imagesModel.rowCount(); i++) {
            var index = imagesModel.index(i, 0)
            var item = {
                name: imagesModel.data(index, ImagesModel.NameRole).toString(),
                fileName: imagesModel.data(index, ImagesModel.FileNameRole).toString(),
                version: imagesModel.data(index, ImagesModel.VersionRole).toString(),
                path: imagesModel.data(index, ImagesModel.PathRole).toString(),
                fileSize: imagesModel.data(index, ImagesModel.FileSizeRole).toString(),
                createTime: imagesModel.data(index, ImagesModel.CreateTimeRole).toString(),
                originalIndex: i
            }
            
            // 应用搜索过滤
            if (root.filterText !== "") {
                if (!item.fileName.toLowerCase().includes(root.filterText.toLowerCase())) {
                    continue
                }
            }
            
            // 应用版本过滤
            if (root.filterVersion !== "") {
                if (item.version !== root.filterVersion) {
                    continue
                }
            }
            
            result.push(item)
        }
        
        // 应用排序
        if (root.sortType === 0) {
            // 导入时间（最晚优先）
            result.sort((a, b) => b.createTime.localeCompare(a.createTime))
        } else if (root.sortType === 1) {
            // 导入时间（最早优先）
            result.sort((a, b) => a.createTime.localeCompare(b.createTime))
        } else if (root.sortType === 2) {
            // 文件大小（从大到小）
            result.sort((a, b) => parseFloat(b.fileSize) - parseFloat(a.fileSize))
        } else if (root.sortType === 3) {
            // 文件大小（从小到大）
            result.sort((a, b) => parseFloat(a.fileSize) - parseFloat(b.fileSize))
        } else if (root.sortType === 4) {
            // 名称（A-Z）
            result.sort((a, b) => a.name.localeCompare(b.name))
        } else if (root.sortType === 5) {
            // 名称（Z-A）
            result.sort((a, b) => b.name.localeCompare(a.name))
        }
        
        root.filteredModel = result
    }

    signal openDetail(string hostIp)
    signal openBatchMenu(var hostList, var button)
    signal openReset(string hostIp)
    signal openReboot(string hostIp)
    signal openClean(string hostIp)
    signal openImportImagePopup()
    signal deleteImage(string imageName, int imageIndex)
    signal changeImagePath(string newPath)
    signal goBack()

    FolderDialog {
        id: folderDialog
        title: qsTr("选择镜像存储目录")
        onAccepted: {
            const selectedPath = FluTools.toLocalPath(folder)
            root.changeImagePath(selectedPath)
        }
    }


    ColumnLayout{
        anchors.fill: parent
        anchors.margins: 10

        RowLayout{
            Layout.fillWidth: true
            Layout.preferredHeight: 32

            FluIconButton{
                iconSource: FluentIcons.ChevronLeft
                display: Button.TextBesideIcon
                iconSize: 13
                text: qsTr("返回")

                onClicked: {
                    root.goBack()
                }
            }

            Item{
                Layout.fillWidth: true
            }
        }

        Rectangle{
            Layout.fillHeight: true
            Layout.fillWidth: true

            ColumnLayout{
                anchors.fill: parent

                RowLayout{
                    Layout.preferredHeight: 32
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20

                    Image {
                        source: "qrc:/res/images/icon_list.svg"
                    }

                    FluText{
                        text: qsTr("机型列表")
                        color: ThemeUI.primaryColor
                        font.pixelSize: 16
                    }

                    Item {
                        Layout.fillWidth: true
                    }
                }

                RowLayout{
                    TabListView{
                        Layout.preferredWidth: 560
                        Layout.preferredHeight: 48
                        radius: 4
                        model: [qsTr("全部"), "Android 10", "Android 13", "Android 14", "Android 15"]
                        onMenuSelected:
                            (index)=> {
                                var versions = ["", "10", "13", "14", "15"]
                                root.filterVersion = versions[index] !== undefined ? versions[index] : ""
                                // 切换版本时重置滚动位置到顶部
                                listView.positionViewAtBeginning()
                            }
                    }

                    Item{
                        Layout.fillWidth: true
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    color: "#F8F9FA"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 20;
                        anchors.rightMargin: 10;
                        spacing: 5


                        Item{
                            Layout.fillHeight: true
                            Layout.preferredWidth: listView.width * itemWidth[0] / itemTotalWidth
                            // border.width: 1
                            FluText {
                                text: qsTr("品牌");
                                anchors.verticalCenter: parent.verticalCenter
                                font.bold: true
                            }
                        }

                        Item{
                            Layout.fillHeight: true
                            Layout.preferredWidth: listView.width * itemWidth[1] / itemTotalWidth
                            // border.width: 1
                            FluText {
                                text: qsTr("机型");
                                anchors.verticalCenter: parent.verticalCenter
                                font.bold: true
                            }
                        }
                        Item{
                            Layout.fillHeight: true
                            Layout.preferredWidth: listView.width * itemWidth[2] / itemTotalWidth
                            // border.width: 1
                            FluText {
                                text: qsTr("屏幕分辨率");
                                anchors.verticalCenter: parent.verticalCenter
                                font.bold: true
                            }
                        }
                        Item{
                            Layout.fillHeight: true
                            Layout.preferredWidth: listView.width * itemWidth[3] / itemTotalWidth
                            // border.width: 1
                            FluText {
                                text: qsTr("Android版本");
                                anchors.verticalCenter: parent.verticalCenter
                                font.bold: true
                            }
                        }
                        Item{
                            Layout.fillHeight: true
                            Layout.preferredWidth: listView.width * itemWidth[4] / itemTotalWidth
                            // border.width: 1
                            FluText {
                                text: qsTr("导入时间");
                                anchors.verticalCenter: parent.verticalCenter
                                font.bold: true
                            }
                        }
                        Item{
                            Layout.fillHeight: true
                            Layout.preferredWidth: listView.width * itemWidth[5] / itemTotalWidth
                            // border.width: 1
                            FluText {
                                text: qsTr("操作");
                                anchors.verticalCenter: parent.verticalCenter
                                font.bold: true
                            }
                        }
                    }
                }

                ListView {
                    id: listView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: tempLateModel
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { }

                    delegate: Rectangle {
                        property bool itemVisible: {
                            if (root.filterVersion === "") {
                                return true
                            }
                            var version = String(asopVersion || "")
                            return version === root.filterVersion
                        }
                        width: listView.width
                        height: itemVisible ? 50 : 0
                        color: "transparent"
                        visible: itemVisible

                        // 顶层覆盖层
                        Rectangle {
                            anchors.fill: parent
                            color: mouseArea.containsMouse ? "#26000000" : "transparent"
                            z: 999
                            Behavior on color { ColorAnimation { duration: 150 } }
                        }

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 20
                            anchors.rightMargin: 10
                            spacing: 5

                            Item{
                                Layout.fillHeight: true
                                Layout.preferredWidth: listView.width * itemWidth[0] / itemTotalWidth
                                // border.width: 1
                                FluText {
                                    // 列表头：品牌
                                    text: brand || ""
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: parent.width
                                    elide: Text.ElideRight
                                    maximumLineCount: 1

                                    MouseArea{
                                        anchors.fill: parent

                                        onClicked: {
                                            FluTools.clipText(brand || "")
                                            showSuccess(qsTr("复制成功"))
                                        }
                                    }
                                }
                            }
                            Item{
                                Layout.fillHeight: true
                                Layout.preferredWidth: listView.width * itemWidth[1] / itemTotalWidth
                                // border.width: 1

                                FluText {
                                    // 列表头：机型 => 显示机型（model）
                                    text: (tempLateModel.data(tempLateModel.index(index, 0), TemplateModel.ModelRole) || "")
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: parent.width
                                    elide: Text.ElideRight
                                    maximumLineCount: 1

                                    MouseArea{
                                        anchors.fill: parent

                                        onClicked: {
                                            var _modelName = tempLateModel.data(tempLateModel.index(index, 0), TemplateModel.ModelRole) || ""
                                            FluTools.clipText(_modelName)
                                            showSuccess(qsTr("复制成功"))
                                        }
                                    }
                                }
                            }
                            Item{
                                Layout.fillHeight: true
                                Layout.preferredWidth: listView.width * itemWidth[2] / itemTotalWidth
                                // border.width: 1
                                FluText {
                                    // 列表头：机型 => 显示布局（layout）
                                    text: layout || ""
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: parent.width
                                    elide: Text.ElideRight
                                    maximumLineCount: 1

                                    MouseArea{
                                        anchors.fill: parent

                                        onClicked: {
                                            FluTools.clipText(layout || "")
                                            showSuccess(qsTr("复制成功"))
                                        }
                                    }
                                }
                            }
                            Item{
                                Layout.fillHeight: true
                                Layout.preferredWidth: listView.width * itemWidth[3] / itemTotalWidth
                                // border.width: 1

                                // FluText {
                                //     // 列表头：Android版本
                                //     text: asopVersion || ""
                                //     anchors.verticalCenter: parent.verticalCenter
                                //     width: parent.width
                                //     elide: Text.ElideRight
                                //     maximumLineCount: 1
                                // }

                                Rectangle{
                                    width: 78
                                    height: 28
                                    color: "#E7F0FF"
                                    anchors.verticalCenter: parent.verticalCenter
                                    radius: 14

                                    FluText {
                                        anchors.centerIn: parent
                                        text: "Android " + (asopVersion || "")
                                        font.pixelSize: 12
                                        color: ThemeUI.primaryColor
                                    }
                                }
                            }
                            Item{
                                Layout.fillHeight: true
                                Layout.preferredWidth: listView.width * itemWidth[4] / itemTotalWidth
                                // border.width: 1
                                FluText {
                                    text: updateTime || ""
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: parent.width
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                }
                            }
                            Item{
                                Layout.fillHeight: true
                                Layout.preferredWidth: listView.width * itemWidth[5] / itemTotalWidth
                                // border.width: 1
                                RowLayout{
                                    anchors.verticalCenter: parent.verticalCenter

                                    FluIconButton{
                                        iconSource: FluentIcons.Delete
                                        visible: false
                                        iconSize: 12
                                        iconColor: "white"
                                        text: qsTr("删除")
                                        textColor: "white"
                                        display: Button.TextBesideIcon
                                        normalColor: "#f06969"
                                        hoverColor: "#f06969"

                                        onClicked: {
                                            root.deleteImage(name || "", index)
                                        }
                                    }
                                }
                            }

                        }

                        Rectangle{
                            anchors.bottom: parent.bottom
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: 1
                            color: "#EBEBEB"
                        }
                    }
                }

            }
        }
    }
}
