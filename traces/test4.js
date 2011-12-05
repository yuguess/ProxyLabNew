const TIMEOUT = 5000;
var tabs = require('lib/tabs');
var setupModule = function() {
    controller = mozmill.getBrowserController();
    controller.window.alert('test 4 begin');
    tabBrowser = new tabs.tabBrowser(controller);
    tabBrowser.closeAllTabs();
}

var teardownTest = function(test) {
    tabBrowser.closeAllTabs();
}

var test1 = function () {
    for (var i = 0; i < 5; i++) {
        tabBrowser.openTab();
   	    controller.open('http://www.calvinklein.com/family/index.jsp?categoryId=4386336&ab=hp_113011handimage');
        controller.waitForPageLoad();
        tabBrowser.closeTab("shortcut");
    }
}
