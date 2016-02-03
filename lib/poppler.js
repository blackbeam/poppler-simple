(function () {
    "use strict";
    var Promise = require("bluebird");
    try {
        try {
            module.exports = require('../build/Debug/poppler');
        } catch (e) {
            module.exports = require('../build/Release/poppler');
        }
    } catch (e) {
        module.exports = require('../build/default/poppler');
    }

    module.exports.PopplerDocument.prototype.getPage = function (num) {
        return new module.exports.PopplerPage(this, num);
    };

    if (module.exports.PopplerDocument.POPPLER_VERSION_MINOR < 23) {
        var _renderToFile = module.exports.PopplerPage.prototype.renderToFile;
        var _renderToBuffer = module.exports.PopplerPage.prototype.renderToBuffer;
        module.exports.PopplerPage.prototype.renderToFile = function () {
            var self = this;
            var args = Array.prototype.slice.call(arguments);
            if ('function' === typeof args[args.length - 1]) {
                var cb = args.pop();
                process.nextTick(function() {
                    try {
                        var result = _renderToFile.apply(self, args);
                        cb(null, result);
                    } catch (error) {
                        cb(error, null);
                    }
                });
            } else {
                return _renderToFile.apply(self, args);
            }
        };
        module.exports.PopplerPage.prototype.renderToBuffer = function () {
            var self = this;
            var args = Array.prototype.slice.call(arguments);
            if ('function' === typeof args[args.length - 1]) {
                var cb = args.pop();
                process.nextTick(function() {
                    try {
                        var result = _renderToBuffer.apply(self, args);
                        cb(null, result);
                    } catch (error) {
                        cb(error, null);
                    }
                });
            } else {
                return _renderToBuffer.apply(self, args);
            }
        };
    }

    module.exports.PopplerPage.prototype.renderToFileAsync = function () {
        var self = this;
        var args = Array.prototype.slice.call(arguments);
        return new Promise(function (resolve, reject) {
            if (typeof args[args.length - 1] === 'function') {
                args.pop();
            }
            args.push(function (err, result) {
                if (err) {
                    reject(err);
                } else {
                    resolve(result);
                }
            });
            self.renderToFile.apply(self, args);
        });
    };

    module.exports.PopplerPage.prototype.renderToBufferAsync = function () {
        var self = this;
        var args = Array.prototype.slice.call(arguments);
        return new Promise(function (resolve, reject) {
            if (typeof args[args.length - 1] === 'function') {
                args.pop();
            }
            args.push(function (err, result) {
                if (err) {
                    reject(err);
                } else {
                    resolve(result);
                }
            });
            self.renderToBuffer.apply(self, args);
        });
    };
})();
