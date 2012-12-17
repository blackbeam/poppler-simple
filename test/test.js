"use strict";
var name0 = __dirname + '/fixtures/0.pdf';
var name90  = __dirname + '/fixtures/90.pdf';
var name180  = __dirname + '/fixtures/180.pdf';
var name270  = __dirname + '/fixtures/270.pdf';
var target0 = 'file://' + name0;
var target90 = 'file://' + name90;
var target180 = 'file://' + name180;
var target270 = 'file://' + name270;
var poppler = require('..');
var fs = require('fs');

var doc0, doc90, doc180, doc270, page0, page90, page180, page270;

module.exports = {
    all: {
        'Is module loaded?': function (test) {
            test.ok(poppler && poppler.PopplerDocument && poppler.PopplerPage);
            test.done();
        },
        'Open not existing pdf': function (test) {
            test.throws(function () {
                var doc = new poppler.PopplerDocument('file:///123.pdf');
                doc = null;
            }, 'Couldn\'t open file - fopen error. Errno: 2.');
            test.done();
        },
        'Open pdf': function (test) {
            doc0 = new poppler.PopplerDocument(target0);
            doc90 = new poppler.PopplerDocument(target90);
            doc180 = new poppler.PopplerDocument(target180);
            doc270 = new poppler.PopplerDocument(target270);
            test.equal(doc0.isLinearized, false);
            test.equal(doc0.pdfVersion, 'PDF-1.4');
            test.equal(doc0.pageCount, 1);
            test.equal(doc0.PDFMajorVersion, 1);
            test.equal(doc0.PDFMinorVersion, 4);
            test.equal(doc0.fileName, name0);
            test.equal(doc90.isLinearized, false);
            test.equal(doc90.pdfVersion, 'PDF-1.4');
            test.equal(doc90.pageCount, 1);
            test.equal(doc90.PDFMajorVersion, 1);
            test.equal(doc90.PDFMinorVersion, 4);
            test.equal(doc90.fileName, name90);
            test.equal(doc180.isLinearized, false);
            test.equal(doc180.pdfVersion, 'PDF-1.4');
            test.equal(doc180.pageCount, 1);
            test.equal(doc180.PDFMajorVersion, 1);
            test.equal(doc180.PDFMinorVersion, 4);
            test.equal(doc180.fileName, name180);
            test.equal(doc270.isLinearized, false);
            test.equal(doc270.pdfVersion, 'PDF-1.4');
            test.equal(doc270.pageCount, 1);
            test.equal(doc270.PDFMajorVersion, 1);
            test.equal(doc270.PDFMinorVersion, 4);
            test.equal(doc270.fileName, name270);
            test.done();
        },
        'Open not existing page': function (test) {
            test.throws(function () {
                var page = doc0.getPage(655536);
                page = null;
            }, 'Page number out of bounds');
            test.done();
        },
        'Open page': function (test) {
            page0 = doc0.getPage(1);
            page90 = doc90.getPage(1);
            page180 = doc180.getPage(1);
            page270 = doc270.getPage(1);
            test.equal(page0.num, 1);
            test.equal(page0.height, 572);
            test.equal(page0.width, 299);
            test.equal(page0.rotate, 0);
            test.equal(page0.isCropped, false);
            test.equal(page0.numAnnots, 0);
            test.deepEqual(page0.media_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            test.deepEqual(page0.crop_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            test.deepEqual(page0.art_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            test.deepEqual(page0.bleed_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            test.deepEqual(page0.trim_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            test.equal(page90.num, 1);
            test.equal(page90.height, 299);
            test.equal(page90.width, 572);
            test.equal(page90.rotate, 90);
            test.equal(page90.isCropped, false);
            test.equal(page90.numAnnots, 0);
            test.deepEqual(page90.media_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            test.deepEqual(page90.crop_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            test.deepEqual(page90.art_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            test.deepEqual(page90.bleed_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            test.deepEqual(page90.trim_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            test.equal(page180.num, 1);
            test.equal(page180.height, 572);
            test.equal(page180.width, 299);
            test.equal(page180.rotate, 180);
            test.equal(page180.isCropped, false);
            test.equal(page180.numAnnots, 0);
            test.deepEqual(page180.media_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            test.deepEqual(page180.crop_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            test.deepEqual(page180.art_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            test.deepEqual(page180.bleed_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            test.deepEqual(page180.trim_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            test.equal(page270.num, 1);
            test.equal(page270.height, 299);
            test.equal(page270.width, 572);
            test.equal(page270.rotate, 270);
            test.equal(page270.isCropped, false);
            test.equal(page270.numAnnots, 0);
            test.deepEqual(page270.media_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            test.deepEqual(page270.crop_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            test.deepEqual(page270.art_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            test.deepEqual(page270.bleed_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            test.deepEqual(page270.trim_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            test.done();
        },
        'Search for text': function (test) {
            test.deepEqual(page0.findText("Российская"), [
                    {
                        x1: 0.21752508361204015,
                        x2: 0.38838531772575263,
                        y1: 0.8903321678321678,
                        y2: 0.9060664335664336
                    }
                ]
            );
            test.deepEqual(page90.findText("Российская"), [
                    {
                        x1: 0.8903321678321678,
                        x2: 0.9060664335664336,
                        y1: 0.6116146822742473,
                        y2: 0.7824749163879599
                    }
                ]
            );
            test.deepEqual(page180.findText("Российская"), [
                    {
                        x1: 0.6116146822742473,
                        x2: 0.7824749163879599,
                        y1: 0.09393356643356647,
                        y2: 0.1096678321678322
                    }
                ]
            );
            test.deepEqual(page270.findText("Российская"), [
                    {
                        x1: 0.09393356643356644,
                        x2: 0.10966783216783217,
                        y1: 0.2175250836120402,
                        y2: 0.3883853177257527
                    }
                ]
            );
            test.deepEqual(page0.findText("qwerty"), []);
            test.deepEqual(page0.findText("я"), [
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
            page0.addAnnot(page0.findText("Российская"));
            page90.addAnnot(page90.findText("Российская"));
            page180.addAnnot(page180.findText("Российская"));
            page270.addAnnot(page270.findText("Российская"));
            page0.addAnnot([]);
            test.equal(page0.numAnnots, 1);
            test.equal(page90.numAnnots, 1);
            test.equal(page180.numAnnots, 1);
            test.equal(page270.numAnnots, 1);
            test.done();
        },
        'Rendering to file': function (test) {
            var out = page0.renderToFile('test/out.png', 'png', 150);
            test.deepEqual(out, {'type': 'file', 'path': 'test/out.png'});
            fs.unlinkSync(out.path);

            out = page0.renderToFile('test/out0.jpeg', 'jpeg', 150, {
                quality: 50
            });
            test.deepEqual(out, {'type': 'file', 'path': 'test/out0.jpeg'});
            fs.unlinkSync(out.path);
            out = page90.renderToFile('test/out90.jpeg', 'jpeg', 150, {
                quality: 50
            });
            test.deepEqual(out, {'type': 'file', 'path': 'test/out90.jpeg'});
            fs.unlinkSync(out.path);
            out = page180.renderToFile('test/out180.jpeg', 'jpeg', 150, {
                quality: 50
            });
            test.deepEqual(out, {'type': 'file', 'path': 'test/out180.jpeg'});
            fs.unlinkSync(out.path);
            out = page270.renderToFile('test/out270.jpeg', 'jpeg', 150, {
                quality: 50
            });
            test.deepEqual(out, {'type': 'file', 'path': 'test/out270.jpeg'});
            fs.unlinkSync(out.path);

            out = page0.renderToFile('test/out.tiff', 'tiff', 150, {
                compression: "lzw"
            });
            test.deepEqual(out, {'type': 'file', 'path': 'test/out.tiff'});
            fs.unlinkSync(out.path);

            test.throws(function () {
                var out = page0.renderToFile('/t/t/t/t/t/t/t/t/t/1123', 'jpeg', 150);
                out = null;
            }, "Can't open output file");

            test.done();
        },
        'Remove annotations from page' : function (test) {
            page0.deleteAnnots();
            page90.deleteAnnots();
            page180.deleteAnnots();
            page270.deleteAnnots();
            test.equal(page0.numAnnots, 0);
            test.equal(page90.numAnnots, 0);
            test.equal(page180.numAnnots, 0);
            test.equal(page270.numAnnots, 0);
            test.done();
        },
        'Rendering slice to file': function (test) {
            var out = page0.renderToFile('test/out.jpeg', 'jpeg', 150, {
                quality: 100,
                slice: {
                    x: 0, y: 0, w: 1, h: 1
                }
            });
            test.deepEqual(out, {'type': 'file', 'path': 'test/out.jpeg'});
            fs.unlinkSync(out.path);

            test.throws(function () {
                page0.renderToFile('test/test.jpeg', 'jpeg', 150, {
                    slice: {x:0, y:0, w:1, h:1.1}
                });
            });
            test.ok(!fs.exists('test/test.jpeg'));
            test.done();
        },
        'Rendering to buffer': function (test) {
            var out = page0.renderToBuffer('jpeg', 72, {
                quality: 100,
                slice: {
                    x: 0, y: 0, w: 1, h: 0.5
                }
            });
            test.equal(out.type, 'buffer');
            test.equal(out.format, 'jpeg');
            test.ok(Buffer.isBuffer(out.data));

            out = page0.renderToBuffer('png', 150, {
                slice: {
                    x: 0, y: 0, w: 1, h: 0.5
                }
            });
            test.equal(out.type, 'buffer');
            test.equal(out.format, 'png');
            test.ok(Buffer.isBuffer(out.data));

            test.throws(function () {
                page0.renderToBuffer('tiff', 65536);
            }, 'Result image is too big');

            out = page0.renderToBuffer('tiff', 150, {
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
            if (gc) {
                doc0 = null;
                doc90 = null;
                doc180 = null;
                doc270 = null;
                gc();
                test.throws(function () {
                    page0.renderToBuffer('jpeg', 72, {
                        quality: 100,
                        slice: {x: 0, y: 0, w: 1, h: 0.5}
                    });
                }, 'Document closed. You must delete this page');
                page0 = null;
                page90 = null;
                page180 = null;
                page270 = null;
                gc();
                test.done();
            } else {
                test.done();
            }
        }
    }
};
