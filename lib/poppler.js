try {
    module.exports = require('../build/Release/poppler');
} catch (e) {
    module.exports = require('../build/default/poppler');
}

module.exports.PopplerDocument.prototype.getPage = function (num) {
    return new module.exports.PopplerPage(this, num);
};
