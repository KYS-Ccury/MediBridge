// =====================================================
// HomePage — UX 개선 (완전 중앙 정렬 및 여백 최적화)
// =====================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "../Components"

Page {
    id: home_page
    background: Rectangle { color: "#F8FAFF" }

    Connections {
        target: pill_controller
        // ⚠ 중복 push 방지: CameraPage 가 이미 처리 (replace) 하므로
        //   HomePage 가 현재 보이는 페이지일 때만 push. 폰 PWA 가 보낸 사진
        //   (PhoneServer 자동 처리 흐름) 케이스에서만 동작.
        function onIdentify_succeeded() {
            if (stack.currentItem && stack.currentItem === home_page) {
                stack.push("IdentifyResultPage.qml")
            }
        }
        function onIdentify_failed(error_code) {
            // 어느 페이지에 있든 토스트는 표시
            app_controller.show_toast(qsTr("식별 실패: ") + error_code)
        }
    }

    // 최상위 컨테이너를 화면 정중앙에 고정
    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.9, 800) // 화면이 너무 넓어질 때 최대 가로 폭 제한
        spacing: 36

        // 1. 타이틀 섹션
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 12
            
            Text { 
                text: "💊"
                font.pixelSize: 64
                Layout.alignment: Qt.AlignHCenter 
            }
            
            Label {
                text: (auth_controller.current_user_email || qsTr("사용자")) + qsTr("님, 반갑습니다")
                font.pixelSize: 18
                color: "#7F8C8D"
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: qsTr("복약 안전 도우미")
                font.pixelSize: 34
                font.bold: true
                color: "#2C3E50"
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: qsTr("알약을 촬영하거나 음성으로 물어보세요")
                font.pixelSize: 16
                color: "#95A5A6"
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignHCenter
            }
        }

        // 2. 메인 액션 카드
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: 20

            ActionCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                iconSource: "🎙️"
                iconBgColor: "#E8F5E9"
                title: qsTr("음성으로 질문하기")
                description: qsTr("\"이 약 뭐예요?\" 같은 질문을 해보세요")
                onClicked: stack.push("VoiceInputPage.qml")
            }

            ActionCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                iconSource: "📷"
                iconBgColor: "#E3F2FD"
                title: qsTr("카메라로 직접 촬영")
                description: qsTr("여러 개의 알약을 한번에 촬영할 수 있습니다")
                
                // 로딩중이 아닐 때만 활성화
                enabled: !pill_controller.is_loading
            
                // 클릭 시 연결 상태 체크
                onClicked: {
                    if (phone_link_controller.is_connected) {
                        stack.push("CameraPage.qml")
                    } else {
                        app_controller.show_toast(qsTr("휴대폰을 먼저 USB로 연결해 주세요."))
                    } 
                }
            }
        }

        // 3. 하단 서브 메뉴 그리드
        GridLayout {
            columns: 2
            columnSpacing: 16
            rowSpacing: 16
            Layout.alignment: Qt.AlignHCenter

            SubMenuCard {
                title: qsTr("내 약 목록")
                iconText: "📋"
                onClicked: stack.push("PillPoolPage.qml")
            }
            SubMenuCard {
                title: qsTr("복약 이력")
                iconText: "🕒"
                onClicked: stack.push("HistoryPage.qml")
            }
            SubMenuCard {
                title: qsTr("통합 보고서")
                iconText: "📊"
                onClicked: stack.push("ReportPage.qml")
            }
            SubMenuCard {
                title: qsTr("최근 식별 결과")
                iconText: "🔍"
                // 사용자 요청: 비활성 대신 항상 활성. 이력 없을 때는 클릭 시 안내.
                onClicked: {
                    if (pill_controller.last_request_id.length > 0) {
                        stack.push("IdentifyResultPage.qml")
                    } else {
                        app_controller.show_toast(
                            qsTr("최근 식별 이력이 없습니다. 먼저 카메라로 약을 촬영해주세요."))
                    }
                }
            }
        }

        // 4. 음성 안내 ON/OFF 토글 (FR-C3-03 — 음성 ↔ 클릭 선택)
        //    TtsAdapter.enabled 와 양방향 바인딩, QSettings 영속.
        Rectangle {
            Layout.fillWidth: true
            height: 56
            radius: 12
            color: tts_adapter.enabled ? "#F0F7FF" : "#FAFAFA"
            border.color: tts_adapter.enabled ? "#D0E2FF" : "#E0E0E0"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                Text {
                    text: tts_adapter.enabled ? "🔊" : "🔇"
                    font.pixelSize: 20
                }
                Label {
                    text: tts_adapter.enabled
                          ? qsTr("음성 안내 켜짐 — 모든 안내를 소리로 들을 수 있습니다.")
                          : qsTr("음성 안내 꺼짐 — 화면 표시만 사용합니다.")
                    font.pixelSize: 15
                    color: tts_adapter.enabled ? "#1565C0" : "#616161"
                    font.bold: true
                    Layout.fillWidth: true
                }
                Switch {
                    checked: tts_adapter.enabled
                    onToggled: tts_adapter.set_enabled(checked)
                    ToolTip.text: qsTr("음성 안내 ON/OFF — 설정은 자동 저장됩니다")
                    ToolTip.visible: hovered
                }
            }
        }
    } // 💡 최상위 ColumnLayout 닫기

    // 페이지 진입 시 능동 가이드 1회 (FR-C6-01)
    Component.onCompleted: {
        tts_adapter.speak_active_guide(
            qsTr("메디브릿지에 오신 것을 환영합니다. 음성으로 질문하거나 카메라로 약을 촬영해보세요."))
    }

    // --- 인라인 컴포넌트 정의 (Page 내부에 위치해야 함) ---
    component ActionCard : Button {
        property string iconSource: ""
        property color iconBgColor: "white"
        property string title: ""
        property string description: ""

        contentItem: RowLayout {
            spacing: 24
            Item { Layout.fillWidth: true } 
            
            Rectangle {
                width: 64; height: 64; radius: 16
                color: iconBgColor
                Text { text: iconSource; font.pixelSize: 32; anchors.centerIn: parent }
            }
            
            ColumnLayout {
                spacing: 4
                Label { text: title; font.pixelSize: 20; font.bold: true; color: "#2C3E50" }
                Label { text: description; font.pixelSize: 15; color: "#95A5A6" }
            }
            
            Item { Layout.fillWidth: true } 
        }
        
        background: Rectangle {
            color: "white"; radius: 20
            border.color: parent.pressed ? "#3498DB" : "#EEEEEE"
            border.width: parent.pressed ? 2 : 1
            layer.enabled: true 
        }
    }

    component SubMenuCard : Button {
        property string iconText: ""
        property string title: ""
        
        Layout.preferredWidth: 220
        Layout.preferredHeight: 64

        contentItem: RowLayout {
            spacing: 12
            Item { Layout.fillWidth: true } 
            Text { text: iconText; font.pixelSize: 20 }
            Label { text: title; font.pixelSize: 16; font.bold: true; color: "#2C3E50" }
            Item { Layout.fillWidth: true } 
        }
        
        background: Rectangle {
            color: "white"; radius: 16
            border.color: parent.pressed ? "#3498DB" : "#EEEEEE"
        }
    }
} // 💡 Page 닫기