// =====================================================
// SignupPage — 회원가입 화면 (모듈 6)
// =====================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"

Page {
    id: signup_page

    Connections {
        target: auth_controller
        function onSignup_succeeded() {
            app_controller.show_toast(qsTr("회원가입 완료. 로그인해주세요."))
            stack.pop()
        }
        function onSignup_failed(error_code) {
            error_label.text = error_code
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: 400
        spacing: 16

        Label {
            text: qsTr("회원가입")
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
            placeholderText: qsTr("비밀번호 (8자 이상)")
            echoMode: TextInput.Password
            font.pixelSize: 18
        }

        TextField {
            id: name_field
            Layout.fillWidth: true
            placeholderText: qsTr("닉네임")
            font.pixelSize: 18
        }

        AppButton {
            Layout.fillWidth: true
            text: qsTr("가입하기")
            enabled: !auth_controller.is_loading &&
                     email_field.text.length > 0 &&
                     pw_field.text.length >= 8 &&
                     name_field.text.length > 0
            onClicked: auth_controller.signup(
                email_field.text, pw_field.text, name_field.text)
        }

        AppButton {
            Layout.fillWidth: true
            variant: "secondary"
            text: qsTr("뒤로")
            onClicked: stack.pop()
        }

        Label {
            id: error_label
            Layout.fillWidth: true
            color: "#F44336"
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
