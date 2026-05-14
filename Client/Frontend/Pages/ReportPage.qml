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
                qsTr("보고서 생성 완료 (") + format + qsTr(") — 시스템 뷰어로 열기"))
        }
        function onGenerate_failed(error_code) {
            app_controller.show_toast(qsTr("보고서 실패: ") + error_code)
        }
    }

    // 빠른 기간 설정
    function set_date_range(days_back) {
        var to = new Date()
        var from = new Date()
        from.setDate(to.getDate() - days_back)
        var fmt = function(d) {
            var m = String(d.getMonth() + 1).padStart(2, "0")
            var dd = String(d.getDate()).padStart(2, "0")
            return d.getFullYear() + "-" + m + "-" + dd
        }
        from_field.text = fmt(from)
        to_field.text = fmt(to)
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
            text: qsTr("기간을 선택하면 해당 기간의 복약 이력 + 약별 주의·부작용 (식약처 e약은요 인용) 을 출력합니다.")
            font.pixelSize: 14
            wrapMode: Text.WordWrap
            color: "#616161"
        }

        // ===== 기간 입력 =====
        RowLayout {
            spacing: 8
            Label { text: qsTr("기간:") }
            TextField {
                id: from_field
                placeholderText: qsTr("YYYY-MM-DD")
                Layout.preferredWidth: 140
                inputMask: "9999-99-99"
            }
            Label { text: " ~ " }
            TextField {
                id: to_field
                placeholderText: qsTr("YYYY-MM-DD")
                Layout.preferredWidth: 140
                inputMask: "9999-99-99"
            }
            Item { Layout.fillWidth: true }
        }

        // 빠른 기간 버튼
        RowLayout {
            spacing: 8
            ToolButton {
                text: qsTr("최근 7일")
                onClicked: set_date_range(7)
            }
            ToolButton {
                text: qsTr("최근 30일")
                onClicked: set_date_range(30)
            }
            ToolButton {
                text: qsTr("최근 90일")
                onClicked: set_date_range(90)
            }
            ToolButton {
                text: qsTr("최근 1년")
                onClicked: set_date_range(365)
            }
            Item { Layout.fillWidth: true }
        }

        Label {
            text: qsTr("출력 형식")
            font.pixelSize: 16
            font.bold: true
            Layout.topMargin: 8
        }

        RowLayout {
            spacing: 16

            AppButton {
                text: qsTr("📄 PDF 저장 + 열기")
                Layout.fillWidth: true
                enabled: !report_controller.is_loading
                         && from_field.text.length === 10
                         && to_field.text.length === 10
                onClicked: report_controller.generate(
                    from_field.text, to_field.text, "pdf")
            }

            AppButton {
                text: qsTr("🌐 HTML 미리보기")
                variant: "secondary"
                Layout.fillWidth: true
                enabled: !report_controller.is_loading
                         && from_field.text.length === 10
                         && to_field.text.length === 10
                onClicked: report_controller.generate(
                    from_field.text, to_field.text, "html")
            }
        }

        // 로딩 인디케이터
        RowLayout {
            visible: report_controller.is_loading
            Layout.alignment: Qt.AlignHCenter
            spacing: 12
            BusyIndicator { running: parent.visible; implicitWidth: 24; implicitHeight: 24 }
            Label {
                text: qsTr("PDF/HTML 생성 중... (wkhtmltopdf 사용)")
                color: "#0F4C81"
                font.pixelSize: 13
            }
        }

        // 마지막 저장 경로 표시
        Rectangle {
            visible: report_controller.last_pdf_path.length > 0
            Layout.fillWidth: true
            implicitHeight: 60
            radius: 6
            color: "#F1F8E9"
            border.color: "#C5E1A5"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Label { text: "✓"; font.pixelSize: 20; color: "#2E7D32" }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Label {
                        text: qsTr("저장 완료 — 시스템 기본 뷰어로 자동 열림")
                        font.pixelSize: 12
                        color: "#2E7D32"
                        font.bold: true
                    }
                    Label {
                        text: report_controller.last_pdf_path
                        font.pixelSize: 10
                        color: "#5B6478"
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }
                }
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
