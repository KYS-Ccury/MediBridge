// =====================================================
// IdentifyResultPage — 식별 결과 표시
// =====================================================
// pill_controller 의 결과를 그대로 표시.
// ⚠ 단정 표현 가공 ❌. 백엔드가 만든 텍스트 그대로 표시.
// =====================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "../Components"

Page {
    id: result_page

    // 페이지 진입 시 신뢰도 기반 대화형 가이드 (FR-C4-01·FR-C6-02)
    //   HIGH  (95↑) : 단일 결과 안내 톤
    //   MEDIUM(70~) : Top-3 비교 안내 — "화면에서 직접 선택해주세요"
    //   LOW   (70↓) : 식별 어려움 + 약사·의사 상담 권유
    // tts_text 가 비어 있을 때만 페이지가 대신 안내 발화 → 중복 방지.
    Component.onCompleted: {
        if (pill_controller.tts_text.length > 0) return;   // PillController 자동 발화에 위임
        var tier = pill_controller.confidence_tier
        if (tier === "LOW") {
            tts_adapter.speak_conversational_guide(
                qsTr("식별이 어렵습니다. 약사·의사에게 직접 확인을 권유드립니다."))
        } else if (tier === "MEDIUM") {
            tts_adapter.speak_conversational_guide(
                qsTr("식별 결과 후보가 여러 개 입니다. 화면에서 선택해주세요."))
        }
    }

    // DUR 결과 변경 시 — 위험 검출 시 추가 발화 (FR-C3-02 위험 안내 / FR-B4-02 템플릿)
    Connections {
        target: pill_controller
        function onDur_result_changed() {
            if (pill_controller.dur_result === "risk_found") {
                tts_adapter.speak_conversational_guide(
                    qsTr("주의: 위험이 검출되었습니다. 약사·의사 상담이 필요합니다."))
            }
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: result_page.width - 48
            anchors.margins: 24
            spacing: 16

            Label {
                text: qsTr("식별 결과")
                font.pixelSize: 24
                font.bold: true
            }

            // 신뢰도 분기 배지
            ConfidenceTierBadge {
                tier: pill_controller.confidence_tier
            }

            // TTS 안내 텍스트 (백엔드 생성, 그대로 표시)
            Label {
                Layout.fillWidth: true
                text: pill_controller.tts_text
                font.pixelSize: 18
                wrapMode: Text.WordWrap
                color: "#212121"
            }

            // ===== 식별 후보 리스트 — pill_controller.candidates 바인딩 =====
            Label {
                text: qsTr("후보 약")
                font.pixelSize: 16
                font.bold: true
                visible: candidates_list.count > 0
            }

            ListView {
                id: candidates_list
                Layout.fillWidth: true
                Layout.preferredHeight: contentHeight
                interactive: false
                model: pill_controller.candidates
                spacing: 8

                delegate: Rectangle {
                    // drug_name 미확정(빈 값) → 노란 배경 강조 (사용자 약 풀에서 선택 안내)
                    property bool name_unknown: !model.drug_name || model.drug_name.length === 0
                    property bool has_crop: model.crop_image && model.crop_image.length > 0
                    width: candidates_list.width
                    height: name_unknown ? 150 : 112
                    color: name_unknown ? "#FFF8E1"
                                        : (model.in_user_pool ? "#E8F5E9" : "white")
                    border.color: name_unknown ? "#FFC107"
                                               : "#D5DCE4"
                    border.width: name_unknown ? 2 : 1
                    radius: 8

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12

                        // 검출된 알약 실물 crop 썸네일 — 어느 카드=어느 실물 식별용
                        Rectangle {
                            Layout.preferredWidth: 84
                            Layout.preferredHeight: 84
                            Layout.alignment: Qt.AlignVCenter
                            color: "#F5F5F5"
                            border.color: "#D5DCE4"
                            radius: 6
                            clip: true
                            Image {
                                anchors.fill: parent
                                anchors.margins: 2
                                source: has_crop ? model.crop_image : ""
                                fillMode: Image.PreserveAspectFit
                                visible: has_crop
                                asynchronous: true
                            }
                            Label {
                                anchors.centerIn: parent
                                visible: !has_crop
                                text: qsTr("이미지\n없음")
                                horizontalAlignment: Text.AlignHCenter
                                font.pixelSize: 10
                                color: "#9E9E9E"
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            RowLayout {
                                Label {
                                    // 식별 미확정 시 안내 텍스트
                                    text: name_unknown
                                          ? qsTr("🔍 식별 미확정 — 아래 특징으로 약 풀에서 선택해주세요")
                                          : model.drug_name
                                    font.pixelSize: name_unknown ? 13 : 16
                                    font.bold: true
                                    color: name_unknown ? "#E65100" : "#1A2238"
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                }
                                Rectangle {
                                    visible: !name_unknown && model.in_user_pool
                                    color: "#2E7D32"
                                    radius: 4
                                    implicitWidth: in_pool_label.implicitWidth + 12
                                    implicitHeight: in_pool_label.implicitHeight + 6
                                    Label {
                                        id: in_pool_label
                                        anchors.centerIn: parent
                                        text: qsTr("내 약 풀")
                                        color: "white"
                                        font.pixelSize: 11
                                        font.bold: true
                                    }
                                }
                            }
                            Label {
                                text: qsTr("코드: ") + model.item_code
                                font.pixelSize: 12
                                color: "#5B6478"
                                visible: !name_unknown
                            }
                            // 매칭 키 — 식별 미확정 시 큰 글자로 강조
                            Label {
                                text: name_unknown
                                      ? (model.match_keys || qsTr("-"))
                                      : (qsTr("매칭: ") + (model.match_keys || qsTr("-")))
                                font.pixelSize: name_unknown ? 14 : 12
                                font.bold: name_unknown
                                color: name_unknown ? "#1A2238" : "#5B6478"
                                visible: model.match_keys && model.match_keys.length > 0
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                            // 식별 미확정 시 — 약 풀에서 선택 안내 버튼
                            AppButton {
                                visible: name_unknown
                                text: qsTr("💊 내 약 풀에서 선택")
                                Layout.preferredWidth: 200
                                onClicked: {
                                    record_dialog.preset_source = "pool"
                                    record_dialog.preset_item_code = ""
                                    record_dialog.preset_drug_name = ""
                                    record_dialog.open()
                                }
                            }
                            // 식별 성공 시 — 이 후보로 바로 등록·기록
                            AppButton {
                                visible: !name_unknown
                                text: qsTr("💊 이 약 기록·등록")
                                Layout.preferredWidth: 200
                                onClicked: {
                                    // 후보 카드 클릭 → record_dialog 가 이 약으로 자동 시작
                                    record_dialog.preset_source = "candidate"
                                    record_dialog.preset_item_code = model.item_code
                                    record_dialog.preset_drug_name = model.drug_name
                                    record_dialog.open()
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 2

                            // 오검출 카드 삭제 (사용자 편집)
                            ToolButton {
                                text: "✕"
                                font.pixelSize: 18
                                Layout.alignment: Qt.AlignHCenter
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("이 후보를 목록에서 제거 (오검출 시)")
                                onClicked: pill_controller.candidates.remove_at(index)
                            }
                            Label {
                                visible: !name_unknown
                                text: Math.round(model.confidence * 100) + "%"
                                font.pixelSize: 22
                                font.bold: true
                                color: "#0F4C81"
                                Layout.alignment: Qt.AlignHCenter
                            }
                            Label {
                                visible: !name_unknown
                                text: qsTr("신뢰도")
                                font.pixelSize: 10
                                color: "#5B6478"
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }
                    }
                }
            }

            // 수동 후보 추가 — 오검출/누락 시 사용자가 약 이름으로 직접 추가
            AppButton {
                Layout.fillWidth: true
                text: qsTr("➕ 약 이름으로 후보 추가")
                variant: "secondary"
                onClicked: add_candidate_dialog.open()
            }

            // 다중 알약 안내 (후보 ≥ 2 시 표시)
            Rectangle {
                visible: candidates_list.count >= 2
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                radius: 6
                color: "#E3F2FD"
                border.color: "#90CAF9"
                Label {
                    anchors.fill: parent
                    anchors.margins: 10
                    text: qsTr("ℹ️ 여러 약이 검출됐어요. 각 후보의 [💊 이 약 기록·등록] 버튼으로 하나씩 기록할 수 있어요.")
                    font.pixelSize: 12
                    color: "#1565C0"
                    wrapMode: Text.WordWrap
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Label {
                visible: candidates_list.count === 0
                text: qsTr("(식별 후보 없음)")
                font.pixelSize: 13
                color: "#9E9E9E"
            }

            // ===== DUR 결과 안내 =====
            Label {
                Layout.fillWidth: true
                Layout.topMargin: 12
                text: pill_controller.dur_result === "risk_found"
                      ? qsTr("⚠ DUR 위험 검출됨 — 아래 안내를 확인하세요.")
                      : qsTr("DUR에 등록 확인되지 않았습니다. 안심을 위해 약사·의사 상담을 권유드립니다.")
                font.pixelSize: 16
                font.bold: true
                wrapMode: Text.WordWrap
                color: pill_controller.dur_result === "risk_found" ? "#C62828" : "#2E7D32"
            }

            // DUR 상세 — pill_controller.dur_details 바인딩
            ListView {
                id: dur_list
                Layout.fillWidth: true
                Layout.preferredHeight: contentHeight
                interactive: false
                visible: pill_controller.dur_result === "risk_found"
                model: pill_controller.dur_details
                spacing: 8

                delegate: DurAlertCard {
                    width: dur_list.width
                    dur_type:        model.dur_type
                    drug_a_name:     model.drug_a_name
                    drug_b_name:     model.drug_b_name
                    prohibit_reason: model.prohibit_reason
                    action_message:  model.action_message
                }
            }

            // ===== 액션 버튼 =====
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 16
                spacing: 16

                AppButton {
                    text: qsTr("복용 기록 남기기")
                    Layout.fillWidth: true
                    enabled: candidates_list.count > 0
                    onClicked: record_dialog.open()
                }

                AppButton {
                    text: qsTr("처음으로")
                    variant: "secondary"
                    Layout.fillWidth: true
                    // 식별 결과 직전 스택은 CameraPage 일 수 있음.
                    // 단순 pop() 시 CameraPage 로 돌아가는 UX 버그가 있어 HomePage 까지 모두 정리.
                    //   stack.pop(null) — HomePage 가 첫 페이지(initialItem 직후 replace) 인 경우
                    //   일관성을 위해 명시적으로 HomePage 로 replace.
                    onClicked: stack.replace("HomePage.qml")
                }
            }
        }
    }

    // ===== 복약 이력 기록 다이얼로그 (개편) =====
    // 약 선택: ① 식별 결과에서 ② 내 약 풀에서 ③ 코드 직접 입력 — 3가지 소스
    // 풀 미등록 약 → "풀 등록 + 기록" / "기록만 남기기" 분기
    // PillCandidateListModel roles:  ItemCodeRole=UserRole+1, DrugNameRole=UserRole+2
    // PoolItemListModel    roles:    PoolIdRole=UserRole+1, ItemCodeRole=UserRole+2, DrugNameRole=UserRole+3
    Dialog {
        id: record_dialog
        title: qsTr("복용 기록")
        modal: true
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.9, 560)
        standardButtons: Dialog.Ok | Dialog.Cancel

        property string sel_item_code: ""
        property string sel_drug_name: ""
        property bool   in_pool: false
        // 다이얼로그 오픈 시 시작 소스: "candidate" / "pool" / "manual"
        property string preset_source: "candidate"
        // 특정 후보 강제 선택 — 비어있으면 식별 첫 후보 사용
        property string preset_item_code: ""
        property string preset_drug_name: ""

        // 입력 코드 → 내 약 풀에 있는지 검사
        function check_in_pool(code) {
            if (!code || code.length === 0) return false
            var m = pill_controller.pool_items
            for (var i = 0; i < m.rowCount(); i++) {
                var c = m.data(m.index(i, 0), Qt.UserRole + 2)   // PoolItemListModel.ItemCodeRole
                if (c === code) return true
            }
            return false
        }

        function set_selection(code, name) {
            sel_item_code = code || ""
            sel_drug_name = (name && name.length > 0) ? name : qsTr("(이름 미상)")
            in_pool = check_in_pool(sel_item_code)
        }

        onOpened: {
            // 약 풀 최신화 (다른 페이지에서 추가됐을 수 있음)
            pill_controller.load_pool(false)

            // preset_item_code 가 지정된 경우(특정 후보 카드 클릭) → 그 약 우선
            //  - 다중 알약 식별 결과에서 카드별 등록 시 사용
            if (preset_item_code.length > 0) {
                set_selection(preset_item_code, preset_drug_name)
                // 후보 ComboBox 도 동일 인덱스로 맞춤
                var m = pill_controller.candidates
                for (var i = 0; i < m.rowCount(); i++) {
                    var ic = m.data(m.index(i, 0), Qt.UserRole + 1)
                    if (ic === preset_item_code) {
                        candidate_combo.currentIndex = i
                        break
                    }
                }
            } else if (candidates_list.count > 0) {
                // 기본값: 식별 첫 후보
                var c = pill_controller.candidates.data(
                    pill_controller.candidates.index(0, 0), Qt.UserRole + 1)
                var n = pill_controller.candidates.data(
                    pill_controller.candidates.index(0, 0), Qt.UserRole + 2)
                set_selection(c, n)
                candidate_combo.currentIndex = 0
            } else {
                set_selection("", "")
            }
            // preset_source 에 따라 시작 소스 결정 — 식별 미확정 시 "pool" 로 진입
            if (preset_source === "pool") {
                source_pool.checked = true
            } else if (preset_source === "manual") {
                source_manual.checked = true
            } else {
                source_candidate.checked = true
            }
            // 다음 호출을 위해 모두 리셋
            preset_source = "candidate"
            preset_item_code = ""
            preset_drug_name = ""
            manual_code_input.text = ""
            qty_input.value = 1
            memo_input.text = ""
            action_register_and_record.checked = true
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            // ----- 1) 소스 선택 -----
            Label {
                text: qsTr("약 선택 방법")
                font.pixelSize: 13
                color: "#5B6478"
                font.bold: true
            }
            ButtonGroup { id: source_group }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                RadioButton {
                    id: source_candidate
                    text: qsTr("식별 결과")
                    ButtonGroup.group: source_group
                    enabled: candidates_list.count > 0
                }
                RadioButton {
                    id: source_pool
                    text: qsTr("내 약 풀")
                    ButtonGroup.group: source_group
                }
                RadioButton {
                    id: source_manual
                    text: qsTr("약 이름 검색")
                    ButtonGroup.group: source_group
                }
            }

            // ----- 2-a) 식별 결과 ComboBox -----
            ComboBox {
                id: candidate_combo
                visible: source_candidate.checked
                Layout.fillWidth: true
                model: pill_controller.candidates
                textRole: "drug_name"
                enabled: count > 0
                // 후보 없음 / 이름 미확정일 때 빈칸 대신 안내 문구
                displayText: count === 0
                    ? qsTr("(식별 미확정 — 약을 식별하지 못했어요)")
                    : (currentText && currentText.length > 0
                       ? currentText
                       : qsTr("(이름 미상 — 다른 방법으로 선택해주세요)"))
                onActivated: function(idx) {
                    var m = pill_controller.candidates
                    var c = m.data(m.index(idx, 0), Qt.UserRole + 1)
                    var n = m.data(m.index(idx, 0), Qt.UserRole + 2)
                    record_dialog.set_selection(c, n)
                }
                Component.onCompleted: {
                    if (count > 0 && source_candidate.checked) currentIndex = 0
                }
            }
            Label {
                visible: source_candidate.checked && candidate_combo.count === 0
                text: qsTr("(식별 미확정 — '내 약 풀' 또는 '직접 입력'을 이용하거나 위에서 후보를 추가하세요)")
                color: "#E65100"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            // ----- 2-b) 내 약 풀 ComboBox -----
            ComboBox {
                id: pool_combo
                visible: source_pool.checked
                Layout.fillWidth: true
                model: pill_controller.pool_items
                textRole: "drug_name"
                enabled: count > 0
                displayText: count === 0
                    ? qsTr("(등록된 약이 없습니다)")
                    : currentText
                onActivated: function(idx) {
                    var m = pill_controller.pool_items
                    var c = m.data(m.index(idx, 0), Qt.UserRole + 2)  // ItemCodeRole
                    var n = m.data(m.index(idx, 0), Qt.UserRole + 3)  // DrugNameRole
                    record_dialog.set_selection(c, n)
                }
                onVisibleChanged: {
                    // 풀 소스로 바꿔 보일 때 첫 항목으로 자동 선택
                    if (visible && count > 0) {
                        currentIndex = 0
                        var m = pill_controller.pool_items
                        var c = m.data(m.index(0, 0), Qt.UserRole + 2)
                        var n = m.data(m.index(0, 0), Qt.UserRole + 3)
                        record_dialog.set_selection(c, n)
                    } else if (visible && count === 0) {
                        record_dialog.set_selection("", "")
                    }
                }
            }
            Label {
                visible: source_pool.checked && pool_combo.count === 0
                text: qsTr("(내 약 풀이 비어 있습니다)")
                color: "#9E9E9E"
                font.pixelSize: 12
            }

            // ----- 2-c) 약 이름 검색 (품목코드 대신 제품명으로) -----
            ColumnLayout {
                visible: source_manual.checked
                Layout.fillWidth: true
                spacing: 6

                ListModel { id: rd_search_results }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    TextField {
                        id: manual_code_input   // (id 유지 — onOpened 리셋 호환)
                        Layout.fillWidth: true
                        placeholderText: qsTr("약 이름 입력 (예: 타이레놀)")
                        onAccepted: pill_controller.search_drug_name(text.trim())
                    }
                    AppButton {
                        text: qsTr("검색")
                        Layout.preferredWidth: 80
                        enabled: manual_code_input.text.trim().length > 0
                        onClicked: pill_controller.search_drug_name(
                                       manual_code_input.text.trim())
                    }
                }

                Connections {
                    target: pill_controller
                    function onDrug_search_completed(results) {
                        if (!record_dialog.visible || !source_manual.checked) return
                        rd_search_results.clear()
                        for (var i = 0; i < results.length; i++) {
                            rd_search_results.append({
                                item_code: results[i].item_code,
                                drug_name: results[i].drug_name
                            })
                        }
                        if (results.length === 0)
                            app_controller.show_toast(
                                qsTr("일치하는 약을 찾지 못했어요."))
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 150
                    visible: rd_search_results.count > 0
                    color: "#FAFAFA"
                    border.color: "#E0E0E0"
                    radius: 6
                    ListView {
                        anchors.fill: parent
                        anchors.margins: 4
                        clip: true
                        model: rd_search_results
                        spacing: 4
                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 44
                            color: record_dialog.sel_item_code === model.item_code
                                   ? "#E3F2FD" : "white"
                            border.color: record_dialog.sel_item_code === model.item_code
                                          ? "#1565C0" : "#D5DCE4"
                            radius: 4
                            MouseArea {
                                anchors.fill: parent
                                onClicked: record_dialog.set_selection(
                                    model.item_code, model.drug_name)
                            }
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 6
                                spacing: 1
                                Label {
                                    text: model.drug_name
                                    font.pixelSize: 13; font.bold: true
                                    color: "#1A2238"; elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Label {
                                    text: qsTr("코드: ") + model.item_code
                                    font.pixelSize: 10; color: "#5B6478"
                                }
                            }
                        }
                    }
                }
            }

            // ----- 3) 선택 요약 + 풀 상태 -----
            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: 4
                color: record_dialog.in_pool ? "#E8F5E9" : "#FFF3E0"
                border.color: record_dialog.in_pool ? "#A5D6A7" : "#FFCC80"
                radius: 6
                implicitHeight: summary_label.implicitHeight + 16
                Label {
                    id: summary_label
                    anchors.fill: parent
                    anchors.margins: 8
                    text: record_dialog.sel_item_code.length === 0
                          ? qsTr("(약을 선택해주세요)")
                          : record_dialog.sel_drug_name + " (" + record_dialog.sel_item_code + ")  ·  "
                            + (record_dialog.in_pool
                               ? qsTr("내 약 풀 ✓")
                               : qsTr("풀에 없음 ⚠"))
                    wrapMode: Text.WordWrap
                    font.pixelSize: 13
                    color: record_dialog.in_pool ? "#2E7D32" : "#E65100"
                }
            }

            // ----- 4) 풀 미등록 시 처리 분기 -----
            ColumnLayout {
                visible: !record_dialog.in_pool && record_dialog.sel_item_code.length > 0
                Layout.fillWidth: true
                spacing: 4
                Layout.topMargin: 4

                Label {
                    text: qsTr("내 약 풀에 없는 약입니다. 어떻게 처리할까요?")
                    font.pixelSize: 12
                    color: "#5B6478"
                }
                ButtonGroup { id: action_group }
                RadioButton {
                    id: action_register_and_record
                    text: qsTr("내 약 풀에 등록 + 복용 기록")
                    ButtonGroup.group: action_group
                    checked: true
                }
                RadioButton {
                    id: action_record_only
                    text: qsTr("기록만 남기기 (풀에 등록하지 않음)")
                    ButtonGroup.group: action_group
                }
            }

            // ----- 5) 개수 -----
            Label { text: qsTr("개수"); font.pixelSize: 13; color: "#5B6478"; Layout.topMargin: 6 }
            SpinBox {
                id: qty_input
                from: 1; to: 99
                value: 1
                Layout.fillWidth: true
            }

            // ----- 6) 메모 -----
            Label { text: qsTr("메모 (선택)"); font.pixelSize: 13; color: "#5B6478"; Layout.topMargin: 6 }
            TextField {
                id: memo_input
                placeholderText: qsTr("예: 점심 식후 / 가벼운 두통")
                Layout.fillWidth: true
            }
        }

        onAccepted: {
            if (record_dialog.sel_item_code.length === 0) {
                app_controller.show_toast(qsTr("기록할 약을 선택해주세요."))
                return
            }
            // 풀에 없는 약 + 사용자가 등록을 원함 → 풀 등록 먼저 (fire-and-forget)
            //   추후 동기화가 필요하면 pool_changed 시그널 후 record 로 변경 가능.
            if (!record_dialog.in_pool && action_register_and_record.checked) {
                console.log("[record_dialog] 풀 등록 + 기록:", record_dialog.sel_item_code)
                pill_controller.add_to_pool(record_dialog.sel_item_code, "MANUAL")
            }
            history_controller.record(record_dialog.sel_item_code,
                                      qty_input.value, memo_input.text)
            app_controller.show_toast(qsTr("복용 기록 요청 전송됨"))
        }
    }

    Connections {
        target: history_controller
        function onRecord_succeeded() { app_controller.show_toast(qsTr("복용 기록 저장 완료")) }
        function onRecord_failed(code) { app_controller.show_toast(qsTr("복용 기록 실패: ") + code) }
    }

    // ===== 수동 후보 추가 다이얼로그 — 약 이름 검색 → 후보 목록에 추가 =====
    //  오검출/누락 시 사용자가 직접 약을 찾아 후보 카드로 넣는다.
    Dialog {
        id: add_candidate_dialog
        title: qsTr("약 이름으로 후보 추가")
        modal: true
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.9, 520)
        standardButtons: Dialog.Close

        ListModel { id: addc_results }
        property string sel_code: ""
        property string sel_name: ""

        onOpened: {
            addc_input.text = ""
            addc_results.clear()
            sel_code = ""
            sel_name = ""
        }

        Connections {
            target: pill_controller
            function onDrug_search_completed(results) {
                if (!add_candidate_dialog.visible) return
                addc_results.clear()
                for (var i = 0; i < results.length; i++) {
                    addc_results.append({
                        item_code: results[i].item_code,
                        drug_name: results[i].drug_name,
                        classification_name: results[i].classification_name || ""
                    })
                }
                if (results.length === 0)
                    app_controller.show_toast(qsTr("일치하는 약을 찾지 못했어요."))
            }
            function onDrug_search_failed(code) {
                if (add_candidate_dialog.visible)
                    app_controller.show_toast(qsTr("검색 실패: ") + code)
            }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                TextField {
                    id: addc_input
                    Layout.fillWidth: true
                    placeholderText: qsTr("약 이름 입력 (예: 타이레놀)")
                    onAccepted: pill_controller.search_drug_name(text.trim())
                }
                AppButton {
                    text: qsTr("검색")
                    Layout.preferredWidth: 80
                    enabled: addc_input.text.trim().length > 0
                    onClicked: pill_controller.search_drug_name(addc_input.text.trim())
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 220
                color: "#FAFAFA"
                border.color: "#E0E0E0"
                radius: 6
                ListView {
                    anchors.fill: parent
                    anchors.margins: 4
                    clip: true
                    model: addc_results
                    spacing: 4
                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 48
                        color: add_candidate_dialog.sel_code === model.item_code
                               ? "#E3F2FD" : "white"
                        border.color: add_candidate_dialog.sel_code === model.item_code
                                      ? "#1565C0" : "#D5DCE4"
                        radius: 4
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                add_candidate_dialog.sel_code = model.item_code
                                add_candidate_dialog.sel_name = model.drug_name
                            }
                        }
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 2
                            Label {
                                text: model.drug_name
                                font.pixelSize: 13; font.bold: true
                                color: "#1A2238"; elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            Label {
                                text: qsTr("코드: ") + model.item_code
                                      + (model.classification_name.length > 0
                                         ? "  ·  " + model.classification_name : "")
                                font.pixelSize: 11; color: "#5B6478"
                                elide: Text.ElideRight; Layout.fillWidth: true
                            }
                        }
                    }
                }
            }

            AppButton {
                Layout.fillWidth: true
                text: add_candidate_dialog.sel_code.length > 0
                      ? qsTr("➕ \"") + add_candidate_dialog.sel_name + qsTr("\" 후보로 추가")
                      : qsTr("➕ 후보로 추가 (먼저 약을 선택하세요)")
                enabled: add_candidate_dialog.sel_code.length > 0
                onClicked: {
                    pill_controller.candidates.append_candidate(
                        add_candidate_dialog.sel_code,
                        add_candidate_dialog.sel_name)
                    app_controller.show_toast(
                        qsTr("후보 추가됨: ") + add_candidate_dialog.sel_name)
                    add_candidate_dialog.close()
                }
            }
        }
    }
}
