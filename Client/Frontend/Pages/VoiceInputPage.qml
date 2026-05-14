// =====================================================
// VoiceInputPage — 뒤로가기 및 실시간 텍스트 창 추가 버전
// =====================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    id: voice_input_page
    background: Rectangle { color: "#F8FAFF" }

    // 1. 상단 뒤로가기 버튼 (Main.qml의 전역 헤더 대신 페이지 전용 헤더 사용 가능)
    header: ToolBar {
        background: Rectangle { color: "white"; border.color: "#EEEEEE"; border.width: 1 }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            
            ToolButton {
                id: back_button
                contentItem: RowLayout {
                    spacing: 8
                    Text { text: "←"; font.pixelSize: 24; color: "#2C3E50" }
                    Text { text: qsTr("뒤로"); font.pixelSize: 18; color: "#2C3E50"; font.bold: true }
                }
                onClicked: {
                    // HomePage(이전 페이지)로 이동
                    if (stack.depth > 1) {
                        stack.pop()
                    }
                }
            }
            Item { Layout.fillWidth: true }
        }
    }

    // 페이지 진입 시 PC 측 듣기 시작 — 폰 PWA 가 보내는 텍스트 수신 활성
    Component.onCompleted: voice_controller.start_listening()
    Component.onDestruction: voice_controller.stop_listening()

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.9, 800)
        spacing: 30

        // 타이틀 섹션
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 8
            Label {
                text: qsTr("음성으로 질문하기")
                font.pixelSize: 30; font.bold: true
                color: "#2C3E50"; Layout.alignment: Qt.AlignHCenter
            }
            Label {
                text: qsTr("폰 PWA 의 텍스트 영역에 음성을 입력하고 [텍스트 전송] 을 눌러주세요")
                font.pixelSize: 16; color: "#7F8C8D"; Layout.alignment: Qt.AlignHCenter
                wrapMode: Text.WordWrap
                Layout.maximumWidth: 700
                horizontalAlignment: Text.AlignHCenter
            }
        }

        // 중앙 마이크 및 상태 카드 — voice_controller.is_listening 와 binding
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.preferredHeight: 220
            color: "white"; radius: 24; border.color: "#EEEEEE"
            layer.enabled: true

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 20

                // 애니메이션 마이크 (듣기 중일 때만 펄스)
                Rectangle {
                    width: 100; height: 100; radius: 50
                    color: voice_controller.is_listening ? "#E8F5E9" : "#F5F5F5"
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: "🎤"; font.pixelSize: 45; anchors.centerIn: parent }

                    SequentialAnimation on opacity {
                        running: voice_controller.is_listening
                        loops: Animation.Infinite
                        NumberAnimation { from: 1.0; to: 0.4; duration: 1000; easing.type: Easing.InOutQuad }
                        NumberAnimation { from: 0.4; to: 1.0; duration: 1000; easing.type: Easing.InOutQuad }
                    }
                }

                Label {
                    text: voice_controller.is_listening
                          ? qsTr("듣고 있습니다... (폰 PWA 의 음성 입력 자동 수신)")
                          : qsTr("대기 중")
                    font.pixelSize: 18; font.bold: true
                    color: voice_controller.is_listening ? "#2E7D32" : "#9E9E9E"
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        // 실시간 텍스트 변환 결과 — voice_controller.current_text 와 binding
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            Label {
                text: qsTr("인식된 내용:")
                font.pixelSize: 16; color: "#34495E"; font.bold: true
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 180
                color: "#F1F4F8"; radius: 16; border.color: "#D0D7DE"
                clip: true

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 16
                    ScrollBar.vertical.policy: ScrollBar.AlwaysOn

                    Text {
                        id: recognized_text_display
                        width: parent.width
                        text: voice_controller.current_text.length > 0
                              ? voice_controller.current_text
                              : qsTr("(폰 PWA 의 텍스트 영역에 음성 입력 후 [텍스트 전송] 을 눌러주세요)")
                        font.pixelSize: 20
                        color: voice_controller.current_text.length > 0 ? "#2C3E50" : "#9E9E9E"
                        wrapMode: Text.WordWrap
                        lineHeight: 1.4
                    }
                }
            }

            // 초기화 버튼
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 12

                Button {
                    text: qsTr("초기화")
                    onClicked: voice_controller.clear_text()
                    enabled: voice_controller.current_text.length > 0
                }
            }
        }

        // 안내 바
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 10
            Text { text: "💡"; font.pixelSize: 18; color: "#4A90E2" }
            Label {
                text: qsTr("폰 PWA → PC → 메인서버 자동 전달. 결과는 의도 분류 또는 약 등록 흐름으로 진행됩니다.")
                font.pixelSize: 13; color: "#4A90E2"
                wrapMode: Text.WordWrap
                Layout.maximumWidth: 700
            }
        }
    }
}

