// Simple Dialog Box Plugin by Taufik Nurrohman
// URL: http://www.dte.web.id + https://plus.google.com/108949996304093815163/about
// Licence: none

(function(a, b) {

  a.setDialog = function(uniqueId, set, config) {
    if (set === "open") {
      var div = b.createElement('div');
        div.className = 'dialog-box';
        div.id = uniqueId;
        div.innerHTML = '<div class="dialog-content">&nbsp;</div><h3 class="dialog-title"><a href="javascript:;" class="dialog-close" title="Close">&times;</a><a href="javascript:;" class="dialog-resize" title="Resize">&harr;</a></h3>';
      b.body.appendChild(div);
    }

    var dialog = b.getElementById(uniqueId), selected = null, defaults = {
      title: '',
      content: '',
      width: 300,
      height: 150,
      top: false,
      left: false
    };

    for (var i in config) { defaults[i] = (typeof config[i] !== 'undefined') ? config[i] : defaults[i]; }

    function _drag_init(e, el) {
      for (var i=0;i<b.getElementsByClassName('dialog-box').length;i++) b.getElementsByClassName('dialog-box')[i].style.zIndex = 9999;
      el.style.zIndex = 10000;
      var posX = e.clientX,
          posY = e.clientY,
          divTop = parseFloat(el.style.top.indexOf('%')>-1?el.offsetTop + el.offsetHeight/2:el.style.top),
          divLeft = parseFloat(el.style.left.indexOf('%')>-1?el.offsetLeft + el.offsetWidth/2:el.style.left)
      var diffX = posX - divLeft,
          diffY = posY - divTop;
      b.onmousemove = function(e){
          e = e || a.event;
          var posX = e.clientX,
              posY = e.clientY,
              aX = posX - diffX,
              aY = posY - diffY;
          el.style.left = aX + 'px';
          el.style.top = aY + 'px';
      }
    }

    dialog.className =  'dialog-box fixed-dialog-box';
    dialog.style.visibility = (set === "open") ? "visible" : "hidden";
    dialog.style.opacity = (set === "open") ? 1 : 0;
    dialog.style.width = defaults.width + 'px';
    dialog.style.height = defaults.height + 'px';
    dialog.style.top = (!defaults.top) ? "50%" : '0px';
    dialog.style.left = (!defaults.left) ? "50%" : '0px';
    dialog.style.marginTop = (!defaults.top) ? '-' + defaults.height/2 + 'px' : defaults.top + 'px';
    dialog.style.marginLeft = (!defaults.left) ? '-' + defaults.width/2 + 'px' : defaults.left + 'px';
    dialog.children[1].innerHTML = defaults.title+dialog.children[1].innerHTML;
    dialog.children[0].innerHTML = defaults.content;
    dialog.children[1].onmousedown = function(e) { if ((e.target || e.srcElement)===this) _drag_init(e, this.parentNode); };
    dialog.children[1].children[0].onclick = function() { a.setDialog(uniqueId, "close", {content:""}); };
    b.onmouseup = function() { b.onmousemove = function() {}; };
    if (set === "close") dialog.remove();
  };

})(window, document);