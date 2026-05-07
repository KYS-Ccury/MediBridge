// =====================================================
// DurDetailListModel — DUR 위험 검출 결과 리스트
// =====================================================
// ⚠ prohibit_reason 은 식약처 본문 그대로 (가공 X).
// ⚠ action_message 는 정해진 템플릿만 ("즉시 약사·의사 상담 필요" 등).
// =====================================================
#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

namespace medibridge::models {

struct DurDetail {
    QString dur_type;             // "병용금기" 등
    QString drug_a_name;
    QString drug_b_name;
    QString prohibit_reason;      // 식약처 본문 그대로
    QString action_message;       // 정해진 템플릿
};

class DurDetailListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        DurTypeRole = Qt::UserRole + 1,
        DrugANameRole,
        DrugBNameRole,
        ProhibitReasonRole,
        ActionMessageRole,
    };

    explicit DurDetailListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void set_details(const QVector<DurDetail>& details);
    void clear();

private:
    QVector<DurDetail> details_;
};

} // namespace medibridge::models
