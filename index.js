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
var chartPages = 0;
var items = {};
var stop;
var imgid = 0;
var subscription;

/** @brief excutes when page finished loading. Creates tables and chart */
function onLoad()
{
	updateTables();
	generateChart();
	checkSubscribedParameterSet();
}

/** @brief generates chart at bottom of page */
function generateChart()
{
	chart = new Chart("canvas", {
		type: "line",
		options: {
			responsive: true,
          	maintainAspectRatio: false,
			animation: false,
			scales: {
				'y-axis-0': {
					type: "linear",
					display: true,
					position: "left"
				},
				'y-axis-1': {
					type: "linear",
					display: true,
					position: "right",
					grid: { drawOnChartArea: false }
				}
			}
		} });
}

function parameterSubmit()
{
	document.getElementById("loader0").style.visibility = "visible";
	inverter.getParamList(function(values)
	{
		document.getElementById("loader0").style.visibility = "hidden";
		document.getElementById("parameters_json").value = JSON.stringify(values);
		document.getElementById("paramdb").submit();
	}, true, 0);
}

function checkSubscribedParameterSet()
{
	if (subscription)
	{
		checkToken(subscription.token, 'Checking your parameter subscription ' + subscription.token, false);
	}
}

/* If a valid token is entered, the belonging dataset is downloaded
 * and applied to the inverter. Token and timestamp are saved to ESP filesystem
 * Token example 5f4d8fa6-b6a4-4f87-9a28-4363bdac5dc9 */
function checkToken(token, message, forceUpdate)
{
	var expr = /^[0-9a-f]{8}\-[0-9a-f]{4}\-[0-9a-f]{4}\-[0-9a-f]{4}\-[0-9a-f]{12}$/i;
	
	if (expr.test(token))
	{
		var xmlhttp=new XMLHttpRequest();
		var req = "https://openinverter.org/parameters/api.php?token=" + token;

		document.getElementById("message").innerHTML = message + "\r\n";
		document.getElementById("parameters_token").value = token;

		xmlhttp.onload = function() 
		{
			var params = JSON.parse(this.responseText);
			var timestamp = params.timestamp;
			
			delete params['timestamp'];			
			document.getElementById("token").value = token;
			
			if (subscription && subscription.timestamp == timestamp && !forceUpdate)
			{
				document.getElementById("message").innerHTML += "Parameters up to date\r\n";
			}
			else if (forceUpdate || confirm("Parameter set updated, apply?"))
			{
				document.getElementById("message").innerHTML += "Applying new parameter set from " + timestamp + "\r\n";

				setParam(params, 0);
				
				var uploadRequest = new XMLHttpRequest();
				var formData = new FormData();
				var subs = "subscription = { 'timestamp': '" + timestamp + "', 'token': '" + token + "' };";
				var blob = new Blob([subs], { type: "text/javascript"});
				formData.append("file", blob, "subscription.js");
				uploadRequest.open("POST", "/edit");
				uploadRequest.send(formData);
			}
		};
		
		xmlhttp.onerror = function()
		{
			alert("error");
		};

		xmlhttp.open("GET", req, true);
		xmlhttp.send();
	}
	else
	{
		var uploadRequest = new XMLHttpRequest();
		var formData = new FormData();
		var subs = "subscription = false;";
		var blob = new Blob([subs], { type: "text/javascript"});
		formData.append("file", blob, "subscription.js");
		uploadRequest.open("POST", "/edit");
		uploadRequest.send(formData);
	}
}

