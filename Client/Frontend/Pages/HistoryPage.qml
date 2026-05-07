// =====================================================
// HistoryPage — 복약 이력 조회 (모듈 2)
// =====================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"

Page {
    id: history_page

    property string from_date: ""
    property string to_date: ""

    Component.onCompleted: history_controller.load_list(from_date, to_date, 1, 20)

    Connections {
        target: history_controller
        function onList_load_failed(error_code) {
            app_controller.show_toast(qsTr("이력 로드 실패: ") + error_code)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: qsTr("복약 이력")
            font.pixelSize: 24
            font.bold: true
        }

        // 기간 필터 자리 (DatePicker — TODO)
        RowLayout {
            spacing: 8
            Label { text: qsTr("기간:") }
            TextField {
                id: from_field
                placeholderText: qsTr("YYYY-MM-DD")
                Layout.preferredWidth: 140
            }
            Label { text: " ~ " }
            TextField {
                id: to_field
                placeholderText: qsTr("YYYY-MM-DD")
                Layout.preferredWidth: 140
            }
            AppButton {
                text: qsTr("조회")
                onClicked: {
                    history_page.from_date = from_field.text
                    history_page.to_date = to_field.text
                    history_controller.load_list(from_date, to_date, 1, 20)
                }
            }
        }

        // 결과 리스트 자리 (history_list_model 컨텍스트 등록 후 ListView)
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#FAFAFA"
            border.color: "#E0E0E0"

            Label {
                anchors.centerIn: parent
                text: qsTr("총 ") + history_controller.total_count + qsTr(" 건")
                color: "#616161"
            }
        }

        AppButton {
            text: qsTr("뒤로")
            variant: "secondary"
            Layout.fillWidth: true
            onClicked: stack.pop()
        }
    }
}
