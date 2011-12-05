const TIMEOUT = 5000;
var tabs = require('lib/tabs');
var setupModule = function() {
    controller = mozmill.getBrowserController();
    controller.window.alert('test 6 begin');
    tabBrowser = new tabs.tabBrowser(controller);
    tabBrowser.closeAllTabs();
}

var teardownTest = function(test) {
    tabBrowser.closeAllTabs();
}

var test1 = function () {
    for (var i = 0; i < 10; i++) {
        tabBrowser.openTab();
        controller.open('http://www.nytimes.com');
    }
    controller.waitForPageLoad();
}

var test2 = function () {
    for (var i = 0; i < 10; i++) {
        tabBrowser.openTab();
        controller.open('http://www.cnn.com');
    }
    controller.waitForPageLoad();
}
