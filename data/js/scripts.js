/*!
    * Start Bootstrap - SB Admin v7.0.0 (https://startbootstrap.com/template/sb-admin)
    * Copyright 2013-2021 Start Bootstrap
    * Licensed under MIT (https://github.com/StartBootstrap/startbootstrap-sb-admin/blob/master/LICENSE)
    */
    // 
// Scripts

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

/// updateZonelist()  For the manual run pf programmes  
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
console.log(data);
      dropdown.length = 0;
      defaultOption = document.createElement('option');
      defaultOption.text = 'Choose a zone.....';
      defaultOption.value = '';
      dropdown.add(defaultOption);
      dropdown.selectedIndex = 0;
console.log(data.length)   
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

function updateStatus(){
    var state = document.getElementById("state");
    var stateinfo = document.getElementById("stateInfo");
        console.log('update Status....');
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          var data = JSON.parse(this.responseText);
          console.log(data);
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
              state.innerHTML="Programme is disabled.....";
              stateinfo.innerHTML= "";
            }
            else if(data.state == "1"){
              state.innerHTML="Sprinkler is Idle Today.....";
              stateinfo.innerHTML= "";
            }
            else if(data.state == "2"){
              state.innerHTML="Manual Cancellation Requested.....";
              stateinfo.innerHTML= "";
            }
            else if(data.state == "3"){
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
              state.innerHTML="Todays schedule has completed.....";
              stateinfo.innerHTML= "";
            }
            else if(data.state == "5"){
              var minutes = data.info;
              var zone = data.zone;
              if (minutes >1){
                state.innerHTML="Zone "+zone+" - "+minutes+" minutes remain";
              }
              else {
                state.innerHTML="Zone "+zone+" - "+minutes+" minute remains";
              }
                stateinfo.innerHTML= "Consumption 0.00g/min";
              //line948 // line 420
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

function cookieChk(){
    var testsession = getCookie("SESSIONID");
    if (testsession == 0){
        console.log("redirect it");
    }
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


function submitManualRun() {
  var dropdown = document.getElementById('manualrunlist');
  var value = dropdown.options[dropdown.selectedIndex].value;
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
  if (this.readyState == 4 && this.status == 200) {
      var data = JSON.parse(this.responseText);
      console.log(data);
  }
};  
xhr.open("POST", "sendManual", true);
xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
xhr.send("zone="+value); 
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

function addSidebar() { 
const content = `
<div class="nav">
<!--
<div class="sb-sidenav-menu-heading">Core</div>
-->

<a class="nav-link" href="index.html">
    <div class="sb-nav-link-icon"><i class="fas fa-tachometer-alt"></i></div>
    Dashboard
</a>
<!--
<div class="sb-sidenav-menu-heading">Interface</div>

 

<a class="nav-link collapsed" href="#" data-bs-toggle="collapse" data-bs-target="#collapseLayouts" aria-expanded="false" aria-controls="collapseLayouts">
    <div class="sb-nav-link-icon"><i class="fas fa-columns"></i></div>
    Layouts
    <div class="sb-sidenav-collapse-arrow"><i class="fas fa-angle-down"></i></div>
</a>



<div class="collapse" id="collapseLayouts" aria-labelledby="headingOne" data-bs-parent="#sidenavAccordion">
    <nav class="sb-sidenav-menu-nested nav">
        <a class="nav-link" href="layout-static.html">Static Navigation</a>
        <a class="nav-link" href="layout-sidenav-light.html">Light Sidenav</a>
    </nav>
</div>



<a class="nav-link collapsed" href="#" data-bs-toggle="collapse" data-bs-target="#collapsePages" aria-expanded="false" aria-controls="collapsePages">
    <div class="sb-nav-link-icon"><i class="fas fa-book-open"></i></div>
    Pages
    <div class="sb-sidenav-collapse-arrow"><i class="fas fa-angle-down"></i></div>
</a>



<div class="collapse" id="collapsePages" aria-labelledby="headingTwo" data-bs-parent="#sidenavAccordion">
    <nav class="sb-sidenav-menu-nested nav accordion" id="sidenavAccordionPages">
        <a class="nav-link collapsed" href="#" data-bs-toggle="collapse" data-bs-target="#pagesCollapseAuth" aria-expanded="false" aria-controls="pagesCollapseAuth">
            Authentication
            <div class="sb-sidenav-collapse-arrow"><i class="fas fa-angle-down"></i></div>
        </a>
        <div class="collapse" id="pagesCollapseAuth" aria-labelledby="headingOne" data-bs-parent="#sidenavAccordionPages">
            <nav class="sb-sidenav-menu-nested nav">
                <a class="nav-link" href="login.html">Login</a>
                <a class="nav-link" href="register.html">Register</a>
                <a class="nav-link" href="password.html">Forgot Password</a>
            </nav>
        </div>
        <a class="nav-link collapsed" href="#" data-bs-toggle="collapse" data-bs-target="#pagesCollapseError" aria-expanded="false" aria-controls="pagesCollapseError">
            Error
            <div class="sb-sidenav-collapse-arrow"><i class="fas fa-angle-down"></i></div>
        </a>
        <div class="collapse" id="pagesCollapseError" aria-labelledby="headingOne" data-bs-parent="#sidenavAccordionPages">
            <nav class="sb-sidenav-menu-nested nav">
                <a class="nav-link" href="401.html">401 Page</a>
                <a class="nav-link" href="404.html">404 Page</a>
                <a class="nav-link" href="500.html">500 Page</a>
            </nav>
        </div>
    </nav>
</div>



<div class="sb-sidenav-menu-heading">Addons</div>
<a class="nav-link" href="charts.html">
    <div class="sb-nav-link-icon"><i class="fas fa-chart-area"></i></div>
    Charts
</a>
<a class="nav-link" href="tables.html">
    <div class="sb-nav-link-icon"><i class="fas fa-table"></i></div>
    Tables
</a>
</div>
</div>

-->

<div class="sb-sidenav-footer">
<div class="small" id="username"></div>
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

var TZdata = [
    {
      "FIELD1": "Africa/Abidjan",
      "FIELD2": "GMT0"
    },
    {
      "FIELD1": "Africa/Accra",
      "FIELD2": "GMT0"
    },
    {
      "FIELD1": "Africa/Addis_Ababa",
      "FIELD2": "EAT-3"
    },
    {
      "FIELD1": "Africa/Algiers",
      "FIELD2": "CET-1"
    },
    {
      "FIELD1": "Africa/Asmara",
      "FIELD2": "EAT-3"
    },
    {
      "FIELD1": "Africa/Bamako",
      "FIELD2": "GMT0"
    },
    {
      "FIELD1": "Africa/Bangui",
      "FIELD2": "WAT-1"
    },
    {
      "FIELD1": "Africa/Banjul",
      "FIELD2": "GMT0"
    },
    {
      "FIELD1": "Africa/Bissau",
      "FIELD2": "GMT0"
    },
    {
      "FIELD1": "Africa/Blantyre",
      "FIELD2": "CAT-2"
    },
    {
      "FIELD1": "Africa/Brazzaville",
      "FIELD2": "WAT-1"
    },
    {
      "FIELD1": "Africa/Bujumbura",
      "FIELD2": "CAT-2"
    },
    {
      "FIELD1": "Africa/Cairo",
      "FIELD2": "EET-2"
    },
    {
      "FIELD1": "Africa/Casablanca",
      "FIELD2": "<+01>-1"
    },
    {
      "FIELD1": "Africa/Ceuta",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Africa/Conakry",
      "FIELD2": "GMT0"
    },
    {
      "FIELD1": "Africa/Dakar",
      "FIELD2": "GMT0"
    },
    {
      "FIELD1": "Africa/Dar_es_Salaam",
      "FIELD2": "EAT-3"
    },
    {
      "FIELD1": "Africa/Djibouti",
      "FIELD2": "EAT-3"
    },
    {
      "FIELD1": "Africa/Douala",
      "FIELD2": "WAT-1"
    },
    {
      "FIELD1": "Africa/El_Aaiun",
      "FIELD2": "<+01>-1"
    },
    {
      "FIELD1": "Africa/Freetown",
      "FIELD2": "GMT0"
    },
    {
      "FIELD1": "Africa/Gaborone",
      "FIELD2": "CAT-2"
    },
    {
      "FIELD1": "Africa/Harare",
      "FIELD2": "CAT-2"
    },
    {
      "FIELD1": "Africa/Johannesburg",
      "FIELD2": "SAST-2"
    },
    {
      "FIELD1": "Africa/Juba",
      "FIELD2": "CAT-2"
    },
    {
      "FIELD1": "Africa/Kampala",
      "FIELD2": "EAT-3"
    },
    {
      "FIELD1": "Africa/Khartoum",
      "FIELD2": "CAT-2"
    },
    {
      "FIELD1": "Africa/Kigali",
      "FIELD2": "CAT-2"
    },
    {
      "FIELD1": "Africa/Kinshasa",
      "FIELD2": "WAT-1"
    },
    {
      "FIELD1": "Africa/Lagos",
      "FIELD2": "WAT-1"
    },
    {
      "FIELD1": "Africa/Libreville",
      "FIELD2": "WAT-1"
    },
    {
      "FIELD1": "Africa/Lome",
      "FIELD2": "GMT0"
    },
    {
      "FIELD1": "Africa/Luanda",
      "FIELD2": "WAT-1"
    },
    {
      "FIELD1": "Africa/Lubumbashi",
      "FIELD2": "CAT-2"
    },
    {
      "FIELD1": "Africa/Lusaka",
      "FIELD2": "CAT-2"
    },
    {
      "FIELD1": "Africa/Malabo",
      "FIELD2": "WAT-1"
    },
    {
      "FIELD1": "Africa/Maputo",
      "FIELD2": "CAT-2"
    },
    {
      "FIELD1": "Africa/Maseru",
      "FIELD2": "SAST-2"
    },
    {
      "FIELD1": "Africa/Mbabane",
      "FIELD2": "SAST-2"
    },
    {
      "FIELD1": "Africa/Mogadishu",
      "FIELD2": "EAT-3"
    },
    {
      "FIELD1": "Africa/Monrovia",
      "FIELD2": "GMT0"
    },
    {
      "FIELD1": "Africa/Nairobi",
      "FIELD2": "EAT-3"
    },
    {
      "FIELD1": "Africa/Ndjamena",
      "FIELD2": "WAT-1"
    },
    {
      "FIELD1": "Africa/Niamey",
      "FIELD2": "WAT-1"
    },
    {
      "FIELD1": "Africa/Nouakchott",
      "FIELD2": "GMT0"
    },
    {
      "FIELD1": "Africa/Ouagadougou",
      "FIELD2": "GMT0"
    },
    {
      "FIELD1": "Africa/Porto-Novo",
      "FIELD2": "WAT-1"
    },
    {
      "FIELD1": "Africa/Sao_Tome",
      "FIELD2": "GMT0"
    },
    {
      "FIELD1": "Africa/Tripoli",
      "FIELD2": "EET-2"
    },
    {
      "FIELD1": "Africa/Tunis",
      "FIELD2": "CET-1"
    },
    {
      "FIELD1": "Africa/Windhoek",
      "FIELD2": "CAT-2"
    },
    {
      "FIELD1": "America/Adak",
      "FIELD2": "HST10HDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Anchorage",
      "FIELD2": "AKST9AKDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Anguilla",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/Antigua",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/Araguaina",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Argentina/Buenos_Aires",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Argentina/Catamarca",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Argentina/Cordoba",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Argentina/Jujuy",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Argentina/La_Rioja",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Argentina/Mendoza",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Argentina/Rio_Gallegos",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Argentina/Salta",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Argentina/San_Juan",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Argentina/San_Luis",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Argentina/Tucuman",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Argentina/Ushuaia",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Aruba",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/Asuncion",
      "FIELD2": "<-04>4<-03>,M10.1.0/0,M3.4.0/0"
    },
    {
      "FIELD1": "America/Atikokan",
      "FIELD2": "EST5"
    },
    {
      "FIELD1": "America/Bahia",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Bahia_Banderas",
      "FIELD2": "CST6CDT,M4.1.0,M10.5.0"
    },
    {
      "FIELD1": "America/Barbados",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/Belem",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Belize",
      "FIELD2": "CST6"
    },
    {
      "FIELD1": "America/Blanc-Sablon",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/Boa_Vista",
      "FIELD2": "<-04>4"
    },
    {
      "FIELD1": "America/Bogota",
      "FIELD2": "<-05>5"
    },
    {
      "FIELD1": "America/Boise",
      "FIELD2": "MST7MDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Cambridge_Bay",
      "FIELD2": "MST7MDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Campo_Grande",
      "FIELD2": "<-04>4"
    },
    {
      "FIELD1": "America/Cancun",
      "FIELD2": "EST5"
    },
    {
      "FIELD1": "America/Caracas",
      "FIELD2": "<-04>4"
    },
    {
      "FIELD1": "America/Cayenne",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Cayman",
      "FIELD2": "EST5"
    },
    {
      "FIELD1": "America/Chicago",
      "FIELD2": "CST6CDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Chihuahua",
      "FIELD2": "MST7MDT,M4.1.0,M10.5.0"
    },
    {
      "FIELD1": "America/Costa_Rica",
      "FIELD2": "CST6"
    },
    {
      "FIELD1": "America/Creston",
      "FIELD2": "MST7"
    },
    {
      "FIELD1": "America/Cuiaba",
      "FIELD2": "<-04>4"
    },
    {
      "FIELD1": "America/Curacao",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/Danmarkshavn",
      "FIELD2": "GMT0"
    },
    {
      "FIELD1": "America/Dawson",
      "FIELD2": "MST7"
    },
    {
      "FIELD1": "America/Dawson_Creek",
      "FIELD2": "MST7"
    },
    {
      "FIELD1": "America/Denver",
      "FIELD2": "MST7MDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Detroit",
      "FIELD2": "EST5EDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Dominica",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/Edmonton",
      "FIELD2": "MST7MDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Eirunepe",
      "FIELD2": "<-05>5"
    },
    {
      "FIELD1": "America/El_Salvador",
      "FIELD2": "CST6"
    },
    {
      "FIELD1": "America/Fortaleza",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Fort_Nelson",
      "FIELD2": "MST7"
    },
    {
      "FIELD1": "America/Glace_Bay",
      "FIELD2": "AST4ADT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Godthab",
      "FIELD2": "<-03>3<-02>,M3.5.0/-2,M10.5.0/-1"
    },
    {
      "FIELD1": "America/Goose_Bay",
      "FIELD2": "AST4ADT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Grand_Turk",
      "FIELD2": "EST5EDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Grenada",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/Guadeloupe",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/Guatemala",
      "FIELD2": "CST6"
    },
    {
      "FIELD1": "America/Guayaquil",
      "FIELD2": "<-05>5"
    },
    {
      "FIELD1": "America/Guyana",
      "FIELD2": "<-04>4"
    },
    {
      "FIELD1": "America/Halifax",
      "FIELD2": "AST4ADT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Havana",
      "FIELD2": "CST5CDT,M3.2.0/0,M11.1.0/1"
    },
    {
      "FIELD1": "America/Hermosillo",
      "FIELD2": "MST7"
    },
    {
      "FIELD1": "America/Indiana/Indianapolis",
      "FIELD2": "EST5EDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Indiana/Knox",
      "FIELD2": "CST6CDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Indiana/Marengo",
      "FIELD2": "EST5EDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Indiana/Petersburg",
      "FIELD2": "EST5EDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Indiana/Tell_City",
      "FIELD2": "CST6CDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Indiana/Vevay",
      "FIELD2": "EST5EDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Indiana/Vincennes",
      "FIELD2": "EST5EDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Indiana/Winamac",
      "FIELD2": "EST5EDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Inuvik",
      "FIELD2": "MST7MDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Iqaluit",
      "FIELD2": "EST5EDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Jamaica",
      "FIELD2": "EST5"
    },
    {
      "FIELD1": "America/Juneau",
      "FIELD2": "AKST9AKDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Kentucky/Louisville",
      "FIELD2": "EST5EDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Kentucky/Monticello",
      "FIELD2": "EST5EDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Kralendijk",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/La_Paz",
      "FIELD2": "<-04>4"
    },
    {
      "FIELD1": "America/Lima",
      "FIELD2": "<-05>5"
    },
    {
      "FIELD1": "America/Los_Angeles",
      "FIELD2": "PST8PDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Lower_Princes",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/Maceio",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Managua",
      "FIELD2": "CST6"
    },
    {
      "FIELD1": "America/Manaus",
      "FIELD2": "<-04>4"
    },
    {
      "FIELD1": "America/Marigot",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/Martinique",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/Matamoros",
      "FIELD2": "CST6CDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Mazatlan",
      "FIELD2": "MST7MDT,M4.1.0,M10.5.0"
    },
    {
      "FIELD1": "America/Menominee",
      "FIELD2": "CST6CDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Merida",
      "FIELD2": "CST6CDT,M4.1.0,M10.5.0"
    },
    {
      "FIELD1": "America/Metlakatla",
      "FIELD2": "AKST9AKDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Mexico_City",
      "FIELD2": "CST6CDT,M4.1.0,M10.5.0"
    },
    {
      "FIELD1": "America/Miquelon",
      "FIELD2": "<-03>3<-02>,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Moncton",
      "FIELD2": "AST4ADT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Monterrey",
      "FIELD2": "CST6CDT,M4.1.0,M10.5.0"
    },
    {
      "FIELD1": "America/Montevideo",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Montreal",
      "FIELD2": "EST5EDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Montserrat",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/Nassau",
      "FIELD2": "EST5EDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/New_York",
      "FIELD2": "EST5EDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Nipigon",
      "FIELD2": "EST5EDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Nome",
      "FIELD2": "AKST9AKDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Noronha",
      "FIELD2": "<-02>2"
    },
    {
      "FIELD1": "America/North_Dakota/Beulah",
      "FIELD2": "CST6CDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/North_Dakota/Center",
      "FIELD2": "CST6CDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/North_Dakota/New_Salem",
      "FIELD2": "CST6CDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Nuuk",
      "FIELD2": "<-03>3<-02>,M3.5.0/-2,M10.5.0/-1"
    },
    {
      "FIELD1": "America/Ojinaga",
      "FIELD2": "MST7MDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Panama",
      "FIELD2": "EST5"
    },
    {
      "FIELD1": "America/Pangnirtung",
      "FIELD2": "EST5EDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Paramaribo",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Phoenix",
      "FIELD2": "MST7"
    },
    {
      "FIELD1": "America/Port-au-Prince",
      "FIELD2": "EST5EDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Port_of_Spain",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/Porto_Velho",
      "FIELD2": "<-04>4"
    },
    {
      "FIELD1": "America/Puerto_Rico",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/Punta_Arenas",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Rainy_River",
      "FIELD2": "CST6CDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Rankin_Inlet",
      "FIELD2": "CST6CDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Recife",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Regina",
      "FIELD2": "CST6"
    },
    {
      "FIELD1": "America/Resolute",
      "FIELD2": "CST6CDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Rio_Branco",
      "FIELD2": "<-05>5"
    },
    {
      "FIELD1": "America/Santarem",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Santiago",
      "FIELD2": "<-04>4<-03>,M9.1.6/24,M4.1.6/24"
    },
    {
      "FIELD1": "America/Santo_Domingo",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/Sao_Paulo",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "America/Scoresbysund",
      "FIELD2": "<-01>1<+00>,M3.5.0/0,M10.5.0/1"
    },
    {
      "FIELD1": "America/Sitka",
      "FIELD2": "AKST9AKDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/St_Barthelemy",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/St_Johns",
      "FIELD2": "NST3:30NDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/St_Kitts",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/St_Lucia",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/St_Thomas",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/St_Vincent",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/Swift_Current",
      "FIELD2": "CST6"
    },
    {
      "FIELD1": "America/Tegucigalpa",
      "FIELD2": "CST6"
    },
    {
      "FIELD1": "America/Thule",
      "FIELD2": "AST4ADT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Thunder_Bay",
      "FIELD2": "EST5EDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Tijuana",
      "FIELD2": "PST8PDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Toronto",
      "FIELD2": "EST5EDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Tortola",
      "FIELD2": "AST4"
    },
    {
      "FIELD1": "America/Vancouver",
      "FIELD2": "PST8PDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Whitehorse",
      "FIELD2": "MST7"
    },
    {
      "FIELD1": "America/Winnipeg",
      "FIELD2": "CST6CDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Yakutat",
      "FIELD2": "AKST9AKDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "America/Yellowknife",
      "FIELD2": "MST7MDT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "Antarctica/Casey",
      "FIELD2": "<+11>-11"
    },
    {
      "FIELD1": "Antarctica/Davis",
      "FIELD2": "<+07>-7"
    },
    {
      "FIELD1": "Antarctica/DumontDUrville",
      "FIELD2": "<+10>-10"
    },
    {
      "FIELD1": "Antarctica/Macquarie",
      "FIELD2": "AEST-10AEDT,M10.1.0,M4.1.0/3"
    },
    {
      "FIELD1": "Antarctica/Mawson",
      "FIELD2": "<+05>-5"
    },
    {
      "FIELD1": "Antarctica/McMurdo",
      "FIELD2": "NZST-12NZDT,M9.5.0,M4.1.0/3"
    },
    {
      "FIELD1": "Antarctica/Palmer",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "Antarctica/Rothera",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "Antarctica/Syowa",
      "FIELD2": "<+03>-3"
    },
    {
      "FIELD1": "Antarctica/Troll",
      "FIELD2": "<+00>0<+02>-2,M3.5.0/1,M10.5.0/3"
    },
    {
      "FIELD1": "Antarctica/Vostok",
      "FIELD2": "<+06>-6"
    },
    {
      "FIELD1": "Arctic/Longyearbyen",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Asia/Aden",
      "FIELD2": "<+03>-3"
    },
    {
      "FIELD1": "Asia/Almaty",
      "FIELD2": "<+06>-6"
    },
    {
      "FIELD1": "Asia/Amman",
      "FIELD2": "EET-2EEST,M2.5.4/24,M10.5.5/1"
    },
    {
      "FIELD1": "Asia/Anadyr",
      "FIELD2": "<+12>-12"
    },
    {
      "FIELD1": "Asia/Aqtau",
      "FIELD2": "<+05>-5"
    },
    {
      "FIELD1": "Asia/Aqtobe",
      "FIELD2": "<+05>-5"
    },
    {
      "FIELD1": "Asia/Ashgabat",
      "FIELD2": "<+05>-5"
    },
    {
      "FIELD1": "Asia/Atyrau",
      "FIELD2": "<+05>-5"
    },
    {
      "FIELD1": "Asia/Baghdad",
      "FIELD2": "<+03>-3"
    },
    {
      "FIELD1": "Asia/Bahrain",
      "FIELD2": "<+03>-3"
    },
    {
      "FIELD1": "Asia/Baku",
      "FIELD2": "<+04>-4"
    },
    {
      "FIELD1": "Asia/Bangkok",
      "FIELD2": "<+07>-7"
    },
    {
      "FIELD1": "Asia/Barnaul",
      "FIELD2": "<+07>-7"
    },
    {
      "FIELD1": "Asia/Beirut",
      "FIELD2": "EET-2EEST,M3.5.0/0,M10.5.0/0"
    },
    {
      "FIELD1": "Asia/Bishkek",
      "FIELD2": "<+06>-6"
    },
    {
      "FIELD1": "Asia/Brunei",
      "FIELD2": "<+08>-8"
    },
    {
      "FIELD1": "Asia/Chita",
      "FIELD2": "<+09>-9"
    },
    {
      "FIELD1": "Asia/Choibalsan",
      "FIELD2": "<+08>-8"
    },
    {
      "FIELD1": "Asia/Colombo",
      "FIELD2": "<+0530>-5:30"
    },
    {
      "FIELD1": "Asia/Damascus",
      "FIELD2": "EET-2EEST,M3.5.5/0,M10.5.5/0"
    },
    {
      "FIELD1": "Asia/Dhaka",
      "FIELD2": "<+06>-6"
    },
    {
      "FIELD1": "Asia/Dili",
      "FIELD2": "<+09>-9"
    },
    {
      "FIELD1": "Asia/Dubai",
      "FIELD2": "<+04>-4"
    },
    {
      "FIELD1": "Asia/Dushanbe",
      "FIELD2": "<+05>-5"
    },
    {
      "FIELD1": "Asia/Famagusta",
      "FIELD2": "EET-2EEST,M3.5.0/3,M10.5.0/4"
    },
    {
      "FIELD1": "Asia/Gaza",
      "FIELD2": "EET-2EEST,M3.4.4/48,M10.5.5/1"
    },
    {
      "FIELD1": "Asia/Hebron",
      "FIELD2": "EET-2EEST,M3.4.4/48,M10.5.5/1"
    },
    {
      "FIELD1": "Asia/Ho_Chi_Minh",
      "FIELD2": "<+07>-7"
    },
    {
      "FIELD1": "Asia/Hong_Kong",
      "FIELD2": "HKT-8"
    },
    {
      "FIELD1": "Asia/Hovd",
      "FIELD2": "<+07>-7"
    },
    {
      "FIELD1": "Asia/Irkutsk",
      "FIELD2": "<+08>-8"
    },
    {
      "FIELD1": "Asia/Jakarta",
      "FIELD2": "WIB-7"
    },
    {
      "FIELD1": "Asia/Jayapura",
      "FIELD2": "WIT-9"
    },
    {
      "FIELD1": "Asia/Jerusalem",
      "FIELD2": "IST-2IDT,M3.4.4/26,M10.5.0"
    },
    {
      "FIELD1": "Asia/Kabul",
      "FIELD2": "<+0430>-4:30"
    },
    {
      "FIELD1": "Asia/Kamchatka",
      "FIELD2": "<+12>-12"
    },
    {
      "FIELD1": "Asia/Karachi",
      "FIELD2": "PKT-5"
    },
    {
      "FIELD1": "Asia/Kathmandu",
      "FIELD2": "<+0545>-5:45"
    },
    {
      "FIELD1": "Asia/Khandyga",
      "FIELD2": "<+09>-9"
    },
    {
      "FIELD1": "Asia/Kolkata",
      "FIELD2": "IST-5:30"
    },
    {
      "FIELD1": "Asia/Krasnoyarsk",
      "FIELD2": "<+07>-7"
    },
    {
      "FIELD1": "Asia/Kuala_Lumpur",
      "FIELD2": "<+08>-8"
    },
    {
      "FIELD1": "Asia/Kuching",
      "FIELD2": "<+08>-8"
    },
    {
      "FIELD1": "Asia/Kuwait",
      "FIELD2": "<+03>-3"
    },
    {
      "FIELD1": "Asia/Macau",
      "FIELD2": "CST-8"
    },
    {
      "FIELD1": "Asia/Magadan",
      "FIELD2": "<+11>-11"
    },
    {
      "FIELD1": "Asia/Makassar",
      "FIELD2": "WITA-8"
    },
    {
      "FIELD1": "Asia/Manila",
      "FIELD2": "PST-8"
    },
    {
      "FIELD1": "Asia/Muscat",
      "FIELD2": "<+04>-4"
    },
    {
      "FIELD1": "Asia/Nicosia",
      "FIELD2": "EET-2EEST,M3.5.0/3,M10.5.0/4"
    },
    {
      "FIELD1": "Asia/Novokuznetsk",
      "FIELD2": "<+07>-7"
    },
    {
      "FIELD1": "Asia/Novosibirsk",
      "FIELD2": "<+07>-7"
    },
    {
      "FIELD1": "Asia/Omsk",
      "FIELD2": "<+06>-6"
    },
    {
      "FIELD1": "Asia/Oral",
      "FIELD2": "<+05>-5"
    },
    {
      "FIELD1": "Asia/Phnom_Penh",
      "FIELD2": "<+07>-7"
    },
    {
      "FIELD1": "Asia/Pontianak",
      "FIELD2": "WIB-7"
    },
    {
      "FIELD1": "Asia/Pyongyang",
      "FIELD2": "KST-9"
    },
    {
      "FIELD1": "Asia/Qatar",
      "FIELD2": "<+03>-3"
    },
    {
      "FIELD1": "Asia/Qyzylorda",
      "FIELD2": "<+05>-5"
    },
    {
      "FIELD1": "Asia/Riyadh",
      "FIELD2": "<+03>-3"
    },
    {
      "FIELD1": "Asia/Sakhalin",
      "FIELD2": "<+11>-11"
    },
    {
      "FIELD1": "Asia/Samarkand",
      "FIELD2": "<+05>-5"
    },
    {
      "FIELD1": "Asia/Seoul",
      "FIELD2": "KST-9"
    },
    {
      "FIELD1": "Asia/Shanghai",
      "FIELD2": "CST-8"
    },
    {
      "FIELD1": "Asia/Singapore",
      "FIELD2": "<+08>-8"
    },
    {
      "FIELD1": "Asia/Srednekolymsk",
      "FIELD2": "<+11>-11"
    },
    {
      "FIELD1": "Asia/Taipei",
      "FIELD2": "CST-8"
    },
    {
      "FIELD1": "Asia/Tashkent",
      "FIELD2": "<+05>-5"
    },
    {
      "FIELD1": "Asia/Tbilisi",
      "FIELD2": "<+04>-4"
    },
    {
      "FIELD1": "Asia/Tehran",
      "FIELD2": "<+0330>-3:30<+0430>,J79/24,J263/24"
    },
    {
      "FIELD1": "Asia/Thimphu",
      "FIELD2": "<+06>-6"
    },
    {
      "FIELD1": "Asia/Tokyo",
      "FIELD2": "JST-9"
    },
    {
      "FIELD1": "Asia/Tomsk",
      "FIELD2": "<+07>-7"
    },
    {
      "FIELD1": "Asia/Ulaanbaatar",
      "FIELD2": "<+08>-8"
    },
    {
      "FIELD1": "Asia/Urumqi",
      "FIELD2": "<+06>-6"
    },
    {
      "FIELD1": "Asia/Ust-Nera",
      "FIELD2": "<+10>-10"
    },
    {
      "FIELD1": "Asia/Vientiane",
      "FIELD2": "<+07>-7"
    },
    {
      "FIELD1": "Asia/Vladivostok",
      "FIELD2": "<+10>-10"
    },
    {
      "FIELD1": "Asia/Yakutsk",
      "FIELD2": "<+09>-9"
    },
    {
      "FIELD1": "Asia/Yangon",
      "FIELD2": "<+0630>-6:30"
    },
    {
      "FIELD1": "Asia/Yekaterinburg",
      "FIELD2": "<+05>-5"
    },
    {
      "FIELD1": "Asia/Yerevan",
      "FIELD2": "<+04>-4"
    },
    {
      "FIELD1": "Atlantic/Azores",
      "FIELD2": "<-01>1<+00>,M3.5.0/0,M10.5.0/1"
    },
    {
      "FIELD1": "Atlantic/Bermuda",
      "FIELD2": "AST4ADT,M3.2.0,M11.1.0"
    },
    {
      "FIELD1": "Atlantic/Canary",
      "FIELD2": "WET0WEST,M3.5.0/1,M10.5.0"
    },
    {
      "FIELD1": "Atlantic/Cape_Verde",
      "FIELD2": "<-01>1"
    },
    {
      "FIELD1": "Atlantic/Faroe",
      "FIELD2": "WET0WEST,M3.5.0/1,M10.5.0"
    },
    {
      "FIELD1": "Atlantic/Madeira",
      "FIELD2": "WET0WEST,M3.5.0/1,M10.5.0"
    },
    {
      "FIELD1": "Atlantic/Reykjavik",
      "FIELD2": "GMT0"
    },
    {
      "FIELD1": "Atlantic/South_Georgia",
      "FIELD2": "<-02>2"
    },
    {
      "FIELD1": "Atlantic/Stanley",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "Atlantic/St_Helena",
      "FIELD2": "GMT0"
    },
    {
      "FIELD1": "Australia/Adelaide",
      "FIELD2": "ACST-9:30ACDT,M10.1.0,M4.1.0/3"
    },
    {
      "FIELD1": "Australia/Brisbane",
      "FIELD2": "AEST-10"
    },
    {
      "FIELD1": "Australia/Broken_Hill",
      "FIELD2": "ACST-9:30ACDT,M10.1.0,M4.1.0/3"
    },
    {
      "FIELD1": "Australia/Currie",
      "FIELD2": "AEST-10AEDT,M10.1.0,M4.1.0/3"
    },
    {
      "FIELD1": "Australia/Darwin",
      "FIELD2": "ACST-9:30"
    },
    {
      "FIELD1": "Australia/Eucla",
      "FIELD2": "<+0845>-8:45"
    },
    {
      "FIELD1": "Australia/Hobart",
      "FIELD2": "AEST-10AEDT,M10.1.0,M4.1.0/3"
    },
    {
      "FIELD1": "Australia/Lindeman",
      "FIELD2": "AEST-10"
    },
    {
      "FIELD1": "Australia/Lord_Howe",
      "FIELD2": "<+1030>-10:30<+11>-11,M10.1.0,M4.1.0"
    },
    {
      "FIELD1": "Australia/Melbourne",
      "FIELD2": "AEST-10AEDT,M10.1.0,M4.1.0/3"
    },
    {
      "FIELD1": "Australia/Perth",
      "FIELD2": "AWST-8"
    },
    {
      "FIELD1": "Australia/Sydney",
      "FIELD2": "AEST-10AEDT,M10.1.0,M4.1.0/3"
    },
    {
      "FIELD1": "Europe/Amsterdam",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Andorra",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Astrakhan",
      "FIELD2": "<+04>-4"
    },
    {
      "FIELD1": "Europe/Athens",
      "FIELD2": "EET-2EEST,M3.5.0/3,M10.5.0/4"
    },
    {
      "FIELD1": "Europe/Belgrade",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Berlin",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Bratislava",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Brussels",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Bucharest",
      "FIELD2": "EET-2EEST,M3.5.0/3,M10.5.0/4"
    },
    {
      "FIELD1": "Europe/Budapest",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Busingen",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Chisinau",
      "FIELD2": "EET-2EEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Copenhagen",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Dublin",
      "FIELD2": "IST-1GMT0,M10.5.0,M3.5.0/1"
    },
    {
      "FIELD1": "Europe/Gibraltar",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Guernsey",
      "FIELD2": "GMT0BST,M3.5.0/1,M10.5.0"
    },
    {
      "FIELD1": "Europe/Helsinki",
      "FIELD2": "EET-2EEST,M3.5.0/3,M10.5.0/4"
    },
    {
      "FIELD1": "Europe/Isle_of_Man",
      "FIELD2": "GMT0BST,M3.5.0/1,M10.5.0"
    },
    {
      "FIELD1": "Europe/Istanbul",
      "FIELD2": "<+03>-3"
    },
    {
      "FIELD1": "Europe/Jersey",
      "FIELD2": "GMT0BST,M3.5.0/1,M10.5.0"
    },
    {
      "FIELD1": "Europe/Kaliningrad",
      "FIELD2": "EET-2"
    },
    {
      "FIELD1": "Europe/Kiev",
      "FIELD2": "EET-2EEST,M3.5.0/3,M10.5.0/4"
    },
    {
      "FIELD1": "Europe/Kirov",
      "FIELD2": "<+03>-3"
    },
    {
      "FIELD1": "Europe/Lisbon",
      "FIELD2": "WET0WEST,M3.5.0/1,M10.5.0"
    },
    {
      "FIELD1": "Europe/Ljubljana",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/London",
      "FIELD2": "GMT0BST,M3.5.0/1,M10.5.0"
    },
    {
      "FIELD1": "Europe/Luxembourg",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Madrid",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Malta",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Mariehamn",
      "FIELD2": "EET-2EEST,M3.5.0/3,M10.5.0/4"
    },
    {
      "FIELD1": "Europe/Minsk",
      "FIELD2": "<+03>-3"
    },
    {
      "FIELD1": "Europe/Monaco",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Moscow",
      "FIELD2": "MSK-3"
    },
    {
      "FIELD1": "Europe/Oslo",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Paris",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Podgorica",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Prague",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Riga",
      "FIELD2": "EET-2EEST,M3.5.0/3,M10.5.0/4"
    },
    {
      "FIELD1": "Europe/Rome",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Samara",
      "FIELD2": "<+04>-4"
    },
    {
      "FIELD1": "Europe/San_Marino",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Sarajevo",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Saratov",
      "FIELD2": "<+04>-4"
    },
    {
      "FIELD1": "Europe/Simferopol",
      "FIELD2": "MSK-3"
    },
    {
      "FIELD1": "Europe/Skopje",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Sofia",
      "FIELD2": "EET-2EEST,M3.5.0/3,M10.5.0/4"
    },
    {
      "FIELD1": "Europe/Stockholm",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Tallinn",
      "FIELD2": "EET-2EEST,M3.5.0/3,M10.5.0/4"
    },
    {
      "FIELD1": "Europe/Tirane",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Ulyanovsk",
      "FIELD2": "<+04>-4"
    },
    {
      "FIELD1": "Europe/Uzhgorod",
      "FIELD2": "EET-2EEST,M3.5.0/3,M10.5.0/4"
    },
    {
      "FIELD1": "Europe/Vaduz",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Vatican",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Vienna",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Vilnius",
      "FIELD2": "EET-2EEST,M3.5.0/3,M10.5.0/4"
    },
    {
      "FIELD1": "Europe/Volgograd",
      "FIELD2": "<+03>-3"
    },
    {
      "FIELD1": "Europe/Warsaw",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Zagreb",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Europe/Zaporozhye",
      "FIELD2": "EET-2EEST,M3.5.0/3,M10.5.0/4"
    },
    {
      "FIELD1": "Europe/Zurich",
      "FIELD2": "CET-1CEST,M3.5.0,M10.5.0/3"
    },
    {
      "FIELD1": "Indian/Antananarivo",
      "FIELD2": "EAT-3"
    },
    {
      "FIELD1": "Indian/Chagos",
      "FIELD2": "<+06>-6"
    },
    {
      "FIELD1": "Indian/Christmas",
      "FIELD2": "<+07>-7"
    },
    {
      "FIELD1": "Indian/Cocos",
      "FIELD2": "<+0630>-6:30"
    },
    {
      "FIELD1": "Indian/Comoro",
      "FIELD2": "EAT-3"
    },
    {
      "FIELD1": "Indian/Kerguelen",
      "FIELD2": "<+05>-5"
    },
    {
      "FIELD1": "Indian/Mahe",
      "FIELD2": "<+04>-4"
    },
    {
      "FIELD1": "Indian/Maldives",
      "FIELD2": "<+05>-5"
    },
    {
      "FIELD1": "Indian/Mauritius",
      "FIELD2": "<+04>-4"
    },
    {
      "FIELD1": "Indian/Mayotte",
      "FIELD2": "EAT-3"
    },
    {
      "FIELD1": "Indian/Reunion",
      "FIELD2": "<+04>-4"
    },
    {
      "FIELD1": "Pacific/Apia",
      "FIELD2": "<+13>-13"
    },
    {
      "FIELD1": "Pacific/Auckland",
      "FIELD2": "NZST-12NZDT,M9.5.0,M4.1.0/3"
    },
    {
      "FIELD1": "Pacific/Bougainville",
      "FIELD2": "<+11>-11"
    },
    {
      "FIELD1": "Pacific/Chatham",
      "FIELD2": "<+1245>-12:45<+1345>,M9.5.0/2:45,M4.1.0/3:45"
    },
    {
      "FIELD1": "Pacific/Chuuk",
      "FIELD2": "<+10>-10"
    },
    {
      "FIELD1": "Pacific/Easter",
      "FIELD2": "<-06>6<-05>,M9.1.6/22,M4.1.6/22"
    },
    {
      "FIELD1": "Pacific/Efate",
      "FIELD2": "<+11>-11"
    },
    {
      "FIELD1": "Pacific/Enderbury",
      "FIELD2": "<+13>-13"
    },
    {
      "FIELD1": "Pacific/Fakaofo",
      "FIELD2": "<+13>-13"
    },
    {
      "FIELD1": "Pacific/Fiji",
      "FIELD2": "<+12>-12<+13>,M11.2.0,M1.2.3/99"
    },
    {
      "FIELD1": "Pacific/Funafuti",
      "FIELD2": "<+12>-12"
    },
    {
      "FIELD1": "Pacific/Galapagos",
      "FIELD2": "<-06>6"
    },
    {
      "FIELD1": "Pacific/Gambier",
      "FIELD2": "<-09>9"
    },
    {
      "FIELD1": "Pacific/Guadalcanal",
      "FIELD2": "<+11>-11"
    },
    {
      "FIELD1": "Pacific/Guam",
      "FIELD2": "ChST-10"
    },
    {
      "FIELD1": "Pacific/Honolulu",
      "FIELD2": "HST10"
    },
    {
      "FIELD1": "Pacific/Kiritimati",
      "FIELD2": "<+14>-14"
    },
    {
      "FIELD1": "Pacific/Kosrae",
      "FIELD2": "<+11>-11"
    },
    {
      "FIELD1": "Pacific/Kwajalein",
      "FIELD2": "<+12>-12"
    },
    {
      "FIELD1": "Pacific/Majuro",
      "FIELD2": "<+12>-12"
    },
    {
      "FIELD1": "Pacific/Marquesas",
      "FIELD2": "<-0930>9:30"
    },
    {
      "FIELD1": "Pacific/Midway",
      "FIELD2": "SST11"
    },
    {
      "FIELD1": "Pacific/Nauru",
      "FIELD2": "<+12>-12"
    },
    {
      "FIELD1": "Pacific/Niue",
      "FIELD2": "<-11>11"
    },
    {
      "FIELD1": "Pacific/Norfolk",
      "FIELD2": "<+11>-11<+12>,M10.1.0,M4.1.0/3"
    },
    {
      "FIELD1": "Pacific/Noumea",
      "FIELD2": "<+11>-11"
    },
    {
      "FIELD1": "Pacific/Pago_Pago",
      "FIELD2": "SST11"
    },
    {
      "FIELD1": "Pacific/Palau",
      "FIELD2": "<+09>-9"
    },
    {
      "FIELD1": "Pacific/Pitcairn",
      "FIELD2": "<-08>8"
    },
    {
      "FIELD1": "Pacific/Pohnpei",
      "FIELD2": "<+11>-11"
    },
    {
      "FIELD1": "Pacific/Port_Moresby",
      "FIELD2": "<+10>-10"
    },
    {
      "FIELD1": "Pacific/Rarotonga",
      "FIELD2": "<-10>10"
    },
    {
      "FIELD1": "Pacific/Saipan",
      "FIELD2": "ChST-10"
    },
    {
      "FIELD1": "Pacific/Tahiti",
      "FIELD2": "<-10>10"
    },
    {
      "FIELD1": "Pacific/Tarawa",
      "FIELD2": "<+12>-12"
    },
    {
      "FIELD1": "Pacific/Tongatapu",
      "FIELD2": "<+13>-13"
    },
    {
      "FIELD1": "Pacific/Wake",
      "FIELD2": "<+12>-12"
    },
    {
      "FIELD1": "Pacific/Wallis",
      "FIELD2": "<+12>-12"
    },
    {
      "FIELD1": "Etc/GMT",
      "FIELD2": "GMT0"
    },
    {
      "FIELD1": "Etc/GMT-0",
      "FIELD2": "GMT0"
    },
    {
      "FIELD1": "Etc/GMT-1",
      "FIELD2": "<+01>-1"
    },
    {
      "FIELD1": "Etc/GMT-2",
      "FIELD2": "<+02>-2"
    },
    {
      "FIELD1": "Etc/GMT-3",
      "FIELD2": "<+03>-3"
    },
    {
      "FIELD1": "Etc/GMT-4",
      "FIELD2": "<+04>-4"
    },
    {
      "FIELD1": "Etc/GMT-5",
      "FIELD2": "<+05>-5"
    },
    {
      "FIELD1": "Etc/GMT-6",
      "FIELD2": "<+06>-6"
    },
    {
      "FIELD1": "Etc/GMT-7",
      "FIELD2": "<+07>-7"
    },
    {
      "FIELD1": "Etc/GMT-8",
      "FIELD2": "<+08>-8"
    },
    {
      "FIELD1": "Etc/GMT-9",
      "FIELD2": "<+09>-9"
    },
    {
      "FIELD1": "Etc/GMT-10",
      "FIELD2": "<+10>-10"
    },
    {
      "FIELD1": "Etc/GMT-11",
      "FIELD2": "<+11>-11"
    },
    {
      "FIELD1": "Etc/GMT-12",
      "FIELD2": "<+12>-12"
    },
    {
      "FIELD1": "Etc/GMT-13",
      "FIELD2": "<+13>-13"
    },
    {
      "FIELD1": "Etc/GMT-14",
      "FIELD2": "<+14>-14"
    },
    {
      "FIELD1": "Etc/GMT0",
      "FIELD2": "GMT0"
    },
    {
      "FIELD1": "Etc/GMT+0",
      "FIELD2": "GMT0"
    },
    {
      "FIELD1": "Etc/GMT+1",
      "FIELD2": "<-01>1"
    },
    {
      "FIELD1": "Etc/GMT+2",
      "FIELD2": "<-02>2"
    },
    {
      "FIELD1": "Etc/GMT+3",
      "FIELD2": "<-03>3"
    },
    {
      "FIELD1": "Etc/GMT+4",
      "FIELD2": "<-04>4"
    },
    {
      "FIELD1": "Etc/GMT+5",
      "FIELD2": "<-05>5"
    },
    {
      "FIELD1": "Etc/GMT+6",
      "FIELD2": "<-06>6"
    },
    {
      "FIELD1": "Etc/GMT+7",
      "FIELD2": "<-07>7"
    },
    {
      "FIELD1": "Etc/GMT+8",
      "FIELD2": "<-08>8"
    },
    {
      "FIELD1": "Etc/GMT+9",
      "FIELD2": "<-09>9"
    },
    {
      "FIELD1": "Etc/GMT+10",
      "FIELD2": "<-10>10"
    },
    {
      "FIELD1": "Etc/GMT+11",
      "FIELD2": "<-11>11"
    },
    {
      "FIELD1": "Etc/GMT+12",
      "FIELD2": "<-12>12"
    },
    {
      "FIELD1": "Etc/UCT",
      "FIELD2": "UTC0"
    },
    {
      "FIELD1": "Etc/UTC",
      "FIELD2": "UTC0"
    },
    {
      "FIELD1": "Etc/Greenwich",
      "FIELD2": "GMT0"
    },
    {
      "FIELD1": "Etc/Universal",
      "FIELD2": "UTC0"
    },
    {
      "FIELD1": "Etc/Zulu",
      "FIELD2": "UTC0"
    }
   ];


