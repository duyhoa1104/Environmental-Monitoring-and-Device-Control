const firebaseConfig = {
    apiKey: "AIzaSyCi4mjxvc3P01vEi0q9jb-frWeLPa3YKj0",
    authDomain: "esp32-lora-dht11.firebaseapp.com",
    databaseURL: "https://esp32-lora-dht11-default-rtdb.firebaseio.com",
    projectId: "esp32-lora-dht11",
    storageBucket: "esp32-lora-dht11.appspot.com",
    messagingSenderId: "354919008589",
    appId: "1:354919008589:web:535d5d3fa17d6b22ce4f04",
  };
  firebase.initializeApp(firebaseConfig);
    // Get a reference to the database service
    var database = firebase.database();
  
//den1

var btnon1 = document.getElementById("onlight1");
var btnoff1 = document.getElementById("offlight1");
//den2
var btnon2 = document.getElementById("onlight2");
var btnoff2 = document.getElementById("offlight2");


btnon1.onclick = function(){
    database.ref("/Kho01").update({
        "led01" : 1
    });
}

btnoff1.onclick = function(){
    database.ref("/Kho01").update({
        "led01" : 0
    });
}

btnon2.onclick = function(){
    database.ref("/Kho01").update({
        "led02" : 1
    });
}
btnoff2.onclick = function(){
    database.ref("/Kho01").update({
        "led02" : 0
    });
}
 //get Temp from Firebase (auto update when data changes)-------------
 database.ref("/Kho01/Temp").on("value", function(snapshot) {
    if(snapshot.exists()){
      var temp = snapshot.val();
      document.getElementById("temp").innerHTML = temp;
    }
    else
      console.log("No data available!")
  });
  database.ref("/Kho01/Humi").on("value", function(snapshot) {
    if(snapshot.exists()){
      var humi = snapshot.val();
      document.getElementById("humidity").innerHTML = humi;
    }
    else
      console.log("No data available!")
  });
  database.ref("/Kho01/RSSI").on("value", function(snapshot) {
    if(snapshot.exists()){
      var rssi = snapshot.val();
      document.getElementById("rssi").innerHTML = rssi;
    }
    else
      console.log("No data available!")
  });
 //auto update ImgDen
 database.ref("/Kho01/led01").on("value", function(snapshot) {
  if(snapshot.exists()){
    var ss = snapshot.val();
    if(ss==1)
      document.getElementById("light1").src = "./assets/image/light_on.png";
    else
      document.getElementById("light1").src = "./assets/image/light_off.png"
  }else
    console.log("No data available!")
});
database.ref("/Kho01/led02").on("value", function(snapshot) {
  if(snapshot.exists()){
    var ss = snapshot.val();
    if(ss==1)
      document.getElementById("light2").src = "./assets/image/light_on.png";
    else
      document.getElementById("light2").src = "./assets/image/light_off.png"
  }
});

