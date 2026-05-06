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
                text: qsTr("약에 대해 궁금한 점을 말씀해 주세요")
                font.pixelSize: 18; color: "#7F8C8D"; Layout.alignment: Qt.AlignHCenter
            }
        }

        // 중앙 마이크 및 상태 카드
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.preferredHeight: 220
            color: "white"; radius: 24; border.color: "#EEEEEE"
            layer.enabled: true

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 20

                // 애니메이션 마이크
                Rectangle {
                    width: 100; height: 100; radius: 50; color: "#E8F5E9"
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: "🎤"; font.pixelSize: 45; anchors.centerIn: parent }
                    
                    SequentialAnimation on opacity {
                        loops: Animation.Infinite
                        NumberAnimation { from: 1.0; to: 0.4; duration: 1000; easing.type: Easing.InOutQuad }
                        NumberAnimation { from: 0.4; to: 1.0; duration: 1000; easing.type: Easing.InOutQuad }
                    }
                }

                Label {
                    text: qsTr("듣고 있습니다...")
                    font.pixelSize: 22; font.bold: true; color: "#2E7D32"
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        // 💡 [신규] 실시간 텍스트 변환 결과 창 (STT Window)
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
                        // 실제 백엔드 연동 시: text: speech_controller.current_text
                        text: qsTr("이 약은 언제 먹어야 하나요? 식후 30분에 복용하는 것이 맞는지 확인해 주세요...") 
                        font.pixelSize: 20; color: "#2C3E50"
                        wrapMode: Text.WordWrap
                        lineHeight: 1.4
                    }
                }
            }
        }

        // 음성 안내 상태바
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 10
            Text { text: "🔊"; font.pixelSize: 18; color: "#4A90E2" }
            Label {
                text: qsTr("음성 안내 중입니다. 소리를 듣고 확인해 보세요.")
                font.pixelSize: 15; color: "#4A90E2"; font.bold: true
            }
        }
    }
}

