import QtQuick
import FlowPlayer
Item {
    id:root
    objectName:"sceneRoot";width:960;height:540;focus:true
    property string failure:""
    readonly property bool modalOpen:sceneMenu.visible||settingsLoader.active
    readonly property bool uiHeld:modalOpen||titleHover.hovered||controls.hovered
    onUiHeldChanged:playerWindow.setUiHeld(uiHeld)
    onModalOpenChanged:playerWindow.setPopupOpen(modalOpen)
    function entry(label,run,checked) {return {label:label,run:run,checked:checked===true}}
    function openSettings() {sceneMenu.visible=false;settingsLoader.active=true}
    function closePopups() {sceneMenu.visible=false;settingsLoader.active=false;root.forceActiveFocus()}
    function openUrl(text) {sceneMenu.urlText=text||"";sceneMenu.open("url",[entry("Open URL",()=>submitUrl(sceneMenu.urlText))],width/2-150,height/2+50)}
    function submitUrl(text) {if(text.trim().length>0){closePopups();mpv.open(text.trim())}}
    function menuItems(kind) {
        let rows=[]
        if(kind==="context")return [entry("Open file · Ctrl+O",()=>playerWindow.openDialog()),entry("Open URL · Ctrl+U",()=>playerWindow.requestUrl()),entry("Audio tracks / delay",()=>openMenu("audio",12,height-12)),entry("Subtitles / delay / size",()=>openMenu("sub",12,height-12)),entry("Playback speed",()=>openMenu("speed",12,height-12)),entry("Aspect ratio",()=>openMenu("aspect",12,height-12)),entry("Video information · I",()=>playerWindow.toggleInfo()),entry("Always on top · Ctrl+T",()=>playerWindow.togglePinned(),playerWindow.pinned||playerWindow.mini),entry("Settings",()=>openSettings())]
        rows.push(entry("‹ Back",()=>openMenu("context",sceneMenu.originX,sceneMenu.originY)))
        if(kind==="audio"||kind==="sub") {
            if(kind==="sub")rows.push(entry("Off",()=>mpv.selectTrack("sub",-1),Number(mpv.playbackOptions.sid||0)<=0))
            for(let track of mpv.tracks) {
                if(track.type!==kind)continue
                let id=Number(track.id)
                rows.push(entry((track.title||("Track "+id))+(track.lang?" · "+track.lang:"")+(track.external?" · external":""),()=>mpv.selectTrack(kind,id),track.selected))
            }
            if(kind==="sub") {
                rows.push(entry("Load subtitle file…",()=>playerWindow.openSubtitleDialog()))
                let delay=Number(mpv.playbackOptions["sub-delay"]||0),scale=Number(mpv.playbackOptions["sub-scale"]||1),position=Number(mpv.playbackOptions["sub-pos"]||100)
                rows.push(entry("Subtitle delay −0.1 s · Z ("+delay.toFixed(1)+")",()=>mpv.setPlaybackOption("sub-delay",delay-.1)))
                rows.push(entry("Subtitle delay +0.1 s · X",()=>mpv.setPlaybackOption("sub-delay",delay+.1)))
                rows.push(entry("Smaller subtitles ("+scale.toFixed(1)+"×)",()=>mpv.setPlaybackOption("sub-scale",scale-.1)))
                rows.push(entry("Larger subtitles",()=>mpv.setPlaybackOption("sub-scale",scale+.1)))
                rows.push(entry("Move subtitles up",()=>mpv.setPlaybackOption("sub-pos",position-5)))
                rows.push(entry("Move subtitles down",()=>mpv.setPlaybackOption("sub-pos",position+5)))
            } else {
                let delay=Number(mpv.playbackOptions["audio-delay"]||0)
                rows.push(entry("Audio delay −0.1 s ("+delay.toFixed(1)+")",()=>mpv.setPlaybackOption("audio-delay",delay-.1)))
                rows.push(entry("Audio delay +0.1 s",()=>mpv.setPlaybackOption("audio-delay",delay+.1)))
            }
        }
        if(kind==="speed")for(let value of [0.5,0.75,1,1.25,1.5,2,3])rows.push(entry(value+"×",()=>mpv.setPlaybackOption("speed",value),Math.abs(mpv.speed-value)<.001))
        if(kind==="aspect")for(let value of ["auto","16:9","4:3","21:9","fill"])rows.push(entry(value,()=>mpv.setAspect(value)))
        return rows
    }
    function openMenu(kind,px,py) {sceneMenu.open(kind,menuItems(kind),px,py)}
    // Bounded in-process diagnostic: no native mode changes, screenshots or focus activation.
    function diagnosticPopupChecks() {
        let result={}
        for(let kind of ["context","audio","sub","speed","aspect"]) {openMenu(kind,8,height-8);result[kind]=sceneMenu.entries.length}
        sceneMenu.visible=false;settingsLoader.active=true
        result.settingsLoaded=settingsLoader.item!==null
        result.sameScene=sceneMenu.parent===root && settingsLoader.parent===root && settingsLoader.item!==null
        closePopups();return JSON.stringify(result)
    }
    Loader {
        anchors.fill:parent;active:mpv.ready&&enableVideoSurface
        sourceComponent:Component {MpvRenderItem {controller:mpv;onFirstFrame:appDiagnostics.firstFrame()}}
    }
    Rectangle {anchors.fill:parent;color:"#252831";visible:playerWindow.videoHidden}
    TitleBar {
        id:title;anchors {left:parent.left;right:parent.right;top:parent.top} z:10
        opacity:playerWindow.controlsVisible?1:0;visible:opacity>0
        Behavior on opacity {NumberAnimation {duration:140}}
        HoverHandler {id:titleHover}
    }
    ControlBar {
        id:controls;anchors {horizontalCenter:parent.horizontalCenter;bottom:parent.bottom;bottomMargin:playerWindow.mini?8:18} z:10
        opacity:playerWindow.controlsVisible?1:0;visible:opacity>0
        Behavior on opacity {NumberAnimation {duration:140}}
        onMenuRequested:(kind,px,py)=>root.openMenu(kind,px,py)
    }
    Text {
        anchors.centerIn:parent;visible:!playerWindow.mini&&(root.failure.length>0||mpv.duration===0)
        text:root.failure.length>0?root.failure:"FlowPlayer\nDrop a video or press Ctrl+O";horizontalAlignment:Text.AlignHCenter;color:"#dddfe5";font.pixelSize:18
    }
    Rectangle {
        property string inputRole:"info"
        anchors {left:parent.left;top:parent.top;margins:46} z:20
        visible:playerWindow.infoVisible;width:Math.min(root.width-24,info.implicitWidth+24);height:Math.min(root.height-50,info.implicitHeight+20);radius:5;color:"#ec20262d";clip:true
        Text {
            id:info;x:12;y:10;color:"#f1f0df";font.pixelSize:13
            text:playerWindow.infoVisible?((mpv.videoInfo.width||0)+" × "+(mpv.videoInfo.height||0)+"\n"+(mpv.videoInfo["video-codec"]||"Audio")+" · "+mpv.hwdecCurrent+"\n"+Number(mpv.videoInfo["container-fps"]||0).toFixed(2)+" fps · "+mpv.droppedFrames+" dropped\n"+(Number(mpv.videoInfo["video-bitrate"]||0)/1000000).toFixed(2)+" Mbps · Cache "+(Math.max(0,mpv.cacheBytes)/1048576).toFixed(1)+" MiB"):""
        }
    }
    SceneMenu {
        id:sceneMenu;anchors.fill:parent
        onDismissed:root.closePopups()
        onChosen:entry=>{sceneMenu.visible=false;root.forceActiveFocus();if(entry.run)entry.run()}
        onUrlSubmitted:url=>root.submitUrl(url)
    }
    Loader {
        id:settingsLoader;anchors.fill:parent;z:110;active:false
        sourceComponent:Component {SettingsPanel {onClosed:root.closePopups()}}
    }
    Toast {id:toast;anchors {horizontalCenter:parent.horizontalCenter;top:parent.top;topMargin:playerWindow.mini?40:58} z:150}
    Connections {
        target:mpv
        function onError(message) {root.failure=message;toast.show(message)}
        function onToast(message) {toast.show(message)}
        function onMetadataChanged() {if(sceneMenu.visible&&(sceneMenu.kind==="audio"||sceneMenu.kind==="sub"))sceneMenu.entries=root.menuItems(sceneMenu.kind)}
    }
    Connections {
        target:playerWindow
        function onContextMenuRequested(position) {root.openMenu("context",position.x,position.y)}
        function onUrlRequested(clipboard) {root.openUrl(clipboard)}
        function onDismissPopup() {root.closePopups()}
    }
}
