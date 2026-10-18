import QtQuick
import QtQuick.Window
Canvas {
    id:icon
    property string name:"play"
    property color tint:"#f0efdf"
    width:24;height:24
    canvasSize:Qt.size(Math.ceil(width*Screen.devicePixelRatio),Math.ceil(height*Screen.devicePixelRatio))
    onNameChanged:requestPaint()
    onTintChanged:requestPaint()
    onCanvasSizeChanged:requestPaint()
    // Static cached texture. Repaint only when the icon or its size changes.
    onPaint:{
        let c=getContext("2d");c.reset();c.clearRect(0,0,canvasSize.width,canvasSize.height);c.scale(canvasSize.width/24,canvasSize.height/24)
        c.fillStyle=tint;c.strokeStyle=tint;c.lineWidth=1.7;c.lineCap="round";c.lineJoin="round"
        function line(points){c.beginPath();c.moveTo(points[0],points[1]);for(let i=2;i<points.length;i+=2)c.lineTo(points[i],points[i+1]);c.stroke()}
        function polygon(points){c.beginPath();c.moveTo(points[0],points[1]);for(let i=2;i<points.length;i+=2)c.lineTo(points[i],points[i+1]);c.closePath();c.fill()}
        function box(x,y,w,h){c.strokeRect(x,y,w,h)}
        switch(name){
        case "play":polygon([5,2,22,12,5,22]);break
        case "pause":c.fillRect(5,3,5,18);c.fillRect(14,3,5,18);break
        case "previous":polygon([12,4,2,12,12,20]);polygon([22,4,12,12,22,20]);break
        case "next":polygon([2,4,12,12,2,20]);polygon([12,4,22,12,12,20]);break
        case "volume":case "mute":
            polygon([2,9,6,9,12,4,12,20,6,15,2,15]);
            if(name==="mute"){line([16,8,22,16]);line([22,8,16,16])}
            else {c.beginPath();c.arc(11,12,6,-.8,.8);c.stroke();c.beginPath();c.arc(11,12,10,-.8,.8);c.stroke()}
            break
        case "audio":box(3,4,17,13);line([7,8,7,13,10,12,10,7,16,6,16,11]);c.beginPath();c.arc(8,13,1.5,0,Math.PI*2);c.fill();c.beginPath();c.arc(14,12,1.5,0,Math.PI*2);c.fill();line([9,20,15,20]);break
        case "subtitles":box(2,4,20,13);line([6,9,10,9]);line([13,9,18,9]);line([6,13,18,13]);line([14,17,14,21,19,17]);break
        case "playlist":box(3,3,17,15);line([6,7,17,7]);line([6,11,14,11]);line([6,15,11,15]);polygon([15,13,23,18,15,23]);break
        case "more":c.beginPath();c.arc(12,12,10,0,Math.PI*2);c.stroke();for(let x of [7,12,17]){c.beginPath();c.arc(x,12,1,0,Math.PI*2);c.fill()}break
        case "fullscreen":line([3,9,3,3,9,3]);line([15,3,21,3,21,9]);line([21,15,21,21,15,21]);line([9,21,3,21,3,15]);break
        case "mini":box(3,5,18,14);c.fillRect(5,14,14,3);break
        case "restore":box(3,8,13,13);line([8,8,8,3,21,3,21,16,16,16]);break
        case "pin":line([8,3,16,3,15,10,19,15,5,15,9,10,8,3]);line([12,15,12,22]);break
        case "minimize":line([5,15,19,15]);break
        case "maximize":box(5,5,14,14);break
        case "close":line([5,5,19,19]);line([19,5,5,19]);break
        case "file":line([6,2,14,2,19,7,19,22,6,22,6,2]);line([14,2,14,7,19,7]);break
        }
    }
}
