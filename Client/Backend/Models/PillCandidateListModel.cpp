#include "PillCandidateListModel.h"

namespace medibridge::models {

PillCandidateListModel::PillCandidateListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int PillCandidateListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : candidates_.size();
}

QVariant PillCandidateListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= candidates_.size()) {
        return {};
    }
    const auto& c = candidates_[index.row()];
    switch (role) {
        case ItemCodeRole:   return c.item_code;
        case DrugNameRole:   return c.drug_name;
        case ConfidenceRole: return c.confidence;
        case MatchKeysRole:  return c.match_keys;
        case InUserPoolRole: return c.in_user_pool;
        default: return {};
    }
}

QHash<int, QByteArray> PillCandidateListModel::roleNames() const
{
    // QML에서 사용할 role 이름 — 사용자 명명 규칙 snake_case
    return {
        {ItemCodeRole,   "item_code"},
        {DrugNameRole,   "drug_name"},
        {ConfidenceRole, "confidence"},
        {MatchKeysRole,  "match_keys"},
        {InUserPoolRole, "in_user_pool"},
    };
}

void PillCandidateListModel::set_candidates(const QVector<PillCandidate>& candidates)
{
    beginResetModel();
    candidates_ = candidates;
    endResetModel();
}

void PillCandidateListModel::clear()
{
    if (candidates_.isEmpty()) return;
    beginResetModel();
    candidates_.clear();
    endResetModel();
}

} // namespace medibridge::models
