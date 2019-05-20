"use strict";

/**************************************************************************
 * A few utility functions
 */
function constrain(x, xmin, xmax) {
    if (x < xmin) {
        return xmin;
    } else if (x > xmax) {
        return xmax;
    } else {
        return x;
    }
}

function map(x, xmin, xmax, ymin, ymax) {
    return ymin + (x * 1.0 - xmin) / (xmax - xmin) * (ymax - ymin); 
}

function log(msg) {
    console.log(msg);
    var log = document.getElementById("log");
    log.value += "\n" + msg;
    log.scrollTop = log.scrollHeight;
}

/**************************************************************************
 * Generic joystick class
 * 
 */
class Joystick {
    constructor (_canvasID, _width, _height, _DofIdX, _DofIdY, _useRectMode, _useCenterSpring, _useIndicator,
        _BGColor, _constraintBGColor, _nippleFGColor) {

        this.width = _width;
        this.height = _height;
        this.dimension = Math.min(_width, _height);

        this.pos = {x: 0, y: 0}; 						// current joystick position (usually set position of robot joint)
        this.indicator = {x: 0, y: 0}; 					// indicator position (usually current position of robot joint)
        this.hold = false;								// is joystick currently being held by mouse or finger touch?
        this.callback = setCommandXY.bind(null, _DofIdX, _DofIdY); 

        this.canvas = document.getElementById(_canvasID);
        this.ctx2d = this.canvas.getContext("2d");
        this.rectMode = _useRectMode;					// use rectangle or circular constraint mode?
        this.useCenterSpring = _useCenterSpring;		// does the joystick return to {0,0} when released?
        this.useIndicator = _useIndicator;				// whether an indicator is used to show robot joint position

        this.scaling = 1.3;								// joystick constraint space size to canvas size ratio
        this.BGColor = _BGColor;						// canvas background (also indocator color)
        this.constraintBGColor = _constraintBGColor;	// constraint space background
        this.nippleFGColor = _nippleFGColor;			// joystick nipple color
        
        this.canvas.width = this.width; 				// canvas is resized at runtime
        this.canvas.height = this.height;

        if ('ontouchstart' in window || navigator.msMaxTouchPoints) {
            log('Touch screen detected');
            this.canvas.addEventListener('touchstart', this.onTouchStart.bind(this), true);
            this.canvas.addEventListener('touchmove', this.onTouchMove.bind(this), true);
            this.canvas.addEventListener('touchend', this.onTouchEnd.bind(this));
            this.canvas.addEventListener('touchcancel', this.onTouchEnd.bind(this));
        } else {
            log('No touch screen. Using mouse.');
            this.canvas.addEventListener('mousedown', this.onMouseDown.bind(this));
            this.canvas.addEventListener('mousemove', this.onMouseMove.bind(this));
            this.canvas.addEventListener('mouseup', this.onMouseUp.bind(this));
            this.canvas.addEventListener('mouseleave', this.onMouseUp.bind(this));
        }

        if (this.useIndicator) {						// simulate slow robot movements; for testing indicator feature
//				this.intervalID = setInterval(this.simulateIndicator.bind(this), 1000);
        }

        this.draw();			
    } // constructor

    draw() {
        var w = this.canvas.width;
        var h = this.canvas.height;
        var c = this.ctx2d;
        var s = this.scaling;
        var d = this.dimension;

        // joystick background
        c.fillStyle = this.BGColor;
        c.fillRect(0, 0, w, h);

        // joystick constraint space background
        var r = d/10;
        c.lineJoin = "round";
        c.lineWidth = r;
        c.fillStyle = c.strokeStyle = this.constraintBGColor;
        if (this.rectMode) { // rectangular constraint space
            c.strokeRect(map(-1, -s, s, 0, w) + r/2, map(-1, -s, s, 0, h) + r/2, 
                    map(1, 0, s, 0, w) - r, map(1, 0, s, 0, h) - r);
            c.fillRect(map(-1, -s, s, 0, w) + r/2, map(-1, -s, s, 0, h) + r/2, 
                    map(1, 0, s, 0, w) - r, map(1, 0, s, 0, h) - r);
        } else { // circular constraint space
            c.beginPath();				
            c.arc(w/2, h/2, map(1, 0, s, 0, d/2), 0, 2*Math.PI);
            c.fill();
        }

        // Indicator of robot joint current position
        if (!this.useIndicator) {
            this.indicator = this.pos;
        }
        c.beginPath();
        c.fillStyle = this.BGColor;
        c.arc(map(this.indicator.x, -s, s, 0, w),
                map(this.indicator.y, -s, s, h, 0),
                d/9, 0, 2 * Math.PI);
        c.fill();

        // Finally, joystick nipple
        c.beginPath();
        c.arc(map(this.pos.x, -s, s, 0, w),
                map(this.pos.y, -s, s, h, 0),
                d/10, 0, 2 * Math.PI);		
        c.fillStyle = this.nippleFGColor;
        c.fill();
    } // draw


