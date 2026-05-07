#include "DurDetailListModel.h"

namespace medibridge::models {

DurDetailListModel::DurDetailListModel(QObject* parent) : QAbstractListModel(parent) {}

int DurDetailListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : details_.size();
}

QVariant DurDetailListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= details_.size()) return {};
    const auto& d = details_[index.row()];
    switch (role) {
        case DurTypeRole:        return d.dur_type;
        case DrugANameRole:      return d.drug_a_name;
        case DrugBNameRole:      return d.drug_b_name;
        case ProhibitReasonRole: return d.prohibit_reason;
        case ActionMessageRole:  return d.action_message;
        default: return {};
    }
}

QHash<int, QByteArray> DurDetailListModel::roleNames() const
{
    return {
        {DurTypeRole,        "dur_type"},
        {DrugANameRole,      "drug_a_name"},
        {DrugBNameRole,      "drug_b_name"},
        {ProhibitReasonRole, "prohibit_reason"},
        {ActionMessageRole,  "action_message"},
    };
}

void DurDetailListModel::set_details(const QVector<DurDetail>& details)
{
    beginResetModel();
    details_ = details;
    endResetModel();
}

void DurDetailListModel::clear()
{
    if (details_.isEmpty()) return;
    beginResetModel();
    details_.clear();
    endResetModel();
}

} // namespace medibridge::models
