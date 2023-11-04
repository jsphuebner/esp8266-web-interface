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


/** @brief this is a little cache to store the current params/spot values. This
 * is here so that different functions can do look-ups without making a full
 * HTTP call to the inverter each time. */

var paramsCache = {

    data: undefined,
    dataById: {},

    get: function(name) {
      if ( paramsCache.data !== undefined )
      {
        if ( name in paramsCache.data ) {
          if ( paramsCache.data[name].enums ) {
                return paramsCache.data[name].enums[paramsCache.data[name].value];
            } else {
              return paramsCache.data[name].value;
            }
        }
      }
      return null;
    },

    getEntry: function(name) {
      return paramsCache.data[name];
    },

    getData: function() { return paramsCache.data; },

    setData: function(data) {
      paramsCache.data = data;

      for (var key in data) {
        paramsCache.dataById[data[key].id] = data[key];
        paramsCache.dataById[data[key].id]['name'] = key;
      }
    },

    getJson: function() { return JSON.stringify(paramsCache.data); },

    getById: function(id) {
      return paramsCache.dataById[id];
    }
}

var inverter = {

	firmwareVersion: 0,

	/** @brief send a command to the inverter */
	sendCmd: function(cmd, replyFunc, repeat)
	{
		var xmlhttp=new XMLHttpRequest();
		var req = "/cmd?cmd=" + cmd;

		xmlhttp.onload = function()
		{
			if (replyFunc) replyFunc(this.responseText);
		}

		if (repeat)
			req += "&repeat=" + repeat;

		xmlhttp.open("GET", req, true);
		xmlhttp.send();
	},

	/** @brief get the params from the inverter */
	getParamList: function(replyFunc, includeHidden)
	{
		var cmd = includeHidden ? "json hidden" : "json";

		inverter.sendCmd(cmd, function(reply) {
			var params = {};
			try {
				params = JSON.parse(reply);

				for (var name in params)
				{
					var param = params[name];
					param.enums = inverter.parseEnum(param.unit);

					if (name == "version")
						inverter.firmwareVersion = parseFloat(param.value);
				}
			} catch(ex) {}
			paramsCache.setData(params);
			if (replyFunc) replyFunc(params);
		});
	},

	getValues: function(items, repeat, replyFunc)
	{
		var process = function(reply)
		{
			var expr = /(\-{0,1}[0-9]+\.[0-9]*)/mg;
			var signalIdx = 0;
			var values = {};

			for (var res = expr.exec(reply); res; res = expr.exec(reply))
			{
				var val = parseFloat(res[1]);

				if (!values[items[signalIdx]])
					values[items[signalIdx]] = new Array()
				values[items[signalIdx]].push(val);
				signalIdx = (signalIdx + 1) % items.length;
			}
			replyFunc(values);
		};

		if (inverter.firmwareVersion < 3.53 || items.length > 10)
			inverter.sendCmd("get " + items.join(','), process, repeat);
		else
			inverter.sendCmd("stream " + repeat + " " + items.join(','), process);
	},


	/** @brief given the 'unit' string provided by the inverter api, parse out
	 * the key value pairs and return them in an array.
	 * @param unit, e.g. "0=None, 1=UdcLow, 2=UdcHigh, 4=UdcBelowUdcSw"
	 * Example return : ['None', 'UdcLow', 'UdcHigh',,'udcBelowUdcSw']. Note,
	 * the extra comma is intentional. The position in the array is determined
	 * by the index on the left hand side of the equals in the 'unit' string.
	 */
	parseEnum: function(unit)
	{
		var expr = /(\-{0,1}[0-9]+)=([a-zA-Z0-9_\-\.]+)[,\s]{0,2}|([a-zA-Z0-9_\-\.]+)[,\s]{1,2}/g;
		var enums = new Array();
		var res = expr.exec(unit);

		if (res)
		{
			do
			{
				enums[res[1]] = res[2];
			} while (res = expr.exec(unit))
			//console.log('enums : ' + enums);
			return enums;
		}
		return false;
	},

	/** @brief helper function, from a list of parameters send parameter with given index to inverter
	 * @param params map of parameters (name -> value)
	 * @param index numerical index which parameter to set */
	setParam: function(params, index)
	{
		var keys = Object.keys(params);

		if (index < keys.length)
		{
			var key = keys[index];
			modal.appendToModal('large', "Setting " + key + " to " + params[key] + "<br>");
			inverter.sendCmd("set " + key + " " + params[key], function(reply) {
				modal.appendToModal('large', reply + "<br>");
				// auto-scroll text in modal as it is added
				modal.largeModalScrollToBottom();
				inverter.setParam(params, index + 1);
			});
		}
	},

    /** @brief Add/Delete a CAN mapping
     * @param direction, tx, rx, or del
     * @param name, spot value name
     * @param id, canid of message
     * @param pos, offset within frame
     * @param bits, length of field
     * @param gain, multiplier
     */
	canMapping: function(direction, name, id, pos, bits, gain)
	{
		var cmd = "can " + direction + " " + name + " " + id + " " + pos + " " + bits + " " + gain;
		inverter.sendCmd(cmd);
	},

    /** @brief get a list of files in the spiffs filesystem on the esp8266 */
	getFiles: function(replyFunc)
	{
		var filesRequest = new XMLHttpRequest();
		filesRequest.onload = function()
		{
			var filesJson = JSON.parse(this.responseText);
			replyFunc(filesJson);
		}
		filesRequest.onerror = function()
		{
			alert("error");
		}
		filesRequest.open("GET", "/list", true);
		filesRequest.send();
	},

	/** @brief delete a file from the spiffs filessytem on the esp8266 */
	deleteFile: function(filename, replyFunc)
	{
		var deleteFileRequest = new XMLHttpRequest();
		deleteFileRequest.onload = function()
		{
			var responseJson = JSON.parse(this.responseText);
			replyFunc(responseJson);
		}
		deleteFileRequest.onerror = function()
		{
			alert("error");
		}
		deleteFileRequest.open("DELETE", "/edit?f=" + filename, true);
		deleteFileRequest.send();
	}


};
