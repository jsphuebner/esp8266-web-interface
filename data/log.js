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

var log = {

	items: [],
	samples: 0,
	textArea: undefined,
	minmax: false,
	stopLogging: true,

    /* @brief add field to data logger */
	addLogItem: function()
	{
	  	var dataLoggerConfiguration = document.getElementById('data-logger-configuration');

	  	// container for the drop down and the delete button
		var selectDiv = document.createElement("div");
		selectDiv.classList.add('logger-field');
		dataLoggerConfiguration.appendChild(selectDiv);

		// Create a drop down and populate it with the possible spot values
		var selectSpotValue = document.createElement("select");
		selectSpotValue.classList.add('logger-field-select');
		for ( var key in paramsCache.getData() )
		{
			if ( ! paramsCache.getEntry(key).isparam )
			{
				var option = document.createElement("option");
				option.value = key;
				option.text = key;
				selectSpotValue.appendChild(option);
			}
		}
		selectDiv.appendChild(selectSpotValue);

		// Add the delete button
		var deleteButton = document.createElement("button");
		var deleteButtonImg = document.createElement('img');
		deleteButtonImg.src = '/icon-trash.png';
		deleteButton.appendChild(deleteButtonImg);
		deleteButton.onclick = function() { this.parentNode.remove(); };
		selectDiv.appendChild(deleteButton);
	},

    /** @brief return a list of fields currently configured for logger */
	getLogItems: function()
	{
	  	log.items = [];
		var formItems = document.forms["data-logger-configuration"].elements;
		for ( var i = 0; i < formItems.length; i++ )
		{
			if ( formItems[i].type === 'select-one' && formItems[i].classList.contains('logger-field-select') )
			{
			  	log.items.push(formItems[i].value);
			}
		}
	},

    /* @brief start collecting log data */
	start: function()
	{
	    log.stopLogging = false;
        log.getLogItems();
		log.textArea = document.getElementById("data-logger-text-area");
		log.samples = document.getElementById("data-logger-samples").value;
		log.minmax = document.getElementById("data-logger-minmax").checked;
		log.textArea.innerHTML = "Timestamp"

		if (log.minmax)
		{
			for (var i = 0; i < log.items.length; i++)
			{
				log.textArea.innerHTML += "," + log.items[i] + " (avg)," + log.items[i] + " (min)," + log.items[i] + " (max)";
			}
		}
		else
		{
			log.textArea.innerHTML += "," + log.items;
	    }

		log.textArea.innerHTML += "\r\n";
		log.acquire(log.samples);
	},

	stop: function()
	{
		log.stopLogging = true;
	},

	save: function()
	{
		var textToWrite = document.getElementById('data-logger-text-area').innerHTML;
		var textFileAsBlob = new Blob([ textToWrite ], { type: 'text/csv' });
		var fileNameToSaveAs = "log.csv";

		var downloadLink = document.createElement("a");
		downloadLink.download = fileNameToSaveAs;
	    downloadLink.innerHTML = "Download File";
		if (window.webkitURL != null)
		{
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
	},

	acquire: function(samples)
	{
		if ( log.stopLogging ){ return; }

		if (!log.items.length) return;

		inverter.getValues(log.items, log.samples,
			function(values)
			{
				var tzoffset = (new Date()).getTimezoneOffset() * 60000; //offset in milliseconds
				var localISOTime = (new Date(Date.now() - tzoffset)).toISOString().slice(0, -1);
				var line = localISOTime;
				for (var name in values)
				{
					var avg = values[name].reduce((acc, c) => acc + c, 0) / log.samples;

					if (log.minmax)
					{
						line += "," + avg.toFixed(2) + "," + Math.min(...values[name]) + "," + Math.max(...values[name]);
					}
					else
					{
						line += "," + avg.toFixed(2);
					}
				}
				line += "\r\n";
				log.textArea.innerHTML += line;
			    log.textArea.scrollTop = log.textArea.scrollHeight;
				log.acquire(samples);
			});
	},
}
