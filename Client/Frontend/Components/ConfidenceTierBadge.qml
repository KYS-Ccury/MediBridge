// =====================================================
// ConfidenceTierBadge — 식별 신뢰도 분기 표시
// =====================================================
// HIGH (95%↑) / MEDIUM (70~95%) / LOW (70%↓) 3단계
// =====================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Rectangle {
    id: badge

    // 사용자 정의 속성
    property string tier: "MEDIUM"   // "HIGH" / "MEDIUM" / "LOW"

    width: tier_label.width + 24
    height: 36
    radius: 18

    color: {
        switch (tier) {
            case "HIGH":   return "#4CAF50"   // 초록
            case "MEDIUM": return "#FF9800"   // 주황
            case "LOW":    return "#F44336"   // 빨강
            default:       return "#9E9E9E"   // 회색
        }
    }

    Label {
        id: tier_label
        anchors.centerIn: parent
        text: {
            switch (badge.tier) {
                case "HIGH":   return qsTr("높음 (95%↑)")
                case "MEDIUM": return qsTr("보통 (70~95%)")
                case "LOW":    return qsTr("낮음 (70%↓)")
                default:       return qsTr("미정")
            }
        }
        color: "white"
        font.pixelSize: 14
        font.bold: true
    }
}
