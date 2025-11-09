#ifndef SELECTEDLISTMODEL_H
#define SELECTEDLISTMODEL_H

#include <QAbstractListModel>
#include "treemodel.h"
#include "structs.h"

class TreeProxyModel;

class SelectedListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit SelectedListModel(QObject *parent = nullptr);

    Q_INVOKABLE void setSourceModel(TreeModel *treeModel);
    Q_INVOKABLE void setProxyModel(TreeProxyModel *proxyModel);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    QHash<int, QByteArray> roleNames() const override;

private slots:
    void onSourceReset();
    void onSourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = {});
    void onSourceRowsInserted(const QModelIndex &parent, int first, int last);
    void onSourceRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void onFilterChanged();

private:
    void updateAllDevicesData();
    bool matchesFilter(const QModelIndex& deviceIndex) const;
    
    TreeModel *m_sourceModel = nullptr;
    TreeProxyModel *m_proxyModel = nullptr;
    QList<DeviceData> m_selectedDevices;
};

#endif // SELECTEDLISTMODEL_H
