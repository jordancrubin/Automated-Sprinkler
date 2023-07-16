/*!
  * scripts.js
  */ 
var rsexist;
var meterzoneval;

function detaQuery(dayval,type){
  let datatable = new simpleDatatables.DataTable("#datatablesSimple");
  var days;
  if (!dayval){days =document.getElementById('metricDays').value;}
  else {days = dayval;}
  const start = Math.floor(Date.now() / 1000);
  const lastmonth = start - (86400*days);
  var myquery = '{"query":[{"edtime?gt":'+lastmonth+',"edtime?lt":'+start+'}]}';
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
  if (this.readyState == 4 && this.status == 200) {
    var data = JSON.parse(this.responseText);
    if (data.items){  
      if (type == 'G'){return data.items;}
      var zonedata = JSON.parse(sessionStorage.getItem("zonedata"));   
      let newerData =  {};
      newerData.data = [];
      for (let i = 0; i < data.items.length; ++i) {
        var name;
        for (let j = 0; j < zonedata.length; ++j) {        
            if (zonedata[j].port == data.items[i].port){
              name = zonedata[j].name;
            break;
          }
        }
        newerData.data[i] = [];
        var date = new Date((data.items[i].key * 1000));
        var time = convertMinsToHrsMins(data.items[i].duration);
        newerData.data[i][0] = date.toDateString();
        newerData.data[i][1] = name;
        newerData.data[i][2] = data.items[i].port;
        newerData.data[i][3] = time;
        newerData.data[i][4] = data.items[i].trigger;
        newerData.data[i][5] = data.items[i].consumption;
      }
      datatable.insert(newerData);
    } 
    else {
      alert("No Data Returned");
    }
   }
  };  
  var dbcreds = JSON.parse(sessionStorage.getItem("dbcreds"));
  var dburl = 'https://database.deta.sh/v1/'+dbcreds.i+'/'+dbcreds.n+'/query';
  xhr.open("POST", dburl, true);
  xhr.setRequestHeader("Content-type", "application/json");
  xhr.setRequestHeader("X-API-Key",dbcreds.a);
  xhr.send(myquery);  
}

function convertMinsToHrsMins (minutes) {
  var h = Math.floor(minutes / 60);
  var m = minutes % 60;
  h = h < 10 ? '0' + h : h; 
  m = m < 10 ? '0' + m : m; 
  return h + ':' + m;
}

function activeProgChange(){
  var sel = document.getElementById("activeprog");
  var opt = sel.options[sel.selectedIndex];
  progval = opt.value; 
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var data = JSON.parse(this.responseText);
      if (data.s == "0"){           
      }
      else {
        alert("Error. Not yet configured....");
        for(var i = 0; i < sel.length; i++) {
          sel[i].selectedIndex =0;
        } 
      }
    }
  };  
  xhr.open("POST", "updateConfig", true);
  xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  xhr.send("event=progchange&value="+progval); 
}

function addRainsenseDropdown() {   
  var val = document.getElementById("rainsenseselector");
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var data = JSON.parse(this.responseText);
      if (data.rsexist == 1){
        rsexist = data.rsexist;
        val.innerHTML = '&nbsp;&nbsp;Rain Sensor&nbsp; <select class="selectpicker" id="rainsenssel" onchange="rainSenseChange()"><option value="0">Disabled</option>'+
                        '<option value="1">Enabled</option></select>';
        getCurrentRainSense();
      }
      else {
        val.innerHTML = '&nbsp;&nbsp;Rain Sensor [Not Installed]';
      }
    } 
  };  
  xhr.open("POST", "updateStatus", true);
  xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  xhr.send(); 
}

function cancelRun() {
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var data = JSON.parse(this.responseText);
    }
  };  
  xhr.open("POST", "cancelRun", true);
  xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  xhr.send(); 
}

function cookieChk(){
  var testsession = getCookie("SESSIONID");
  if (testsession == 0){
  }
}

