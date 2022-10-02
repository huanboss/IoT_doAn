var date_graph = [];
var Heart_graph = [];
var Spo2_graph = [];
var BodyTemp_graph = [];
var Eco2_graph = [];

function exportChart(con, io) {
    var m_time
    var newHeart
    var newSpo2
    var newBodyTemp
    var newEco2

    var sql1 = "SELECT * FROM sensors ORDER BY ID DESC limit 1"
    con.all(sql1, function (err, result, fields) {
        if (err) throw err;
        console.log("Data selected");
        result.forEach(function (value) {
            m_time = value.Time.toString().slice(10, 24);
            newHeart = value.Heart
            newSpo2 = value.Spo2
            newBodyTemp = value.BodyTemp
            newEco2 = value.Eco2
            console.log(m_time + " " + value.Heart + " " + value.Spo2 + " " + value.BodyTemp + " " + value.Eco2);

            io.sockets.emit('server-update-data',
                { id: value.ID, time: m_time, Heart: value.Heart, Spo2: value.Spo2, BodyTemp: value.BodyTemp, Eco2: value.Eco2})
        })
        if (Spo2_graph.length < 20) {
            Spo2_graph.push(newSpo2);
        }
        else {
            for (i = 0; i < 19; i++) {
                Spo2_graph[i] = Spo2_graph[i + 1];
            }
            Spo2_graph[19] = newSpo2;
        }

        if (Heart_graph.length < 20) {
            Heart_graph.push(newHeart);
        }
        else {
            for (u = 0; u < 19; u++) {
                Heart_graph[u] = Heart_graph[u + 1];
            }
            Heart_graph[19] = newHeart;
        }

        if (date_graph.length < 20) {
            date_graph.push(m_time);
        }
        else {
            for (x = 0; x < 19; x++) {
                date_graph[x] = date_graph[x + 1];
            }
            date_graph[19] = m_time;
        }

        if (BodyTemp_graph.length < 20) {
            BodyTemp_graph.push(newBodyTemp);
        }
        else {
            for (x = 0; x < 19; x++) {
                BodyTemp_graph[x] = BodyTemp_graph[x + 1];
            }
            BodyTemp_graph[19] = newBodyTemp;
        }

        if (Eco2_graph.length < 20) {
            Eco2_graph.push(newEco2);
        }
        else {
            for (x = 0; x < 19; x++) {
                Eco2_graph[x] = Eco2_graph[x + 1];
            }
            Eco2_graph[19] = newEco2;
        }

        io.sockets.emit("server-send-Heart_graph", Heart_graph);
        io.sockets.emit("server-send-Spo2_graph", Spo2_graph);
        io.sockets.emit("server-send-date_graph", date_graph);
        io.sockets.emit("server-send-BodyTemp_graph", BodyTemp_graph);
        io.sockets.emit("server-send-Eco2_graph", Eco2_graph);
    });

}

module.exports = exportChart