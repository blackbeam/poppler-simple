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
        'Open not existing pdf': function (test) {
            test.throws(function () {
                doc = new poppler.PopplerDocument('file:///123.pdf');
            }, 'Couldn\'t open file - fopen error. Errno: 2.');
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
        'Open not existing page': function (test) {
            test.throws(function () {
                page = new poppler.PopplerPage(doc, 65536);
            }, 'Page number out of bounds');
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
            search_results = page.findText("qwerty");
            test.deepEqual(search_results, []);
            search_results = page.findText("я");
            test.deepEqual(search_results, [
                {
                    x1: 0.3716160869565219, x2: 0.38838531772575263,
                    y1: 0.8903321678321678, y2: 0.9060664335664336
                }, {
                    x1: 0.5230859866220736, x2: 0.5398552173913044,
                    y1: 0.8903321678321678, y2: 0.9060664335664336
                }, {
                    x1: 0.5900528762541806, x2: 0.6068221070234113,
                    y1: 0.863479020979021, y2: 0.8792132867132867
                }, {
                    x1: 0.2310341137123746, x2: 0.24809096989966553,
                    y1: 0.6241083916083916, y2: 0.6378496503496504
                }, {
                    x1: 0.5485525083612042, x2: 0.5656093645484952,
                    y1: 0.5951573426573427, y2: 0.6088986013986013
                }, {
                    x1: 0.4864352842809363, x2: 0.5034921404682272,
                    y1: 0.4927797202797203, y2: 0.506520979020979
                }, {
                    x1: 0.6230237458193979, x2: 0.6400806020066888,
                    y1: 0.4927797202797203, y2: 0.506520979020979
                }]);
            test.done();
        },
        'Add annotations to page' : function (test) {
            page.addAnnot({x1: 0.4, y1: 0.4, x2: 0.6, y2: 0.45});
            page.addAnnot({x1: 0, y1: 0, x2: 100, y2: 100});
            test.equal(page.numAnnots, 2);
            test.done();
        },
        'Rendering to file': function (test) {
            var out = page.renderToFile('test/out.png', 'png', 150);
            test.deepEqual(out, {'type': 'file', 'path': 'test/out.png'});
            fs.unlinkSync(out.path);

            out = page.renderToFile('test/out.jpeg', 'jpeg', 150, {
                quality: 50
            });
            test.deepEqual(out, {'type': 'file', 'path': 'test/out.jpeg'});
            fs.unlinkSync(out.path);

            out = page.renderToFile('test/out.tiff', 'tiff', 150, {
                compression: "lzw"
            });
            test.deepEqual(out, {'type': 'file', 'path': 'test/out.tiff'});
            fs.unlinkSync(out.path);

            test.throws(function () {
                out = page.renderToFile('/t/t/t/t/t/t/t/t/t/1123', 'jpeg', 150);
            }, "Can't open output file");

            test.done();
        },
        'Remove annotations from page' : function (test) {
            page.deleteAnnots();
            test.equal(page.numAnnots, 0);
            test.done();
        },
        'Rendering slice to file': function (test) {
            var out = page.renderToFile('test/out.jpeg', 'jpeg', 150, {
                quality: 100,
                slice: {
                    x: 0, y: 0, w: 1, h: 1
                }
            });
            test.deepEqual(out, {'type': 'file', 'path': 'test/out.jpeg'});
            fs.unlinkSync(out.path);

            test.throws(function () {
                page.renderToFile('test/test.jpeg', 'jpeg', 150, {
                    slice: {x:0, y:0, w:1, h:1.1}
                });
            });
            test.ok(!fs.exists('test/test.jpeg'));
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

            out = page.renderToBuffer('png', 150, {
                slice: {
                    x: 0, y: 0, w: 1, h: 0.5
                }
            });
            test.equal(out.type, 'buffer');
            test.equal(out.format, 'png');
            test.ok(Buffer.isBuffer(out.data));

            test.throws(function () {
                page.renderToBuffer('tiff', 65536);
            }, 'Result image is too big');

            out = page.renderToBuffer('tiff', 150, {
                compression: "lzw",
                slice: {
                    x: 0, y: 0, w: 1, h: 0.5
                }
            });
            test.equal(out.type, 'buffer');
            test.equal(out.format, 'tiff');
            test.ok(Buffer.isBuffer(out.data));

            test.done();
        },
        'Freeing': function (test) {
            doc = null;
            gc();
            test.throws(function () {
                page.renderToBuffer('jpeg', 72, {
                    quality: 100,
                    slice: {x: 0, y: 0, w: 1, h: 0.5}
                });
            }, 'Document closed. You must delete this page');
            page = null;
            gc();
            test.done();
        }
    }
};
