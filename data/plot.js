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

var plot = {

    stop: true,

    /** @brief launch new tab showing gauges based fields selected from the plot&gauge page */
    launchGauges: function()
    {
        var items = ui.getPlotItems();
        var req = "gauges.html?items=" + items.names.join(',')
        window.open(req);
    },

    /** @brief generates chart at bottom of page */
    generateChart: function()
    {
        chart = new Chart("canvas", {
            type: "line",
            options: {
                animation: {
                    duration: 0
                },
                scales: {
                    yAxes: [{
                        type: "linear",
                        display: true,
                        position: "left",
                        id: "left"
                    }, {
                        type: "linear",
                        display: true,
                        position: "right",
                        id: "right",
                        gridLines: { drawOnChartArea: false }
                    }]
                }
            } });
    },

    /** @brief start plotting selected spot values */
    startPlot: function()
    {
        items = ui.getPlotItems();
        var colours = [ 'rgb(255, 99, 132)', 'rgb(54, 162, 235)', 'rgb(255, 159, 64)', 'rgb(153, 102, 255)', 'rgb(255, 205, 86)', 'rgb(75, 192, 192)' ];

        chart.config.data.datasets = new Array();

        for (var signalIdx = 0; signalIdx < items.names.length; signalIdx++)
        {
            var newDataset = {
                    label: items.names[signalIdx],
                    data: [],
                    borderColor: colours[signalIdx % colours.length],
                    backgroundColor: colours[signalIdx % colours.length],
                    fill: false,
                    pointRadius: 0,
                    yAxisID: items.axes[signalIdx]
                };
            chart.config.data.datasets.push(newDataset);
        }

        ui.setAutoReload(false);
        time = 0;
        chart.update();
        plot.stop = false;
        document.getElementById("pauseButton").disabled = false;
        plot.acquire();
    },

    /** @brief Stop plotting */
    stopPlot: function()
    {
        plot.stop = true;
        document.getElementById("pauseButton").disabled = false;
        ui.setAutoReload(true);
    },

    /** @brief pause or resume plotting */
    pauseResumePlot: function()
    {
        if (plot.stop)
        {
            plot.stop = false;
            plot.acquire();
        }
        else
        {
            plot.stop = true;
        }
    },

    acquire: function()
    {
        if (plot.stop) return;
        if (!items.names.length) return;
        var burstLength = document.getElementById('burstLength').value;
        var maxValues = document.getElementById('maxValues').value;

        inverter.getValues(items.names, burstLength,
        function(values)
        {
            for (var i = 0; i < burstLength; i++)
            {
                chart.config.data.labels.push(time);
                time++;
            }
            chart.config.data.labels.splice(0, Math.max(chart.config.data.labels.length - maxValues, 0));

            for (var name in values)
            {
                var data = chart.config.data.datasets.find(function(element) { return element.label == name }).data;

                for (var i = 0; i < values[name].length; i++)
                {
                    data.push(values[name][i])
                    data.splice(0, Math.max(data.length - maxValues, 0));
                }
            }

            chart.update();
            plot.acquire();
        });
    },


}