    simulateIndicator() {
        var k = 0.1;
        var dx = this.pos.x - this.indicator.x;
        var dy = this.pos.y - this.indicator.y;
        var md = 0.1 * (1. / 2.); // max displacement = period (100ms) * max_speed (1 * joystick range / 5 seconds)
        dx = constrain(dx, -md, md);
        dy = constrain(dy, -md, md);
        this.indicator.x += dx;
        this.indicator.y += dy;
        this.draw();
    }

    /* 
           UI events 
    */
    onMouseDown(event) {
        event.preventDefault();
        this.hold = true;
        this.pos = this.getPos(event);
        this.draw();
        this.callback(this.pos);
    }

    onMouseMove(event) {
        if (this.hold) {
            event.preventDefault();				
            this.pos = this.getPos(event);
            this.draw();
            this.callback(this.pos);
        }
    }

    onMouseUp(event) {
        event.preventDefault();
        if (this.hold) {
            this.hold = false;
            if (this.useCenterSpring) {
                this.pos = {x : 0, y : 0};		
            }
            this.draw();
            this.callback(this.pos);
        }
    }

    onTouchStart(event) {
        var touches = event.changedTouches;
        event.preventDefault();
        if (touches.length == 1) { // TODO: handle multitouch events
            this.hold = true;
            this.pos = this.getPos(touch[0]);
            this.draw();
            this.callback(this.pos);
        }
    }

    onTouchMove(event) {
        event.preventDefault();
        var touches = event.changedTouches;
        if (touches.length == 1 && this.hold) {
            this.pos = this.getPos(touches[0]);
            this.draw();
            this.callback(this.pos);
        }
    }

    onTouchEnd(event) {
        event.preventDefault();
        this.hold = false;
        if (this.useCenterSpring) {
            this.pos = {x: 0, y: 0};
        }
        this.draw();
        this.callback(this.pos);
    }

    getPos(event) {
        //	clientX \in [0 .. width] <--> unconstrainedX \in [-scaling .. scaling]
        //	clientY \in [height .. 0] <--> unconstrainedY \in [-scaling .. scaling]
        var rect = event.target.getBoundingClientRect();
        var x = map(event.clientX - rect.left, 0, rect.width, -this.scaling, this.scaling);
        var y = map(event.clientY - rect.top, rect.height, 0, -this.scaling, this.scaling);
        // Apply joystick constraints
        if (this.rectMode) {
            // rectangular constraint mode: 
            // constrained{X, Y} \in [-1 .. 1]
            x = constrain(x, -1.0, 1.0);
            y = constrain(y, -1.0, 1.0);
        } else { 
            // circular constraint mode
            // constrained{X, Y} colinear to unconstrained{X, Y}; with norm <= 1.0
            var norm = Math.sqrt(x*x + y*y);
            if (norm > 1.0) {
                x *= 1.0 / norm;
                y *= 1.0 / norm;
            }
        }
        return {x: x, y: y};			
    } // getPos
} // class Joystick


/**
 * 
 * @param {*} DofIdX 
 * @param {*} DofIdY 
 * @param {*} pos 
 */
function setCommandXY(DofIdX, DofIdY, pos) {
    if (DofIdX != null) {
        setPos[DofIdX] = pos.x; 
        document.getElementById('dataTable').rows[DofIdX+1].cells[2].innerHTML = pos.x.toFixed(2); //map(pos.x, -1.0, 1.0, -90, 90).toFixed(2);
    }
    if (DofIdY != null) {
        setPos[DofIdY] = pos.y; 
        document.getElementById('dataTable').rows[DofIdY+1].cells[2].innerHTML = pos.y.toFixed(2);  //map(pos.y, -1.0, 1.0, -90, 90).toFixed(2);
    }
    if (wsTimer == null) { // only send event if periodic send is not activated
        send("Start!");
    }
}



/******************************************************************************
* Websocket communication handling
*/
var ws = null;
var connected = false;
var wsTimer = null;
var t0 = null;


function reconnect() {
// reconnect, if necessary
if (ws === null || ws.readyState === ws.CLOSED) {
    var address = document.getElementById('formAddress').value;
    ws = new WebSocket(address);
    ws.onopen = function () {
        var tstamp = new Date().valueOf();
        ws.send('Connect at ' + tstamp);
        log('Connected');
        connected = true;
    };
    
    ws.onerror = function (error) {
        console.error('Client: WebSocket Error ' + error);
        log('An error occurred');
        connected = false;
    };

    ws.onmessage = function (e) {
        console.log('< {' + e.data + '}');
        log('< Got ' + e.data);
        parseMessage(e.data);
        connected = true;
    };
}

// Send current position at periodic interval
if (wsTimer === null) {
    wsTimer = setInterval(function() {
        var tstamp = new Date().valueOf();
        if (t0 === null) t0 = tstamp;

/* 			var msg = "{";
        for (var i = 0; i < setPos.length; i++) {
            msg += "'" + String.fromCharCode(i + "a".charCodeAt()) + "':" + setPos[i].toFixed(1) + ",";
        }
        msg += "'t':" +(tstamp - t0) + "}";
*/			
        var msg = "@"  + setPos.map(x => Math.round(100*x)).join(",") + ",t:" + (tstamp-t0);

         send(msg);
    }, 1000);
}
} // reconnect

