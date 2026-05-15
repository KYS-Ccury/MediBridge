// =====================================================
// PillCandidateListModel — 식별 후보 리스트 (QAbstractListModel)
// =====================================================
// QML ListView 의 model 로 바인딩.
// 신뢰도 분기 표시 (HIGH/MEDIUM/LOW)에 사용.
// =====================================================
#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

namespace medibridge::models {

struct PillCandidate {
    QString item_code;
    QString drug_name;
    double  confidence = 0.0;
    QString match_keys;     // "engraving, shape, color" 식의 콤마 결합
    bool    in_user_pool = false;
    QString crop_image;     // data URI ("data:image/jpeg;base64,...") or empty
};

class PillCandidateListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        ItemCodeRole = Qt::UserRole + 1,
        DrugNameRole,
        ConfidenceRole,
        MatchKeysRole,
        InUserPoolRole,
        CropImageRole,
    };

    explicit PillCandidateListModel(QObject* parent = nullptr);

    // QAbstractListModel 인터페이스 (Qt 표준)
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // 데이터 갱신 (Controller에서 호출)
    void set_candidates(const QVector<PillCandidate>& candidates);
    void clear();

    // 사용자 편집 — 오검출 카드 삭제 / 수동 후보 추가 (QML 에서 호출)
    Q_INVOKABLE void remove_at(int row);
    Q_INVOKABLE void append_candidate(const QString& item_code,
                                      const QString& drug_name);

private:
    QVector<PillCandidate> candidates_;
};

} // namespace medibridge::models
