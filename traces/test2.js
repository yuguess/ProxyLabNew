const TIMEOUT = 8000;
var tabs = require('lib/tabs');
var setupModule = function() {
    controller = mozmill.getBrowserController();
    controller.window.alert('test 2 begin');
    tabBrowser = new tabs.tabBrowser(controller);
    tabBrowser.closeAllTabs();
}

var teardownTest = function(test) {
    tabBrowser.closeAllTabs();
}

var test1 = function () {
	controller.click(new elementslib.Elem(controller.menus['file-menu']['menu_newNavigatorTab']));  
   	controller.open('http://www.google.com');
    controller.waitForPageLoad();
    controller.closeTab();
}

var test2 = function () {
	controller.click(new elementslib.Elem(controller.menus['file-menu']['menu_newNavigatorTab']));  
   	controller.open('http://www.cs.cmu.edu/~213');
    controller.waitForPageLoad();
}

var test3 = function () {
	controller.click(new elementslib.Elem(controller.menus['file-menu']['menu_newNavigatorTab']));  
   	controller.open('http://www.cs.cmu.edu/');
    controller.waitForPageLoad();
}

var test4 = function () {
	controller.click(new elementslib.Elem(controller.menus['file-menu']['menu_newNavigatorTab']));  
   	controller.open('http://www.nytimes.com');
    controller.waitForPageLoad();
}

var test5 = function () {
	controller.click(new elementslib.Elem(controller.menus['file-menu']['menu_newNavigatorTab']));  
   	controller.open('http://www.cnn.com/');
    controller.waitForPageLoad();
}

var test6 = function () {
	controller.click(new elementslib.Elem(controller.menus['file-menu']['menu_newNavigatorTab']));  
   	controller.open('http://www.youtube.com/');
    controller.waitForPageLoad();
}

var test7 = function () {
    tabBrowser.closeAllTabs();
}

