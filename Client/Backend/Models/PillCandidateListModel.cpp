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
        case CropImageRole:  return c.crop_image;
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
        {CropImageRole,  "crop_image"},
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

void PillCandidateListModel::remove_at(int row)
{
    if (row < 0 || row >= candidates_.size()) return;
    beginRemoveRows({}, row, row);
    candidates_.removeAt(row);
    endRemoveRows();
}

void PillCandidateListModel::append_candidate(const QString& item_code,
                                              const QString& drug_name)
{
    if (item_code.isEmpty()) return;
    const int row = candidates_.size();
    beginInsertRows({}, row, row);
    PillCandidate c;
    c.item_code  = item_code;
    c.drug_name  = drug_name;
    c.confidence = 1.0;          // 사용자 수동 지정 → 확정 취급
    c.match_keys = QStringLiteral("사용자 직접 추가");
    candidates_.push_back(c);
    endInsertRows();
}

} // namespace medibridge::models