/** @brief generates parameter and spotvalue tables */
function updateTables()
{
	document.getElementById("loader1").style.visibility = "visible";
	document.getElementById("loader2").style.visibility = "visible";

	inverter.getParamList(function(values) 
	{
		var tableParam = document.getElementById("params");
		var tableSpot = document.getElementById("spotValues");
		var lastCategory = "";
		var params = {};

		while (tableParam.rows.length > 1) tableParam.deleteRow(1);
		while (tableSpot.rows.length > 1) tableSpot.deleteRow(1);

		for (var name in values)
		{
			var param = values[name];
			if (param.isparam)
			{
				var valInput;
				var unit = param.unit;
				var index = "-";
				params[name] = param.value;

				if (param.category != lastCategory)
				{
					addRow(tableParam, [ '<BUTTON onclick="toggleVisibility(\'' + 
						param.category + '\');" style="background: none; border: none; font-weight: bold;">- ' + 
						param.category + '</BUTTON>' ]);
					lastCategory = param.category;
				}
				
				if (param.enums)
				{
					if (param.enums[param.value])
					{
					    valInput = '<SELECT onchange="sendCmd(\'set ' + name + ' \' + this.value)">';

					    for (var idx in param.enums)
					    {
     						valInput += '<OPTION value="' + idx + '"';
						    if (idx == param.value)
							    valInput += " selected";
						    valInput += '>' + param.enums[idx] + '</OPTION>';
					    }
					}
					else
					{
 						valInput = "<ul>";
 						for (var key in param.enums)
 						{
 							if (param.value & key)
 								valInput += "<li>" + param.enums[key];
 						}
 						valInput += "</ul>";
					}
					unit = "";
				}
				else
				{
					valInput = '<INPUT type="number" min="' + param.minimum + '" max="' + param.maximum + 
						'" step="0.05" value="' + param.value + '" onchange="sendCmd(\'set ' + name + ' \' + this.value)"/>';
				}
				
				if (param.i !== undefined)
				    index = param.i;
				
				addRow(tableParam, [ index, name, valInput, unit, param.minimum, param.maximum, param.default ]);
			}
			else
			{
				var can = param.canid != undefined;
				var checkHtml = '<INPUT type="checkbox" data-name="' + name + '" data-axis="y-axis-0" /> l';
				checkHtml += ' <INPUT type="checkbox" data-name="' + name + '" data-axis="y-axis-1" /> r';
				var canIdHtml = '<INPUT type="number" step="1" min="0" max="2047" id="canid' + name + '" value="' + (can ? param.canid : "") + '"/>';
				var canPosHtml = '<INPUT type="number" step="1" min="0" max="63" id="canpos' + name + '" value="' + (can ? param.canoffset : "") + '"/>';
				var canBitsHtml = '<INPUT type="number" step="1" min="1" max="32" id="canbits' + name + '" value="' + (can ? param.canlength : "") + '"/>';
				var canGainHtml = '<INPUT type="number" step="1" min="1" max="100" id="cangain' + name + '" value="' + (can ? param.cangain : "") + '"/>';
				var buttonHtml = '<BUTTON id="' + name + '" onclick="canmap(\'tx\', this.id)">TX</BUTTON>';
				buttonHtml += ' <BUTTON id="' + name + '" onclick="canmap(\'rx\', this.id)">RX</BUTTON>'
				var unit = param.unit;
				
				if (can)
				{
					buttonHtml = param.isrx ? "RX" : "TX";
					buttonHtml += ' <BUTTON id="' + name + '" onclick="canmap(\'del\', this.id)">Unmap</BUTTON>';
				}

				if (param.enums)
				{
					if (param.enums[param.value])
 					{
 						display = param.enums[param.value];
 					}
 					else
 					{
 						var active = [];
 						for (var key in param.enums)
 						{
 							if (param.value & key)
 								active.push(param.enums[key]);
 						}
 						display = active.join('|');
 					}
					unit = "";
				}
				else
				{
					display = param.value;
				}

				addRow(tableSpot, [ name, display, unit, checkHtml, canIdHtml, canPosHtml, canBitsHtml, canGainHtml, buttonHtml ]);
			}
		}
		document.getElementById("paramDownload").href = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(params, null, 2));
		document.getElementById("loader1").style.visibility = "hidden";
		document.getElementById("loader2").style.visibility = "hidden";
		
		if (document.getElementById("autorefresh").checked)
 			updateTables();	
	}, false, 0);
}

