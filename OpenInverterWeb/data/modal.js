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

var modal = {
	
	/** @brief Show the large dialog
	 * @param modal - name of the modal. Valid values : large, small */
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

    /** @brief Hide the modal dialog
     * @param modal - name of the modal. Valid values : large, small */
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

	/** @brief Set the header text on the modal
	 * @param modal - name of the modal. Valid values : large, small
	 * @param headerText - string to place into the header field of the modal */
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

	/** @brief Empty the contents of the large modal
	 * @param modal - name of the modal. Valid values : large, small */
	emptyModal: function(modal)
	{
		var m = document.getElementById(modal + "-modal-content");
		if ( m !== undefined ){
			m.innerHTML = "";
		}
		else {
			console.log("warning, emptyModal, bad modal choice");
		}
	},

	/** @brief Append content to large modal
	 * @param modal - name of the modal. Valid values : large, small
	 * @appendText - string to append to the body of the modal. Note, can be html. */
	appendToModal: function(modal, appendText)
	{
		var modalId = modal + "-modal-content";
		//console.log("appendToModal : looking for div " + modalId);
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