function getCookie(cname) {
  var name = cname + "=";
  var decodedCookie = decodeURIComponent(document.cookie);
  var ca = decodedCookie.split(';');
  for(var i = 0; i <ca.length; i++) {
    var c = ca[i];
    while (c.charAt(0) == ' ') {
      c = c.substring(1);
    }
    if (c.indexOf(name) == 0) {
      return c.substring(name.length, c.length);
    }
  }
  return "";
}

function getCurrentProg(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var data = JSON.parse(this.responseText);
      if (data.p){                 
        var prog = document.getElementById('activeprog');
        for (var opt, j = 0; opt = prog[j]; j++) {
          if (data.p ==  prog[j].value){
            prog.options.selectedIndex = j;
          }
        }  
      }
    }
  };  
  xhr.open("POST", "getProg", true);
  xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  xhr.send(); 
}

function getCreds(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var data = JSON.parse(this.responseText);
      if (data.n){    
        sessionStorage.setItem("dbcreds", JSON.stringify(data));             
      }
    }
  };  
  xhr.open("GET", "getCreds", true);
  xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  xhr.send(); 
}

function readMeter(){ 
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var data = JSON.parse(this.responseText);      
      if (data.p){                 
        var meter = document.getElementById('CurrentMeterReading');
        meter.value = data.p;
      }
    }
  };  
  xhr.open("POST", "readMeter", true);
  xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  xhr.send(); 
}

function getCurrentRainSense(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var data = JSON.parse(this.responseText);
      if (data.p){                 
        var prog = document.getElementById('rainsenssel');
        for (var opt, j = 0; opt = prog[j]; j++) {
          if (data.p ==  prog[j].value){
            prog.options.selectedIndex = j;
          }
        }  
      }
    }
  };  
  xhr.open("POST", "getRainsense", true);
  xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  xhr.send(); 
}

function getURLParameter(sParam){
  var sPageURL = window.location.search.substring(1);
  var sURLVariables = sPageURL.split('&');
  for (var i = 0; i < sURLVariables.length; i++){
    var sParameterName = sURLVariables[i].split('=');
    if (sParameterName[0] == sParam){
      return sParameterName[1];
    }
  }
}

function LogoutDialogue() {
  if (confirm("Are you sure you want to logout?")) {
    const Http = new XMLHttpRequest();
    const url='/logout';
    Http.open("GET", url);
    Http.send();
    Http.onreadystatechange = (e) => {
      window.location.href = "login.html";
    }           
  } 
  else { }
}

function manualRunListChange() {
  var dropdown = document.getElementById('manualrunlist');
  var value = dropdown.options[dropdown.selectedIndex].value;
  var element = document.getElementById("manualrunbutton");
  if (value == ''){
    element.style.display = "none";
  }
  else{
    element.style.display = "block";
  }
}

