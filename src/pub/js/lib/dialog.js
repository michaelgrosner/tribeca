// Simple Dialog Box Plugin by Taufik Nurrohman
// URL: http://www.dte.web.id + https://plus.google.com/108949996304093815163/about
// Licence: none

(function(a, b) {

	var uniqueId = new Date().getTime();

	(function() { // Create the dialog box markup
		var div = b.createElement('div'),
			ovr = b.createElement('div');
			div.className = 'dialog-box';
			div.id = 'dialog-box-' + uniqueId;
			div.innerHTML = '<h3 class="dialog-title">&nbsp;</h3><a href="javascript:;" class="dialog-minmax" title="Minimize">&ndash;</a><a href="javascript:;" class="dialog-close" title="Close">&times;</a><div class="dialog-content">&nbsp;</div><div class="dialog-action"></div>';
			ovr.className = 'dialog-box-overlay';
		b.body.appendChild(div);
		b.body.appendChild(ovr);
	})();

	var maximize = false,
		dialog = b.getElementById('dialog-box-' + uniqueId), // The HTML of dialog box
		dialog_title = dialog.children[0],
		dialog_minmax = dialog.children[1],
		dialog_close = dialog.children[2],
		dialog_content = dialog.children[3],
		dialog_action = dialog.children[4],
		dialog_overlay = dialog.nextSibling;

	a.setDialog = function(set, config) {

		var selected = null, // Object of the element to be moved
			x_pos = 0,
			y_pos = 0, // Stores x & y coordinates of the mouse pointer
			x_elem = 0,
			y_elem = 0, // Stores top, left values (edge) of the element
			defaults = {
				title: dialog_title.innerHTML,
				content: dialog_content.innerHTML,
				width: 300,
				height: 150,
				top: false,
				left: false,
				buttons: {
					"Close": function() {
						setDialog('close');
					}
				},
				specialClass: "",
				fixed: true,
				overlay: false
			}; // Default options...

		for (var i in config) { defaults[i] = (typeof(config[i])) ? config[i] : defaults[i]; }

		// Will be called when user starts dragging an element
		function _drag_init(elem) {
			selected = elem; // Store the object of the element which needs to be moved
			x_elem = x_pos - selected.offsetLeft;
			y_elem = y_pos - selected.offsetTop;
		}

		// Will be called when user dragging an element
		function _move_elem(e) {
			x_pos = b.all ? a.event.clientX : e.pageX;
			y_pos = b.all ? a.event.clientY : e.pageY;
			if (selected !== null) {
				selected.style.left = !defaults.left ? ((x_pos - x_elem) + selected.offsetWidth/2) + 'px' : ((x_pos - x_elem) - defaults.left) + 'px';
				selected.style.top = !defaults.top ? ((y_pos - y_elem) + selected.offsetHeight/2) + 'px' : ((y_pos - y_elem) - defaults.top) + 'px';
			}
		}

		// Destroy the object when we are done
		function _destroy() {
			selected = null;
		}

		dialog.className =  "dialog-box " + (defaults.fixed ? 'fixed-dialog-box ' : '') + defaults.specialClass;
		dialog.style.visibility = (set == "open") ? "visible" : "hidden";
		dialog.style.opacity = (set == "open") ? 1 : 0;
		dialog.style.width = defaults.width + 'px';
		dialog.style.height = defaults.height + 'px';
		dialog.style.top = (!defaults.top) ? "50%" : '0px';
		dialog.style.left = (!defaults.left) ? "50%" : '0px';
		dialog.style.marginTop = (!defaults.top) ? '-' + defaults.height/2 + 'px' : defaults.top + 'px';
		dialog.style.marginLeft = (!defaults.left) ? '-' + defaults.width/2 + 'px' : defaults.left + 'px';
		dialog_title.innerHTML = defaults.title;
		dialog_content.innerHTML = defaults.content;
		dialog_action.innerHTML = "";
		dialog_overlay.style.display = (set == "open" && defaults.overlay) ? "block" : "none";

		if (defaults.buttons) {
			for (var j in defaults.buttons) {
				var btn = b.createElement('a');
					btn.className = 'btn';
					btn.href = 'javascript:;';
					btn.innerHTML = j;
					btn.onclick = defaults.buttons[j];
				dialog_action.appendChild(btn);
			}
		} else {
			dialog_action.innerHTML = '&nbsp;';
		}

		// Bind the draggable function here...
		dialog_title.onmousedown = function() {
			_drag_init(this.parentNode);
			return false;
		};

		dialog_minmax.innerHTML = '&ndash;';
		dialog_minmax.title = 'Minimize';
		dialog_minmax.onclick = dialogMinMax;

		dialog_close.onclick = function() {
			setDialog("close", {content:""});
		};

		b.onmousemove = _move_elem;
		b.onmouseup = _destroy;

		maximize = (set == "open") ? true : false;

	};

	// Maximized or minimized dialog box
	function dialogMinMax() {
		if (maximize) {
			dialog.className += ' minimize';
			dialog_minmax.innerHTML = '+';
			dialog_minmax.title = dialog_title.innerHTML.replace(/<.*?>/g,"");
			maximize = false;
		} else {
			dialog.className = dialog.className.replace(/(^| )minimize($| )/g, "");
			dialog_minmax.innerHTML = '&ndash;';
			dialog_minmax.title = 'Minimize';
			maximize = true;
		}
	}

})(window, document);