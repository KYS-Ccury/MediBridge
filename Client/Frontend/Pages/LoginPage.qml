// =====================================================
// LoginPage — 로그인 화면 (모듈 6)
// =====================================================
// auth_controller.login(email, password) 호출만.
// 결과는 Connections로 수신 (Main.qml 의 onLogin_succeeded 가 페이지 전환 처리).
// =====================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "../Components"

Page {
    id: login_page

    Connections {
        target: auth_controller
        function onLogin_failed(error_code) {
            error_label.text = error_code
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: 400
        spacing: 16

        Label {
            text: qsTr("메디브릿지 로그인")
            font.pixelSize: 28
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        TextField {
            id: email_field
            Layout.fillWidth: true
            placeholderText: qsTr("이메일")
            font.pixelSize: 18
            inputMethodHints: Qt.ImhEmailCharactersOnly
        }

        TextField {
            id: pw_field
            Layout.fillWidth: true
            placeholderText: qsTr("비밀번호")
            echoMode: TextInput.Password
            font.pixelSize: 18
        }

        AppButton {
            Layout.fillWidth: true
            text: qsTr("로그인")
            enabled: !auth_controller.is_loading &&
                     email_field.text.length > 0 &&
                     pw_field.text.length > 0
            onClicked: auth_controller.login(email_field.text, pw_field.text)
        }

        AppButton {
            Layout.fillWidth: true
            variant: "secondary"
            text: qsTr("회원가입")
            onClicked: stack.push("SignupPage.qml")
        }

        Label {
            id: error_label
            Layout.fillWidth: true
            color: "#F44336"
            font.pixelSize: 14
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            running: auth_controller.is_loading
            visible: auth_controller.is_loading
        }
    }
}
