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

/*
function onLoad()
{
	inverter.firmwareVersion = 4;
	inverter.getParamList();
}
*/

var log = {

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
				for ( var key in paramsCache.data )
				{
						if ( ! paramsCache.data[key].isparam )
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
			  var items = new Array();
			  var formItems = document.forms["data-logger-configuration"].elements;
			  for ( var i = 0; i < formItems.length; i++ )
			  {
			  	  if ( formItems[i].type === 'select-one' && formItems[i].classList.contains('logger-field-select') )
			  	  {
			  	  	 items.push(formItems[i].value);
			  	  }
			  }
			  return items;
	  },

    /* @brief start collecting log data */
		start: function()
		{
        var items = log.getLogItems();
        console.log("Starting logging for " + items);

        /*
				if (items.length)
				{
						items = [];
						return;
				}
				*/
				
				var textArea = document.getElementById("data-logger-text-area");
				var samples = document.getElementById("data-logger-samples").value;
				console.log("Logger will fetch " + samples + " samples");
				var paramPart = document.location.href.split("items=");
				//items = paramPart[1].split(",");
				var minmax = document.getElementById("data-logger-minmax").checked;
				
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
				
				log.acquire(samples);
		},

		save: function()
		{
			  var textToWrite = document.getElementById('textArea').innerHTML;
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
		},
}

