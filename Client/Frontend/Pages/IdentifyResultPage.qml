// =====================================================
// IdentifyResultPage — 식별 결과 표시
// =====================================================
// pill_controller 의 결과를 그대로 표시.
// ⚠ 단정 표현 가공 ❌. 백엔드가 만든 텍스트 그대로 표시.
// =====================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "../Components"

Page {
    id: result_page

    // 페이지 진입 시 신뢰도 기반 대화형 가이드 (FR-C4-01·FR-C6-02)
    //   HIGH  (95↑) : 단일 결과 안내 톤
    //   MEDIUM(70~) : Top-3 비교 안내 — "화면에서 직접 선택해주세요"
    //   LOW   (70↓) : 식별 어려움 + 약사·의사 상담 권유
    // tts_text 가 비어 있을 때만 페이지가 대신 안내 발화 → 중복 방지.
    Component.onCompleted: {
        if (pill_controller.tts_text.length > 0) return;   // PillController 자동 발화에 위임
        var tier = pill_controller.confidence_tier
        if (tier === "LOW") {
            tts_adapter.speak_conversational_guide(
                qsTr("식별이 어렵습니다. 약사·의사에게 직접 확인을 권유드립니다."))
        } else if (tier === "MEDIUM") {
            tts_adapter.speak_conversational_guide(
                qsTr("식별 결과 후보가 여러 개 입니다. 화면에서 선택해주세요."))
        }
    }

    // DUR 결과 변경 시 — 위험 검출 시 추가 발화 (FR-C3-02 위험 안내 / FR-B4-02 템플릿)
    Connections {
        target: pill_controller
        function onDur_result_changed() {
            if (pill_controller.dur_result === "risk_found") {
                tts_adapter.speak_conversational_guide(
                    qsTr("주의: 위험이 검출되었습니다. 약사·의사 상담이 필요합니다."))
            }
        }
    }

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

            // ===== 식별 후보 리스트 — pill_controller.candidates 바인딩 =====
            Label {
                text: qsTr("후보 약")
                font.pixelSize: 16
                font.bold: true
                visible: candidates_list.count > 0
            }

            ListView {
                id: candidates_list
                Layout.fillWidth: true
                Layout.preferredHeight: contentHeight
                interactive: false
                model: pill_controller.candidates
                spacing: 8

                delegate: Rectangle {
                    width: candidates_list.width
                    height: 92
                    color: model.in_user_pool ? "#E8F5E9" : "white"
                    border.color: "#D5DCE4"
                    border.width: 1
                    radius: 8

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            RowLayout {
                                Label {
                                    text: model.drug_name
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "#1A2238"
                                }
                                Rectangle {
                                    visible: model.in_user_pool
                                    color: "#2E7D32"
                                    radius: 4
                                    implicitWidth: in_pool_label.implicitWidth + 12
                                    implicitHeight: in_pool_label.implicitHeight + 6
                                    Label {
                                        id: in_pool_label
                                        anchors.centerIn: parent
                                        text: qsTr("내 약 풀")
                                        color: "white"
                                        font.pixelSize: 11
                                        font.bold: true
                                    }
                                }
                            }
                            Label {
                                text: qsTr("코드: ") + model.item_code
                                font.pixelSize: 12
                                color: "#5B6478"
                            }
                            Label {
                                text: qsTr("매칭: ") + (model.match_keys || qsTr("-"))
                                font.pixelSize: 12
                                color: "#5B6478"
                                visible: model.match_keys && model.match_keys.length > 0
                            }
                        }

                        ColumnLayout {
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 2

                            Label {
                                text: Math.round(model.confidence * 100) + "%"
                                font.pixelSize: 22
                                font.bold: true
                                color: "#0F4C81"
                                Layout.alignment: Qt.AlignHCenter
                            }
                            Label {
                                text: qsTr("신뢰도")
                                font.pixelSize: 10
                                color: "#5B6478"
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }
                    }
                }
            }

            Label {
                visible: candidates_list.count === 0
                text: qsTr("(식별 후보 없음)")
                font.pixelSize: 13
                color: "#9E9E9E"
            }

            // ===== DUR 결과 안내 =====
            Label {
                Layout.fillWidth: true
                Layout.topMargin: 12
                text: pill_controller.dur_result === "risk_found"
                      ? qsTr("⚠ DUR 위험 검출됨 — 아래 안내를 확인하세요.")
                      : qsTr("DUR에 등록 확인되지 않았습니다. 안심을 위해 약사·의사 상담을 권유드립니다.")
                font.pixelSize: 16
                font.bold: true
                wrapMode: Text.WordWrap
                color: pill_controller.dur_result === "risk_found" ? "#C62828" : "#2E7D32"
            }

            // DUR 상세 — pill_controller.dur_details 바인딩
            ListView {
                id: dur_list
                Layout.fillWidth: true
                Layout.preferredHeight: contentHeight
                interactive: false
                visible: pill_controller.dur_result === "risk_found"
                model: pill_controller.dur_details
                spacing: 8

                delegate: DurAlertCard {
                    width: dur_list.width
                    dur_type:        model.dur_type
                    drug_a_name:     model.drug_a_name
                    drug_b_name:     model.drug_b_name
                    prohibit_reason: model.prohibit_reason
                    action_message:  model.action_message
                }
            }

            // ===== 액션 버튼 =====
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 16
                spacing: 16

                AppButton {
                    text: qsTr("복용 기록 남기기")
                    Layout.fillWidth: true
                    enabled: candidates_list.count > 0
                    onClicked: record_dialog.open()
                }

                AppButton {
                    text: qsTr("처음으로")
                    variant: "secondary"
                    Layout.fillWidth: true
                    // 식별 결과 직전 스택은 CameraPage 일 수 있음.
                    // 단순 pop() 시 CameraPage 로 돌아가는 UX 버그가 있어 HomePage 까지 모두 정리.
                    //   stack.pop(null) — HomePage 가 첫 페이지(initialItem 직후 replace) 인 경우
                    //   일관성을 위해 명시적으로 HomePage 로 replace.
                    onClicked: stack.replace("HomePage.qml")
                }
            }
        }
    }

    // ===== 복약 이력 기록 다이얼로그 =====
    // 식별된 첫 후보의 item_code 로 history_controller.record() 호출.
    // 사용자는 개수·메모만 입력.
    Dialog {
        id: record_dialog
        title: qsTr("복용 기록")
        modal: true
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.9, 480)
        standardButtons: Dialog.Ok | Dialog.Cancel

        // 첫 후보 item_code/drug_name 캐시 (다이얼로그 열 때 갱신)
        property string sel_item_code: ""
        property string sel_drug_name: ""

        onOpened: {
            if (candidates_list.count > 0) {
                var first = pill_controller.candidates.data(
                    pill_controller.candidates.index(0, 0),
                    Qt.UserRole + 1)   // ItemCodeRole = UserRole+1
                sel_item_code = first || ""
                var name = pill_controller.candidates.data(
                    pill_controller.candidates.index(0, 0),
                    Qt.UserRole + 2)   // DrugNameRole
                sel_drug_name = name || ""
            }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Label {
                text: qsTr("기록할 약")
                font.pixelSize: 13
                color: "#5B6478"
            }
            Label {
                text: record_dialog.sel_drug_name.length > 0
                      ? record_dialog.sel_drug_name + " (" + record_dialog.sel_item_code + ")"
                      : qsTr("(후보 없음)")
                font.pixelSize: 16
                font.bold: true
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Label { text: qsTr("개수"); font.pixelSize: 13; color: "#5B6478"; Layout.topMargin: 8 }
            SpinBox {
                id: qty_input
                from: 1; to: 99
                value: 1
                Layout.fillWidth: true
            }

            Label { text: qsTr("메모 (선택)"); font.pixelSize: 13; color: "#5B6478"; Layout.topMargin: 8 }
            TextField {
                id: memo_input
                placeholderText: qsTr("예: 아침 식후 30분")
                Layout.fillWidth: true
            }
        }

        onAccepted: {
            if (sel_item_code.length === 0) {
                app_controller.show_toast(qsTr("기록할 약이 없습니다."))
                return
            }
            history_controller.record(sel_item_code, qty_input.value, memo_input.text)
            app_controller.show_toast(qsTr("복용 기록 요청 전송됨"))
            memo_input.text = ""
            qty_input.value = 1
        }
    }

    Connections {
        target: history_controller
        function onRecord_succeeded() { app_controller.show_toast(qsTr("복용 기록 저장 완료")) }
        function onRecord_failed(code) { app_controller.show_toast(qsTr("복용 기록 실패: ") + code) }
    }
}
