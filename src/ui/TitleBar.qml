import QtQuick
Rectangle {
    id: bar
    height: 38
    gradient:Gradient {GradientStop{position:0;color:UiPalette.surfaceTop} GradientStop{position:1;color:UiPalette.surfaceBottom}}
    Text {
        anchors { left: parent.left; leftMargin: 14; right: buttons.left; rightMargin: 8; verticalCenter: parent.verticalCenter }
        text: mpv.title; color: UiPalette.text; font.pixelSize: 13; elide: Text.ElideRight
    }
    Row {
        id: buttons
        anchors.right: parent.right
        Repeater {
            model: [0,1,2,3,4,5]
            Rectangle {
                required property int modelData
                objectName:modelData===4?"maximizeButton":""
                property string inputRole: "button"
                width: playerWindow.mini ? 36 : 42; height: 38
                color: hit.containsMouse || (modelData===4 && playerWindow.maxHovered) ? (modelData===5 ? UiPalette.hover : UiPalette.hover) : "transparent"
                PlayerIcon {anchors.centerIn:parent;width:17;height:17;tint:(parent.modelData===0&&(playerWindow.pinned||playerWindow.mini))?UiPalette.accent:UiPalette.text;name:["pin",playerWindow.mini?"restore":"mini",playerWindow.fullscreen?"exitFullscreen":"fullscreen","minimize",(playerWindow.maximized||playerWindow.fullscreen||playerWindow.mini)?"restore":"maximize","close"][parent.modelData]}
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