function send(data) {
reconnect();
if (ws.readyState === ws.OPEN) {
    console.log('> ' + data);
    log('> ' + data);
    ws.send(data);
} else {
    log('No connexion. Attempting to reconnect... (> ' + data + ')');
}
}

function parseMessage(msg) {
if (msg.charAt(0) == '#' || msg.charAt(0) == '@') {
    msg = msg.slice(1); // remove first character
    curPos = msg.split(',').slice(0,10).map(x => x * .01);

    // Todo: generalize usage of DofIdX and DofIdY for indicator
    navJoystick.indicator = {x: curPos[1], y: curPos[0]};
    headJoystick.indicator = {x: curPos[2], y: curPos[3]};
    leftArmJoystick.indicator = {x: curPos[5], y: curPos[4]};
    rightArmJoystick.indicator = {x: curPos[8], y: curPos[7]};
    headJoystick.draw();
    leftArmJoystick.draw();
    rightArmJoystick.draw();

    for (var i = 0; i < curPos.length; i++) {
        var table = document.getElementById('dataTable');
        if (i+1 < table.rows.length) {
            table.rows[i+1].cells[3].innerHTML = curPos[i].toFixed(2); //map(curPos[i], -1.0, 1.0, -90.0, 90.0).toFixed(2);
        } else {
            console.log("Table length overflow! " + curPos.length);
        }
    }

    } else {
        log("Wrong format '" + msg + "'");
    }
}

function updateDoFTable(dofId, value) {
    var table = document.getElementById('dataTable');
    if (0 <= dofId && dofId < table.rows.length-1) {
        table.rows[dofId+1].cells[3].innerHTML = value.toFixed(2);
    }
}

function setup() {

navJoystick = new Joystick(/* _canvasID */ "canvasNavJoystick", 
    /* _width, _height */200, 200,
    /* DofIdX, DofIdY */ 1, 0,
//    /* _callback(DofIdX, DofIdY) */setCommandXY.bind(null, 1, 0), 
    /* _useRectMode, _useCenterSpring, _useIndicator */false, true, false, 
    /* _BGColor, _constraintBGColor, _nippleFGColor */'#eee', '#ffc653', '#ffaa00');

headJoystick = new Joystick/* _canvasID */ ("canvasHeadJoystick", 
    /* _width, _height */350, 200,
    /* DofIdX, DofIdY */ 2, 3,
//    /* _callback(DofIdX, DofIdY) */setCommandXY.bind(null, 2, 3), 
    /* _useRectMode, _useCenterSpring, _useIndicator */true, false, true, 
    /* _BGColor, _constraintBGColor, _nippleFGColor */'#ffd3c2', '#ff8453', '#ff4900');

leftArmJoystick = new Joystick(/* _canvasID */ "canvasLeftArmJoystick", 
    /* _width, _height */300, 200,
    /* DofIdX, DofIdY */ 8, 7,
//    /* _callback(DofIdX, DofIdY) */setCommandXY.bind(null, 8, 7), 
    /* _useRectMode, _useCenterSpring, _useIndicator */true, false, true, 
    /* _BGColor, _constraintBGColor, _nippleFGColor */'#bccbef', '#5278d0', '#0d40b7');

rightArmJoystick = new Joystick(/* _canvasID */ "canvasRightArmJoystick", 
    /* _width, _height */300, 200,
    /* DofIdX, DofIdY */ 5, 4,
//    /* _callback(DofIdX, DofIdY) */setCommandXY.bind(null, 5, 4), 
    /* _useRectMode, _useCenterSpring, _useIndicator */true, false, true, 
    /* _BGColor, _constraintBGColor, _nippleFGColor */'#bccbef', '#5278d0', '#0d40b7');



$('#buttonConnect').click(function () {
    send('Reconnect');
    $('#buttonConnect').removeClass("btn-primary").addClass("btn-success");
});

$('#buttonTest').click(function () {
    document.getElementById('formAddress').value="ws://demos.kaazing.com/echo";
    send('Reconnect');
    $('#buttonTest').removeClass("btn-primary").addClass("btn-success");
});

log("Remote controller loaded");
}

var M_FORWARD = 0,
    M_TURN_RIGHT = 1,
    H_PAN = 2, 
    H_TILT = 3, 
    R_SHOULDER_ELEVATION = 4, 
    R_SHOULDER_EXTENSION = 5, 
    R_HAND = 6, 
    L_SHOULDER_ELEVATION = 7,
    L_SHOULDER_EXTENSION = 8, 
    L_HAND = 9;

var setPos = [0,0,0,0,0,0,0,0,0,0];
var curPos = [0,0,0,0,0,0,0,0,0,0];
var navJoystick;
var headJoystick;
var leftArmJoystick;
var rightArmJoystick;


window.onload = setup();

