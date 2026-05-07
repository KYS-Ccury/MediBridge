// =====================================================
// PillPoolPage — 사용자 약 풀 관리 (모듈 1)
// =====================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"

Page {
    id: pool_page

    Component.onCompleted: pill_controller.load_pool(false)

    Connections {
        target: pill_controller
        function onPool_changed() { pill_controller.load_pool(false) }
        function onPool_load_failed(error_code) {
            app_controller.show_toast(qsTr("약 풀 로드 실패: ") + error_code)
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

        // 약 풀 리스트 (TODO: pool_item_list_model 컨텍스트 등록 후 ListView)
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#FAFAFA"
            border.color: "#E0E0E0"

            Label {
                anchors.centerIn: parent
                text: qsTr("(pool_item_list_model 등록 후 ListView로 표시)")
                color: "#9E9E9E"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            AppButton {
                text: qsTr("음성으로 등록")
                Layout.fillWidth: true
                onClicked: app_controller.show_toast(
                    qsTr("폰의 PWA 페이지에서 마이크 입력해주세요"))
            }

            AppButton {
                text: qsTr("직접 입력")
                Layout.fillWidth: true
                onClicked: app_controller.show_toast(qsTr("직접 입력 — TODO"))
            }

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
