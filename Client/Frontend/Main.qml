// =====================================================
// Main.qml — 앱 진입점 (Window + StackView)
// =====================================================
// Q3=A: StackView 라우팅
// Q2=A: Material 스타일 (Main.cpp에서 QQuickStyle::setStyle("Material"))
//
// ⚠ 본 파일은 UI만 담당 — 비즈니스 로직 ❌.
//   모든 동작은 *_controller 의 Q_INVOKABLE 메소드 호출 또는
//   *_changed/_succeeded/_failed 시그널 수신만.
// =====================================================
import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "Components"

ApplicationWindow {
    id: app_window
    visible: true
    width: 1024
    height: 720
    title: qsTr("메디브릿지")

    // Material 테마 (노인층 친화 — 큰 글자, 명확한 색상)
    Material.theme: Material.Light
    Material.primary: Material.Blue
    Material.accent: Material.LightBlue

    // -------------------- 상단 헤더 --------------------
    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.margins: 8

            Label {
                text: qsTr("메디브릿지")
                font.pixelSize: 22
                font.bold: true
                Layout.fillWidth: true
            }

            // 폰 USB 연결 상태 표시 (Components/PhoneStatusIndicator.qml)
            PhoneStatusIndicator {
                Layout.alignment: Qt.AlignVCenter
            }

            // 로그아웃 버튼 (인증 시에만 표시)
            ToolButton {
                visible: auth_controller.is_authenticated
                text: qsTr("로그아웃")
                font.pixelSize: 16
                onClicked: auth_controller.logout()
            }
        }
    }

    // -------------------- 라우터 (StackView) --------------------
    StackView {
        id: stack
        anchors.fill: parent
        initialItem: "Pages/LoginPage.qml"
    }

    // -------------------- 전역 시그널 핸들러 --------------------
    Connections {
        target: auth_controller

        function onLogin_succeeded() {
            stack.replace("Pages/HomePage.qml")
        }
        function onLogout_completed() {
            stack.replace("Pages/LoginPage.qml")
        }
        function onSession_expired() {
            // FR-C7-04 — 자동 로그인 토큰 만료 시 토스트로 안내
            app_controller.show_toast(
                qsTr("세션이 만료되어 다시 로그인이 필요합니다."))
        }
    }

    // 전역 토스트
    Connections {
        target: app_controller
        function onToast_requested(message) {
            toast_label.text = message
            toast.open()
            toast_timer.restart()
        }
    }

    Popup {
        id: toast
        x: (parent.width - width) / 2
        y: parent.height - height - 40
        width: 400
        padding: 16
        modal: false
        closePolicy: Popup.CloseOnPressOutside

        Label {
            id: toast_label
            anchors.fill: parent
            font.pixelSize: 16
            wrapMode: Text.WordWrap
        }

        Timer {
            id: toast_timer
            interval: 3000
            onTriggered: toast.close()
        }
    }

    // 전역 로딩 오버레이
    Rectangle {
        anchors.fill: parent
        color: "#80000000"
        visible: app_controller.is_global_loading
        z: 999

        BusyIndicator {
            anchors.centerIn: parent
            running: parent.visible
        }
    }
}
