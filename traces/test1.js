const TIMEOUT = 8000;
var tabs = require('lib/tabs');
var setupModule = function (module) {
   module.controller = mozmill.getBrowserController();
}

var setupTest = function(test) {
    // Open a tab for wikipedia
    controller.window.alert('test 1 begin');
    controller.click(new elementslib.Elem(controller.menus['file-menu']['menu_newNavigatorTab']));
    controller.open('http://www.wikipedia.org');
    tabBrowser = new tabs.tabBrowser(controller);
    tabBrowser.closeAllTabs();
}

var teardownTest = function(test) {
    tabBrowser.closeAllTabs();
}

var teardownTest = function(test) {
    controller
}

var testFoo1 = function () {
    controller.window.alert('Doing a test');
}

var testFoo2 = function() {
    controller.window.alert('Doing another test');
}
