#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QString>
#include <QHash>
#include <QByteArray>
#include <QList>
#include <QModelIndex>
#include <QVariant>

struct TemplateItem {
    QString brand;
    QString model;
    QString layout;
    QString name;
    QString filePath;
    QString asopVersion;
    QString updateTime;
    QString id;
    QString pwd;
};

class TemplateModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum TemplateRoles {
        BrandRole = Qt::UserRole + 1,
        ModelRole,
        LayoutRole,
        NameRole,
        FilePathRole,
        AsopVersionRole,
        UpdateTimeRole,
        IdRole,
        PwdRole
    };
    Q_ENUM(TemplateRoles)
    explicit TemplateModel(QObject* parent = nullptr);
    ~TemplateModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    QHash<int, QByteArray> roleNames() const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    Q_INVOKABLE void addItem(const QString& brand, const QString& model, const QString& layout, const QString& name, const QString& filePath, const QString& asopVersion);
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void saveConfig();
    Q_INVOKABLE void reloadConfig();
    Q_INVOKABLE void setFilePath(const QString& filePath);
    Q_INVOKABLE QString filePath() const { return m_filePath; }

private:
    void loadConfig();

private:
    QList<TemplateItem> m_items;
    QString m_filePath;
};

