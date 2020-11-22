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

var inverter = {
	firmwareVersion: 0,
	
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
			return enums;
		}
		return false;
	}


};
