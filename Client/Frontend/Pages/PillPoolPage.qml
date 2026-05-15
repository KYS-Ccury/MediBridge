// =====================================================
// PillPoolPage — 사용자 약 풀 관리 (모듈 1)
// =====================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"

Page {
    id: pool_page

    // 진입 시 약 풀 로드 — 인증 가드 통과 후에만 호출
    //   자동 로그인 시 토큰 set 타이밍 이슈로 401 받을 가능성을 차단.
    function trigger_load() {
        if (auth_controller.is_authenticated) {
            console.log("[PillPoolPage] load_pool 호출 — authed=true")
            pill_controller.load_pool(false)
        } else {
            console.warn("[PillPoolPage] 미인증 상태 — load_pool 보류 (auth 변경 시 자동 재시도)")
        }
    }
    Component.onCompleted: trigger_load()

    Connections {
        target: pill_controller
        function onPool_changed() { pill_controller.load_pool(false) }
        function onPool_loaded()  {
            console.log("[PillPoolPage] pool_loaded — model rows:",
                        pill_controller.pool_items.rowCount())
        }
        function onPool_load_failed(error_code) {
            app_controller.show_toast(qsTr("약 풀 로드 실패: ") + error_code)
        }
    }

    // 자동 로그인 직후 페이지가 먼저 그려지고 token set 이 뒤늦은 경우 재시도
    Connections {
        target: auth_controller
        function onIs_authenticated_changed() {
            if (auth_controller.is_authenticated && pool_list.count === 0) {
                console.log("[PillPoolPage] auth 변경 감지 → load_pool 재시도")
                pill_controller.load_pool(false)
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: qsTr("등록된 약 풀")
            font.pixelSize: 24
            font.bold: true
        }

        // 약 풀 리스트 — pill_controller.pool_items 바인딩
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#FAFAFA"
            border.color: "#E0E0E0"
            radius: 8

            ListView {
                id: pool_list
                anchors.fill: parent
                anchors.margins: 8
                clip: true
                model: pill_controller.pool_items
                spacing: 6

                delegate: Rectangle {
                    width: pool_list.width
                    height: 80
                    color: model.is_active ? "white" : "#F5F5F5"
                    border.color: "#D5DCE4"
                    border.width: 1
                    radius: 6

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Label {
                                text: model.drug_name
                                font.pixelSize: 16
                                font.bold: true
                                color: model.is_active ? "#1A2238" : "#9E9E9E"
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            RowLayout {
                                spacing: 8
                                Label {
                                    text: qsTr("코드: ") + model.item_code
                                    font.pixelSize: 11
                                    color: "#5B6478"
                                }
                                Rectangle {
                                    radius: 4
                                    color: "#E3F2FD"
                                    implicitWidth: reg_label.implicitWidth + 10
                                    implicitHeight: reg_label.implicitHeight + 4
                                    Label {
                                        id: reg_label
                                        anchors.centerIn: parent
                                        text: model.reg_method
                                        font.pixelSize: 10
                                        color: "#1565C0"
                                        font.bold: true
                                    }
                                }
                                Label {
                                    visible: !model.is_active
                                    text: qsTr("비활성")
                                    font.pixelSize: 10
                                    color: "#C62828"
                                    font.bold: true
                                }
                            }
                            Label {
                                text: qsTr("등록일: ") + model.created_at.substring(0, 10)
                                font.pixelSize: 10
                                color: "#9E9E9E"
                            }
                        }

                        ToolButton {
                            text: "✕"
                            font.pixelSize: 18
                            visible: model.is_active
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("이 약을 풀에서 제거")
                            onClicked: pill_controller.remove_from_pool(model.pool_id)
                        }
                    }
                }

                // 빈 풀 안내
                Label {
                    anchors.centerIn: parent
                    text: qsTr("등록된 약이 없습니다.\n아래 [음성으로 등록] 또는 [직접 입력] 으로 추가하세요.")
                    horizontalAlignment: Text.AlignHCenter
                    color: "#9E9E9E"
                    visible: pool_list.count === 0
                    wrapMode: Text.WordWrap
                }
            }
        }

        Label {
            text: qsTr("총 ") + pool_list.count + qsTr("건")
            font.pixelSize: 12
            color: "#5B6478"
        }

        // 등록 수단 3가지 (요구사항 FR-C5-01 + 확장: 단일 촬영 등록)
        // ① 음성 인식 등록 (폰 STT)
        // ② 카메라 촬영 → 식별 → 풀 등록
        // ③ 코드 직접 입력
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            AppButton {
                text: qsTr("🎙 음성")
                Layout.fillWidth: true
                ToolTip.text: qsTr("폰 PWA 의 마이크로 약 이름 발화 → 자동 등록")
                ToolTip.visible: hovered
                onClicked: stack.push("VoiceInputPage.qml")
            }

            AppButton {
                text: qsTr("📷 촬영")
                Layout.fillWidth: true
                ToolTip.text: qsTr("폰 카메라로 약 촬영 → 식별 결과에서 풀 등록")
                ToolTip.visible: hovered
                enabled: phone_link_controller.is_connected
                onClicked: {
                    if (phone_link_controller.is_connected) {
                        stack.push("CameraPage.qml")
                    } else {
                        app_controller.show_toast(qsTr("휴대폰을 먼저 USB로 연결해 주세요."))
                    }
                }
            }

            AppButton {
                text: qsTr("⌨ 직접 입력")
                Layout.fillWidth: true
                ToolTip.text: qsTr("식약처 품목기준코드로 직접 등록")
                ToolTip.visible: hovered
                onClicked: manual_add_dialog.open()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Layout.topMargin: 4

            AppButton {
                text: qsTr("전체 리셋")
                variant: "danger"
                Layout.fillWidth: true
                onClicked: confirm_reset_dialog.open()
            }

            AppButton {
                text: qsTr("뒤로")
                variant: "secondary"
                Layout.fillWidth: true
                onClicked: stack.pop()
            }
        }
    }

    // ===== 직접 입력 다이얼로그 — item_code 직접 추가 =====
    Dialog {
        id: manual_add_dialog
        title: qsTr("약 직접 등록")
        modal: true
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.9, 480)
        standardButtons: Dialog.Ok | Dialog.Cancel

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Label {
                text: qsTr("식약처 품목기준코드 (item_code)")
                font.pixelSize: 13
                color: "#5B6478"
            }
            TextField {
                id: code_input
                placeholderText: qsTr("예: 999800001")
                Layout.fillWidth: true
                inputMethodHints: Qt.ImhDigitsOnly
            }

            Label {
                text: qsTr("※ 코드 확인 — 시드: 999800001 (타이레놀500), 999800005 (이부프로펜200), 999800007 (베아제), 999800010 (아스피린)")
                font.pixelSize: 11
                color: "#9E9E9E"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        onAccepted: {
            if (code_input.text.trim().length === 0) {
                app_controller.show_toast(qsTr("코드를 입력해주세요"))
                return
            }
            pill_controller.add_to_pool(code_input.text.trim(), "MANUAL")
            code_input.text = ""
        }
    }

    // 전체 리셋 확인 다이얼로그 (안전 가드 — UI 차원에서도 확인)
    Dialog {
        id: confirm_reset_dialog
        title: qsTr("전체 리셋 확인")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.No

        Label {
            text: qsTr("등록된 모든 약을 비활성 처리합니다.\n계속하시겠습니까?")
            wrapMode: Text.WordWrap
        }

        onAccepted: pill_controller.reset_pool()
    }
}
