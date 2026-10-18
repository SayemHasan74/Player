import QtQuick
Rectangle {
    id:controls
    objectName:"playbackControls"
    property string inputRole:"control"
    property bool showRemaining:true
    readonly property bool narrow:width<460
    readonly property bool compact:width<620
    readonly property bool hovered:hover.hovered||seek.dragging
    signal menuRequested(string kind,real px,real py)
    width:Math.min(694,parent.width-24);height:narrow?150:106;radius:9
    gradient:Gradient {GradientStop{position:0;color:"#be55584c"} GradientStop{position:1;color:"#c54c5044"}}
    HoverHandler{id:hover}
    function menu(kind,item){let p=controls.mapToItem(controls.parent,Math.max(0,width-280),0);menuRequested(kind,p.x,p.y)}
    function stamp(seconds){seconds=Math.max(0,Math.floor(seconds));let h=Math.floor(seconds/3600),m=Math.floor(seconds/60)%60,s=seconds%60;return (h>0?h+":"+String(m).padStart(2,"0"):String(m))+":"+String(s).padStart(2,"0")}
    FontMetrics{id:timeMetrics;font:elapsed.font}
    function diagnosticLayout(){
        let results=[]
        for(let size of [336,496,694]){controls.width=size;volumeRow.forceLayout();transport.forceLayout();actions.forceLayout();results.push({width:size,volumeClear:volumeRow.x+volumeRow.width<=transport.x,actionsClear:controls.narrow||transport.x+transport.width<=actions.x,seekWidth:seek.width})}
        controls.width=Qt.binding(()=>Math.min(694,controls.parent.width-24));return results
    }
    Row {
        id:volumeRow;objectName:"volumeGroup";x:compact?14:22;y:16;spacing:8
        ActionButton {icon:mpv.mute?"mute":"volume";width:28;height:36;iconSize:24;hint:"Mute · M";onActivated:mpv.toggleMute()}
        Item {
            property string inputRole:"volume";width:controls.narrow?50:controls.compact?64:100;height:36
            Rectangle {anchors.verticalCenter:parent.verticalCenter;width:parent.width;height:4;radius:2;color:"#85897d"}
            Rectangle {anchors.verticalCenter:parent.verticalCenter;width:parent.width*Math.min(1,mpv.volume/150);height:4;radius:2;color:"#168cce"}
            Rectangle {anchors.verticalCenter:parent.verticalCenter;x:Math.max(-8,Math.min(parent.width-8,parent.width*mpv.volume/150-8));width:18;height:18;radius:9;color:"#bfc3b6"}
            MouseArea {anchors.fill:parent;onPressed:event=>mpv.setVolume(event.x/width*150);onPositionChanged:event=>{if(pressed)mpv.setVolume(event.x/width*150)}}
        }
    }
    Row {
        id:transport;objectName:"transportGroup";width:controls.compact?132:194;x:(controls.width-width)/2;y:11;spacing:controls.compact?8:25
        ActionButton {objectName:"previousButton";icon:"previous";width:controls.compact?36:46;height:44;iconSize:controls.compact?26:30;hint:"Previous file";onActivated:history.navigate(-1)}
        ActionButton {objectName:"playPauseButton";icon:mpv.pause?"play":"pause";width:controls.compact?44:52;height:44;iconSize:controls.compact?32:40;hint:"Play / Pause · Space";onActivated:{mpv.togglePause();appDiagnostics.overlayClick()}}
        ActionButton {objectName:"nextButton";icon:"next";width:controls.compact?36:46;height:44;iconSize:controls.compact?26:30;hint:"Next file";onActivated:history.navigate(1)}
    }
    Row {
        id:actions;objectName:"actionGroup";anchors.right:parent.right;anchors.rightMargin:controls.compact?12:20;y:controls.narrow?58:16;spacing:3
        ActionButton {icon:"subtitles";width:controls.compact?28:30;hint:"Subtitles · S";onActivated:controls.menu("sub",this)}
        ActionButton {icon:"audio";width:controls.compact?28:30;hint:"Audio tracks · A";onActivated:controls.menu("audio",this)}
        ActionButton {icon:"playlist";width:controls.compact?28:30;hint:"Playlist";onActivated:mpv.notify("No playlist")}
        ActionButton {icon:"more";width:controls.compact?28:30;hint:"More · Speed / Fullscreen / Settings";onActivated:controls.menu("context",this)}
    }
    Text {id:elapsed;x:28;width:Math.max(44,timeMetrics.advanceWidth(controls.stamp(mpv.duration)));anchors.bottom:parent.bottom;anchors.bottomMargin:15;text:controls.stamp(controls.visible?mpv.position:0);color:"#f0efdf";font.pixelSize:17}
    Text {
        id:remaining;width:Math.max(50,timeMetrics.advanceWidth("−"+controls.stamp(mpv.duration)));horizontalAlignment:Text.AlignRight;anchors.right:parent.right;anchors.rightMargin:20;anchors.bottom:parent.bottom;anchors.bottomMargin:15
        text:controls.showRemaining?"−"+controls.stamp(controls.visible?mpv.duration-mpv.position:0):controls.stamp(mpv.duration);color:"#f0efdf";font.pixelSize:17
        MouseArea {anchors.fill:parent;onClicked:controls.showRemaining=!controls.showRemaining}
    }
    SeekBar {id:seek;anchors.left:elapsed.right;anchors.leftMargin:27;anchors.right:remaining.left;anchors.rightMargin:4;anchors.bottom:parent.bottom;anchors.bottomMargin:13;height:28}
}
