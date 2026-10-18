import QtQuick
Rectangle {
    id: bar
    height: 38
    color: playerWindow.mini ? "#252831" : "#b322252a"
    Text {
        anchors { left: parent.left; leftMargin: 14; right: buttons.left; rightMargin: 8; verticalCenter: parent.verticalCenter }
        text: mpv.title; color: "#f0f0e7"; font.pixelSize: 13; elide: Text.ElideRight
    }
    Row {
        id: buttons
        anchors.right: parent.right
        Repeater {
            model: [0,1,2,3,4,5]
            Rectangle {
                required property int modelData
                property string inputRole: "button"
                width: playerWindow.mini ? 36 : 42; height: 38
                color: hit.containsMouse || (modelData===4 && playerWindow.maxHovered) ? (modelData===5 ? "#b83341" : "#596067") : "transparent"
                Text {
                    anchors.centerIn: parent; color: "#f2f1df"; font.pixelSize: 17
                    text: [(playerWindow.pinned||playerWindow.mini) ? "●" : "♧", playerWindow.mini ? "↗" : "▱", "⛶", "−", "□", "×"][modelData]
                }
                MouseArea {
                    id: hit; anchors.fill: parent; hoverEnabled: true
                    onClicked: {
                        switch(parent.modelData) {
                        case 0: playerWindow.togglePinned(); break
                        case 1: playerWindow.toggleMini(); break
                        case 2: playerWindow.toggleFullscreen(); break
                        case 3: playerWindow.requestMinimize(); break
                        case 4: playerWindow.toggleMaximized(); break
                        case 5: playerWindow.close(); break
                        }
                    }
                    onDoubleClicked: mouse => { mouse.accepted=true }
                }
            }
        }
    }
}
