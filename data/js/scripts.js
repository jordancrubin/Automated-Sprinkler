/*!
  * scripts.js
  */ 
var rsexist;
var meterzoneval = 0;

function getHistoryData(dayval,type){
  if (!document.getElementById("datatablesSimple")) return;
  let datatable = new simpleDatatables.DataTable("#datatablesSimple");
  var days;
  if (!dayval){days =document.getElementById('metricDays').value;}
  else {days = dayval;}
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
  if (this.readyState == 4 && this.status == 200) {
    var rawData = JSON.parse(this.responseText);
    var items = [];
    if (rawData) {
        Object.keys(rawData).forEach(function(key) {
            var item = rawData[key];
            items.push(item);
        });
    }
    items.reverse();
    var data = { items: items };
    if (data.items.length > 0){  
      if (type == 'G'){return data.items;}
      var zonedata = JSON.parse(sessionStorage.getItem("zonedata"));   
      let newerData =  {};
      newerData.data = [];
      for (let i = 0; i < data.items.length; ++i) {
        var name = "Unknown";
        if (zonedata) {
            for (let j = 0; j < zonedata.length; ++j) {        
                if (zonedata[j].port == data.items[i].port){
                name = zonedata[j].name;
                break;
                }
            }
        }
        newerData.data[i] = [];
        var date = new Date((data.items[i].key * 1000));
        var time = convertMinsToHrsMins(data.items[i].duration);
        newerData.data[i][0] = date.toDateString();
        newerData.data[i][1] = date.toLocaleTimeString();
        newerData.data[i][2] = name;
        newerData.data[i][3] = data.items[i].port;
        newerData.data[i][4] = time;
        newerData.data[i][5] = data.items[i].trigger ? "Manual" : "Auto";
        if (data.items[i].note) {
          newerData.data[i][5] = data.items[i].note;
        }
        newerData.data[i][6] = data.items[i].consumption;
      }
      datatable.insert(newerData);
    } 
    else {
    }
   }
  };  
  xhr.open("GET", "getHistory?days="+days, true);
  xhr.send();  
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
      // On successful cancellation, toggle the manual run button back
      toggleManualRunButton(false);
    }
    if (this.readyState == 4 && this.status == 200) {
      var data = JSON.parse(this.responseText);
    }
  };  
  xhr.open("POST", "cancelRun", true);
  xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  xhr.send(); 
}

function confirmDeleteHistory() {
  var myModal = new bootstrap.Modal(document.getElementById('deleteModal'));
  myModal.show();
}

function deleteHistory() {
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      location.reload();
    }
  };
  xhr.open("POST", "deleteHistory", true);
  xhr.send();
}

function exportToCSV() {
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var rawData = JSON.parse(this.responseText);
      var items = [];
      if (rawData) {
          Object.keys(rawData).forEach(function(key) {
              var item = rawData[key];
              items.push(item);
          });
      }
      items.reverse();
      
      var csv = "Date,Time,Name,Zone,Duration,Auto,Consumption\n";
      var zonedata = JSON.parse(sessionStorage.getItem("zonedata"));
      
      items.forEach(function(row) {
          var name = "Unknown";
          if (zonedata) {
            for (let j = 0; j < zonedata.length; ++j) {        
                if (zonedata[j].port == row.port){
                name = zonedata[j].name;
                break;
                }
            }
          }
          var date = new Date((row.key * 1000));
          var time = convertMinsToHrsMins(row.duration);
          var trigger = row.trigger ? "Manual" : "Auto";
          
          if (name.indexOf(',') > -1) name = '"' + name + '"';

          csv += date.toDateString() + ",";
          csv += date.toLocaleTimeString() + ",";
          csv += name + ",";
          csv += row.port + ",";
          csv += time + ",";
          csv += trigger + ",";
          csv += row.consumption + "\n";
      });
  
      var blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
      var link = document.createElement("a");
      if (link.download !== undefined) {
          var url = URL.createObjectURL(blob);
          link.setAttribute("href", url);
          link.setAttribute("download", "utilization.csv");
          link.style.visibility = 'hidden';
          document.body.appendChild(link);
          link.click();
          document.body.removeChild(link);
      }
    }
  };
  xhr.open("GET", "getHistory?days=365", true);
  xhr.send();
}

function cookieChk(){
  var testsession = getCookie("SESSIONID");
  if (testsession == 0){
    window.location.href = "login.html";
    console.log('cookieChk failed, redirecting to login.html');
}
}

