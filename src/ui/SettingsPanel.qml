import QtQuick
Item {
    id:settings
    property string inputRole:"settings"
    signal closed()
    property var fields:[
        {key:"audioLanguage",label:"Audio languages",type:"text"},
        {key:"subtitleLanguage",label:"Subtitle languages",type:"text"},
        {key:"autoResume",label:"Auto-resume on launch",type:"bool"},
        {key:"autoplayNext",label:"Autoplay next",type:"bool"},
        {key:"previousRestarts",label:"Previous after 5 s restarts",type:"bool"},
        {key:"stayOnLastFrame",label:"Keep last frame at playlist end",type:"bool"},
        {key:"playlistIgnore",label:"Ignore extras (comma separated)",type:"text"},
        {key:"lockAspectRatio",label:"Lock aspect ratio",type:"bool"},
        {key:"highQuality",label:"High quality video",type:"bool"},
        {key:"hardwareDecoding",label:"Hardware decoding",type:"bool"},
        {key:"seekStep",label:"Seek step (seconds)",type:"number"},
        {key:"exactSeekStep",label:"Exact seek step",type:"number"},
        {key:"largeSeekStep",label:"Large seek step",type:"number"},
        {key:"tinyPreview",label:"Mini video preview",type:"bool"},
        {key:"defaultSpeed",label:"Default speed",type:"number"}]
    MouseArea {anchors.fill:parent;onClicked:settings.closed()}
    Rectangle {
        anchors.centerIn:parent;width:Math.min(510,parent.width-16);height:Math.min(500,parent.height-16);radius:9;color:UiPalette.surface;border.color:UiPalette.border
        MouseArea {anchors.fill:parent;onClicked:event=>{event.accepted=true}}
        Text {x:16;y:12;text:"Settings";color:UiPalette.text;font.pixelSize:18}
        ActionButton {anchors.right:parent.right;anchors.rightMargin:8;y:4;label:"×";onActivated:settings.closed()}
        Flickable {
            x:12;y:46;width:parent.width-24;height:Math.max(20,parent.height-56);contentHeight:column.height;clip:true;boundsBehavior:Flickable.StopAtBounds
            Column {
                id:column;width:parent.width;spacing:4
                Repeater {
                    model:settings.fields
                    Item {
                        id:row
                        required property var modelData
                        width:column.width;height:38
                        Text {anchors {left:parent.left;verticalCenter:parent.verticalCenter} width:parent.width*.56;text:row.modelData.label;color:UiPalette.text;font.pixelSize:13;elide:Text.ElideRight}
                        Rectangle {
                            anchors.right:parent.right;anchors.verticalCenter:parent.verticalCenter;width:parent.width*.42;height:32;radius:4;color:UiPalette.surface;border.color:UiPalette.border
                            TextInput {
                                property bool textEntry:true
                                visible:row.modelData.type!=="bool";anchors.fill:parent;anchors.margins:7;color:UiPalette.text;font.pixelSize:13;clip:true;selectByMouse:true
                                text:String(preferences.values[row.modelData.key])
                                onEditingFinished:{if(row.modelData.type==="text")preferences.setValue(row.modelData.key,text);else if(isFinite(Number(text)))preferences.setValue(row.modelData.key,Number(text))}
                            }
                            Text {visible:row.modelData.type==="bool";anchors.centerIn:parent;text:preferences.values[row.modelData.key]?"On":"Off";color:UiPalette.text;font.pixelSize:13}
                            MouseArea {visible:row.modelData.type==="bool";anchors.fill:parent;onClicked:preferences.setValue(row.modelData.key,!preferences.values[row.modelData.key])}
                        }
                    }
                }
                Text {width:parent.width;text:"Language order: en,eng,ja,jpn\nPlaylist reordering lasts for this session.\n"+preferences.filePath;wrapMode:Text.Wrap;color:UiPalette.muted;font.pixelSize:11}
            }
        }
    }
}
