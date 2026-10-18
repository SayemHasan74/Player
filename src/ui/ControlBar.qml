import QtQuick
Rectangle {
    id: controls
    property string inputRole: "control"
    width: Math.min(690,parent.width-32); height: 106; radius: 10
    color: "#da777965"
    function stamp(seconds) {
        seconds=Math.max(0,Math.floor(seconds));let h=Math.floor(seconds/3600),m=Math.floor(seconds/60)%60,s=seconds%60
        return (h>0?h+":"+String(m).padStart(2,"0"):String(m))+":"+String(s).padStart(2,"0")
    }
    component Action: Item {
        property string glyph
        property int glyphSize: 25
        signal activated()
        width: 36; height: 40
        Text { anchors.centerIn: parent; text: parent.glyph; color: mouse.containsMouse?"white":"#f1f0d9"; font.pixelSize: parent.glyphSize }
        MouseArea { id: mouse; anchors.fill: parent; hoverEnabled: true; onClicked: parent.activated(); onDoubleClicked: event => {event.accepted=true} }
    }
    Row {
        anchors {left: parent.left; leftMargin: 16; top: parent.top; topMargin: 12}
        visible: controls.width>=480
        Action { glyph: mpv.mute?"♪":"◖"; onActivated: mpv.toggleMute() }
        Item {
            property string inputRole: "volume"
            width: Math.max(45,Math.min(100,controls.width*.14)); height: 40
            Rectangle { anchors.verticalCenter: parent.verticalCenter; width: parent.width; height: 4; radius: 2; color: "#999d8d" }
            Rectangle { anchors.verticalCenter: parent.verticalCenter; width: parent.width*Math.min(1,mpv.volume/100); height: 4; color: "#2aa0db" }
            Rectangle { anchors.verticalCenter: parent.verticalCenter; x: Math.min(parent.width-10,parent.width*mpv.volume/100-5); width: 14; height: 14; radius: 7; color: "#ced0bd" }
            MouseArea { anchors.fill: parent; onPressed: event => mpv.setVolume(event.x/width*100); onPositionChanged: event => {if(pressed)mpv.setVolume(event.x/width*100)} }
        }
    }
    Row {
        anchors {horizontalCenter: controls.width>=480?parent.horizontalCenter:undefined; left: controls.width<480?parent.left:undefined; leftMargin: 12; top: parent.top; topMargin: 8}
        spacing: 6
        Action { glyph: "◀◀"; glyphSize: 18; onActivated: mpv.seek(-5) }
        Action { width: 44; glyph: mpv.pause?"▶":"Ⅱ"; glyphSize: 33; onActivated: {mpv.togglePause();appDiagnostics.overlayClick()} }
        Action { glyph: "▶▶"; glyphSize: 18; onActivated: mpv.seek(5) }
    }
    Row {
        anchors {right: parent.right; rightMargin: 12; top: parent.top; topMargin: 12}
        Action { glyph: "ⓘ"; glyphSize: 21; onActivated: playerWindow.toggleInfo() }
        Action { glyph: playerWindow.mini?"↗":"▱"; onActivated: playerWindow.toggleMini() }
        Action { glyph: "⛶"; onActivated: playerWindow.toggleFullscreen() }
    }
    Text { id: elapsed; anchors {left: parent.left; leftMargin: 20; bottom: parent.bottom; bottomMargin: 13} text: controls.stamp(mpv.position); color: "#f1f0df"; font.pixelSize: 14 }
    Text { id: remaining; anchors {right: parent.right; rightMargin: 20; bottom: parent.bottom; bottomMargin: 13} text: "−"+controls.stamp(mpv.duration-mpv.position); color: "#f1f0df"; font.pixelSize: 14 }
    SeekBar { anchors {left: elapsed.right; leftMargin: 12; right: remaining.left; rightMargin: 12; bottom: parent.bottom; bottomMargin: 9} height: 28 }
}
