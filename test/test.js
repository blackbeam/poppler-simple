var target = 'file://' + __dirname + '/fixtures/rsl01000000001.pdf';
var poppler = require('../build/Release/poppler');
var path = require('path');
var fs = require('fs');

var doc, page, render;

module.exports = {
    all: {
        'Is module loaded?': function (test) {
            test.ok(poppler && poppler.PopplerDocument && poppler.PopplerPage);
            test.done();
        },
        'Open pdf': function (test) {
            doc = new poppler.PopplerDocument(target);
            test.equal(doc.isLinearized, true);
            test.equal(doc.pdfVersion, 'PDF-1.4');
            test.equal(doc.pageCount, 24);
            test.equal(doc.PDFMajorVersion, 1);
            test.equal(doc.PDFMinorVersion, 4);
            test.done();
        },
        'Open page': function (test) {
            page = new poppler.PopplerPage(doc, 1);
            test.equal(page.num, 1);
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
        'Add annotations to page' : function (test) {
            page.addAnnot({x1: 0.4, y1: 0.4, x2: 0.6, y2: 0.45});
            test.done();
        },
        'Rendering to file': function (test) {
            var out = page.renderToFile('/tmp/out.png', 'png', 150);
            test.deepEqual(out, {'type': 'file', 'path': '/tmp/out.png'});
            fs.unlinkSync(out.path);
            out = page.renderToFile('/tmp/out.jpeg', 'jpeg', 150, {
                quality: 50
            });
            test.deepEqual(out, {'type': 'file', 'path': '/tmp/out.jpeg'});
            fs.unlinkSync(out.path);
            out = page.renderToFile('/tmp/out.tiff', 'tiff', 150, {
                compression: "lzw"
            });
            test.deepEqual(out, {'type': 'file', 'path': '/tmp/out.tiff'});
            fs.unlinkSync(out.path);
            test.done();
        },
        'Remove annotations from page' : function (test) {
            page.deleteAnnots();
            test.done();
        },
        'Rendering slice to file': function (test) {
            var out = page.renderToFile('/tmp/out.jpeg', 'jpeg', 150, {
                quality: 100,
                slice: {
                    x: 0, y: 0, w: 1, h: 1
                }
            });
            test.deepEqual(out, {'type': 'file', 'path': '/tmp/out.jpeg'});
            fs.unlinkSync(out.path);
            test.done();
        },
        'Rendering to buffer': function (test) {
            var out = page.renderToBuffer('jpeg', 72, {
                quality: 100,
                slice: {
                    x: 0, y: 0, w: 1, h: 0.5
                }
            });
            test.equal(out.type, 'buffer');
            test.equal(out.format, 'jpeg');
            test.ok(Buffer.isBuffer(out.data));
            fs.writeFileSync('/tmp/slice.jpeg', out.data);
            fs.unlinkSync('/tmp/slice.jpeg');
            test.done();
        }
    }
};