function getCookie(cname) {
  var name = cname + "=";
  var decodedCookie = decodeURIComponent(document.cookie);
  console.log(decodedCookie);
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

function getLogHistory() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            var lines = this.responseText.split('\n').filter(Boolean); // Split by newline and remove empty lines
            msgArray = lines.reverse(); // Newest first
            if (msgArray.length > 100) {
                msgArray = msgArray.slice(0, 100);
            }
            sessionStorage.setItem("logdata", JSON.stringify(msgArray));
            if (document.getElementById("eventLog")) {
                document.getElementById("eventLog").value = msgArray.join("\n");
            }
        }
    };
    xhr.open("GET", "/getLogs", true);
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
    // Show the modal
    $('#logoutModal').modal('show');

    // Handle the click on the confirm button, using .one() to prevent multiple bindings
    $('#confirmLogoutBtn').one('click', function() {
        $('#logoutModal').modal('hide');
        const Http = new XMLHttpRequest();
        const url = '/logout';
        Http.open("GET", url);
        Http.send();
        Http.onreadystatechange = (e) => { window.location.href = "login.html"; }
    });
}

function toggleManualRunButton(isRunning) {
    const manualRunButton = document.getElementById("manualrunbutton").querySelector('button');
    if (isRunning) {
        manualRunButton.textContent = "Cancel Manual Run";
        manualRunButton.classList.remove('btn-success');
        manualRunButton.classList.add('btn-warning');
        manualRunButton.onclick = function() { cancelRun(); };
    } else {
        manualRunButton.textContent = "Begin Run";
        manualRunButton.classList.remove('btn-warning');
        manualRunButton.classList.add('btn-success');
        manualRunButton.onclick = function() { submitManualRun(); };
    }
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

function waitForReboot(url){
  setTimeout(function(){
    var interval = setInterval(function(){
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
           clearInterval(interval);
           window.location.href = url;
        }
      };
      xhr.open("GET", url + "?t=" + new Date().getTime(), true);
      xhr.timeout = 1000;
      xhr.send();
    }, 3000);
  }, 5000);
}

function printProgrammes(){
  var content ='';
  for (let i = 0; i < 3; i++) {
    var letter = 'A';
    if(i==1){letter='B';}
    if(i==2){letter='C';}
    var subContent = `
    <div class="card mb-4">
        <div class="card-header">
            <i class="fas fa-clock me-1"></i> Programme ${letter}
        </div>
        <div class="card-body">
            <div class="row">
                <div class="col-md-4">
                    <div class="form-floating mb-3">
                        <input type="text" class="form-control" id="st${letter}" name="st${letter}" placeholder="1800">
                        <label for="st${letter}">Start Time (HHMM)</label>
                    </div>
                </div>
                <div class="col-md-8">
                    <label class="mb-2">Active Days</label><br>
                    <div class="btn-group" role="group">
                        <input type="checkbox" class="btn-check" id="${letter}-1" autocomplete="off">
                        <label class="btn btn-outline-primary" for="${letter}-1">Mon</label>
                        <input type="checkbox" class="btn-check" id="${letter}-2" autocomplete="off">
                        <label class="btn btn-outline-primary" for="${letter}-2">Tue</label>
                        <input type="checkbox" class="btn-check" id="${letter}-3" autocomplete="off">
                        <label class="btn btn-outline-primary" for="${letter}-3">Wed</label>
                        <input type="checkbox" class="btn-check" id="${letter}-4" autocomplete="off">
                        <label class="btn btn-outline-primary" for="${letter}-4">Thu</label>
                        <input type="checkbox" class="btn-check" id="${letter}-5" autocomplete="off">
                        <label class="btn btn-outline-primary" for="${letter}-5">Fri</label>
                        <input type="checkbox" class="btn-check" id="${letter}-6" autocomplete="off">
                        <label class="btn btn-outline-primary" for="${letter}-6">Sat</label>
                        <input type="checkbox" class="btn-check" id="${letter}-7" autocomplete="off">
                        <label class="btn btn-outline-primary" for="${letter}-7">Sun</label>
                    </div>
                </div>
            </div>
            <hr>
            <h6 class="card-subtitle mb-2 text-muted">Zone Durations (Minutes)</h6>
            <div class="table-responsive">
                <table class="table table-striped table-hover order-list${letter}">
                    <thead>
                        <tr>
                            <th>Zone Name</th>
                            <th style="width: 150px;">Duration</th>
                        </tr>
                    </thead>
                    <tbody>
                    </tbody>
                </table>
            </div>
        </div>
    </div>`;
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

function owaChange(){
  var sel = document.getElementById("owaselector");
  var val = sel.options[sel.selectedIndex].value;
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var data = JSON.parse(this.responseText);
      if (data.s != "0") alert("Update error, try again...");
    }
  };  
  xhr.open("POST", "updateConfig", true);
  xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  xhr.send("event=owachange&value="+val); 
}