function printProgrammes(){
  var content ='';
  for (let i = 0; i < 3; i++) {
    var letter = 'A';
    if(i==1){letter='B';}
    if(i==2){letter='C';}
    var subContent = 
    '<div class="row">'+
    '<div class="col-xs-4 col-md-6">'+
    '  <div class="alert alert-info" id="pwinfo">'+
    '    PROGRAMME ['+letter+']'+
    '  </div>'+
    '</div>'+
  '</div>'+                         
  '<div class="row">'+
  '<div class="col-xs-4 col-md-3">'+
      '<div class="form-group">'+
       '<!-- <form>-->'+
       '<div class="form-group">'+
       '<label for="starttime'+letter+'">Start Time 0000-2359'+
       '<input type="text" class="form-control" id="st'+letter+'" name="st'+letter+'" placeholder="1800">'+
         '</label>'+
       '<label class="radio-inline">'+   
         '<div class="weekDays-selector">'+
           '&nbsp;<input type="checkbox" id="'+letter+'-1" class="weekday" /> '+
           '<label for="weekday-mon">M</label> '+
           '<input type="checkbox" id="'+letter+'-2" class="weekday" /> '+
           '<label for="weekday-tue">T</label> '+
           '<input type="checkbox" id="'+letter+'-3" class="weekday" /> '+
           '<label for="weekday-wed">W</label> '+
           '<input type="checkbox" id="'+letter+'-4" class="weekday" /> '+
           '<label for="weekday-thu">T</label> '+
           '<input type="checkbox" id="'+letter+'-5" class="weekday" /> '+
           '<label for="weekday-fri">F</label> '+
           '<input type="checkbox" id="'+letter+'-6" class="weekday" /> '+
           '<label for="weekday-sat">S</label> '+
           '<input type="checkbox" id="'+letter+'-7" class="weekday" /> '+
           '<label for="weekday-sun">S</label> '+
         '</div>'+  
       '</label>'+
   '</div>'+
   '</div>'+
   '</div>'+
'</div>'+
'</div>'+
'</br></br>'+
'<div class="row">'+
'<div class="col-xs-4 col-md-6">'+
'<div class="alert alert-info">'+
' Zone Usage'+
'</div>'+
'</div>'+
'</div>'+
'<div class="row">'+
'<div class="col-xs-4 col-md-6">'+
   '<div class="form-group">'+
     '<form>'+
       '<div class="form-group">'+
           '<table id="myTable" class=" table order-list'+letter+'">'+
               '<thead>'+
                   '<tr>'+
                       '<td>Name</td>'+
                       '<td>Duration</td>'+
                   '</tr>'+
               '</thead>'+
               '<tbody>'+
               '</tbody>'+
               '<tfoot>'+
                   '<tr>'+
                       '<td>'+    
                           '</br>'+
                       '</td>'+
                   '</tr>'+
                   '<tr>'+
                   '</tr>'+
               '</tfoot>'+
           '</table>'+
       '</div>'+
    '</div>'+  
'</div>';
  content = content+subContent;
  }  
  document.getElementById("programmeListing").innerHTML = content;
}

function rainSenseChange(){
  var sel = document.getElementById("rainsenssel");
  var opt = sel.options[sel.selectedIndex];
  rainsenseval = opt.value; 
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var data = JSON.parse(this.responseText);
      if (data.s == "0"){  }
      else {
        alert("Update error, try again...");
      }
    }
  };  
  xhr.open("POST", "updateConfig", true);
  xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  xhr.send("event=rainsensechange&value="+rainsenseval); 
}

function submitManualRun() {
  var dropdown = document.getElementById('manualrunlist');
  var value = dropdown.options[dropdown.selectedIndex].value;
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var data = JSON.parse(this.responseText);
    }
  };  
  xhr.open("POST", "sendManual", true);
  xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  xhr.send("zone="+value); 
}

function submitMeter() {
  var value = document.getElementById('newMeterReading').value;
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var data = JSON.parse(this.responseText);
    }
  };  
  xhr.open("POST", "sendNewMeter", true);
  xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  xhr.send("value="+value); 
}

function submitMetric() {
  var days = document.getElementById('metricDays').value;
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var data = JSON.parse(this.responseText);
    }
  };  
  xhr.open("POST", "metricRequest", true);
  xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  xhr.send("statsdays="+days); 
}

function submitLeaktest() {
  var minutes = document.getElementById('leaktesttime').value;
  var minutesval = sessionStorage.getItem("minutesval");
console.log("Submitting leak test for "+minutes+" minutes");
console.log("minval is "+minutesval);

  if ((minutes+2) >= minval) { 
    alert("The test cannot start, it will interrupt the schedule. Try again later");
    return 0;
  }
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var data = JSON.parse(this.responseText);
    }
  };  
  xhr.open("POST", "leakTestRequest", true);
  xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  xhr.send("testminutes="+minutes); 
}

function tzPopulate(){
  var selectElem = document.getElementById("timezone");
  for (var i = 0; i < TZdata.length; i++){
    var name = TZdata[i].FIELD1;
    var value = TZdata[i].FIELD2;
    var element = document.createElement("option");
    element.innerText = name;
    element.value = value;
    selectElem.append(element);
  }    
}

