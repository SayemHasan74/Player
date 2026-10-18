import QtQuick
Item {
    id: seek
    property string inputRole: "seek"
    property real fraction: mpv.duration > 0 ? Math.max(0,Math.min(1,mpv.position/mpv.duration)) : 0
    property real preview: fraction
    function jump(x) { preview=Math.max(0,Math.min(1,x/width)); mpv.seekTo(preview*mpv.duration) }
    Rectangle { anchors {left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter} height: 5; radius: 2; color: "#6b6e62" }
    Rectangle { anchors {left: parent.left; verticalCenter: parent.verticalCenter} width: parent.width*(hit.pressed?seek.preview:seek.fraction); height: 5; radius: 2; color: "#bfc1ad" }
    Rectangle { x: Math.max(0,Math.min(parent.width-width,parent.width*(hit.pressed?seek.preview:seek.fraction)-width/2)); anchors.verticalCenter: parent.verticalCenter; width: 4; height: 20; radius: 1; color: "#ecebdc" }
    MouseArea {
        id: hit; anchors.fill: parent; hoverEnabled: true; preventStealing: true
        onPressed: mouse => seek.jump(mouse.x)
        onPositionChanged: mouse => { if(pressed)seek.jump(mouse.x) }
        onReleased: mouse => seek.jump(mouse.x)
        onDoubleClicked: mouse => { mouse.accepted=true }
    }
}
