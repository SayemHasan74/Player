import QtQuick
Item {
    id:home;objectName:"homeScreen"
    property string inputRole:"home"
    property string selectedKey:""
    property int hoveredIndex:-1
    readonly property var selectedRecord:list.currentIndex>=0&&list.currentIndex<history.items.length?history.items[list.currentIndex]:null
    readonly property var detailRecord:hoveredIndex>=0&&hoveredIndex<history.items.length?history.items[hoveredIndex]:selectedRecord
    signal itemMenu(var record,real px,real py)
    function focusContinue(){let index=history.continueIndex>=0?history.continueIndex:0;for(let i=0;i<history.items.length;i++)if(history.items[i].file_key===selectedKey)index=i;list.currentIndex=history.items.length>0?index:-1;home.forceActiveFocus()}
    function select(index){if(history.items.length===0)return;list.currentIndex=Math.max(0,Math.min(history.items.length-1,index));selectedKey=history.items[list.currentIndex].file_key}
    function playCurrent(){if(selectedRecord&&selectedRecord.available)history.open(selectedRecord.path)}
    function ago(stamp){let m=Math.max(0,Math.floor((Date.now()-Number(stamp))/60000));return m<1?"Just now":m<60?m+" min ago":m<1440?Math.floor(m/60)+(m<120?" hour ago":" hours ago"):Math.floor(m/1440)+(m<2880?" day ago":" days ago")}
    function details(record){return !record?"":record.folder+" · "+(record.available?(record.finished?"Watched":Math.ceil(Math.max(0,Number(record.duration)-Number(record.position))/60)+" min left"):"Unavailable")+" · "+ago(record.last_played)}
    Component.onCompleted:{focusContinue();appDiagnostics.homeReady(history.items.length)}
    Connections{target:history;function onItemsChanged(){if(!list.moving&&home.activeFocus)home.focusContinue()}}
    Keys.onDownPressed:select(list.currentIndex+1)
    Keys.onUpPressed:select(list.currentIndex-1)
    Keys.onRightPressed:select(list.currentIndex+1)
    Keys.onLeftPressed:select(list.currentIndex-1)
    Keys.onReturnPressed:playCurrent()
    Keys.onEnterPressed:playCurrent()
    Keys.onSpacePressed:playCurrent()
    Rectangle{anchors.fill:parent;color:"#1e2229"}
    Rectangle{
        id:brand;anchors{top:parent.top;bottom:parent.bottom;left:parent.left}
        width:home.width>=480?home.width*.28:0;visible:width>0;color:"#1d2027"
        Rectangle{anchors{right:parent.right;top:parent.top;bottom:parent.bottom}width:1;color:"#4a5263"}
        Column{
            anchors.centerIn:parent;spacing:5
            Text{text:"FlowPlayer";color:"#edeef1";font.pixelSize:18;anchors.horizontalCenter:parent.horizontalCenter}
            Text{text:"0.4";color:"#bcbfcb";font.pixelSize:11;anchors.horizontalCenter:parent.horizontalCenter}
        }
    }
    Rectangle{
        anchors{left:brand.right;right:parent.right;top:parent.top;bottom:parent.bottom}
        gradient:Gradient{orientation:Gradient.Horizontal;GradientStop{position:0;color:"#19494b"}GradientStop{position:.48;color:"#283b50"}GradientStop{position:1;color:"#34234e"}}
    }
    Column{
        id:actions;anchors{left:brand.right;right:parent.right;top:parent.top;leftMargin:19;rightMargin:20;topMargin:46}spacing:2
        Repeater{
            model:["Open…","Open URL…"]
            Rectangle{
                required property string modelData;required property int index
                width:actions.width;height:24;radius:4;color:actionHit.containsMouse?"#5c798089":index===0?"#3a718285":"transparent"
                Text{anchors{left:parent.left;leftMargin:10;verticalCenter:parent.verticalCenter}text:parent.modelData;color:"#eceef2";font.pixelSize:12}
                Text{anchors{right:parent.right;rightMargin:9;verticalCenter:parent.verticalCenter}text:parent.index===0?"Ctrl+O":"Ctrl+U";color:"#aeb5c5";font.pixelSize:10}
                MouseArea{id:actionHit;anchors.fill:parent;hoverEnabled:true;onClicked:{if(parent.index===0)playerWindow.openDialog();else playerWindow.requestUrl()}}
            }
        }
    }
    Text{
        id:continueLabel;anchors{left:actions.left;right:actions.right;top:actions.bottom;topMargin:12}
        text:history.continueIndex>=0?"Continue watching":"Recent videos";color:"#c8cbd6";font.pixelSize:11;height:16
    }
    ListView{
        id:list;anchors{left:actions.left;right:actions.right;top:continueLabel.bottom;topMargin:4;bottom:detail.top;bottomMargin:10}
        model:history.homeModel;clip:true;spacing:3;boundsBehavior:Flickable.StopAtBounds;keyNavigationEnabled:false
        delegate:Rectangle{
            id:row;required property var record;required property int index
            width:list.width;height:28;radius:3;opacity:record.available?1:.45
            color:list.currentIndex===index?"#555f6687":rowHit.containsMouse?"#34495770":"#1c4b5168"
            Image{id:thumb;x:8;y:4;width:20;height:20;source:row.record.thumbnail||"";sourceSize:Qt.size(40,40);asynchronous:true;fillMode:Image.PreserveAspectCrop}
            PlayerIcon{x:10;y:5;width:16;height:18;name:"file";visible:thumb.status!==Image.Ready;tint:"#e6e8ee"}
            Text{anchors{left:thumb.right;leftMargin:7;right:parent.right;rightMargin:8;verticalCenter:parent.verticalCenter}text:row.record.title+(row.record.available?"":" · Unavailable");color:"#f0eff4";font.pixelSize:12;elide:Text.ElideMiddle}
            Rectangle{anchors{left:parent.left;bottom:parent.bottom}height:2;width:parent.width*Math.max(0,Math.min(1,Number(row.record.position)/Math.max(1,Number(row.record.duration))));color:row.index===history.continueIndex?"#8cb7c7":"#727d9d"}
            MouseArea{
                id:rowHit;anchors.fill:parent;hoverEnabled:true;acceptedButtons:Qt.LeftButton|Qt.RightButton
                onEntered:home.hoveredIndex=row.index
                onExited:home.hoveredIndex=-1
                onClicked:event=>{home.select(row.index);if(event.button===Qt.RightButton){let p=row.mapToItem(home,event.x,event.y);home.itemMenu(row.record,p.x,p.y)}else if(row.record.available)history.open(row.record.path)}
            }
        }
        Text{anchors.centerIn:parent;visible:history.items.length===0;text:"Open a video to begin";color:"#c6c9d5";font.pixelSize:13}
    }
    Column{
        id:detail;anchors{left:actions.left;right:actions.right;bottom:parent.bottom;bottomMargin:17}height:54;spacing:5
        Text{width:parent.width;text:home.detailRecord?home.detailRecord.title:"";color:"#dcdfea";font.pixelSize:11;wrapMode:Text.Wrap;maximumLineCount:2;elide:Text.ElideMiddle}
        Text{width:parent.width;text:home.details(home.detailRecord);color:"#aeb7c9";font.pixelSize:10;wrapMode:Text.Wrap;maximumLineCount:2;elide:Text.ElideRight}
    }
}
