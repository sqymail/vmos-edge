#ifndef TREEPROXYMODEL_H
#define TREEPROXYMODEL_H

#include <QObject>
#include <QSortFilterProxyModel>
#include <QCollator>

class TreeProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QString searchFilter READ searchFilter WRITE setSearchFilter NOTIFY searchFilterChanged)
    Q_PROPERTY(bool showRunningOnly READ showRunningOnly WRITE setShowRunningOnly NOTIFY showRunningOnlyChanged)
    Q_PROPERTY(bool showAllDevices READ showAllDevices WRITE setShowAllDevices NOTIFY showAllDevicesChanged)

public:
    TreeProxyModel(QObject *parent = nullptr);
    
    // 重写 setSourceModel 以连接信号
    void setSourceModel(QAbstractItemModel *sourceModel) override;

    // 过滤属性
    QString searchFilter() const;
    void setSearchFilter(const QString &filter);
    
    // 状态过滤属性
    bool showRunningOnly() const;
    void setShowRunningOnly(bool show);
    
    bool showAllDevices() const;
    void setShowAllDevices(bool show);
    
    // 计算过滤后的设备数量（通过代理索引）
    Q_INVOKABLE int getFilteredDeviceCountForHost(const QModelIndex& proxyIndex) const;
    // 计算过滤后的设备数量（通过hostId）
    Q_INVOKABLE int getFilteredDeviceCountByHostId(const QString& hostId) const;

signals:
    void searchFilterChanged();
    void showRunningOnlyChanged();
    void showAllDevicesChanged();

protected:
    // 核心筛选逻辑
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
    QVariant data(const QModelIndex &index, int role) const override;

private slots:
    void onSourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);

private:
    QCollator collator; // 可以提升为类成员或 static 局部，避免频繁构造
    
    // 过滤条件
    QString m_searchFilter;
    bool m_showRunningOnly;  // 是否只显示运行中的设备
    bool m_showAllDevices;    // 是否显示所有设备
    
    // 辅助方法
    bool matchesSearchFilter(const QModelIndex &sourceIndex) const;
    bool matchesStateFilter(const QModelIndex &sourceIndex) const;
    bool hasMatchingChildren(const QModelIndex &parent) const;
    void autoCheckMatchingDevices();
    void clearAllDeviceChecks();
};

#endif // TREEPROXYMODEL_H