function updateStatus(type){
  var meterInterval;
  if (type == 'index'){
    var state = document.getElementById("state");
    var stateinfo = document.getElementById("stateInfo");
    var raininfo = document.getElementById("rainState");
  }
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var data = JSON.parse(this.responseText);    
      if (type == 'leaktest'){
        if(data.state == "3"){
          console.log("dd "+data.info);       
            sessionStorage.setItem("minutesval", data.info);
            return 0;
        }
      }
      valvestate = sessionStorage.getItem("valvestate");
      if (valvestate != 'CLOSED'){
        if (data.valve == 'CLOSED'){
          setTimeout(()=> { detaQuery(30,'T'); console.log("updated!"); }, 10000);
        }
      }
      sessionStorage.setItem("valvestate", data.valve);
      if(rsexist){
        var sel = document.getElementById("rainsenssel");
        var opt = sel.options[sel.selectedIndex];
        rainsenseval = opt.value; 
        if (rainsenseval == 1){
          if (data.rsstate == '0'){
            raininfo.innerHTML="Rain Sensor is <font color='red'>Engaged</font>";
          }
          else {
            raininfo.innerHTML="Rain Sensor is <font color='green'>Disengaged</font>";
          }       
        }
        else {raininfo.innerHTML="";}
      }
      if (data.valve == 'CLOSED'){
        valveState.innerHTML="Valve is <font color='green'>Closed</font>";
      }
      else if(data.valve == 'OPEN'){
        valveState.innerHTML="Valve is <font color='red'>Open</font>";
      }
      else if(data.valve == 'MIDWAY'){
        valveState.innerHTML="Valve is <font color='orange'>Midway</font>";
      }
      else{
        valveState.innerHTML="Valve Status <font color='blue'>Unknown</font>";
      } 
      if (data.state == "0"){
        celement.style.display = "none";
        state.innerHTML="Programme is disabled.....";
        stateinfo.innerHTML= "";
      }
      else if(data.state == "1"){
         celement.style.display = "none";
         state.innerHTML="Sprinkler is Idle Today.....";
         stateinfo.innerHTML= "";
      }
      else if(data.state == "2"){
        state.innerHTML="Manual Cancellation Requested.....";
        stateinfo.innerHTML= "";
      }
      else if(data.state == "3"){
        celement.style.display = "none";
        state.innerHTML="Programme begins in.....";
        if (data.info > 60){  
          var hours = Math.floor(data.info / 60);
          var minutes = data.info%60;
          stateinfo.innerHTML= hours+" hours "+minutes+" minutes";
        }
        else {
          stateinfo.innerHTML= data.info+" minutes";
        }
      }
      else if(data.state == "4"){
        celement.style.display = "none";
        state.innerHTML="Todays schedule has completed.....";
        stateinfo.innerHTML= "";
      }
      else if(data.state == "5"){
        celement.style.display = "block";
        var minutes = data.info;
        var zone = data.zone;
        if (minutes >1){
          state.innerHTML="Zone "+zone+" - "+minutes+" minutes remain";
        }
        else {
          state.innerHTML="Zone "+zone+" - "+minutes+" minute remains";
        }
          stateinfo.innerHTML= "Consumption <span id=\"consumeval\">"+meterzoneval+"</span>/min";
      }
      else if(data.state == "6"){
        var zone = data.zone;
        celement.style.display = "block";
        state.innerHTML="Zone "+zone+" *Rain Delay*";
        stateinfo.innerHTML= "";
      }
        else {
          alert("Update error, try again...");
        }
    }
  };  
  xhr.open("POST", "updateStatus", true);
  xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  xhr.send(); 
}

function updateZoneList() {
  var dropdown = document.getElementById('manualrunlist');     
    var option;
    dropdown.length = 0;
    defaultOption = document.createElement('option');
    defaultOption.text = "Refreshing Configured Zones";
    dropdown.add(defaultOption);
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {    
        var data = JSON.parse(this.responseText);
        sessionStorage.setItem("zonedata", JSON.stringify(data)); 
        dropdown.length = 0;
        defaultOption = document.createElement('option');
        defaultOption.text = 'Choose a zone.....';
        defaultOption.value = '';
        dropdown.add(defaultOption);
        dropdown.selectedIndex = 0;  
        for (let i = 0; i < data.length; i++) {       
          option = document.createElement('option');      
          option.text = data[i].val+" - "+data[i].name;
          option.value = data[i].val;
          dropdown.add(option);
        }
      }
    };  
    xhr.open("GET", "getZoneList", true);
    xhr.send();
  }