/** @brief Adds row to a table
 * If table has multiple columns and only one cell value is
 * provided, the cell is spanned across entire table
 * @param table DOM object of table
 * @param content Array of strings with contents for each cell */
function addRow(table, content)
{
	var tr = table.insertRow(-1); //add row to end
	var colSpan = table.rows[0].cells.length - content.length + 1;

	for (var i = 0; i < content.length; i++)
	{
		var cell = tr.insertCell(-1);
		cell.colSpan = colSpan;
		cell.innerHTML = content[i];
	}
}

/** @brief toggles visibility of parameter category
 * @param name name of category to show/hide */
function toggleVisibility(name)
{
	var rows = document.getElementById("params").rows;
	var found = false;
	
	for (var i = 0; i < rows.length; i++)
	{
		if (found)
		{
			if (rows[i].cells.length > 1)
				rows[i].style.display = rows[i].style.display == "" ? "none" : "";
			else
				found = false;
		}

		if (!found)
		{
			found = rows[i].cells.length == 1 && (rows[i].cells[0].innerText.endsWith(name) || !name);
			
			if (found)
			{
				var str = rows[i].cells[0].firstChild.firstChild.nodeValue;
				rows[i].cells[0].firstChild.firstChild.nodeValue = (str.startsWith('-') ? '+' : '-') + str.substring(1);
			}
		}
	}
}

/** @brief Clears inverter reply section */
function clearMessages()
{
	document.getElementById("message").innerHTML = "";
}

/** @brief Maps a spot value to a CAN message
 * @param direction "rx" or "tx"
 * @param name name of spot value to be mapped */
function canmap(direction, name)
{
    var canid = document.getElementById('canid' + name).value;
    var canpos = document.getElementById('canpos' + name).value;
    var canbits = document.getElementById('canbits' + name).value;
    var cangain = document.getElementById('cangain' + name).value;
    var cmd = "can " + direction + " " + name + " " + canid + " " + canpos + " " + canbits + " " + cangain;
    
    sendCmd(cmd);
}

/** @brief Loads a parameterset from json file and sends each parameter to the inverter */
function loadParametersFromFile()
{
	var file = document.getElementById('paramfile');
	
	if(file.files.length)
	{
		var reader = new FileReader();

		reader.onload = function(e)
		{
			var params = JSON.parse(e.target.result);
			document.getElementById("message").innerHTML = "Start setting parameters\r\n";
			setParam(params, 0);
		};

		reader.readAsBinaryString(file.files[0]);
	}
}

/** @brief helper function, from a list of parameters send parameter with given index to inverter
 * @param params map of parameters (name -> value)
 * @param index numerical index which parameter to set */
function setParam(params, index)
{
	var keys = Object.keys(params); 
	
	if (index < keys.length)
	{
		var key = keys[index];
		document.getElementById("message").innerHTML += "Setting " + key + " to " + params[key] + " - ";

		inverter.sendCmd("set " + key + " " + params[key], function(reply) {
			document.getElementById("message").innerHTML += reply;
			setParam(params, index + 1);
		});
	}
}

/** @brief send arbitrary command to inverter and print result
 * @param cmd command string to be sent */
function sendCmd(cmd)
{
	inverter.sendCmd(cmd, function(reply)
	{
		document.getElementById("message").innerHTML = reply;
	});
}

/** @brief open new page with gauges for selected spot values */
function showGauges()
{
	var items = getPlotItems();
	var req = "gauges.html?items=" + items.names.join(',')

	window.open(req);
}

/** @brief open new page with gauges for selected spot values */
function showLog()
{
	var items = getPlotItems();
	var req = "log.html?items=" + items.names.join(',')

	window.open(req);
}

