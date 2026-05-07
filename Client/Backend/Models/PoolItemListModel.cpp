#include "PoolItemListModel.h"

namespace medibridge::models {

PoolItemListModel::PoolItemListModel(QObject* parent) : QAbstractListModel(parent) {}

int PoolItemListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : items_.size();
}

QVariant PoolItemListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= items_.size()) return {};
    const auto& it = items_[index.row()];
    switch (role) {
        case PoolIdRole:    return it.pool_id;
        case ItemCodeRole:  return it.item_code;
        case DrugNameRole:  return it.drug_name;
        case RegMethodRole: return it.reg_method;
        case IsActiveRole:  return it.is_active;
        case CreatedAtRole: return it.created_at;
        default: return {};
    }
}

QHash<int, QByteArray> PoolItemListModel::roleNames() const
{
    return {
        {PoolIdRole,    "pool_id"},
        {ItemCodeRole,  "item_code"},
        {DrugNameRole,  "drug_name"},
        {RegMethodRole, "reg_method"},
        {IsActiveRole,  "is_active"},
        {CreatedAtRole, "created_at"},
    };
}

void PoolItemListModel::set_items(const QVector<PoolItem>& items)
{
    beginResetModel();
    items_ = items;
    endResetModel();
}

void PoolItemListModel::clear()
{
    if (items_.isEmpty()) return;
    beginResetModel();
    items_.clear();
    endResetModel();
}

} // namespace medibridge::models
