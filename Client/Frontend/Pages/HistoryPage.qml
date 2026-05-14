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

    Component.onCompleted: history_controller.load_list(from_date, to_date, 1, 50)

    Connections {
        target: history_controller
        function onList_load_failed(error_code) {
            app_controller.show_toast(qsTr("이력 로드 실패: ") + error_code)
        }
    }

    // 오늘·이번 주·이번 달 빠른 필터 함수
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
        to_field.text   = fmt(to)
        history_page.from_date = from_field.text
        history_page.to_date   = to_field.text
        history_controller.load_list(from_date, to_date, 1, 50)
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

        // ===== 기간 필터 =====
        RowLayout {
            spacing: 8
            Layout.fillWidth: true

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
            AppButton {
                text: qsTr("조회")
                onClicked: {
                    history_page.from_date = from_field.text
                    history_page.to_date = to_field.text
                    history_controller.load_list(from_date, to_date, 1, 50)
                }
            }
            Item { Layout.fillWidth: true }
        }

        // 빠른 기간 버튼들
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
                text: qsTr("전체 (필터 해제)")
                onClicked: {
                    from_field.text = ""
                    to_field.text = ""
                    history_page.from_date = ""
                    history_page.to_date = ""
                    history_controller.load_list("", "", 1, 50)
                }
            }
            Item { Layout.fillWidth: true }
            Label {
                text: qsTr("총 ") + history_controller.total_count + qsTr(" 건")
                font.pixelSize: 13
                font.bold: true
                color: "#0F4C81"
            }
        }

        // ===== 결과 리스트 — history_controller.items 바인딩 =====
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#FAFAFA"
            border.color: "#E0E0E0"
            radius: 8

            ListView {
                id: history_list
                anchors.fill: parent
                anchors.margins: 8
                clip: true
                model: history_controller.items
                spacing: 6
                ScrollBar.vertical: ScrollBar { active: true }

                delegate: Rectangle {
                    width: history_list.width
                    height: 76
                    color: "white"
                    border.color: "#D5DCE4"
                    border.width: 1
                    radius: 6

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12

                        // 시간 슬롯 색상 바
                        Rectangle {
                            implicitWidth: 6
                            Layout.fillHeight: true
                            radius: 3
                            color: {
                                if (model.time_slot === "아침")      return "#FFB74D"
                                if (model.time_slot === "점심")      return "#4CAF50"
                                if (model.time_slot === "저녁")      return "#42A5F5"
                                if (model.time_slot === "취침")      return "#7E57C2"
                                return "#BDBDBD"
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            RowLayout {
                                Label {
                                    text: model.drug_name
                                    font.pixelSize: 15
                                    font.bold: true
                                    color: "#1A2238"
                                }
                                Label {
                                    text: qsTr("×") + model.quantity
                                    font.pixelSize: 14
                                    color: "#0F4C81"
                                    font.bold: true
                                }
                            }
                            RowLayout {
                                spacing: 8
                                Label {
                                    text: model.intake_datetime.replace("T", " ").substring(0, 16)
                                    font.pixelSize: 11
                                    color: "#5B6478"
                                }
                                Label {
                                    visible: model.time_slot && model.time_slot.length > 0
                                    text: model.time_slot
                                    font.pixelSize: 11
                                    color: "#5B6478"
                                    font.bold: true
                                }
                            }
                            Label {
                                visible: model.memo && model.memo.length > 0
                                text: model.memo
                                font.pixelSize: 11
                                color: "#9E9E9E"
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }
                    }
                }

                Label {
                    anchors.centerIn: parent
                    visible: history_list.count === 0
                    text: qsTr("(해당 기간 복용 기록이 없습니다)")
                    color: "#9E9E9E"
                }
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
