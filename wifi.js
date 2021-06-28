var wifi = {

	wifiValidatePasswordLength: function(pw)
	{
		document.getElementById("apsubmit").disabled = pw.length < 8;
	},

	populateWiFiTab: function()
	{
		console.log("updating wifi");
		var wifiTab = document.getElementById("wifi");
		var wifiFetchRequest = new XMLHttpRequest();
		wifiFetchRequest.onload = function()
		{
			wifiTab.innerHTML = this.responseText;
		}
		wifiFetchRequest.open("GET", "/wifi");
		wifiFetchRequest.send();
	},

	/*
	submitWiFiChange: function()
	{
		// get the form values
		var apSSID = document.getElementById("apSSID").value;
		var apPW = document.getElementById("apPW").value;
		var staSSID = document.getElementById("staSSID").value;
		var staPW = document.getElementById("staPW").value;
		// submit the changes
		var wifiTab = document.getElementById("wifi");
		var wifiUpdateRequest = new XMLHttpRequest();
		wifiUpdateRequest.onload = function()
		{
			wifiTab.innerHTML = this.responseText;
		}
		wifiUpdateRequest.open("POST", "/wifi")
		wifiUpdateRequest.send()
	},
	*/

}