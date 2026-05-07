// =====================================================
// HomePage — 메인 화면 (식별·약 풀·복약 이력·보고서 진입)
// =====================================================
import QtQuick
import QtQuick.Controls
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
        spacing: 24

        Label {
            text: qsTr("환영합니다, ") +
                  (auth_controller.current_user_email || qsTr("사용자")) +
                  qsTr(" 님")
            font.pixelSize: 24
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: qsTr("폰으로 알약을 촬영하고 음성으로 질문하세요.")
            font.pixelSize: 16
            Layout.alignment: Qt.AlignHCenter
            color: "#616161"
        }

        // 폰 사용 안내 (USB 연결 미완료 시)
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

        // 메뉴 버튼들
        GridLayout {
            Layout.alignment: Qt.AlignHCenter
            columns: 2
            columnSpacing: 16
            rowSpacing: 16

            AppButton {
                text: qsTr("약 풀 관리")
                Layout.preferredWidth: 280
                onClicked: stack.push("PillPoolPage.qml")
            }

            AppButton {
                text: qsTr("복약 이력")
                Layout.preferredWidth: 280
                onClicked: stack.push("HistoryPage.qml")
            }

            AppButton {
                text: qsTr("통합 보고서")
                Layout.preferredWidth: 280
                onClicked: stack.push("ReportPage.qml")
            }

            AppButton {
                text: qsTr("최근 식별 결과")
                variant: "secondary"
                Layout.preferredWidth: 280
                enabled: pill_controller.last_request_id.length > 0
                onClicked: stack.push("IdentifyResultPage.qml")
            }
        }

        Item { Layout.fillHeight: true }
    }
}
