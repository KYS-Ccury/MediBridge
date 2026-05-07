// =====================================================
// AppButton — 노인층 친화 큰 버튼 (재사용)
// =====================================================
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Button {
    id: app_button

    // 사용자 정의 속성
    property string variant: "primary"     // "primary" / "secondary" / "danger"

    font.pixelSize: 20
    font.bold: true
    height: 56
    leftPadding: 24
    rightPadding: 24

    Material.background: {
        switch (variant) {
            case "primary":   return Material.Blue
            case "secondary": return Material.Grey
            case "danger":    return Material.Red
            default:          return Material.Blue
        }
    }
    Material.foreground: "white"
}
