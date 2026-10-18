import QtQuick
Rectangle {
    id: controls
    property string inputRole: "control"
    property bool showRemaining: true
    readonly property bool hovered: hover.hovered || seek.dragging
    signal menuRequested(string kind, real px, real py)
    width: Math.min(750,parent.width-24); height: width<460?150:106; radius: 9
    gradient: Gradient { GradientStop {position:0;color:"#e02f343b"} GradientStop {position:1;color:"#ed1c2027"} }
    border.color: "#49515b"
    HoverHandler { id: hover }
    function menu(kind,item) {let point=controls.mapToItem(controls.parent,Math.max(0,width-280),0);menuRequested(kind,point.x,point.y)}
    function stamp(seconds) {seconds=Math.max(0,Math.floor(seconds));let h=Math.floor(seconds/3600),m=Math.floor(seconds/60)%60,s=seconds%60;return (h>0?h+":"+String(m).padStart(2,"0"):String(m))+":"+String(s).padStart(2,"0")}
    Row {
        id: volumeRow; x: 10; y: 8
        ActionButton { label:mpv.mute?"♪":"◖";hint:"Mute · M";onActivated:mpv.toggleMute() }
        Item {
            property string inputRole:"volume";width: controls.width<600?46:84;height:36
            Rectangle {anchors.verticalCenter:parent.verticalCenter;width:parent.width;height:4;radius:2;color:"#6b737f"}
            Rectangle {anchors.verticalCenter:parent.verticalCenter;width:parent.width*Math.min(1,mpv.volume/150);height:4;color:"#47a5d4"}
            Rectangle {anchors.verticalCenter:parent.verticalCenter;x:Math.max(0,Math.min(parent.width-10,parent.width*mpv.volume/150-5));width:10;height:10;radius:5;color:"#d9dcce"}
            MouseArea {anchors.fill:parent;onPressed:event=>mpv.setVolume(event.x/width*150);onPositionChanged:event=>{if(pressed)mpv.setVolume(event.x/width*150)}}
        }
    }
    Row {
        x: (controls.width-108)/2; y:8; spacing:0
        ActionButton {label:"◀◀";textSize:17;hint:"Previous";onActivated:mpv.notify("No playlist")}
        ActionButton {label:mpv.pause?"▶":"Ⅱ";textSize:28;width:40;hint:"Play / Pause · Space";onActivated:{mpv.togglePause();appDiagnostics.overlayClick()}}
        ActionButton {label:"▶▶";textSize:17;hint:"Next";onActivated:mpv.notify("No playlist")}
    }
    Row {
        anchors.right:parent.right;anchors.rightMargin:8;y:controls.width<460?50:8
        ActionButton {width:controls.width<600?28:34;label:"♫";hint:"Audio tracks · A";onActivated:controls.menu("audio",this)}
        ActionButton {width:controls.width<600?28:34;label:"▤";hint:"Subtitles · S";onActivated:controls.menu("sub",this)}
        ActionButton {width:controls.width<600?28:34;label:mpv.speed.toFixed(1)+"×";textSize:12;hint:"Playback speed";onActivated:controls.menu("speed",this)}
        ActionButton {width:controls.width<600?28:34;label:"☷";hint:"Playlist";onActivated:mpv.notify("No playlist — available in Part 5")}
        ActionButton {width:controls.width<600?28:34;label:"⛶";hint:"Fullscreen · F";onActivated:playerWindow.toggleFullscreen()}
        ActionButton {width:controls.width<600?28:34;label:"⋯";hint:"More / Settings";onActivated:controls.menu("context",this)}
    }
    Text {id:elapsed;x:16;anchors.bottom:parent.bottom;anchors.bottomMargin:13;text:controls.stamp(controls.visible?mpv.position:0);color:"#efefdf";font.pixelSize:13}
    Text {
        id:remaining;anchors.right:parent.right;anchors.rightMargin:16;anchors.bottom:parent.bottom;anchors.bottomMargin:13
        text:controls.showRemaining?"−"+controls.stamp(controls.visible?mpv.duration-mpv.position:0):controls.stamp(mpv.duration);color:"#efefdf";font.pixelSize:13
        MouseArea {anchors.fill:parent;onClicked:controls.showRemaining=!controls.showRemaining}
    }
    SeekBar {id:seek;anchors.left:elapsed.right;anchors.leftMargin:10;anchors.right:remaining.left;anchors.rightMargin:10;anchors.bottom:parent.bottom;anchors.bottomMargin:9;height:28}
}
