/*
 * Overrides window.open with a function that creates a
 * iframe and loads the url in that
 *
 * Use with a CSS containing this for best effect.

#uzbl_popup {
    background: white;
    position: absolute;
    left: 50px;
    right: 50px;
    top: 50px;
    height: 500px;
}

#uzbl_popup iframe {
    width: 100%;
    height: 100%;
}
*/

__orig_open = window.open

uzbl.open = function (url, name, specs, replace) {
	frame = uzbl.get_popup_frame()
	return __orig_open(url, frame.name, specs, replace)
}

/* Creates a popup iframe if doesn't exist */
uzbl.get_popup_frame = function() {
	frame = document.getElementsByName("uzbl_popup")
	if(frame.length == 0) {
		div = document.createElement('DIV')
		div.id = 'uzbl_popup'

		close = document.createElement('A')
		close.onclick = function() {
			document.body.removeChild(div)
		}
		close.appendChild(document.createTextNode('close'))
		div.appendChild(close)

		frame = document.createElement('IFRAME')
		frame.name = 'uzbl_popup'
		div.appendChild(frame)

		document.body.appendChild(div)
	} else {
		frame = frame[0];
	}
	return frame
}

window.open = uzbl.open
