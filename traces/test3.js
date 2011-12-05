const TIMEOUT = 5000;
var tabs = require('lib/tabs');
var setupModule = function() {
    controller = mozmill.getBrowserController();
    controller.window.alert('test 3 begin');
    tabBrowser = new tabs.tabBrowser(controller);
    tabBrowser.closeAllTabs();
}

var teardownTest = function(test) {
    tabBrowser.closeAllTabs();
}

var test1 = function () {
    for (var i = 0; i < 5; i++) {
        tabBrowser.openTab();
   	    controller.open('http://www.nytimes.com');
        controller.waitForPageLoad();
        tabBrowser.closeTab("shortcut");
    }
}

var test2 = function () {
    for (var i = 0; i < 5; i++) {
        tabBrowser.openTab();
   	    controller.open('http://www.cnn.com');
        controller.waitForPageLoad();
        tabBrowser.closeTab("shortcut");
    }
}

var test3 = function () {
    for (var i = 0; i < 5; i++) {
        tabBrowser.openTab();
   	    controller.open('http://www.yahoo.com');
        controller.waitForPageLoad();
        tabBrowser.closeTab("shortcut");
    }
}
