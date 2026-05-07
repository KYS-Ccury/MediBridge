// =====================================================
// HomePage — 메인 화면 (촬영·약 풀·복약 이력·보고서 진입)
// =====================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "../Components"

Page {
    id: home_page

    Connections {
        target: pill_controller
        function onIdentify_succeeded() {
            stack.push("IdentifyResultPage.qml")
        }
        function onIdentify_failed(error_code) {
            app_controller.show_toast(qsTr("식별 실패: ") + error_code)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: qsTr("환영합니다, ") +
                  (auth_controller.current_user_email || qsTr("사용자")) +
                  qsTr(" 님")
            font.pixelSize: 24
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: qsTr("폰 카메라로 알약을 비춘 상태에서 아래 '촬영' 버튼을 누르세요.")
            font.pixelSize: 14
            wrapMode: Text.WordWrap
            Layout.alignment: Qt.AlignHCenter
            color: "#616161"
        }

        // ----- 폰 미연결 안내 -----
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            visible: !phone_link_controller.is_connected
            color: "#FFF3E0"
            border.color: "#FF9800"
            radius: 8

            Label {
                anchors.centerIn: parent
                text: qsTr("⚠ 폰을 USB로 연결한 후 사용하세요.")
                font.pixelSize: 16
                color: "#E65100"
            }
        }

        // ----- ⭐ 촬영 버튼 (가장 큰 액션) -----
        AppButton {
            text: pill_controller.is_loading
                  ? qsTr("⏳ 처리 중...")
                  : qsTr("📸 촬영 + 식별")
            Layout.fillWidth: true
            Layout.preferredHeight: 88
            font.pixelSize: 26
            enabled: phone_link_controller.is_connected && !pill_controller.is_loading
            onClicked: pill_controller.capture_and_identify()
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("폰 화면을 캡쳐하여 식별합니다 (사진은 폰 갤러리에 저장되지 않음).")
            font.pixelSize: 12
            color: "#9E9E9E"
            horizontalAlignment: Text.AlignHCenter
        }

        // ----- 메뉴 그리드 -----
        GridLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 16
            columns: 2
            columnSpacing: 16
            rowSpacing: 16

            AppButton {
                text: qsTr("약 풀 관리")
                variant: "secondary"
                Layout.preferredWidth: 240
                onClicked: stack.push("PillPoolPage.qml")
            }

            AppButton {
                text: qsTr("복약 이력")
                variant: "secondary"
                Layout.preferredWidth: 240
                onClicked: stack.push("HistoryPage.qml")
            }

            AppButton {
                text: qsTr("통합 보고서")
                variant: "secondary"
                Layout.preferredWidth: 240
                onClicked: stack.push("ReportPage.qml")
            }

            AppButton {
                text: qsTr("최근 식별 결과")
                variant: "secondary"
                Layout.preferredWidth: 240
                enabled: pill_controller.last_request_id.length > 0
                onClicked: stack.push("IdentifyResultPage.qml")
            }
        }

        Item { Layout.fillHeight: true }
    }
}
