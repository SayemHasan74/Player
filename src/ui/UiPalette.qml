pragma Singleton
import QtQuick
QtObject {
    // Neutral iOS-style gray material. Transparency picks up the content
    // naturally; no blue background tint, blur or shader effect.
    readonly property color surfaceTop:"#e348484a"
    readonly property color surfaceBottom:"#e3333335"
    readonly property color surface:"#e33b3b3d"
    readonly property color background:"#29292b"
    readonly property color border:"#68686b"
    readonly property color hover:"#636366"
    readonly property color selection:"#7a636366"
    readonly property color field:"#6648484a"
    readonly property color text:"#f2f2f7"
    readonly property color muted:"#aeaeb2"
    readonly property color track:"#747477"
    readonly property color thumb:"#d1d1d6"
    readonly property color accent:"#0a84ff"
}