function addSidebar() { 
const content = `
<div class="nav">
<a class="nav-link" href="index.html">
    <div class="sb-nav-link-icon"><i class="fas fa-tachometer-alt"></i></div>
    Dashboard
</a>

<div class="sb-sidenav-menu-heading">Configure</div>
<a class="nav-link" href="configure.html">
    <div class="sb-nav-link-icon"><i class="fas fa-chart-area"></i></div>
    Main Config
</a>
<a class="nav-link" href="programme.html">
    <div class="sb-nav-link-icon"><i class="fas fa-table"></i></div>
    Programmes
</a>
<a class="nav-link" href="setMeter.html">
    <div class="sb-nav-link-icon"><i class="fa fa-leaf"></i></div>
    Set Meter
</a>
<div class="sb-sidenav-menu-heading">Addons</div>
<a class="nav-link" href="leakcheck.html">
    <div class="sb-nav-link-icon"><i class="fa fa-bath"></i></div>
    Leak Test
</a>
</div>
<div class="sb-sidenav-footer">
<div class="small" id="username"></div>
</div>
<a class="nav-link" href="#!" onclick="LogoutDialogue()">
<div class="sb-nav-link-icon"><i class="fas fa-user-circle"></i> Logout</div></a>
</div>
`;
document.getElementById("sidenav-menu-content").innerHTML = content;
document.getElementById("username").innerHTML = "Logged in as: "+getCookie("USER");
}

window.addEventListener('DOMContentLoaded', event => {
    // Toggle the side navigation
    const sidebarToggle = document.body.querySelector('#sidebarToggle');
    if (sidebarToggle) {
        // Uncomment Below to persist sidebar toggle between refreshes
        // if (localStorage.getItem('sb|sidebar-toggle') === 'true') {
        //     document.body.classList.toggle('sb-sidenav-toggled');
        // }
        sidebarToggle.addEventListener('click', event => {
            event.preventDefault();
            document.body.classList.toggle('sb-sidenav-toggled');
            localStorage.setItem('sb|sidebar-toggle', document.body.classList.contains('sb-sidenav-toggled'));
        });
    }
});

var msgArray = [];
var lastid;
if (!!window.EventSource) {
  var source = new EventSource('/events');
  source.addEventListener('open', function(e) {
    console.log(e);

console.log("Events Connected");
}, false);

source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
}, false);

source.addEventListener('meter', function(e) {
  console.log(e);
  meterzoneval = e.data;
}, false);

source.addEventListener('stats', function(e) {
  console.log(e);
  if(e.data ==1){
    console.log("reloading stats...");
    location.reload();
   };
}, false);

source.addEventListener('metrics', function(e) {
  console.log(e);
  metrics = e;
}, false);

source.addEventListener('message', function(e) {
  if (sessionStorage.getItem("logdata")){
    msgArray = JSON.parse(sessionStorage.getItem("logdata"));
  }
  console.log(e);
  var thisid = e.lastEventId;
  console.log("TID "+thisid);
  if (thisid == lastid){  console.log("repeat");  return 0;}
  lastid = thisid;
  if (e.data != 'INIT'){
    if (msgArray.length > 0){msgArray.unshift(e.data);}
    else {msgArray.push(e.data);}
    if (msgArray.length == 20){
      msgArray.pop();
    }
    sessionStorage.setItem("logdata", JSON.stringify(msgArray));
    console.log(msgArray);
  }

  var storedArray = JSON.parse(sessionStorage.getItem("logdata"));//no brackets
  console.log(storedArray);
  if (document.getElementById("eventLog")){
    var textarea = document.getElementById("eventLog");
    textarea.value = msgArray.join("\n");
  }
  console.log("message", e.data);
}, false);

source.addEventListener('temperature', function(e) {
  console.log("temperature", e.data);
  document.getElementById("temp").innerHTML = e.data;
}, false);
}
else {
  console.log("WE source already");
}