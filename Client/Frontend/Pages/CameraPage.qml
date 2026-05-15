// =====================================================
// CameraPage.qml — PC 제어 카메라 촬영 화면 (Screencap 기반)
// =====================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtMultimedia
import "../Components"

Page {
    id: camera_page
    background: Rectangle { color: "#F8FAFF" }

    // --------------------------------------------------
    // 상태 및 이벤트 연결
    // --------------------------------------------------
    Connections {
        target: pill_controller
        function onIdentify_succeeded() {
            // ⚠ 페이지 전환 전에 스트림을 먼저 멈추고 싱크를 끊는다.
            //   안 그러면 CameraPage(=VideoOutput) 파괴 후에도 백그라운드
            //   캡처 프레임이 삭제된 QVideoSink 로 들어가 액세스 위반(크래시).
            if (typeof camera_stream_controller !== "undefined") {
                camera_stream_controller.stop_stream()
                camera_stream_controller.videoSink = null
            }
            stack.replace("IdentifyResultPage.qml")
        }
        function onIdentify_failed(error_code) {
            app_controller.show_toast(qsTr("촬영/식별 실패: ") + error_code)
        }
    }

    // --------------------------------------------------
    // 상단 헤더 (뒤로가기)
    // --------------------------------------------------
    header: ToolBar {
        background: Rectangle { color: "white"; border.color: "#EEEEEE"; border.width: 1 }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            
            ToolButton {
                id: back_button
                contentItem: RowLayout {
                    spacing: 8
                    Text { text: "←"; font.pixelSize: 24; color: "#2C3E50" }
                    Text { text: qsTr("뒤로"); font.pixelSize: 18; color: "#2C3E50"; font.bold: true }
                }
                onClicked: {
                    stack.pop()
                }
            }
            Item { Layout.fillWidth: true }
        }
    }

    // --------------------------------------------------
    // 메인 컨텐츠 — 창이 작아도 잘리지 않도록 ScrollView 로 감쌈
    // --------------------------------------------------
    ScrollView {
        id: cam_scroll
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        Item {
            width: cam_scroll.availableWidth
            implicitHeight: Math.max(cam_col.implicitHeight + 48,
                                     cam_scroll.availableHeight)

            ColumnLayout {
                id: cam_col
                anchors.horizontalCenter: parent.horizontalCenter
                y: Math.max(24, (parent.height - implicitHeight) / 2)
                width: Math.min(cam_scroll.availableWidth * 0.9, 600)
                spacing: 40

        // 1. 안내 텍스트
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 12
            
            Text {
                text: "📱 ↔️ 💻"
                font.pixelSize: 48
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: qsTr("스마트폰 카메라 준비 완료")
                font.pixelSize: 28
                font.bold: true
                color: "#2C3E50"
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: qsTr("스마트폰 화면에 약이 잘 보이도록 비춘 후,\n아래의 '촬영하기' 버튼을 눌러주세요.")
                font.pixelSize: 18
                color: "#7F8C8D"
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignHCenter
            }
        }

        // 2. 뷰파인더
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 320; height: 420
            color: "black"; radius: 16; clip: true

            // 실제 스마트폰 화면이 출력되는 곳
            VideoOutput {
                id: livePreview // 💡 ID 추가: 이 ID가 있어야 C++에서 참조 가능합니다.
                anchors.fill: parent
                fillMode: VideoOutput.PreserveAspectCrop
            }

            // 안내용 초록 가이드라인
            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.8; height: width
                color: "transparent"
                border.color: "#4CAF50"
                border.width: 3
                radius: 12
                opacity: pill_controller.is_loading ? 0 : 1
            }

            // 로딩 표시 유지
            BusyIndicator {
                anchors.centerIn: parent
                running: pill_controller.is_loading
                visible: pill_controller.is_loading
            }
            Component.onCompleted: {
                if (typeof camera_stream_controller !== "undefined") {
                    camera_stream_controller.videoSink = livePreview.videoSink
                    camera_stream_controller.start_stream()
                }
            }
            Component.onDestruction: {
                if (typeof camera_stream_controller !== "undefined") {
                    camera_stream_controller.stop_stream()
                }
            }
        }

        // 3. PC 제어 촬영 버튼
        AppButton {
            id: capture_btn
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 240
            text: qsTr("📸 촬영하기")
            
            // 연결되어 있고, 로딩 중이 아닐 때만 활성화
            enabled: phone_link_controller.is_connected && !pill_controller.is_loading
            
            onClicked: {
                app_controller.show_toast(qsTr("스마트폰 화면을 캡처합니다..."))
                
                // ⚠ 캡처 실행 명령!
                // 백엔드의 PhoneCaptureService.capture_screen()을 트리거하는 함수 호출
                // PillController에 이 함수가 연결되어 있어야 합니다.
                pill_controller.capture_and_identify()
            }
        }
                Component.onCompleted: camera_stream_controller.start_stream()
                Component.onDestruction: camera_stream_controller.stop_stream()
            } // ColumnLayout(cam_col) 닫기
        } // Item 닫기
    } // ScrollView 닫기
}