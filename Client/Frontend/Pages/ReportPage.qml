// =====================================================
// ReportPage — 통합 보고서 생성 (모듈 5)
// =====================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../Components"

Page {
    id: report_page

    Connections {
        target: report_controller
        function onGenerated(format, output_path) {
            app_controller.show_toast(
                qsTr("보고서 생성 완료: ") + format + " → " + output_path)
        }
        function onGenerate_failed(error_code) {
            app_controller.show_toast(qsTr("보고서 실패: ") + error_code)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: qsTr("의사 상담용 통합 보고서")
            font.pixelSize: 24
            font.bold: true
        }

        Label {
            text: qsTr("기간을 선택하면 해당 기간의 복약 이력 + 약별 부작용 인용을 출력합니다.")
            font.pixelSize: 14
            wrapMode: Text.WordWrap
            color: "#616161"
        }

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
        }

        Label {
            text: qsTr("출력 형식")
            font.pixelSize: 16
        }

        RowLayout {
            spacing: 16

            AppButton {
                text: qsTr("PDF 저장")
                Layout.fillWidth: true
                enabled: !report_controller.is_loading
                onClicked: report_controller.generate(
                    from_field.text, to_field.text, "pdf")
            }

            AppButton {
                text: qsTr("HTML 미리보기")
                variant: "secondary"
                Layout.fillWidth: true
                enabled: !report_controller.is_loading
                onClicked: report_controller.generate(
                    from_field.text, to_field.text, "html")
            }
        }

        Item { Layout.fillHeight: true }

        AppButton {
            text: qsTr("뒤로")
            variant: "secondary"
            Layout.fillWidth: true
            onClicked: stack.pop()
        }
    }
}
