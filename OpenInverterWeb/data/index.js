/*
 * This file is part of the esp8266 web interface
 *
 * Copyright (C) 2018 Johannes Huebner <dev@johanneshuebner.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

var chart;
var items = {};
var stop;
var imgid = 0;
var subscription;

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

 

/** @brief uploads file to web server, Flash using Serial-Wire-Debug. Start address bootloader = 0x08000000, firmware = 0x08001000*/
function uploadSWDFile() 
{
	var xmlhttp = new XMLHttpRequest();
	var form = document.getElementById('swdform');
	
	if (form.getFormData)
		var fd = form.getFormData();
	else
		var fd = new FormData(form);
	var file = document.getElementById('swdfile').files[0];

	xmlhttp.onload = function()
	{
		var xhr = new XMLHttpRequest();
		xhr.seenBytes = 0;
		xhr.seenTotalPages = 0;
		xhr.onreadystatechange = function() {
		  if(xhr.readyState == 3) {
		    var data = xhr.response.substr(xhr.seenBytes);

		    if(data.indexOf("Error") != -1) {
		    	document.getElementById("swdbar").style.width = "100%";
				document.getElementById("swdbar").innerHTML = "<p>" + data + "</p>";
		    }else{
			    var s = data.split('\n');
				xhr.seenTotalPages += (s.length - 1) * 16;
				//console.log("pages: " + s.length + " Size: " + ((s.length -1) * 16));

			    var progress = Math.round(100 * xhr.seenTotalPages / file.size);
			    document.getElementById("swdbar").style.width = progress + "%";
			    document.getElementById("swdbar").innerHTML = "<p>" +  progress + "%</p>";
				
			    xhr.seenBytes = xhr.responseText.length;
			}
		  }
		};
		if (file.name.endsWith('loader.bin'))
		{
			xhr.open('GET', '/swd/mem/flash?bootloader&file=' + file.name, true);
		}else{
			xhr.open('GET', '/swd/mem/flash?flash&file=' + file.name, true);
		}
    	xhr.send();
	}
	xmlhttp.open("POST", "/edit");
	xmlhttp.send(fd);
}

