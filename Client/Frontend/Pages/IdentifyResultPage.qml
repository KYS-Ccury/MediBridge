// =====================================================
// IdentifyResultPage — 식별 결과 표시
// =====================================================
// pill_controller 의 결과를 그대로 표시.
// ⚠ 단정 표현 가공 ❌. 백엔드가 만든 텍스트 그대로 표시.
// =====================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"

Page {
    id: result_page

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: result_page.width - 48
            anchors.margins: 24
            spacing: 16

            Label {
                text: qsTr("식별 결과")
                font.pixelSize: 24
                font.bold: true
            }

            // 신뢰도 분기 배지
            ConfidenceTierBadge {
                tier: pill_controller.confidence_tier
            }

            // TTS 안내 텍스트 (백엔드 생성, 그대로 표시)
            Label {
                Layout.fillWidth: true
                text: pill_controller.tts_text
                font.pixelSize: 18
                wrapMode: Text.WordWrap
                color: "#212121"
            }

            // DUR 결과 안내
            Label {
                Layout.fillWidth: true
                text: pill_controller.dur_result === "risk_found"
                      ? qsTr("⚠ DUR 위험 검출됨 — 아래 안내를 확인하세요.")
                      : qsTr("DUR에 등록 확인되지 않았습니다. 안심을 위해 약사·의사 상담을 권유드립니다.")
                font.pixelSize: 16
                wrapMode: Text.WordWrap
                color: pill_controller.dur_result === "risk_found" ? "#C62828" : "#2E7D32"
            }

            // (위험 케이스 시) DurAlertCard 표시 자리
            // Repeater 로 dur_detail_list_model 바인딩 예정 (TODO: 모델 컨텍스트 등록 후)
            Label {
                visible: pill_controller.dur_result === "risk_found"
                text: qsTr("(DUR 상세 카드는 dur_detail_list_model 컨텍스트 등록 후 Repeater로 표시)")
                font.pixelSize: 12
                color: "#9E9E9E"
            }

            // 후보 리스트 자리 (PillCandidateListModel 바인딩 예정)
            Label {
                text: qsTr("(식별 후보 리스트는 pill_candidate_list_model 등록 후 ListView로 표시)")
                font.pixelSize: 12
                color: "#9E9E9E"
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                AppButton {
                    text: qsTr("복용 기록 남기기")
                    Layout.fillWidth: true
                    onClicked: {
                        // TODO: 복약 이력 기록 다이얼로그
                        app_controller.show_toast(qsTr("복용 기록 — TODO"))
                    }
                }

                AppButton {
                    text: qsTr("처음으로")
                    variant: "secondary"
                    Layout.fillWidth: true
                    onClicked: stack.pop()
                }
            }
        }
    }
}
