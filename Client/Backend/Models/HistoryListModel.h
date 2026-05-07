// =====================================================
// HistoryListModel — 복약 이력 리스트 (QAbstractListModel)
// =====================================================
#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

namespace medibridge::models {

struct HistoryItem {
    QString intake_id;
    QString item_code;
    QString drug_name;
    QString intake_datetime;    // ISO 8601
    int     quantity = 0;
    QString memo;
    QString time_slot;          // "아침" / "점심" / "저녁" / "취침"
};

class HistoryListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        IntakeIdRole = Qt::UserRole + 1,
        ItemCodeRole,
        DrugNameRole,
        IntakeDatetimeRole,
        QuantityRole,
        MemoRole,
        TimeSlotRole,
    };

    explicit HistoryListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void set_items(const QVector<HistoryItem>& items);
    void clear();

private:
    QVector<HistoryItem> items_;
};

} // namespace medibridge::models
