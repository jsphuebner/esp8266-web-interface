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

var ui = {

    // The API endpoint to query to get firmware release available in Github
	githubFirmwareReleaseURL: 'https://api.github.com/repos/jsphuebner/stm32-sine/releases',

  //Handle for auto refresh interval
  autoRefreshHandle: 0,

	// temp variable to store updates from Parameter Database
	paramUpdates: "",

	// Status of visibility of parameter categories. E.g. Motor, Inverter. true = visible, false = not visible.
	categoryVisible: {},

	shrinkNavbar: function() {
		document.getElementById("navbar").style.width = "80px";
		var cw = document.getElementById("content-wrapper");
		cw.style.left = "80px";
		cw.style.width = "calc(100% - 80px)";
		var logo = document.getElementById("logo");
		logo.style.width = "80px";
		logo.style.height = "50px";
		// buttons
		var buttons = document.getElementsByClassName("buttonimg");
		for ( let i = 0; i < buttons.length; i++ ) {
			console.log("button ", i, " ", buttons[i]);
			buttons[i].style.width = "60px";
		}
		// hide toggles, version box
		var itemsToHideOnSmallScreen = document.getElementsByClassName("small-screen-hide");
		for ( let i = 0; i < itemsToHideOnSmallScreen.length; i++ ) {
			console.log("item ", i, " ", itemsToHideOnSmallScreen[i]);
			itemsToHideOnSmallScreen[i].style.display = "none";
		}
	},

	growNavbar: function() {
		document.getElementById("navbar").style.width = "180px";
		var cw = document.getElementById("content-wrapper");
		cw.style.left = "180px";
		cw.style.width = "calc(100% - 180px)";
		var logo = document.getElementById("logo");
		logo.style.width = "180px";
		logo.style.height = "100px";
		// buttons
		var buttons = document.getElementsByClassName("buttonimg");
		for ( let i = 0; i < buttons.length; i++ ) {
			buttons[i].style.width = "24px";
		}
		// show toggles, version box
		var itemsToShowOnBigScreen = document.getElementsByClassName("small-screen-hide");
		for ( let i = 0; i < itemsToShowOnBigScreen.length; i++ ) {
			itemsToShowOnBigScreen[i].style.display = "block";
		}
	},

	/** @brief switch to a different page tab */
	openPage: function(pageName, elmnt, color)
	{
		// hide all tabs
	    var i, tabdiv, tablinks;
	    tabdiv = document.getElementsByClassName("tabdiv");
	    for (i = 0; i < tabdiv.length; i++) {
	        tabdiv[i].style.display = "none";
	    }

	    // un-highlight all tabs
	    tablinks = document.getElementsByClassName("tablink");
	    for (i = 0; i < tablinks.length; i++) {
	        tablinks[i].style.backgroundColor = "";
	    }

	    // show selected tab
	    document.getElementById(pageName).style.display = "flex";
	    elmnt.style.backgroundColor = color;

	    // Right menu has nothing in it for spot values, so hide it
	    var mainRights = document.getElementsByClassName("main-right");
	    if ( pageName == "spotvalues" ) {
			for ( i = 0; i < mainRights.length; i++ ) {
				mainRights[i].style.display = "none";
			}
	    } else {
			for ( i = 0; i < mainRights.length; i++ ) {
				mainRights[i].style.display = "block";
			}
	    }
	},

	/** @brief excutes when page finished loading. Creates tables and chart */
	onLoad: function()
	{
		// Set up listener to execute commands when enter is pressed (dashboard, command box)
		var commandinput = document.getElementById('commandinput');
		commandinput.addEventListener("keyup", function(event)
		{
			if ( event.keyCode == 13 )
			{
	            event.preventDefault();
	            ui.dashboardCommand();
			}
		});

		ui.updateTables();
		plot.generateChart();
		ui.parameterDatabaseCheckForUpdates();
		ui.populateSpotValueDropDown();
		ui.populateExistingCanMappingTable();
		wifi.populateWiFiTab();
		ui.populateFileList();
		ui.refreshStatusBox();
		ui.refreshMessagesBox();
	},

	/** @brief automatically update data on the UI */
	refresh: function()
	{
		ui.updateTables();
		ui.refreshStatusBox();
		ui.refreshMessagesBox();
	},

	/** @brief send arbitrary command to inverter and print result
	 * @param cmd command string to be sent */
	sendCmd: function(cmd)
	{
		inverter.sendCmd(cmd, function(reply)
		{
			document.getElementById("message").innerHTML = reply;
		});
	},

	/** @brief generates parameter and spotvalue tables */
	updateTables: function()
	{
		var tableParam = document.getElementById("params");

		// Don't run if any one of the param boxes are highlighted (i.e. don't clobber what the user is typing)
		var paramFields = tableParam.querySelectorAll('input, select');
		for ( var i = 0; i < paramFields.length; i++ )
		{
			if ( paramFields[i] === document.activeElement )
			{
				return;
			}
		}

		document.getElementById("spinner-div").style.visibility = "visible";

		inverter.getParamList(function(values)
		{

			var tableSpot = document.getElementById("spotValues");
			var lastCategory = "";
			var params = {};

			while (tableParam.rows.length > 1) tableParam.deleteRow(1);
			while (tableSpot.rows.length > 1) tableSpot.deleteRow(1);

			for (var name in values)
			{
				var param = values[name];

				// Get docstring
				var docstring = docstrings.get(name);
				if ( ! docstring == "" )
				{
					var nameWithTooltip = "<div class=\"tooltip\">" + name + "<span class=\"tooltiptext\">" + docstring + "</span></div>";
				}
				else
				{
					nameWithTooltip = name;
				}

				if (param.isparam)
				{
					var valInput;
					var unit = param.unit;
					var index = "-";
					params[name] = param.value;

					// Initialise categoryVisible toggles if needed (on first load for example). Make visible by default.
					if ( !(param.category in ui.categoryVisible) )
						ui.categoryVisible[param.category] = true;

                    // If we're starting a new category, insert the category header row.
					if (param.category != lastCategory)
					{
						var icon = ui.categoryVisible[param.category] ? '-' : '+';
						ui.addRow(tableParam, [ '<BUTTON onclick="ui.toggleVisibility(\'' +
							param.category + '\');" style="background: none; border: none; font-weight: bold;">' + icon + ' ' +
							param.category + '</BUTTON>' ], true);
						lastCategory = param.category;
					}

					if (param.enums)
					{
						if (param.enums[param.value])
						{

						    valInput = '<SELECT onchange="ui.showParamUpdateModal(\'' + name + '\', this.value)">';

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
							'" step="0.05" value="' + param.value + '" onchange="ui.showParamUpdateModal(\'' + name + '\', this.value)"/>';
					}

					if (param.i !== undefined)
					    index = param.i;

					ui.addRow(tableParam, [ index, nameWithTooltip, valInput, unit, param.minimum, param.maximum, param.default ], ui.categoryVisible[param.category]);
				}
				else
				{
					var checkHtml = '<INPUT type="checkbox" data-name="' + name + '" data-axis="left" /> l';
					checkHtml += ' <INPUT type="checkbox" data-name="' + name + '" data-axis="right" /> r';
					var unit = param.unit;

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

					ui.addRow(tableSpot, [ nameWithTooltip, display, unit ], true);
				}
			}
      ui.populateVersion();
			document.getElementById("paramDownload").href = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(params, null, 2));
			document.getElementById("spinner-div").style.visibility = "hidden";
		});
	},

	/** @brief Adds row to a table
	 * If table has multiple columns and only one cell value is
	 * provided, the cell is spanned across entire table
	 * @param table DOM object of table
	 * @param content Array of strings with contents for each cell */
	addRow: function(table, content, visible)
	{
		var tr = table.insertRow(-1); //add row to end
		tr.style.display = visible ? "" : "none";
		var colSpan = table.rows[0].cells.length - content.length + 1;

		for (var i = 0; i < content.length; i++)
		{
			var cell = tr.insertCell(-1);
			cell.colSpan = colSpan;
			cell.innerHTML = content[i];
		}
	},

	/** @brief fill out version box in the bottom left corner of the screen */
	populateVersion: function()
	{
		var versionDiv = document.getElementById("version");
		versionDiv.innerHTML = "";
		var firmwareVersion = String(paramsCache.get('version'));
		versionDiv.innerHTML += "firmware : " + firmwareVersion + "<br>";
		versionDiv.innerHTML += "web : v2.0"
	},

	/** @brief If beta features are visible, hide them. If hidden, show them. */
	toggleBetaFeaturesVisibility: function() {
		var betaFeatures = document.getElementsByClassName('beta-feature');
        var betaFeaturesCheckbox = document.getElementById('beta-features-checkbox');

		for ( var i = 0; i < betaFeatures.length; i++ )
		{
			if ( betaFeaturesCheckbox.checked )
			{
        betaFeatures[i].style.display = 'block';
			}
			else
			{
				betaFeatures[i].style.display = 'none';
			}
		}
	},

	/** @brief If beta features are visible, hide them. If hidden, show them. */
	setAutoReload: function(enable)	{
      var autoReloadCheckbox = document.getElementById('auto-reload-checkbox');

      // run the poll function every 2 seconds
      if (enable) {
        autoReloadCheckbox.checked = true;
        ui.autoRefreshHandle = setInterval(ui.refresh, 2000);
      }
      else {
        autoReloadCheckbox.checked = false;
        clearInterval(ui.autoRefreshHandle);
      }
	},

	/**
	 * ~~~ DASHBOARD ~~~
	 */

    /** @brief refresh the data in the status box (top left corner of dashboard page) */
	refreshStatusBox: function()
	{

		var statusDiv = document.getElementById('top-left');

		var status = paramsCache.get('status');

		if ( status == null ){
			return;
		}

		var lasterr = paramsCache.get('lasterr');
		var udc = paramsCache.get('udc');
		var tmphs = paramsCache.get('tmphs');
		var opmode = paramsCache.get('opmode');

		statusDiv.innerHTML = "";

		var tbl = document.createElement('table');
		var tbody = document.createElement('tbody');
		// status
		var tr = document.createElement('tr');
		var td = document.createElement('td');
		td.appendChild(document.createTextNode('Status'));
		tr.appendChild(td);
		td = document.createElement('td');
		td.appendChild(document.createTextNode(status));
		tr.appendChild(td);
		tbody.appendChild(tr);
		// opmode
		tr = document.createElement('tr');
	    td = document.createElement('td');
		td.appendChild(document.createTextNode('Opmode'));
		tr.appendChild(td);
		td = document.createElement('td');
		td.appendChild(document.createTextNode(opmode));
		tr.appendChild(td);
		tbody.appendChild(tr);
		// lasterr
		tr = document.createElement('tr');
		td = document.createElement('td');
		td.appendChild(document.createTextNode('Last error'));
		tr.appendChild(td);
		td = document.createElement('td');
		td.appendChild(document.createTextNode(lasterr));
		tr.appendChild(td);
		tbody.appendChild(tr);
		// udc
		tr = document.createElement('tr');
		td = document.createElement('td');
		td.appendChild(document.createTextNode('Battery voltage (udc)'));
		tr.appendChild(td);
		td = document.createElement('td');
		td.appendChild(document.createTextNode(udc));
		tr.appendChild(td);
		tbody.appendChild(tr);
		// tmphs
		tr = document.createElement('tr');
		td = document.createElement('td');
		td.appendChild(document.createTextNode('Inverter temperature'));
		tr.appendChild(td);
		td = document.createElement('td');
		td.appendChild(document.createTextNode(tmphs));
		tr.appendChild(td);
		tbody.appendChild(tr);


		tbl.appendChild(tbody);
		statusDiv.appendChild(tbl);

    },

    /** @brief execute command entered in the command box on the dashboard */
    dashboardCommand: function()
    {
    	// Get command entered
    	var commandinput = document.getElementById('commandinput').value;
    	// Get output box
    	var commandoutput = document.getElementById('commandoutput');
    	inverter.sendCmd(commandinput, function(reply){
            commandoutput.innerHTML += reply + "<br>";
            // Scroll output if needed
    	    commandoutput.scrollTop = commandoutput.scrollHeight;
    	});
    },

    /** @brief get error messages from inverter and put them in the messages box on the dashboard page */
    refreshMessagesBox: function(){
    	var messageBox = document.getElementById('message');
    	inverter.sendCmd('errors', function(reply){
            messageBox.innerHTML = reply;
    	});
    },


    /**
     * ~~~ UPDATE ~~~
     */

    /** @brief uploads file to web server, if bin-file uploaded, starts a firmware upgrade */
	uploadFirmwareFile: function()
	{
		// disable auto updates so as to not interrupt the update process
		ui.setAutoReload(false);

		modal.emptyModal('small');
		modal.showModal('small');

		var uploadFirmwareFileRequest = new XMLHttpRequest();
		var form = document.getElementById('upload-firmware-form');

		if (form.getFormData)
			var fd = form.getFormData();
		else
			var fd = new FormData(form);
		var file = document.getElementById('update-firmware-file').files[0].name;

		uploadFirmwareFileRequest.onload = function()
		{
			modal.appendToModal('small', '<p>Installing firmware...</p>');
			modal.appendToModal('small', '<div id="progress" class="graph"><div id="upload-firmware-bar" style="width: 0"></div></div>');

			if (file.endsWith(".bin"))
			{
				ui.runUpdateStep(-1, "/" + file);
			}
			else {
				modal.hideModal('small');
				alert('Error, wrong file type. You must use the "stm32_foc.bin" or "stm32_sine.bin" file to update the firmware');
			}
			document.getElementById("upload-firmware-bar").innerHTML = "<p>Upload complete</p>";
			document.getElementById("upload-firmware-bar").style.width = "100%";
		}

		uploadFirmwareFileRequest.open("POST", "/edit");
		uploadFirmwareFileRequest.send(fd);
	},

	/** @brief Runs a step of a firmware upgrade
	 * Step -1 is resetting controller
	 * Steps i=0..n send page i
	 * @param step step to execute
	 * @param file file path of upgrade image on server */
	runUpdateStep: function(step, file)
	{
		var runUpdateRequest = new XMLHttpRequest();
		runUpdateRequest.onload = function()
		{
			step++;
			var uploadFirmwareBar = document.getElementById("upload-firmware-bar");
			var result = JSON.parse(this.responseText);
			var totalPages = result.pages;
			var progress = Math.round(100 * step / totalPages);
			uploadFirmwareBar.style.width = progress + "%";
			uploadFirmwareBar.innerHTML = "<p>" +  progress + "%</p>";
			if (step < totalPages)
				ui.runUpdateStep(step, file);
			else
			{
				uploadFirmwareBar.innerHTML = "<p>Update Done!</p>";
				setTimeout(function() { modal.hideModal('small'); ui.refresh(); }, 3000);
			}
		}
		runUpdateRequest.open("GET", "/fwupdate?step=" + step + "&file=" + file);
		runUpdateRequest.send();
	},


	/** @brief uploads file to web server, if bin-file uploaded, starts a firmware upgrade */
	uploadFile: function()
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
			// Show popup reporting upload completion
			modal.emptyModal('small');
			modal.appendToModal('small', 'File upload complete');
			modal.showModal('small');
			// Refresh the list of files on the 'files' page
			ui.populateFileList();
			setTimeout(function() { modal.hideModal('small') }, 2000);
		}

		xmlhttp.open("POST", "/edit");
		xmlhttp.send(fd);
	},

    /** @brief show the 'Erase flash' confirmation dialog box */
    showEraseFlashConfirmationDialog: function()
    {
        modal.emptyModal('small');
        var msg = "<p>Are you sure you want to erase the flash?</p>";
        msg += "<div style=\"display:flex\">";
        msg += "<a href=\"/swd/zero\"><button><img src=\"/icon-trash-2.png\">Erase flash</button></a>";
        msg += "<button onclick=\"modal.hideModal('small');\"><img src=\"/icon-x-square.png\">Cancel</button>";
        msg += "</div>";
        modal.appendToModal('small', msg);
        modal.showModal('small');
    },

    /** @brief show the 'Hard resest' confirmation dialog box */
	showHardResetConfirmationDialog: function()
	{
		modal.emptyModal('small');
        var msg = "<p>Are you sure you want to perform a hard reset?</p>";
        msg += "<div style=\"display:flex\">";
        msg += "<button onclick=\"ui.performHardReset();\"><img src=\"/icon-rotate-ccw.png\">Hard reset</button>";
        msg += "<button onclick=\"modal.hideModal('small');\"><img src=\"/icon-x-square.png\">Cancel</button>";
        msg += "</div>";
        modal.appendToModal('small', msg);
        modal.showModal('small');
	},

	/** @brief actually perform a hard reset */
	performHardReset: function()
	{
		// disable automatic refresh while we're resetting
		autoRefresh = false;

		var resetRequest = new XMLHttpRequest();
		resetRequest.onload = function()
		{
			updateTables();
			modal.hideModal('small');
			var msg = "<p>Hard reset complete</p>";
			msg += "<div style=\"display:flex\">";
			msg += "<button onclick=\"modal.hideModal('small');\"><img src=\"/icon-x-square.png\">Close</button>";
			msg += "</div>";
			modal.appendToModal('small', msg);
			modal.showModal('small');
			// turn automatic refresh back on
			autoRefresh = true;
		};
		resetRequest.open('GET', '/swd/reset?hard', true);
		resetRequest.send();
	},


    /** @brief Show the modal to update the firmware from a file */
    showUpdateFirmwareModal: function()
    {
    	modal.emptyModal('large');
    	modal.setModalHeader('large', "Update firmware from file");
    	var form = `
   	      <form id="update-form">
    	    <a onclick="ui.installFirmwareUpdate();"><button>
    	        <img class="buttonimg" src="/icon-check-circle.png">Install firmware</button></a>
    	  </form>
    	  <div id="progress" class="graph" style="display:none;">
		    <div id="bar" style="width: 0"></div>
		  </div>
    	`;
    	modal.appendToModal('large', form);
    	modal.showModal('large');
    },

    /** Over-the-air updates */

    /** @brief fetch a list of firmware release available from Github */
    populateReleasesDropdown: function(selectId)
    {
    	var releases = undefined;
    	var getReleasesRequest = new XMLHttpRequest();
    	var select = document.getElementById(selectId);

    	getReleasesRequest.onload = function()
    	{
    		var releases = JSON.parse(this.responseText);

    		for ( let r = 0; r < releases.length; r++ )
    		{
    			// each release can have multiple assets (sine vs foc), step through them
    			for ( let a = 0; a < releases[r].assets.length; a++ )
    			{
    				if ( releases[r].assets[a].name.endsWith('.bin') )
    				{
    					var rName = releases[r].tag_name + " : " + releases[r].assets[a].name;
    					var rUrl = releases[r].assets[a].browser_download_url;
        				var selection = "<option value=\"" + rUrl + "\">" + rName + "</option>";
        				select.innerHTML += selection;
    				}

    			}
    		}
    	};

    	getReleasesRequest.onerror = function()
		{
			alert("error");
		};

		getReleasesRequest.open("GET", ui.githubFirmwareReleaseURL, true);
		getReleasesRequest.send();
    },

    /** @brief bring up the modal for installing a new firmware over-the-air */
    showOTAUpdateFirmwareModal: function() {
    	// empty the modal in case there's still something in there
    	modal.emptyModal('large');
    	modal.setModalHeader('large', 'Over the air firmware update');

    	// Insert the form
    	var form = `
    	  <form id="ota-update-form">
    	    <p>Choose a release to install</p>
    	    <select id="ota-release"></select>
    	    <a onclick="ui.installOTAFirmwareUpdate();"><button>
    	        <img class="buttonimg" src="/icon-check-circle.png">Install firmware</button></a>
    	  </form>
          <div id="ota-release-selected-div" style="display:none;"></div>
    	  <div id="progress" class="graph" style="display:none;">
		    <div id="bar" style="width: 0"></div>
		  </div>
    	`;
    	modal.appendToModal('large', form);

    	// get list of available releases
    	ui.populateReleasesDropdown('ota-release');

        modal.showModal('large');
    },

    /** @brief install over-the-air update */
    installOTAFirmwareUpdate: function()
    {
    	console.log("installOTAFirmwareUpdate start");
    	// get release selected
    	var releaseURL = document.getElementById('ota-release').value;

    	// hide the form
    	var otaUpdateForm = document.getElementById('ota-update-form').display = 'none';

    	// Display what version we're installing
    	var otaReleaseSelectedDiv = document.getElementById('ota-release-selected-div');
    	otaReleaseSelectedDiv.innerHTML += "<p>Installing firmware from " + releaseURL;

    	// fetch the release
    	var releaseRequest = new XMLHttpRequest();
    	releaseRequest.responseType = "blob";
    	releaseRequest.onload = function()
    	{
    		var releaseBlob = releaseRequest.response;
    		console.log(releaseBlob);
    		// build form we will submit to /edit to upload the file blob
    		var editFormData = new FormData();
    		editFormData.append("updatefile", releaseBlob, "stm32.bin");
    		var uploadRequest = new XMLHttpRequest();
    		uploadRequest.onload = function(response)
    		{
    			console.log(response);
    		}
    		uploadRequest.open("POST", "/edit");
    		uploadRequest.send(editFormData);
    	}
    	releaseRequest.open("GET", releaseURL);
    	releaseRequest.send();
    },

    /** @brief uploads file to web server, if bin-file uploaded, starts a firmware upgrade */
    doOTAUpdate: function()
	{
		// disable automatic refreshing while we update
		autoRefresh = false;

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
				runUpdate(-1, "/" + file);
			}
			document.getElementById("bar").innerHTML = "<p>Upload complete</p>";
			setTimeout(function() { document.getElementById("bar").innerHTML = "" }, 5000);
		}

		xmlhttp.open("POST", "/edit");
		xmlhttp.send(fd);
	},

	/** bootloader */

	swdUpdate: function()
	{
		//
	},


    /**
     * ~~~ PARAMETERS ~~~
     */

    /** @brief Show modal box with the result of parameter update */
    showParamUpdateModal: async function(param, value)
    {
    	var c = 'set ' + param + ' ' + value;
    	modal.emptyModal('small');
    	modal.showModal('small');
    	modal.appendToModal('small', 'Setting ' + param + ' to ' + value + "<br>");
    	inverter.sendCmd(c, function(reply)
		{
			modal.appendToModal('small', reply);
		});
		await sleep(2000);
		modal.hideModal('small');
    },

    /** @brief Show confirmation that params have been saved */
    showParamsSavedModal: async function()
    {
    	ui.sendCmd('save');
    	modal.emptyModal('small');
    	var msg = "<p style=\"padding:20px;text-align:center;\">Parameters saved</p>";
    	modal.appendToModal('small', msg);
    	modal.showModal('small');
    	await sleep(2000);
    	modal.hideModal('small');
    },

    /** @brief Show a modal box asking user to confirm if they wish to restore params to those saved in flash */
	showRestoreParamsFromFlashConfirmationModal: function()
	{
		modal.emptyModal('small');
		var msg = "<p>Are you sure you want to discard any unsaved parameter settings and revert to the last saved state?";
		msg += "<div style=\"display:flex;\">";
		msg += "<button onclick=\"ui.restoreParamsFromFlash();\"><img class=\"buttonimg\" src=\"/icon-rotate-ccw.png\">Restore</button>";
		msg += "<button onclick=\"modal.hideModal('small');\"><img class=\"buttonimg\" src=\"/icon-x-square.png\">Cancel</button>";
		msg += "</div>";
		modal.appendToModal('small', msg);
		modal.showModal('small');
	},

	/** @brief Roll back any changes made to params to last saved state */
	restoreParamsFromFlash: function()
	{
		modal.hideModal('small');
		ui.sendCmd('load');
		ui.refresh();
	},

    /** @brief Send params to the online Parameter Database */
	parameterSubmit: function()
	{
		inverter.getParamList(function(values)
		{
			document.getElementById("parameters_json").value = paramsCache.getJson();
			document.getElementById("paramdb").submit();
		}, true);
	},

	/** @brief toggles visibility of parameter category
	 * @param name name of category to show/hide */
	toggleVisibility: function(name)
	{
		var rows = document.getElementById("params").rows;
		var found = false;
		ui.categoryVisible[name] = false;

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
				found = rows[i].cells.length == 1 && (rows[i].cells[0].innerText.replace(/[\n\r]/g, '').endsWith(name) || !name);

				if (found)
				{
					var str = rows[i].cells[0].firstChild.firstChild.nodeValue;
					rows[i].cells[0].firstChild.firstChild.nodeValue = (str.startsWith('-') ? '+' : '-') + str.substring(1);
				}
			}
		}
	},

	/** @brief Loads a parameterset from json file and sends each parameter to the inverter */
	loadParametersFromFile: function()
	{
	    modal.setModalHeader('large', "Loading parameters");
	    modal.emptyModal('large');
	    modal.showModal('large');

		var file = document.getElementById('paramfile');

		if(file.files.length)
		{
			var reader = new FileReader();

			reader.onload = function(e)
			{
				var params = JSON.parse(e.target.result);
				inverter.setParam(params, 0);
			};

			reader.readAsBinaryString(file.files[0]);
		}
	},

	/** @brief fetch a parameter set from the Parameter database */
	parameterDatabaseFetchParameterSet: function(token, replyFunc)
	{
		var fetchTokenRequest = new XMLHttpRequest();
		var req = "https://openinverter.org/parameters/api.php?token=" + token;

		fetchTokenRequest.onload = function()
		{
			var params =  JSON.parse(this.responseText);
			replyFunc(params);
		}

		fetchTokenRequest.onerror = function()
		{
			alert("error");
		}

		fetchTokenRequest.open("GET", req, true);
		fetchTokenRequest.send();
	},

	/** @brief show the dialog allowing the user to subscribe to a parameter set from the Parameter Database */
	showSubscribeModal: function()
	{
		modal.emptyModal('large');
    	modal.setModalHeader('large', "Subscribe to parameter set");
    	var form = `
    	  <p>The Parameter Database is a way for OpenInverter community members to share parameter settings with each other.
    	  Community members may upload their parameter settings, along with other key information (e.g., inverter and motor type), to the database.
    	  This provides a single place to use as a reference when trying to determine the correct parameters to use for your hardware.</p>
    	  <p>You can browse the Parameter Database <a href="https://openinverter.org/parameters/">here</a>.</p>
    	  <p>You may choose to 'subscribe' to a parameter set. Your inverter will automatically synchronise its settings with those in the Parameter Database as they are adjusted and refined.

    	  Enter the subscription token for the parameter set you wish to subscribe to in the box below.</p>
    	  <p>Note: your inverter needs internet access for this feature to work.</p>

   	      <form id="parameter-subscribe-form">
   	          Subscription token : <input id="subscription-token" type="text" size="40">
    	      <a onclick="ui.parameterDatabaseSubscribe();"><button>
    	          <img class="buttonimg" src="/icon-check-circle.png">Subscribe</button></a>
    	  </form>

    	  <div id="progress" class="graph" style="display:none;">
		    <div id="bar" style="width: 0"></div>
		  </div>
    	`;
    	modal.appendToModal('large', form);
    	modal.showModal('large');
	},


	/** @brief create subscription.js file with token and timestamp */
	parameterDatabaseSaveSubscription: function(token, timestamp)
	{
		var saveSubscriptionRequest = new XMLHttpRequest();
		var formData = new FormData();
		var subs = "subscription = { 'timestamp': '" + timestamp + "', 'token': '" + token + "' };";
		var blob = new Blob([subs], { type: "text/javascript"});
		formData.append("file", blob, "subscription.js");
		saveSubscriptionRequest.open("POST", "/edit");
		saveSubscriptionRequest.send(formData);
		//FIXME handle error
	},

	/** @brief subscribe to a parameter set from the Parameter Database.
	 *
	 * This function creates a file called subscription.js on the spiffs
	 * filesystem, with contents that look like this:
	 *     subscription = { 'timestamp': <timestamp>, 'token': <token> }
	 *
	 * token : token identifying a parameter set in the Parameter Database
	 * timestamp : timestamp the above parameter set was last updated.
	 */
	parameterDatabaseSubscribe: function()
	{
		// Get the token entered by the user
		var subscriptionToken = document.getElementById('subscription-token').value;

		modal.emptyModal('large');
		modal.appendToModal('large', "<p>Applying parameter set " + subscriptionToken);

		// Fetch the parameter set from the Parameter Database
		ui.parameterDatabaseFetchParameterSet(subscriptionToken,
			function(params)
			{
				// Save subscription.js file to spiffs
				ui.parameterDatabaseSaveSubscription(subscriptionToken, params.timestamp);

				// Apply the params to the inverter
				inverter.setParam(params, 0);
			});
	},

	/** @brief pop up modal asking user to confirm that they wish to cancel
	 * their subscription to parameter set.
	 */
	parameterDatabaseShowUnsubscribeDialog: function()
	{
		modal.emptyModal('small');
		var msg = "<p>Are you sure you want to unsubscribe from the Parameter Database parameter set?</p>";
        msg += "<div style=\"display:flex\">";
        msg += "<button onclick=\"ui.parameterDatabaseCancelSubscription();\"><img src=\"/icon-trash-2.png\">Unsubscribe</button>";
        msg += "<button onclick=\"modal.hideModal('small');\"><img src=\"/icon-x-square.png\">Cancel</button>";
        msg += "</div>";
        modal.appendToModal('small', msg);
        modal.showModal('small');
	},

	parameterDatabaseCancelSubscription: function()
	{
		ui.deleteFile('/subscription.js');
		modal.hideModal('small');
	},

	/** @brief Pull down the parameter set from the Parameter Database. If there are updates,
	 * ask the user if they want to apply them.
	 */
	parameterDatabaseCheckForUpdates: function()
	{
		console.log("Checking for parameter database updates");
		if (subscription)
		{
			// Fetch the latest param set from the db
			ui.parameterDatabaseFetchParameterSet(subscription.token,
				function(params)
				{
					if ( subscription.timestamp != params.timestamp )
					{
						console.log("Found updates in the parameter database");
						// Show modal asking if params should be updated
						modal.emptyModal('small');
						msg = "<p>A new version of the parameter set you are subscribed to in the Parameter Database is available. ";
						msg += "Would you like to apply this update to the inverter?</p>";
						msg += "<div style=\"display:flex\">";
						msg += "<button onclick=\"ui.parameterDatabaseApplyUpdates();\"><img src=\"/icon-check-circle.png\">Apply</button>";
						msg += "<button onclick=\"modal.hideModal('small');\"><img src=\"/icon-x-square.png\">Cancel</button>";
						modal.appendToModal('small', msg);
						modal.showModal('small');

						// Save new params for later use
						ui.paramUpdates = params;
					}
				});
		}
	},

	parameterDatabaseApplyUpdates: function()
	{
		modal.hideModal('small');
		modal.emptyModal('large');
		modal.setModalHeader('large', 'Applying updates from Parameter Database');
		modal.showModal('large');
		inverter.setParam(ui.paramUpdates, 0);
		ui.parameterDatabaseSaveSubscription(subscription.token, ui.paramUpdates.timestamp);
	},