function submitManualRun() {
  var dropdown = document.getElementById('manualrunlist');
  var value = dropdown.options[dropdown.selectedIndex].value;
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var data = JSON.parse(this.responseText);
      toggleManualRunButton(true); // Toggle button to "Cancel"
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
    var celement = document.getElementById("cancelbutton");
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
      if (document.getElementById("owaselector")) {
        var owaSel = document.getElementById("owaselector");
        var owaLbl = document.getElementById("owalabel");
        var owaContainer = document.getElementById("owa_container");
        if (data.owa_setup) {
            if (owaContainer) owaContainer.style.display = "";
            if (owaLbl) owaLbl.style.color = "green";
            owaSel.disabled = false;
            owaSel.value = data.use_owa ? "1" : "0";
        } else {
            if (owaContainer) owaContainer.style.display = "none";
            if (owaLbl) owaLbl.style.color = "red";
            owaSel.disabled = true;
            owaSel.value = "0";
        }
      }
      if (data.w_desc) {
        document.getElementById("weatherTemp").innerHTML = data.w_temp.toFixed(1) + "&deg;C";
        document.getElementById("weatherDesc").innerHTML = data.w_desc;
        document.getElementById("weatherPop").innerHTML = "Rain Chance (24h): " + data.w_pop.toFixed(0) + "%";
        if (data.w_icon) {
            var iconImg = document.getElementById("weatherIcon");
            iconImg.src = "http://openweathermap.org/img/wn/" + data.w_icon + "@2x.png";
            iconImg.style.display = "inline";
        }
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
      if (document.getElementById("manualRunCard")) {
        if (data.state == "0") {
            document.getElementById("manualRunCard").style.display = "none";
        } else {
            document.getElementById("manualRunCard").style.display = "block";
        }
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
          if (data.info == 1) {
            stateinfo.innerHTML= data.info+" minute";
          } else {
            stateinfo.innerHTML= data.info+" minutes";
          }
        }
      }
      else if(data.state == "4"){
        celement.style.display = "none";
        state.innerHTML="Todays schedule has completed.....";
        stateinfo.innerHTML= "";
      }
      else if(data.state == "5"){
        celement.style.display = "block";
        if (data.info > 0) { // Check if a manual run is active
            toggleManualRunButton(true);
        }
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
        celement.style.display = "block";
        state.innerHTML="<span style='color: blue; font-weight: bold;'>RAIN DELAY</span>";
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
    if (dropdown) {
      dropdown.length = 0;
      defaultOption = document.createElement('option');
      defaultOption.text = "Refreshing Configured Zones";
      dropdown.add(defaultOption);
    }
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {    
        var data = JSON.parse(this.responseText);
        sessionStorage.setItem("zonedata", JSON.stringify(data)); 
        if (dropdown) {
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
<a class="nav-link" href="consumption.html">
    <div class="sb-nav-link-icon"><i class="fas fa-tint"></i></div>
    Consumption
</a>
<a class="nav-link" href="utilization.html">
    <div class="sb-nav-link-icon"><i class="fas fa-table"></i></div>
    Utilization
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
    if (document.getElementById("datatablesSimple")) {
      console.log("reloading stats...");
      location.reload();
    }
   };
}, false);

source.addEventListener('metrics', function(e) {
  console.log(e);
  metrics = e;
}, false);

source.addEventListener('message', function(e) {
  if (sessionStorage.getItem("logdata")){
    try {
      msgArray = JSON.parse(sessionStorage.getItem("logdata"));
    } catch (err) {
      msgArray = [];
    }
  }
  console.log(e);
  var thisid = e.lastEventId;
  console.log("TID "+thisid);
  if (thisid == lastid){  console.log("repeat");  return 0;}
  lastid = thisid;
  if (e.data != 'INIT'){
    if (msgArray.length > 0){msgArray.unshift(e.data);}
    else {msgArray.push(e.data);}
    if (msgArray.length > 100){
      msgArray.pop();
    }
    sessionStorage.setItem("logdata", JSON.stringify(msgArray));
    console.log(msgArray);
  }

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