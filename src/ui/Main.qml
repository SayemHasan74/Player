import QtQuick
import FlowPlayer
Item {
    id: root
    width: 960; height: 540
    property string failure: ""
    property string toastMessage: ""
    Loader {
        anchors.fill: parent
        active: mpv.ready && enableVideoSurface
        sourceComponent: Component {
            MpvRenderItem { controller: mpv; onFirstFrame: appDiagnostics.firstFrame() }
        }
    }
    // Keep the same video item alive. A plain scene item covers it in Mini;
    // vid=no stops decoder/render updates without replacing the native window.
    Rectangle { anchors.fill: parent; color: "#252831"; visible: playerWindow.mini && !playerWindow.tinyPreview }
    TitleBar { id: title; anchors {left: parent.left; right: parent.right; top: parent.top} z: 10 }
    ControlBar { anchors {horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 22} visible: !playerWindow.mini; z: 10 }
    SeekBar { anchors {left: parent.left; right: parent.right; top: title.bottom; leftMargin: 12; rightMargin: 12} height: 26; visible: playerWindow.mini; z: 10 }
    Text {
        anchors.centerIn: parent
        visible: !playerWindow.mini && (root.failure.length>0||mpv.duration===0)
        text: root.failure.length>0?root.failure:"FlowPlayer\nDrop a video here or press Ctrl+O"
        horizontalAlignment: Text.AlignHCenter; color: "#dddfe5"; font.pixelSize: 18
    }
    Rectangle {
        anchors {left: parent.left; top: title.bottom; margins: 14} z: 20
        visible: playerWindow.infoVisible && !playerWindow.mini; width: info.width+24; height: info.height+20; radius: 5; color: "#de202329"
        Text {
            id: info; x: 12; y: 10; color: "#f1f0df"; font.pixelSize: 14
            text: (mpv.videoInfo.width||0)+" × "+(mpv.videoInfo.height||0)+"\n"+(mpv.videoInfo["video-codec"]||"—")+" · "+mpv.hwdecCurrent+"\n"+Number(mpv.videoInfo["container-fps"]||0).toFixed(2)+" fps · "+mpv.droppedFrames+" dropped\n"+(Number(mpv.videoInfo["video-bitrate"]||0)/1000000).toFixed(2)+" Mbps"
        }
    }
    Rectangle {
        anchors {horizontalCenter: parent.horizontalCenter; top: title.bottom; topMargin: playerWindow.mini?0:24} z: 30
        visible: toastTimer.running && !playerWindow.mini; width: toastText.width+28; height: 36; radius: 6; color: "#dd252831"
        Text { id: toastText; anchors.centerIn: parent; text: root.toastMessage; color: "#f1f0df"; font.pixelSize: 14 }
    }
    Timer { id: toastTimer; interval: 1100 }
    Connections {
        target: mpv
        function onError(message) {root.failure=message}
        function onToast(message) {root.toastMessage=message;toastTimer.restart()}
    }
}
