// =====================================================
// DurAlertCard — DUR 위험 안내 카드
// =====================================================
// ⚠ 본 컴포넌트는 백엔드가 만든 텍스트를 그대로 표시만 한다.
// 단정 표현 가공·재작성 ❌. (요구사항 분석서 §9.3)
// =====================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: card

    // 사용자 정의 속성 — 백엔드가 채움
    property string dur_type: ""              // "병용금기" 등
    property string drug_a_name: ""
    property string drug_b_name: ""
    property string prohibit_reason: ""       // 식약처 본문 그대로 인용
    property string action_message: ""        // 정해진 템플릿

    width: parent ? parent.width : 600
    implicitHeight: content_layout.implicitHeight + 32
    radius: 8
    color: "#FFEBEE"   // 연한 빨강 배경 (위험 안내)
    border.color: "#F44336"
    border.width: 2

    ColumnLayout {
        id: content_layout
        anchors.fill: parent
        anchors.margins: 16
        spacing: 8

        // 카테고리 라벨
        Label {
            text: "[" + card.dur_type + "]"
            font.pixelSize: 18
            font.bold: true
            color: "#C62828"
        }

        // 약 페어
        Label {
            text: card.drug_a_name + " + " + card.drug_b_name
            font.pixelSize: 20
            font.bold: true
        }

        // 사유 (식약처 본문 그대로)
        Label {
            Layout.fillWidth: true
            text: qsTr("사유: ") + card.prohibit_reason
            font.pixelSize: 16
            wrapMode: Text.WordWrap
        }

        // 조치 (정해진 템플릿)
        Label {
            Layout.fillWidth: true
            text: qsTr("조치: ") + card.action_message
            font.pixelSize: 16
            font.bold: true
            color: "#C62828"
            wrapMode: Text.WordWrap
        }
    }
}
