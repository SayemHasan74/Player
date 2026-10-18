import QtQuick
Item {
    id:root
    property string inputRole:"playlist"
    property bool opened:false
    signal closed()
    function close(){opened=false;exitTimer.restart()}
    Component.onCompleted:{opened=true;list.forceActiveFocus()}
    Timer {id:exitTimer;interval:140;onTriggered:root.closed()}
    MouseArea {anchors.fill:parent;onClicked:root.close()}
    Rectangle {
        id:panel;objectName:"playlistPanel"
        width:Math.min(340,Math.max(260,root.width*.32),root.width-28);height:root.height
        x:root.opened?root.width-width:root.width
        Behavior on x {NumberAnimation {duration:140}}
        color:UiPalette.surface;clip:true
        MouseArea {anchors.fill:parent;onClicked:event=>{event.accepted=true}}
        Text {x:16;y:12;text:"PLAYLIST";color:UiPalette.muted;font.pixelSize:12;font.letterSpacing:1}
        ActionButton {anchors.right:parent.right;y:0;label:"×";hint:"Close · L";onActivated:root.close()}
        ListView {
            id:list;x:0;y:44;width:panel.width;height:Math.max(0,panel.height-94);clip:true
            model:history.playlistModel;currentIndex:history.currentIndex;spacing:1
            boundsBehavior:Flickable.StopAtBounds
            Keys.onEscapePressed:root.close()
            Keys.onReturnPressed:history.playIndex(currentIndex)
            delegate:Rectangle {
                id:row;required property var record;required property int index
                property bool current:index===history.currentIndex
                property bool reordering:false
                property bool suppressClick:false
                width:list.width;height:Math.max(43,name.implicitHeight+18)
                color:reordering?UiPalette.hover:current?UiPalette.selection:hover.hovered?UiPalette.hover:"transparent"
                HoverHandler {id:hover;onHoveredChanged:if(hovered)detail.text=row.record.title||row.record.path}
                Text {x:10;y:12;width:18;text:row.current?"▶":row.record.finished?"✓":"";color:UiPalette.text;font.pixelSize:11}
                Text {id:name;x:31;y:9;width:row.width-45;text:row.record.title||row.record.path;wrapMode:Text.WrapAnywhere;color:row.current?UiPalette.text:UiPalette.text;font.pixelSize:12}
                Rectangle {x:31;anchors.bottom:parent.bottom;width:(parent.width-45)*Math.min(1,Number(row.record.position||0)/Math.max(1,Number(row.record.duration||0)));height:2;color:row.record.finished?UiPalette.muted:UiPalette.accent}
                MouseArea {
                    anchors.fill:parent;preventStealing:row.reordering
                    onPressed:row.suppressClick=false
                    onClicked:if(!row.suppressClick)history.playIndex(row.index)
                    onPressAndHold:{row.reordering=true;row.suppressClick=true}
                    onReleased:event=>{if(row.reordering){let from=row.index;let point=mapToItem(list.contentItem,event.x,event.y);let target=list.indexAt(point.x,point.y);row.reordering=false;if(target>=0)history.reorder(from,target)}}
                    onCanceled:row.reordering=false
                }
            }
        }
        Text {id:detail;x:12;anchors.bottom:count.top;anchors.bottomMargin:4;width:parent.width-24;maximumLineCount:2;elide:Text.ElideRight;wrapMode:Text.WrapAnywhere;text:history.currentIndex>=0&&history.playlist.length>history.currentIndex?history.playlist[history.currentIndex].title:"Open a file to build a playlist";color:UiPalette.muted;font.pixelSize:10}
        Text {id:count;x:12;anchors.bottom:parent.bottom;anchors.bottomMargin:8;text:history.playlist.length+" files · hold and drag to reorder";color:UiPalette.muted;font.pixelSize:10}
    }
}
