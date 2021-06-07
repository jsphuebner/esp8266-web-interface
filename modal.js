
var modal = {

	// LARGE MODAL
	
	/** @brief Show the large modal dialog */
	showLargeModal: function()
	{
        document.getElementById("large-modal-overlay").style.display = 'block';
	},

    /** @brief Hide the large modal dialog */
	hideLargeModal: function()
	{
        document.getElementById("large-modal-overlay").style.display = 'none';
	},

	//* @brief Set the header text on the large modal */
	setLargeModalHeader: function(headerText)
	{
        document.getElementById("large-modal-header").innerHTML = headerText;
	},

	//* @brief Empty the contents of the large modal */
	emptyLargeModal: function()
	{
		document.getElementById("large-modal-content").innerHTML = "";
	},

	//* @brief Append content to large modal */
	appendToLargeModal: function(appendText)
	{
        document.getElementById("large-modal-content").innerHTML += appendText;
	},

	//* @brief auto-scroll large modal content */
	largeModalScrollToBottom(){
		var largeModalContent = document.getElementById("large-modal-content");
		largeModalContent.scrollTop = largeModalContent.scrollHeight;
	},

    // SMALL MODAL

    /** @brief show the small modal pop up */
	showSmallModal: function()
	{
        document.getElementById("small-modal-overlay").style.display = 'block';
	},

    /** @brief hide the small modal pop up */
	hideSmallModal: function()
	{
        document.getElementById("small-modal-overlay").style.display = 'none';
	},

	/** @brief Set the header text on the small modal */
	setSmallModalHeader: function(headerText)
	{
        document.getElementById("small-modal-header").innerHTML = headerText;
	},

	/** @brief Empty the contents of the small modal */
	emptySmallModal: function()
	{
		document.getElementById("small-modal-overlay-content").innerHTML = "";
	}
};