/** @brief export chart as image */
function exportChartImage()
{
	var render = chart.canvas.toDataURL('image/png', 1.0);
	var d = new Date();

   var data = atob(render.substring('data:image/png;base64,'.length)),
        asArray = new Uint8Array(data.length);

    for (var i = 0, len = data.length; i < len; ++i) {
        asArray[i] = data.charCodeAt(i);
    }
    var blob = new Blob([asArray.buffer], { type: 'image/png' });

    var url = URL.createObjectURL(blob);
    var a = document.createElement('a');
    a.href = url;
    a.download = 'chart ' + d.getDate() + '-' + (d.getMonth() + 1) + '-' + d.getFullYear() + ' ' + (d.getHours() % 12 || 12) + '-' + d.getMinutes() + ' ' + (d.getHours() >= 12 ? 'pm' : 'am') + '.png';
    document.body.appendChild(a);
    a.click();
    setTimeout(function () {
        document.body.removeChild(a);
        window.URL.revokeObjectURL(url);
    }, 0);
}

/** @brief uploads file to web server, if bin-file uploaded, starts a firmware upgrade */
function uploadFile() 
{
	var xmlhttp = new XMLHttpRequest();
	var form = document.getElementById('uploadform');
	
	if (form.getFormData)
		var fd = form.getFormData();
	else
		var fd = new FormData(form);
	var file = document.getElementById('updatefile').files[0].name;

	xmlhttp.onload = function() 
	{
		if (file.endsWith(".bin"))
		{
			document.getElementById("bar").innerHTML = "<p>Upload Complete</p>";
			runUpdate(-1, "/" + file);
		}else{
			document.getElementById("bar").innerHTML = "<p>.bin File Only</p>";
		}
		setTimeout(function() { document.getElementById("bar").innerHTML = "" }, 5000);
	}

	xmlhttp.open("POST", "/edit");
	xmlhttp.send(fd);
}

