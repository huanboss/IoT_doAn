var express = require('express')  // Module xử lí chung
var mysql = require('mysql2')     // Module cho phép sử dụng cơ sở dữ liệu mySQL 
var mqtt = require('mqtt')        // Module cho phép sử dụng giao thức mqtt
const con = require(`./db`)
var awsIot = require('aws-iot-device-sdk');

var app = express()
var port = 80                   // Port của localhost do mình chọn

var exportCharts = require('./export.js') // Require file export.js

app.use(express.static("public"))
app.set("views engine", "ejs")
app.set("views", "./views")

var server = require("http").Server(app)
var io = require('socket.io')(server)

app.get('/', function (req, res) {
    res.render('home.ejs')
})

app.get('/history', function (req, res) {
    res.render('history.ejs')
})

app.get('/pages-register', function (req, res) {
    res.render('pages-register.ejs')
})

app.get('/pages-login', function (req, res) {
    res.render('pages-login.ejs')
})

app.get('/users-profile', function (req, res) {
    res.render('users-profile.ejs')
})

server.listen(port, function () {
    console.log('Server listening on port ' + port)
})

//---------------------- MQTT -------------------------

//var client = mqtt.connect("mqtt://broker.hivemq.com:1883");

var device = awsIot.device({
    keyPath: "./private.pem.key",
   certPath: "./certificate.pem.crt",
     caPath: "./AmazonRootCA1.pem",
   clientId: "esp32",
       host: "a29cq9d2hqsfp3-ats.iot.ap-southeast-1.amazonaws.com"
 });

// declare topics

var topic4 = "lamp-topic";
var topic5 = "fan-topic";
var topic6 = "dehumi-topic";

var topic_list = ["nguyenvanhuan-B18DCDT088"];

//console.log("connected flag  " + client.connected);
device.on("connect", function () {
    console.log("connected mqtt ");
});

device.on("error", function (error) {
    console.log("Can't connect" + error);
    process.exit(1)
});

device.subscribe("nguyenvanhuan-B18DCDT088");

// SQL--------Temporarily use PHPMyAdmin------------------------------
// var con = mysql.createConnection({
//         host: 'localhost',
//         port: 3306,
//         user: 'anhld',
//         password: 'Ninoizmylove2610',
//         database: 'ptit_database'
//     });

 //---------------------------------------------CREATE TABLE-------------------------------------------------
var sql = "CREATE TABLE IF NOT EXISTS sensors(ID INTEGER PRIMARY KEY AUTOINCREMENT , Time datetime not null, Heart int(3) not null, Spo2 int(3) not null, BodyTemp int(4) not null, Eco2 int(4) not null )"
con.run(sql, function (err) {
    if (err)
        throw err;
    console.log("Table created");
});


var newHeart
var newSpo2
var newBodyTemp
var newEco2

//--------------------------------------------------------------------
var cnt_check = 0;
device.on('message', function (topic, message) {
    console.log("message is " + message)
    console.log("topic is " + topic)
    const objData = JSON.parse(message)
    if (topic == "nguyenvanhuan-B18DCDT088") {
        cnt_check = cnt_check + 1
        newHeart  = objData.Heart;
        newSpo2  = objData.Spo2;
        newBodyTemp = objData.BodyTemp;
        newEco2 = objData.Eco2;
    }

    if (cnt_check == 1) {
        
        console.log("ready to save")
        var n = new Date()
        var month = n.getMonth() + 1
        var Date_and_Time = n.getFullYear() + "-" + month + "-" + n.getDate() + " " + n.getHours() + ":" + n.getMinutes() + ":" + n.getSeconds();

        var sql = "INSERT INTO sensors (Time, Heart, Spo2, BodyTemp , Eco2) VALUES ('" + Date_and_Time.toString() + "', '" + newHeart + "', '" + newSpo2 + "', '" + newBodyTemp + "', '" + newEco2 + "')"
        con.run(sql, function (err, result) {
            if (err) throw err;
            console.log("Table inserted");
            console.log(Date_and_Time + " " + newHeart + " " + newSpo2 + " " + newBodyTemp + " " + newEco2)
        });

        exportCharts(con, io)
        cnt_check = 0
    }
})

//----Socket---------Control devices----------------------------

io.on('connection', function (socket) {
    console.log(socket.id + " connected")
    socket.on('disconnect', function () {
        console.log(socket.id + " disconnected")
    })
    
    socket.on("lamp-topic-change", function (data) {
        if (data == "on") {
            console.log('lamp-topic ON')
            device.publish(topic4, '1');
        }
        else {
            console.log('lamp-topic  OFF')
            device.publish(topic4, '0');
        }
    })

    socket.on("fan-topic-change", function (data) {
        if (data == "on") {
            console.log('fan-topic ON')
            device.publish(topic5, '1');
        }
        else {
            console.log('fan-topic OFF')
            device.publish(topic5, '0');
        }
    })


    socket.on("dehumi-topic-change", function (data) {
        if (data == "on") {
            console.log('dehumi-topic ON')
            device.publish(topic6, '1');
        }
        else {
            console.log('dehumi-topic OFF')
            device.publish(topic6, '0');
        }
    })

    // Send data to History page
    var sql1 = "SELECT * FROM sensors ORDER BY ID"
    con.all(sql1, function (err, result, fields) {
        if (err) throw err;
        console.log("Full Data selected");
        var fullData = []
        result.forEach(function (value) {
            var m_time = value.Time.toString();
            fullData.unshift({ id: value.ID, time: m_time, Heart: value.Heart, Spo2: value.Spo2 , BodyTemp: value.BodyTemp , Eco2: value.Eco2})
        })
        io.sockets.emit('send-full', fullData)
    })
})
