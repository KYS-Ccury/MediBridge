#include "HistoryListModel.h"

namespace medibridge::models {

HistoryListModel::HistoryListModel(QObject* parent) : QAbstractListModel(parent) {}

int HistoryListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : items_.size();
}

QVariant HistoryListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= items_.size()) return {};
    const auto& it = items_[index.row()];
    switch (role) {
        case IntakeIdRole:        return it.intake_id;
        case ItemCodeRole:        return it.item_code;
        case DrugNameRole:        return it.drug_name;
        case IntakeDatetimeRole:  return it.intake_datetime;
        case QuantityRole:        return it.quantity;
        case MemoRole:            return it.memo;
        case TimeSlotRole:        return it.time_slot;
        default: return {};
    }
}

QHash<int, QByteArray> HistoryListModel::roleNames() const
{
    return {
        {IntakeIdRole,        "intake_id"},
        {ItemCodeRole,        "item_code"},
        {DrugNameRole,        "drug_name"},
        {IntakeDatetimeRole,  "intake_datetime"},
        {QuantityRole,        "quantity"},
        {MemoRole,            "memo"},
        {TimeSlotRole,        "time_slot"},
    };
}

void HistoryListModel::set_items(const QVector<HistoryItem>& items)
{
    beginResetModel();
    items_ = items;
    endResetModel();
}

void HistoryListModel::clear()
{
    if (items_.isEmpty()) return;
    beginResetModel();
    items_.clear();
    endResetModel();
}

} // namespace medibridge::models
