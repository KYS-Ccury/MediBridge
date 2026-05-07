// =====================================================
// PoolItemListModel — 사용자 약 풀 리스트
// =====================================================
#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

namespace medibridge::models {

struct PoolItem {
    int     pool_id = 0;
    QString item_code;
    QString drug_name;
    QString reg_method;       // "VOICE" / "MANUAL" / "IMAGE"
    bool    is_active = true;
    QString created_at;
};

class PoolItemListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        PoolIdRole = Qt::UserRole + 1,
        ItemCodeRole,
        DrugNameRole,
        RegMethodRole,
        IsActiveRole,
        CreatedAtRole,
    };

    explicit PoolItemListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void set_items(const QVector<PoolItem>& items);
    void clear();

private:
    QVector<PoolItem> items_;
};

} // namespace medibridge::models
