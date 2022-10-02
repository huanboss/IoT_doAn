
const sqlite3 = require('sqlite3').verbose();

const con = new sqlite3.Database("./db.sqlite3" , sqlite3.OPENREADWRITE , (err) =>{
  if (err)  return console.error(err.message);
  console.log("connected")
})

module.exports = con;