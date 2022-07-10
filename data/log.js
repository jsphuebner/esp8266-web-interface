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

var items = [];
var textArea;
var minmax = false;

function onLoad()
{
	inverter.firmwareVersion = 4;
	inverter.getParamList();
}

function start()
{
	if (items.length)
	{
		items = [];
		return;
	}
	
	textArea = document.getElementById("textArea");
	var samples = document.getElementById("samples").value;
	var paramPart = document.location.href.split("items=");
	items = paramPart[1].split(",");
	minmax = document.getElementById("minmax").checked;
	
	textArea.innerHTML = "Timestamp"
	
	if (minmax)
	{
		for (var i = 0; i < items.length; i++)
		{
			textArea.innerHTML += "," + items[i] + " (avg)," + items[i] + " (min)," + items[i] + " (max)";
		}
	}
	else
	{
		textArea.innerHTML += "," + paramPart[1];
	}
	
	textArea.innerHTML += "\r\n";
	
	acquire(samples);
}

function save()
{
  var textToWrite = document.getElementById('textArea').innerHTML;
  var textFileAsBlob = new Blob([ textToWrite ], { type: 'text/csv' });
  var fileNameToSaveAs = "log.csv";

  var downloadLink = document.createElement("a");
  downloadLink.download = fileNameToSaveAs;
  downloadLink.innerHTML = "Download File";
  if (window.webkitURL != null) {
    // Chrome allows the link to be clicked without actually adding it to the DOM.
    downloadLink.href = window.webkitURL.createObjectURL(textFileAsBlob);
  } else {
    // Firefox requires the link to be added to the DOM before it can be clicked.
    downloadLink.href = window.URL.createObjectURL(textFileAsBlob);
    downloadLink.onclick = function(event) {document.body.removeChild(event.target)};
    downloadLink.style.display = "none";
    document.body.appendChild(downloadLink);
  }

  downloadLink.click();
}

function acquire(samples)
{
	if (!items.length) return;

	inverter.getValues(items, samples,
	function(values) 
	{
		var tzoffset = (new Date()).getTimezoneOffset() * 60000; //offset in milliseconds
		var localISOTime = (new Date(Date.now() - tzoffset)).toISOString().slice(0, -1);
		var line = localISOTime;
		for (var name in values)
		{
			var avg = values[name].reduce((acc, c) => acc + c, 0) / samples;
			
			if (minmax)
			{
				line += "," + avg.toFixed(2) + "," + Math.min(...values[name]) + "," + Math.max(...values[name]);
			}
			else
			{
				line += "," + avg.toFixed(2);
			}
		}
		line += "\r\n";
		textArea.innerHTML += line;
		textArea.scrollTop = textArea.scrollHeight;
		acquire(samples);
	});
}

