import QtQuick
import FlowPlayer

Item {
    id: root
    width: 960
    height: 540
    property string failure: ""
    Loader {
        anchors.fill: parent
        active: mpv.ready && enableVideoSurface
        sourceComponent: Component {
            MpvRenderItem {
                controller: mpv
                onFirstFrame: appDiagnostics.firstFrame()
            }
        }
    }
    Rectangle {
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: 38
        color: "#b315171b"
        Text {
            anchors { left: parent.left; leftMargin: 14; right: controls.left; rightMargin: 8; verticalCenter: parent.verticalCenter }
            text: mpv.title
            color: "#eeeeee"
            font.pixelSize: 13
            elide: Text.ElideRight
        }
        Row {
            id: controls
            anchors {right: parent.right; top: parent.top}
            Repeater {
                model: [0, 1, 2]
                Rectangle {
                    required property int index
                    required property int modelData
                    width: 46; height: 38
                    color: hit.containsMouse || (index === 1 && playerWindow.maxHovered) ? (index === 2 ? "#be3038" : "#45494e") : "transparent"
                    Rectangle { anchors.centerIn: parent; visible: index === 0; width: 12; height: 1; color: "white" }
                    Rectangle { anchors.centerIn: parent; visible: index === 1; width: 11; height: 10; color: "transparent"; border.color: "white" }
                    Rectangle { anchors.centerIn: parent; visible: index === 2; width: 14; height: 1; rotation: 45; color: "white" }
                    Rectangle { anchors.centerIn: parent; visible: index === 2; width: 14; height: 1; rotation: -45; color: "white" }
                    MouseArea {
                        id: hit
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (index === 0) playerWindow.requestMinimize()
                            else if (index === 1) playerWindow.toggleMaximized()
                            else playerWindow.close()
                        }
                    }
                }
            }
        }
    }
    Rectangle {
        x: 16; y: parent.height - 68; width: 138; height: 52
        radius: 5
        color: playMouse.containsMouse ? "#d149505a" : "#cf292f37"
        border.color: "#66717f"
        Text {anchors.centerIn: parent; text: mpv.pause ? "Play" : "Pause"; color: "white"; font.pixelSize: 15}
        MouseArea {
            id: playMouse
            anchors.fill: parent
            hoverEnabled: true
            onClicked: { mpv.togglePause(); appDiagnostics.overlayClick() }
        }
    }
    Text {
        anchors.centerIn: parent
        visible: root.failure.length > 0 || mpv.duration === 0
        text: root.failure.length > 0 ? root.failure : "FlowPlayer\nDrop a video here or press Ctrl+O"
        horizontalAlignment: Text.AlignHCenter
        color: "#dddfe5"
        font.pixelSize: 18
    }
    Connections {
        target: mpv
        function onError(message) { root.failure = message }
    }
}
