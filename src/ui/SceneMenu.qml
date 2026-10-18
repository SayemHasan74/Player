import QtQuick
Item {
    id:menu
    property string inputRole:"menu"
    property string kind:"context"
    property int selectedIndex:0
    property var entries:[]
    property real originX:10
    property real originY:10
    property alias urlText:urlInput.text
    signal dismissed()
    signal chosen(var entry)
    signal urlSubmitted(string url)
    visible:false;z:100
    function open(type,items,px,py) {kind=type;entries=items;selectedIndex=0;originX=px;originY=py;visible=true;list.contentY=0;forceActiveFocus();if(kind==="url")urlInput.forceActiveFocus()}
    Keys.onDownPressed:{selectedIndex=Math.min(entries.length-1,selectedIndex+1);list.contentY=Math.min(Math.max(0,list.contentHeight-list.height),selectedIndex*34)}
    Keys.onUpPressed:{selectedIndex=Math.max(0,selectedIndex-1);list.contentY=Math.max(0,Math.min(list.contentY,selectedIndex*34))}
    Keys.onReturnPressed:{if(entries.length>0)chosen(entries[selectedIndex])}
    Keys.onEnterPressed:{if(entries.length>0)chosen(entries[selectedIndex])}
    MouseArea {anchors.fill:parent;onClicked:menu.dismissed()}
    Rectangle {
        id:panel
        x:Math.max(6,Math.min(menu.width-width-6,menu.originX))
        y:Math.max(6,Math.min(menu.height-height-6,menu.originY-height-8))
        width:Math.min(300,menu.width-12);height:Math.min(menu.height-12,list.contentHeight+12+(menu.kind==="url"?42:0))
        radius:7;color:"#fa252a32";border.color:"#555e6b"
        MouseArea {anchors.fill:parent;onClicked:event=>{event.accepted=true}}
        TextInput {
            id:urlInput;property bool textEntry:true
            visible:menu.kind==="url";x:12;y:10;width:parent.width-24;height:30;color:"white";font.pixelSize:13;clip:true;selectByMouse:true

            onAccepted:menu.urlSubmitted(text)
        }
        Flickable {
            id:list;x:6;y:menu.kind==="url"?42:6;width:parent.width-12;height:parent.height-y-6
            contentHeight:rows.height;clip:true;boundsBehavior:Flickable.StopAtBounds
            Column {
                id:rows;width:list.width
                Repeater {
                    model:menu.entries
                    Rectangle {
                        required property var modelData
                        required property int index
                        width:rows.width;height:34;radius:4;color:hit.containsMouse||menu.selectedIndex===index?"#465364":"transparent"
                        Text {anchors {left:parent.left;leftMargin:10;right:parent.right;rightMargin:8;verticalCenter:parent.verticalCenter} text:(parent.modelData.checked?"✓  ":"")+parent.modelData.label;color:"#eff0e6";font.pixelSize:13;elide:Text.ElideRight}
                        MouseArea {id:hit;anchors.fill:parent;hoverEnabled:true;onClicked:menu.chosen(parent.modelData);onDoubleClicked:event=>{event.accepted=true}}
                    }
                }
            }
        }
    }
}
