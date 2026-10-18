import QtQuick
Rectangle {
    id: button
    property string inputRole: "button"
    property string label: ""
    property string hint: ""
    property bool checked: false
    property int textSize: 19
    readonly property bool hovered: hit.containsMouse
    signal activated()
    width: 34; height: 36; radius: 5
    color: hit.containsMouse ? "#3d424b" : checked ? "#313c4b" : "transparent"
    Text { anchors.centerIn: parent; text: button.label; color: button.enabled?"#f0efdf":"#707784"; font.pixelSize: button.textSize }
    MouseArea { id: hit; anchors.fill: parent; hoverEnabled: true; onClicked: button.activated(); onDoubleClicked: event => {event.accepted=true} }
    Rectangle {
        visible: hit.containsMouse && button.hint.length>0
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.top; bottomMargin: 6 }
        width: hintText.width+16; height: 24; radius: 4; color: "#ee242830"; z: 100
        Text { id: hintText; anchors.centerIn: parent; text: button.hint; color: "#eeeeee"; font.pixelSize: 11 }
    }
}