/*
	checkSubscribedParameterSet: function()
	{
		if (subscription)
		{
			ui.checkToken(subscription.token, 'Checking your parameter subscription ' + subscription.token, false);
		}
	},
*/

	/* If a valid token is entered, the belonging dataset is downloaded
	 * and applied to the inverter. Token and timestamp are saved to ESP filesystem
	 * Token example 5f4d8fa6-b6a4-4f87-9a28-4363bdac5dc9 */
/*
	checkToken: function(token, message, forceUpdate)
	{
		var expr = /^[0-9a-f]{8}\-[0-9a-f]{4}\-[0-9a-f]{4}\-[0-9a-f]{4}\-[0-9a-f]{12}$/i;

		if (expr.test(token))
		{
			var xmlhttp = new XMLHttpRequest();
			var req = "https://openinverter.org/parameters/api.php?token=" + token;

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

					inverter.setParam(params, 0);

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
	},
*/

	/**
	 * ~~~ SPOT VALUES ~~~
	 */


	/**
	 * ~~~ PLOT & GAUGE ~~~
	 */


    /** @brief Add new field chooser to plot configuration form */
	addPlotItem: function()
	{
		// Get the form
		var plotFields = document.getElementById("plotConfiguration");

		// container for the two drop downs
		var selectDiv = document.createElement("div");
		selectDiv.classList.add('plotField');
		plotFields.appendChild(selectDiv);

		// Create a drop down and populate it with the possible spot values
		var selectSpotValue = document.createElement("select");
		selectSpotValue.classList.add('plotFieldSelect');
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

		// Create the left/right drop down
		var selectLeftRight = document.createElement("select");
		selectLeftRight.classList.add("leftright");

		var optionLeft = document.createElement("option");
		optionLeft.value = 'left';
		optionLeft.text = 'left';
		selectLeftRight.appendChild(optionLeft);

		var optionRight = document.createElement("option");
		optionRight.value = 'right';
		optionRight.text = 'right';
		selectLeftRight.appendChild(optionRight);
		selectDiv.appendChild(selectLeftRight);

		// Add the delete button
		var deleteButton = document.createElement("button");
		var deleteButtonImg = document.createElement('img');
		deleteButtonImg.src = '/icon-trash.png';
		deleteButton.appendChild(deleteButtonImg);
		deleteButton.onclick = function() { this.parentNode.remove(); };
		selectDiv.appendChild(deleteButton);
	},

    /** @brief get the current configuration of the plot. I.e., what values should it show. */
	getPlotItems: function()
	{
		var items = {};
    	items.names = new Array();
	    items.axes = new Array();
		var formItems = document.forms["plotConfiguration"].elements;
		for ( var i = 0; i < formItems.length; i++ )
		{
            // Gather up field selections
			if ( formItems[i].type === 'select-one' && formItems[i].classList.contains('plotFieldSelect') )
			{
				items.names.push(formItems[i].value);
			}

			// Gather up left/right selections
			if ( formItems[i].type === 'select-one' && formItems[i].classList.contains('leftright') )
			{
				items.axes.push(formItems[i].value);
			}
		}
        return items;
	},

	/**
	 * DATA LOGGER
	 */

	/**
	 * CAN MAPPING
	 */

	/** @brief Add a CAN Mapping */
	canMapping: function()
	{
		// fetch values from form
		var direction = document.getElementById('txrx').value;
		var name = document.getElementById('add-can-mapping-spot-value-drop-down').value;
		var canid = document.getElementById('canid').value;
	    var canpos = document.getElementById('canpos').value;
	    var canbits = document.getElementById('canbits').value;
	    var cangain = document.getElementById('cangain').value;
	    // send new CAN mapping to inverter
	    inverter.canMapping(direction, name, canid, canpos, canbits, cangain);
	    // hide form
	    modal.hideModal('can-mapping');
	    // refresh CAN mapping table
	    ui.populateExistingCanMappingTable();
	},

	/** @brief Populate the table of existing CAN mappings */
	populateExistingCanMappingTable: function()
	{
		var existigCanMappingTable = document.getElementById("existingCanMappingTable");
		// emtpy the table
		while (existigCanMappingTable.rows.length > 1) existigCanMappingTable.deleteRow(1);
		inverter.getParamList(function(values) {
			for (var name in values) {
				var param = values[name];
				if (typeof param.canid !== 'undefined'){
					var tr = existigCanMappingTable.insertRow(-1);
					// name of spot value
					var canNameCell = tr.insertCell(-1);
					canNameCell.innerHTML = name;
					// tx/rx
					var canTxRxCell = tr.insertCell(-1);
					canTxRxCell.innerHTML = param.isrx ? "Receive" : "Transmit";;
					// canid
					var canIdCell = tr.insertCell(-1);
		        	canIdCell.innerHTML = param.canid;
		        	// canoffset
					var canOffsetCell = tr.insertCell(-1);
		        	canOffsetCell.innerHTML = param.canoffset;
		        	// canlength
					var canLengthCell = tr.insertCell(-1);
		        	canLengthCell.innerHTML = param.canlength;
		        	// cangain
					var canGainCell = tr.insertCell(-1);
		        	canGainCell.innerHTML = param.cangain;
		        	// delete button
					var canDeleteCell = tr.insertCell(-1);
					var cmd = "inverter.canMapping('del', '" + name + "');ui.populateExistingCanMappingTable();";
		        	canDeleteCell.innerHTML = "<button onclick=\"" + cmd + "\"><img class=\"buttonimg\" src=\"/icon-trash.png\">Delete mapping</button>";
				}
			}
		});
	},

	/** @brief Populate the 'spot value' drop-down on the 'Add new CAN mapping' form */
	populateSpotValueDropDown: function()
	{
		var select = document.getElementById("add-can-mapping-spot-value-drop-down");
		inverter.getParamList(function(values) {
			for (var name in values) {
				var param = values[name];
				var el = document.createElement("option");
				el.textContent = name;
				el.value = name;
				select.appendChild(el);
			}
		});
	},


	/**
	 * FILES
	 */

    /** @brief populate the list of files table */
	populateFileList: function()
	{
		var filesTable = document.getElementById('filesTable');
		// emtpy the table
		while (filesTable.rows.length > 1) filesTable.deleteRow(1);
		// fetch file list and populate table
		inverter.getFiles(function(files)
		{
			for ( var i = 0; i < files.length; i++ )
			{
				var tr = filesTable.insertRow(-1);
				// filename name
				var fileNameCell = tr.insertCell(-1);
				fileNameCell.innerHTML = "<a href=" + files[i]['name'] + ">" + files[i]['name'] + "</a>";
				// delete button
				var deleteFileCell = tr.insertCell(-1);
				deleteFileCell.innerHTML = "<button onclick=\"ui.showDeleteFileConfirmationModal('" + files[i]['name'] + "');\"><img class=\"buttonimg\" src=\"/icon-trash.png\">Delete File</button>";
			}
		});
	},

	showDeleteFileConfirmationModal: function(filename)
	{
		modal.emptyModal('small');
		var msg = "<p>Are you sure you want to delete file '" + filename + "'?</p>";
		msg += "<div style=\"display:flex\">";
		msg += "<button onclick=\"ui.deleteFile('/" + filename + "');\"><img class=\"buttonimg\" src=\"/icon-trash.png\">Delete file</button>";
		msg += "<button onclick=\"modal.hideModal('small');\"><img class=\"buttonimg\" src=\"/icon-x-square.png\">Cancel</button>";
		msg += "</div>";
		modal.appendToModal('small', msg);
		modal.showModal('small');
	},

	deleteFile: function(filename)
	{
		var deleteFileRequest = new XMLHttpRequest();
		var params = {}
		params.f = "/" + filename;
		deleteFileRequest.onload = function()
    	{
    		// re-build file list
    		ui.populateFileList();
    		// hide modal
    		modal.hideModal('small');
    	};

    	deleteFileRequest.onerror = function()
		{
			alert("error");
		};

		deleteFileRequest.open("DELETE", "/edit?f=" + filename, true);
		deleteFileRequest.send();

	},

	/**
	 * WIFI SETTINGS
	 */
}
