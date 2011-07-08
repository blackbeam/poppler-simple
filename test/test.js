var target = 'file://' + __dirname + '/fixtures/rsl01000000001.pdf';
var poppler = require('../build/default/poppler');
var path = require('path');
var fs = require('fs');

var doc, page, render;

module.exports = {
    all: {
        'Is module loaded?': function (test) {
            test.ok(poppler && poppler.PopplerDocument && poppler.PopplerPage);
            test.done()
        },
        'Open pdf': function (test) {
            doc = new poppler.PopplerDocument(target);
            test.equal(doc.isLinearized, true);
            test.equal(doc.pdfVersion, 'PDF-1.4');
            test.equal(doc.pageCount, 24);
            test.done();
        },
        'Open page': function (test) {
            page = new poppler.PopplerPage(doc, 1);
            test.deepEqual(page.images, [ { x1: 0, x2: 1246, y1: -0.15999999999996817, y2: 2383.84 } ]); 
            test.equal(page.index, 1);
            test.equal(page.height, 572);
            test.equal(page.width, 299);
            test.deepEqual(page.crop_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            test.done();
        },
        'Search for text': function (test) {
            var search_results = page.findText("Российская");
            test.deepEqual(search_results, [
                    {
                        x1: 0.21752508361204015,
                        x2: 0.38838531772575263,
                        y1: 0.8903321678321678,
                        y2: 0.9060664335664336
                    }
                ]
            );
            test.done();
        },
        'Rendering to raw pixbuf': function (test) {
            render = page.render(72, true);
            test.equal(render.type, 'pixbuf');
            test.equal(render.data.pixels.length, 514800);
            test.equal(render.data.height, 572);
            test.equal(render.data.width, 299);
            test.equal(render.data.has_alpha, false);
            test.done();
        },
        'Render to file': function (test) {
            render = page.render(90, false);
            test.equal(render.type, 'file');
            test.ok(render.path && render.path.length > 0);
            test.ok(path.existsSync(render.path));
            fs.unlinkSync(render.path);
            test.done();
        }
    }
}
