// =====================================================
// PhoneStatusIndicator — 폰 USB 연결 상태 LED + 라벨
// =====================================================
// phone_link_controller 의 is_connected · device_state 를 바인딩.
// =====================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: indicator
    spacing: 8

    // LED 원형
    Rectangle {
        width: 14
        height: 14
        radius: 7
        color: {
            if (phone_link_controller.is_connected) return "#4CAF50"  // 초록
            if (phone_link_controller.device_state === "unauthorized") return "#FF9800"
            if (phone_link_controller.device_state === "offline")      return "#F44336"
            return "#9E9E9E"   // 미연결
        }

        // 깜빡임 애니메이션 (연결 시도 중)
        SequentialAnimation on opacity {
            running: !phone_link_controller.is_connected
            loops: Animation.Infinite
            NumberAnimation { from: 1.0; to: 0.3; duration: 800 }
            NumberAnimation { from: 0.3; to: 1.0; duration: 800 }
        }
    }

    // 상태 라벨
    Label {
        text: {
            if (phone_link_controller.is_connected) {
                return qsTr("폰 연결됨") +
                       (phone_link_controller.reverse_active ? " ✓" : "")
            }
            switch (phone_link_controller.device_state) {
                case "unauthorized": return qsTr("USB 디버깅 허용 필요")
                case "offline":      return qsTr("폰 응답 없음")
                default:             return qsTr("폰 미연결")
            }
        }
        font.pixelSize: 14
    }

    // 수동 재연결 버튼 (Q4=C)
    ToolButton {
        text: qsTr("재연결")
        font.pixelSize: 12
        visible: !phone_link_controller.is_connected
        onClicked: phone_link_controller.retry_connect()
    }
}
