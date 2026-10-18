import QtQuick
Item {
    id: seek
    property string inputRole: "seek"
    readonly property bool dragging: hit.pressed
    readonly property bool hovered: hit.containsMouse
    property real fraction: seek.visible&&mpv.duration>0 ? Math.max(0,Math.min(1,mpv.position/mpv.duration)) : 0
    property real targetFraction: 0
    property real hoverFraction: 0
    signal activityHeldChanged()
    function stamp(seconds) { seconds=Math.max(0,Math.floor(seconds));let h=Math.floor(seconds/3600),m=Math.floor(seconds/60)%60,s=seconds%60;return (h>0?h+":"+String(m).padStart(2,"0"):String(m))+":"+String(s).padStart(2,"0") }
    function updateTarget(x) {targetFraction=Math.max(0,Math.min(1,x/Math.max(1,width)))}
    Rectangle { anchors {left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter} height: 6; radius: 3; color: UiPalette.track }
    Repeater {
        model: mpv.networkSource ? mpv.bufferedRanges : []
        Rectangle {
            required property var modelData
            x: Math.max(0,modelData.start/Math.max(1,mpv.duration))*seek.width
            width: Math.max(0,Math.min(seek.width-x,(modelData.end-modelData.start)/Math.max(1,mpv.duration)*seek.width))
            anchors.verticalCenter: parent.verticalCenter; height: 5; color: UiPalette.muted
        }
    }
    Rectangle { anchors {left: parent.left; verticalCenter: parent.verticalCenter} width: parent.width*(seek.dragging?seek.targetFraction:seek.fraction); height: 5; radius: 2; color: UiPalette.thumb }
    Repeater {
        model: mpv.chapters
        Rectangle { required property var modelData; x: modelData.time/Math.max(1,mpv.duration)*seek.width; anchors.verticalCenter: parent.verticalCenter; width: 2; height: 9; color: UiPalette.background }
    }
    Rectangle { x: Math.max(0,Math.min(parent.width-width,parent.width*(seek.dragging?seek.targetFraction:seek.fraction)-width/2)); anchors.verticalCenter: parent.verticalCenter; width: 4; height: 24; radius: 1; color: UiPalette.text }
    MouseArea {
        id: hit; anchors.fill: parent; hoverEnabled: true; preventStealing: true
        onPressed: mouse => {seek.updateTarget(mouse.x);mpv.seekTo(seek.targetFraction*mpv.duration,false)}
        onPositionChanged: mouse => {seek.hoverFraction=Math.max(0,Math.min(1,mouse.x/Math.max(1,width)));if(pressed)seek.updateTarget(mouse.x)}
        onReleased: mouse => {seek.updateTarget(mouse.x);mpv.seekTo(seek.targetFraction*mpv.duration,true)}
        onDoubleClicked: mouse => {mouse.accepted=true}
    }
    // A drag-only command throttle, not a rendering timer. Stops on release.
    Timer { interval: 65; repeat: true; running: seek.dragging; onTriggered: mpv.seekTo(seek.targetFraction*mpv.duration,false) }
    Rectangle {
        visible: seek.hovered||seek.dragging
        x: Math.max(0,Math.min(seek.width-width,seek.width*(seek.dragging?seek.targetFraction:seek.hoverFraction)-width/2))
        anchors.bottom: parent.top; anchors.bottomMargin: 2; width: label.width+16; height: 26; radius: 4; color: UiPalette.surface; z: 40
        Text { id: label; anchors.centerIn: parent; text: seek.stamp((seek.dragging?seek.targetFraction:seek.hoverFraction)*mpv.duration); color: UiPalette.text; font.pixelSize: 12 }
    }
}
