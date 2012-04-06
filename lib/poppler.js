try {
    module.exports = require('../build/Release/poppler');
} catch (e) {
    module.exports = require('../build/default/poppler');
}