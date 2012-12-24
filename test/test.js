/*global it:true, describe:true, require:true, __dirname:true, gc:true, before:true */
/*jshint node:true */
"use strict";

var names = ['/fixtures/0.pdf', '/fixtures/90.pdf', '/fixtures/180.pdf', '/fixtures/270.pdf'].map(
    function (x) {
        return __dirname + x;
    }
);

var targets = names.map(function (x) {
    return 'file://' + x;
});

var a = require('assert');
var poppler = require('..');
var fs = require('fs');
var docs = [];
var pages = [];

describe('poppler module', function () {
    it('should be loaded', function () {
        a.ok(poppler && poppler.PopplerDocument && poppler.PopplerPage);
    });
});

describe('PopplerDocument', function () {
    it('should throw on non existing document', function () {
        a.throws(function () {
            var doc = new poppler.PopplerDocument('file:///123.pdf');
            doc = null;
        }, new RegExp("Couldn't open file - fopen error. Errno: 2."));
    });
    it('should open pdf file', function () {
        docs = targets.map(function (x) {
            return new poppler.PopplerDocument(x);
        });
        for (var i = docs.length - 1; i >= 0; i--) {
            var d = docs[i];
            a.equal(d.isLinearized, false);
            a.equal(d.pdfVersion, 'PDF-1.4');
            a.equal(d.pageCount, 1);
            a.equal(d.PDFMajorVersion, 1);
            a.equal(d.PDFMinorVersion, 4);
            a.equal(d.fileName, names[i]);
        }
    });
    it('should throw on non existing page', function () {
        a.throws(function () {
            var page = docs[0].getPage(65536);
            page = null;
        }, new RegExp('Page number out of bounds'));
    });
    it('should open pages', function () {
        pages = docs.map(function (x) {
            return x.getPage(1);
        });
        var tmp = [
            {
                height: 572,
                width: 299,
                rotate: 0
            },
            {
                height: 299,
                width: 572,
                rotate: 90
            },
            {
                height: 572,
                width: 299,
                rotate: 180
            },
            {
                height: 299,
                width: 572,
                rotate: 270
            }
        ];
        for (var i = pages.length - 1; i >= 0; i--) {
            var p = pages[i];
            a.equal(p.num, 1);
            a.equal(p.height, tmp[i].height);
            a.equal(p.width, tmp[i].width);
            a.equal(p.rotate, tmp[i].rotate);
            a.equal(p.isCropped, false);
            a.equal(p.numAnnots, 0);
            a.deepEqual(p.media_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            a.deepEqual(p.crop_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            a.deepEqual(p.trim_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            a.deepEqual(p.art_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            a.deepEqual(p.bleed_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
        }
    });
});

describe('PopplerPage', function () {
    it('should search for text', function () {
        var results = pages.map(function (x) {
            return x.findText("ко");
        });
        var tmp = [
            [
                {
                    x1: 0.45370444816053507,
                    x2: 0.48796471571906347,
                    y1: 0.863479020979021,
                    y2: 0.8792132867132867
                },
                {
                    x1: 0.7397859866220733,
                    x2: 0.772362040133779,
                    y1: 0.7812412587412587,
                    y2: 0.7969755244755244
                }
            ],
            [
                {
                    x1: 0.863479020979021,
                    x2: 0.8792132867132867,
                    y1: 0.5120352842809365,
                    y2: 0.546295551839465
                },
                {
                    x1: 0.7812412587412587,
                    x2: 0.7969755244755244,
                    y1: 0.227637959866221,
                    y2: 0.2602140133779267
                }
            ],
            [
                {
                    x1: 0.227637959866221,
                    x2: 0.2602140133779267,
                    y1: 0.20302447552447553,
                    y2: 0.21875874125874126
                },
                {
                    x1: 0.5120352842809365,
                    x2: 0.546295551839465,
                    y1: 0.12078671328671334,
                    y2: 0.13652097902097907
                }
            ],
            [
                {
                    x1: 0.2030244755244755,
                    x2: 0.21875874125874123,
                    y1: 0.7397859866220733,
                    y2: 0.772362040133779
                },
                {
                    x1: 0.12078671328671332,
                    x2: 0.13652097902097904,
                    y1: 0.45370444816053507,
                    y2: 0.48796471571906347
                }
            ]
        ];
        a.deepEqual(results, tmp);
    });
    it('should add annotations', function () {
        pages.forEach(function (x) {
            x.addAnnot(x.findText('ко'));
        });
        pages.forEach(function (x) {
            a.equal(x.numAnnots, 1);
        });
    });
    it('should remove annotations', function () {
        pages.forEach(function (x) {
            x.deleteAnnots();
            a.equal(x.numAnnots, 0);
        });
    });
    describe('render to file', function () {
        it('should render to png', function () {
            pages.forEach(function (x) {
                var out = x.renderToFile('test/out.png', 'png', 50, {
                    slice: { x: 0, y: 0, w: 1, h: 0.5 }
                });
                fs.unlinkSync(out.path);
                a.deepEqual(out, {type: 'file', path: 'test/out.png'});
            });
        });
        it('should render to jpeg', function () {
            pages.forEach(function (x) {
                var out = x.renderToFile('test/out.jpeg', 'jpeg', 50, {quality: 100});
                fs.unlinkSync(out.path);
                a.deepEqual(out, {type: 'file', path: 'test/out.jpeg'});
            });
        });
        it('should render to tiff', function () {
            pages.forEach(function (x) {
                var out = x.renderToFile('test/out.tiff', 'tiff', 50, {compression: 'lzw'});
                fs.unlinkSync(out.path);
                a.deepEqual(out, {type: 'file', path: 'test/out.tiff'});
            });
        });
        it('should throw on bad output path', function () {
            pages.forEach(function (x) {
                a.throws(function () {
                    var out = x.renderToFile('/t/t/t/t/t/t/t/123', 'jpeg', 50);
                    out = null;
                }, new RegExp("Can't open output file"));
            });
        });
    });
    describe('render to buffer', function () {
        it('should render to png', function () {
            pages.forEach(function (x) {
                var out = x.renderToBuffer('png', 50, {
                    slice: { x: 0, y: 0, w: 1, h: 0.5 }
                });
                a.equal(out.type, 'buffer');
                a.equal(out.format, 'png');
                a.ok(Buffer.isBuffer(out.data));
                a.ok(out.data.length > 0);
            });
        });
        it('should render to jpeg', function () {
            pages.forEach(function (x) {
                var out = x.renderToBuffer('jpeg', 50, {quality: 100});
                a.equal(out.type, 'buffer');
                a.equal(out.format, 'jpeg');
                a.ok(Buffer.isBuffer(out.data));
                a.ok(out.data.length > 0);
            });
        });
        it('should render to tiff', function () {
            pages.forEach(function (x) {
                var out = x.renderToBuffer('tiff', 50, {compression: 'lzw'});
                a.equal(out.type, 'buffer');
                a.equal(out.format, 'tiff');
                a.ok(Buffer.isBuffer(out.data));
                a.ok(out.data.length > 0);
            });
        });
    });
});

describe('freeing', function () {
    before(function () {
        for (var i = docs.length - 1; i >= 0; i--) {
            delete docs[i];
            docs[i] = null;
        }
        gc();
    });

    it('page should throw if document deleted', function () {
        a.throws(function () {
            pages.forEach(function (x) {
                x.renderToBuffer('jpeg', 72);
            }, new RegExp("Document closed. You must delete this page"));
        });
    });
});