/** @brief hard-reset SWD, different from soft-reset*/
function resetSWD()
{		
	var xhr = new XMLHttpRequest();
	xhr.onload = function()
	{
		document.getElementById("swdbar").innerHTML = "<p>Hard-Reset</p>";
		updateTables();
	};
	xhr.open('GET', '/swd/reset?hard', true);
	xhr.send();
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
		if (file.name.endsWith(".bin"))
		{
			document.getElementById("swdbar").innerHTML = "<p>Upload Complete ...wait for ESP8266 LED to stop flasing</p>";
		}else{
			document.getElementById("swdbar").innerHTML = "<p>.bin File Only</p>";
		}

		var xhr = new XMLHttpRequest();
		xhr.seenBytes = 0;
		xhr.seenTotalPages = 0;
		xhr.onreadystatechange = function() {
		  if(xhr.readyState == 3) {
		    var data = xhr.response.substr(xhr.seenBytes);
		    //console.log(data);

		    if(data.indexOf("Error") != -1) {
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

/** @brief Runs a step of a firmware upgrade
 * Step -1 is resetting controller
 * Steps i=0..n send page i
 * @param step step to execute
 * @param file file path of upgrade image on server */
function runUpdate(step,file)
{
	var xmlhttp=new XMLHttpRequest();
	xmlhttp.responseType = "json";
	xmlhttp.onload = function()
	{
		step++;
		var totalPages = this.response.pages;
		var progress = Math.round(100 * step / totalPages);
		document.getElementById("bar").style.width = progress + "%";
		document.getElementById("bar").innerHTML = "<p>" +  progress + "%</p>";
		if (step < totalPages)
			runUpdate(step, file);
		else
			document.getElementById("bar").innerHTML = "<p>Update Done!</p>";
	}
	xmlhttp.open("GET", "/fwupdate?step=" + step + "&file=" + file);
	xmlhttp.send();
}

/** @brief start plotting selected spot values */
function startPlot()
{
	items = getPlotItems();
	var colours = [ 'rgb(255, 99, 132)', 'rgb(54, 162, 235)', 'rgb(255, 159, 64)', 'rgb(153, 102, 255)', 'rgb(255, 205, 86)', 'rgb(75, 192, 192)' ];

	chart.config.data.labels = new Array();
	chart.config.data.datasets = new Array();

	for (var signalIdx = 0; signalIdx < items.names.length; signalIdx++)
	{
		var yAxisKey = items.names[signalIdx];
		var newDataset = {
		        label: items.names[signalIdx],
		        data: [],
		        borderColor: colours[signalIdx % colours.length],
		        backgroundColor: colours[signalIdx % colours.length],
		        pointRadius: 0,
		        yAxisID: items.axes[signalIdx],
		        normalized: true
		    };
		chart.config.data.datasets.push(newDataset);
	}

	chart.update();
	stop = false;
	document.getElementById("pauseButton").disabled = false;

	//Scroll Chart Reset
	var chartArea = document.getElementById("chartArea");
	var chartAreaWrapper = document.getElementById("chartAreaWrapper");
	chartArea.style.width = chartAreaWrapper.clientWidth + "px";
	
	var chartDelay = document.getElementById('chartDelay').value;
	serialTimeout(chartDelay);

	if (items.names.length) acquire();
}

/** @brief changes Serial.setTimeout() in esp8266 */
function serialTimeout(value)
{
	var xmlhttp = new XMLHttpRequest();
	xmlhttp.open("GET", "/baud?timeout=" + (value / 2), true);
	xmlhttp.send();
}

/** @brief Stop plotting */
function stopPlot()
{
	stop = true;
	document.getElementById("pauseButton").innerHTML = "Pause Plot";
	document.getElementById("pauseButton").disabled = false;
}

/** @brief pause or resume plotting */
function pauseResumePlot()
{
	if (stop)
	{
		stop = false;
		acquire();
		document.getElementById("pauseButton").innerHTML = "Pause Plot";
	}
	else
	{
		stop = true;
		document.getElementById("pauseButton").innerHTML = "Resume Plot";
	}
}

/** @brief chart stream results, loop needs to be as efficient as possible */
function acquire()
{
	if (stop) return;

	burstLength = document.getElementById('burstLength').value;
	maxPages = document.getElementById('maxPages').value;
	maxValues = document.getElementById('maxValues').value;
	autoScroll = document.getElementById('autoScroll').checked;
    
    inverter.getValues(items.names, burstLength,
	function(values)
	{
		var d = new Date();
		chart.config.data.labels.push(d.getHours() + ":" + d.getMinutes() + ":" + d.getSeconds());
		for (var i = 1; i < burstLength; i++)
		{
			chart.config.data.labels.push("");
		}

		var count = 0;
		for (var name in values)
		{
			var data = chart.config.data.datasets.find(function(element) { return element.label == name }).data;

			for (var i = 0; i < values[name].length; i++)
			{
				data.push(values[name][i]);
			}
			count++;
		}

		chart.update();

		//Scroll Chart
        var dataPoints = String(chart.config.data.labels.length / maxValues)[0];
        if(chartPages != dataPoints)
        {
        	chartPages = dataPoints;
        	//console.log(chartPages);
        	var chartArea = document.getElementById("chartArea");
        	var chartAreaWrapper = document.getElementById("chartAreaWrapper");
        	var chartAreaWidth = chartAreaWrapper.clientWidth + chart.width;
			chartArea.style.width = chartAreaWidth  + "px";
			if(autoScroll)
				chartAreaWrapper.scrollLeft = chartAreaWidth;

			if(chartPages >= maxPages) {
				data.splice(0, maxValues);
				chart.config.data.labels.splice(0, maxValues);
				chartPages--;
			}
        }
		acquire();
	});
}

function getPlotItems()
{
	var items = {};
	items.names = new Array();
	items.axes = new Array();

	var matches = document.querySelectorAll("#spotValues input[type=checkbox]");

	for (var i = 0; i < matches.length; i++)
	{
		if (matches[i].checked)
		{
			items.names.push(matches[i].dataset.name);
			items.axes.push(matches[i].dataset.axis);
		}
	}
	return items;
}

