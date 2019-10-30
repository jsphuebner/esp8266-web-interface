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

var gauges = {};
var items = new Array();

function onLoad()
{
    createGauges();
    acquire();
}

/**
 * @brief Creates gauges for all canvases found on the page
 */
function createGauges()
{
	var div = document.getElementById("gauges");
	var paramPart = document.location.href.split("items=");
	items = paramPart[1].split(",");
	
	for (var i = 0; i < items.length; i++)
	{
		var name = items[i];
		var canvas = document.createElement("CANVAS");
		canvas.setAttribute("id", name);
		div.appendChild(canvas);
		
		var gauge = new RadialGauge( 
		{ 
			renderTo: name, 
			title: name,
			width: 300, 
			height: 300, 
			minValue: 0, 
			maxValue: 1,
			majorTicks: [0, 1]
		});

		gauge.draw();
		gauges[name] = gauge;
	}
}

function calcTicks(min, max)
{
	var N = 6;
	var ticks =  [ min ];
	var dist = (max - min) / N;
	var tick = min;
	
	for (var i = 0; i < N; i++)
	{
		tick += dist;
		ticks.push(Math.round(tick));
	}
	return ticks;
}

function acquire()
{
	if (!items.length) return;

	inverter.getValues(items, 1,
	function(values) 
	{
		for (var name in values)
		{
			var val = values[name][0];
			gauges[name].options.minValue = Math.min(gauges[name].options.minValue, Math.floor(val * 0.7));
			gauges[name].options.maxValue = Math.max(gauges[name].options.maxValue, Math.ceil(val * 1.5));
			gauges[name].options.majorTicks = calcTicks(gauges[name].options.minValue, gauges[name].options.maxValue);
			gauges[name].value = val;
			gauges[name].update();
		}
		acquire();
	});
}
