import QtQuick
Item {
    id:home
    property string inputRole:"home"
    property string selectedKey:""
    signal itemMenu(var record,real px,real py)
    function focusContinue() {let index=history.continueIndex>=0?history.continueIndex:0;for(let i=0;i<history.items.length;i++)if(history.items[i].file_key===selectedKey)index=i;list.currentIndex=index;home.forceActiveFocus()}
    function select(index) {list.currentIndex=Math.max(0,Math.min(history.items.length-1,index));if(list.currentIndex<history.items.length)selectedKey=history.items[list.currentIndex].file_key}
    function playCurrent() {if(list.currentIndex>=0&&list.currentIndex<history.items.length){let item=history.items[list.currentIndex];if(item.available)history.open(item.path)}}
    function ago(stamp) {let m=Math.max(0,Math.floor((Date.now()-Number(stamp))/60000));return m<1?"Just now":m<60?m+" min ago":m<1440?Math.floor(m/60)+" hours ago":Math.floor(m/1440)+" days ago"}
    Component.onCompleted:{focusContinue();appDiagnostics.homeReady(history.items.length)}
    Connections {target:history;function onItemsChanged(){if(!list.moving&&home.activeFocus)home.focusContinue()}}
    Keys.onDownPressed:select(list.currentIndex+1)
    Keys.onUpPressed:select(list.currentIndex-1)
    Keys.onRightPressed:select(list.currentIndex+1)
    Keys.onLeftPressed:select(list.currentIndex-1)
    Keys.onReturnPressed:playCurrent()
    Keys.onEnterPressed:playCurrent()
    Keys.onSpacePressed:playCurrent()
    Rectangle {anchors.fill:parent;color:"#20232b"}
    Rectangle {
        id:brand;anchors {top:parent.top;bottom:parent.bottom;left:parent.left}
        width:home.width>=600?Math.min(200,home.width*.25):0;visible:width>0;color:"#1c2027"
        Column {
            anchors {horizontalCenter:parent.horizontalCenter;top:parent.top;topMargin:Math.min(90,home.height*.16)} spacing:8
            Text {text:"FlowPlayer";color:"#eeeef4";font.pixelSize:23;font.weight:Font.DemiBold}
            Text {text:"Your videos, remembered.";color:"#9ba1b2";font.pixelSize:11}
        }
    }
    Rectangle {
        anchors {left:brand.right;right:parent.right;top:parent.top;bottom:parent.bottom}
        gradient:Gradient {orientation:Gradient.Horizontal;GradientStop{position:0;color:"#17494c"} GradientStop{position:1;color:"#32204f"}}
    }
    Column {
        id:actions;anchors {left:brand.right;right:parent.right;top:parent.top;leftMargin:20;rightMargin:20;topMargin:48} spacing:4
        Repeater {
            model:["Open…","Open URL…"]
            Rectangle {
                required property string modelData;required property int index
                width:actions.width;height:30;radius:4;color:actionHit.containsMouse?"#405e687a":"#284f6372"
                Text {anchors {left:parent.left;leftMargin:10;verticalCenter:parent.verticalCenter} text:parent.modelData;color:"#eeeef4";font.pixelSize:13}
                Text {anchors {right:parent.right;rightMargin:10;verticalCenter:parent.verticalCenter} text:parent.index===0?"Ctrl+O":"Ctrl+U";color:"#aeb4c4";font.pixelSize:11}
                MouseArea {id:actionHit;anchors.fill:parent;hoverEnabled:true;onClicked:{if(parent.index===0)playerWindow.openDialog();else playerWindow.requestUrl()}}
            }
        }
    }
    ListView {
        id:list;anchors {left:brand.right;right:parent.right;top:actions.bottom;bottom:parent.bottom;leftMargin:20;rightMargin:20;topMargin:16;bottomMargin:16}
        model:history.items;clip:true;spacing:5;boundsBehavior:Flickable.StopAtBounds;keyNavigationEnabled:false
        delegate:Rectangle {
            id:row;required property var modelData;required property int index
            width:list.width;height:90;radius:5;opacity:modelData.available?1:.45
            color:list.currentIndex===index?"#45576889":rowHit.containsMouse?"#30495770":"#182f3a50"
            Image {id:thumb;x:9;y:18;width:96;height:54;source:row.modelData.thumbnail||"";asynchronous:true;fillMode:Image.PreserveAspectFit}
            Rectangle {x:9;y:18;width:96;height:54;color:"#292d39";visible:thumb.status!==Image.Ready;Text{anchors.centerIn:parent;text:"▶";color:"#9299ac";font.pixelSize:22}}
            Column {
                anchors {left:thumb.right;right:parent.right;leftMargin:12;rightMargin:10;verticalCenter:parent.verticalCenter} spacing:4
                Text {width:parent.width;text:(row.index===history.continueIndex?"Continue watching · ":"")+row.modelData.title;color:"#f3f1f7";font.pixelSize:13;elide:Text.ElideMiddle}
                Text {width:parent.width;text:row.modelData.folder+" · "+home.ago(row.modelData.last_played);color:"#b1b6c9";font.pixelSize:11;elide:Text.ElideMiddle}
                Rectangle {width:parent.width;height:3;color:"#45505b";Rectangle {height:3;width:parent.width*Math.max(0,Math.min(1,Number(row.modelData.position)/Math.max(1,Number(row.modelData.duration))));color:"#78b8d0"}}
                Text {text:!row.modelData.available?"Unavailable":row.modelData.finished?"Watched":Math.ceil(Math.max(0,Number(row.modelData.duration)-Number(row.modelData.position))/60)+" min left";color:"#bfc4d3";font.pixelSize:11}
            }
            MouseArea {
                id:rowHit;anchors.fill:parent;hoverEnabled:true;acceptedButtons:Qt.LeftButton|Qt.RightButton
                onClicked:event=>{list.currentIndex=row.index;if(event.button===Qt.RightButton){let p=row.mapToItem(home,event.x,event.y);home.itemMenu(row.modelData,p.x,p.y)}else if(row.modelData.available)history.open(row.modelData.path)}
            }
        }
        Text {anchors.centerIn:parent;visible:history.items.length===0;text:"Open a video to begin";color:"#c8cbd8";font.pixelSize:16}
    }
}
