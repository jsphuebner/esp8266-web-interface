
var modal = {
	
	/** @brief Show the large dialog
	 * @param modal name of the modal. Valid values : large, small, can-mapping. */
	showModal: function(modal)
	{
		var m = document.getElementById(modal + "-modal-overlay");
		if ( m !== undefined ){
			m.style.display = 'block';
		}
		else {
			console.log("warning, showModal, bad modal choice");
		}
	},

    /** @brief Hide the modal dialog */
	hideModal: function(modal)
	{
		var m = document.getElementById(modal + "-modal-overlay");
		if ( m !== undefined ){
			m.style.display = 'none';
		}
		else {
			console.log("warning, hideModal, bad modal choice");
		}
	},

	//* @brief Set the header text on the large modal */
	setModalHeader: function(modal, headerText)
	{
		var m = document.getElementById(modal + "-modal-header");
		if ( m !== undefined ){
			m.innerHTML = headerText;
		}
		else {
			console.log("warning, setModalHeader, bad modal choice");
		}
	},

	//* @brief Empty the contents of the large modal */
	emptyModal: function(modal)
	{
		var m = document.getElementById(modal + "-modal-content");
		console.log("modal " + m);
		if ( m !== undefined ){
			m.innerHTML = "";
		}
		else {
			console.log("warning, emptyModal, bad modal choice");
		}
	},

	//* @brief Append content to large modal */
	appendToModal: function(modal, appendText)
	{
		var modalId = modal + "-modal-content";
		console.log("appendToModal : looking for div " + modalId);
		var modalContent = document.getElementById(modalId);
		if ( modalContent !== undefined ){
			modalContent.innerHTML += appendText;
		}
		else {
			console.log("warning, appendToModal, bad modal choice");
		}
	},

	//* @brief auto-scroll large modal content */
	largeModalScrollToBottom(){
		var largeModalContent = document.getElementById("large-modal-content");
		largeModalContent.scrollTop = largeModalContent.scrollHeight;
	},

};