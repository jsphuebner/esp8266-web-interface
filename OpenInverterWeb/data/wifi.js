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

}