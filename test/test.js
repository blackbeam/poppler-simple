/*global it:true, describe:true, require:true, __dirname:true, gc:true, before:true */
/*jshint node:true */
'use strict';

var Promise = require('bluebird').Promise;
var a = require('assert');
var poppler = require('..');
var fs = require('fs');
var docs = [];
var pages = [];

const NAMES = [
    '/fixtures/0.pdf',
    '/fixtures/90.pdf',
    '/fixtures/180.pdf',
    '/fixtures/270.pdf',
];

const PATHS = {
    png: [
        'test/expected/0.png',
        'test/expected/90.png',
        'test/expected/180.png',
        'test/expected/270.png',
    ],
    jpeg: [
        'test/expected/0.jpeg',
        'test/expected/90.jpeg',
        'test/expected/180.jpeg',
        'test/expected/270.jpeg',
    ],
    tiff: [
        'test/expected/0.tiff',
        'test/expected/90.tiff',
        'test/expected/180.tiff',
        'test/expected/270.tiff',
    ],
};

const OPTS = {
    png: { slice: { x: 0, y: 0, w: 1, h: 0.5 } },
    jpeg: { quality: 100 },
    tiff: { compression: 'none' },
};

var names = NAMES.map(
    function (x) {
        return __dirname + x;
    }
);

var targets = names.map(function (x) {
    return 'file://' + x;
});

function getExpectedFileName(iteration, format) {
    return PATHS[format][iteration];
}

function getOutFileName(iteration, format) {
    var rand = Math.ceil(Math.random() * 100000000000);
    return 'test/out' + rand + '.' + format;
}

function renderToFile(pages, format) {
    var promises = pages
        .map(function (x, iteration) {
            var path = getOutFileName(iteration, format);
            var out = x.renderToFile(path, format, 50, OPTS[format]);
            a.deepEqual(out, { type: 'file', path: path });
            a.ok(fs.statSync(out.path).size > 0);
            return out;
        })
        .map(function (out) {
            fs.unlinkSync(out.path);
        });
    return Promise.all(promises);
}

function renderToBuffer(pages, format) {
    var promises = pages
        .map(function (x) {
            var out = x.renderToBuffer(format, 50, OPTS[format]);
            a.equal(out.type, 'buffer');
            a.equal(out.format, format);
            a.ok(Buffer.isBuffer(out.data));
            a.ok(out.data.length > 0);
            return out;
        });
    return Promise.all(promises);
}

function renderToFileCb(pages, format) {
    var promises = pages
        .map(function (x, iteration) {
            var path = getOutFileName(iteration, format);
            return new Promise(function (resolve) {
                x.renderToFile(path, format, 50, OPTS[format], function (err, out) {
                    a.deepEqual(out, { type: 'file', path: path });
                    a.ok(fs.statSync(out.path).size > 0);
                    resolve(out);
                });
            });
        });
    return Promise.map(promises, function (out, iteration) {
        fs.unlinkSync(out.path);
    });
}

function renderToBufferCb(pages, format) {
    var promises = pages
        .map(function (x) {
            return new Promise(function (resolve) {
                x.renderToBuffer(format, 50, OPTS[format], function (err, out) {
                    a.equal(err, null);
                    a.equal(out.type, 'buffer');
                    a.equal(out.format, format);
                    a.ok(Buffer.isBuffer(out.data));
                    a.ok(out.data.length > 0);
                    resolve(out);
                });
            });
        });
    return Promise.all(promises);
}

function renderToBufferAsync(pages, format) {
    var promises = pages
        .map(function (x) {
            return x.renderToBufferAsync(format, 50, OPTS[format])
                .then(function (out) {
                    a.equal(out.type, 'buffer');
                    a.equal(out.format, format);
                    a.ok(Buffer.isBuffer(out.data));
                    a.ok(out.data.length > 0);
                    return out;
                });
        });
    return Promise.all(promises);
}

describe('poppler module', function () {
    it('should be loaded', function () {
        a.ok(poppler && poppler.PopplerDocument && poppler.PopplerPage);
    });
});

describe('PopplerDocument', function () {
    it('should throw on non existing document', function () {
        this.timeout(0);
        a.throws(function () {
            new poppler.PopplerDocument('file:///123.pdf');
        }, new RegExp('Couldn\'t open file - fopen error. Errno: 2.'));
    });
    it('should open pdf file', function () {
        this.timeout(0);
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
    it('should open pdf file from buffer', function () {
        this.timeout(0);
        docs = names.map(function (x) {
            return new poppler.PopplerDocument(fs.readFileSync(x));
        });
        for (var i = docs.length - 1; i >= 0; i--) {
            var d = docs[i];
            a.equal(d.isLinearized, false);
            a.equal(d.pdfVersion, 'PDF-1.4');
            a.equal(d.pageCount, 1);
            a.equal(d.PDFMajorVersion, 1);
            a.equal(d.PDFMinorVersion, 4);
            a.equal(d.fileName, null);
        }
    });
    it('should throw on non existing page', function () {
        this.timeout(0);
        a.throws(function () {
            docs[0].getPage(65536);
        }, new RegExp('Page number out of bounds'));
    });
    it('should open pages', function () {
        this.timeout(0);
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
            if ((poppler.POPPLER_VERSION_MAJOR === 0 &&
                poppler.POPPLER_VERSION_MINOR >= 20) ||
                poppler.POPPLER_VERSION_MAJOR > 0) {
                a.equal(p.numAnnots, 0);
            }
            a.deepEqual(p.media_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            a.deepEqual(p.crop_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            a.deepEqual(p.trim_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            a.deepEqual(p.art_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
            a.deepEqual(p.bleed_box, { x1: 0, x2: 299, y1: 0, y2: 572 });
        }
    });
});

describe('PopplerPage', function () {
    it('should return word list', function () {
        this.timeout(0);
        var results = pages.map(function (x) {
            return x.getWordList();
        });
        a.equal(results[0].length, 45);
        a.equal(results[1].length, 45);
        a.equal(results[2].length, 45);
        a.equal(results[3].length, 45);
        a.deepEqual(results[0][0], {
            x1: 0.21752508361204015,
            x2: 0.38838531772575263,
            y1: 0.8903321678321678,
            y2: 0.9060664335664337,
            text: 'Российская'
        });
        a.deepEqual(results[1][0], {
            x1: 0.6241083916083916,
            x2: 0.6378496503496504,
            y1: 0.7519090301003345,
            y2: 0.9871571906354514,
            text: 'ЭТНОСОЦИАЛЬНАЯ'
        });
        a.deepEqual(results[2][0], {
            x1: 0.5975623411371238,
            x2: 0.6588628762541806,
            y1: 0.8016363636363637,
            y2: 0.8142237762237763,
            text: '1882'
        });
        a.deepEqual(results[3][0], {
            x1: 0.39110139860139864,
            x2: 0.40484265734265734,
            y1: 0.7972115050167224,
            y2: 0.8783848829431437,
            text: 'вв.)'
        });
    });
    it('should search for text', function () {
        this.timeout(0);
        var results = pages.map(function (x) {
            return x.findText('ко');
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
    if ((poppler.POPPLER_VERSION_MAJOR === 0 &&
        poppler.POPPLER_VERSION_MINOR >= 20) ||
        poppler.POPPLER_VERSION_MAJOR > 0) {
        it('should add annotations', function () {
            this.timeout(0);
            pages.forEach(function (x) {
                x.addAnnot(x.findText('ко'));
            });
            pages.forEach(function (x) {
                a.equal(x.numAnnots, 1);
            });
        });
        it('should remove annotations', function () {
            this.timeout(0);
            pages.forEach(function (x) {
                x.deleteAnnots();
                a.equal(x.numAnnots, 0);
            });
        });
        it('should remove only typeHighlight annotations', function () {
            this.timeout(0);
            var p = new poppler.PopplerDocument(__dirname + '/fixtures/annot.pdf').getPage(1);
            p.addAnnot(p.findText('Лейла'));
            a.equal(p.numAnnots, 9);
            p.deleteAnnots();
            a.equal(p.numAnnots, 8);
        });
    }
    describe('render to file', function () {
        it('should render to png', function () {
            this.timeout(0);
            return renderToFile(pages, 'png');
        });
        it('should render to jpeg', function () {
            this.timeout(0);
            return renderToFile(pages, 'jpeg');
        });
        it('should render to tiff', function () {
            this.timeout(0);
            return renderToFile(pages, 'tiff');
        });
        it('should throw on wrong arguments', function () {
            this.timeout(0);
            pages.forEach(function (x) {
                a.throws(function () {
                    x.renderToFile('foo');
                }, new RegExp('Arguments'));
            });
        });
        it('should throw on bad output path', function () {
            this.timeout(0);
            pages.forEach(function (x) {
                a.throws(function () {
                    x.renderToFile('/t/t/t/t/t/t/t/123', 'jpeg', 50);
                }, new RegExp('Could not open output stream'));
            });
        });
        it('should throw on unknown format', function () {
            this.timeout(0);
            pages.forEach(function (x) {
                a.throws(function () {
                    x.renderToFile('test/out.bmp', 'bmp', 50);
                }, new RegExp('Unsupported compression method'));
            });
        });
        it('should throw on bad PPI value', function () {
            this.timeout(0);
            pages.forEach(function (x) {
                a.throws(function () {
                    x.renderToFile('test/x.jpeg', 'jpeg', -1);
                }, new RegExp('PPI\' value must be greater then 0'));
            });
        });
        it('should throw on bad writer options', function () {
            this.timeout(0);
            pages.forEach(function (x) {
                a.throws(function () {
                    x.renderToFile('test/x.jpeg', 'jpeg', 50, { quality: 'foobar' });
                }, new RegExp('\'quality\' option value must be 0 - 100 interval integer'));
            });
        });
    });
    describe('render to file async', function () {
        it('should render to png', function () {
            this.timeout(0);
            return renderToFileCb(pages, 'png');
        });
        it('should render to jpeg', function () {
            this.timeout(0);
            return renderToFileCb(pages, 'jpeg');
        });
        it('should render to tiff', function () {
            this.timeout(0);
            return renderToFileCb(pages, 'tiff');
        });
        it('should pass error on bad output path', function (done) {
            this.timeout(0);
            pages[0].renderToFile('/t/t/t/t/t/t/t/123', 'jpeg', 50, function (err, out) {
                a.equal(out, undefined);
                a.equal(err.message, 'Could not open output stream');
                done();
            });
        });
        it('should pass error on unknown format', function (done) {
            this.timeout(0);
            pages[0].renderToFile('test/out.bmp', 'bmp', 50, function (err, out) {
                a.equal(out, undefined);
                a.equal(err.message, 'Unsupported compression method');
                done();
            });
        });
        it('should pass error on bad PPI value', function (done) {
            this.timeout(0);
            pages[0].renderToFile('test/x.jpeg', 'jpeg', -1, function (err, out) {
                a.equal(out, undefined);
                a.equal(err.message, '\'PPI\' value must be greater then 0');
                done();
            });
        });
        it('should pass error on bad writer options', function (done) {
            this.timeout(0);
            pages[0].renderToFile('test/x.jpeg', 'jpeg', 50, { quality: 'foobar' }, function (err, out) {
                a.equal(out, undefined);
                a.equal(err.message, '\'quality\' option value must be 0 - 100 interval integer');
                done();
            });
        });
    });
    describe('render to buffer', function () {
        it('should render to png', function () {
            this.timeout(0);
            return renderToBuffer(pages, 'png');
        });
        it('should render to jpeg', function () {
            this.timeout(0);
            return renderToBuffer(pages, 'jpeg');
        });
        it('should render to tiff', function () {
            this.timeout(0);
            return renderToBuffer(pages, 'tiff');
        });
    });
    describe('render to buffer async', function () {
        it('should render to png', function () {
            this.timeout(0);
            return renderToBufferCb(pages, 'png');
        });
        it('should render to jpeg', function () {
            this.timeout(0);
            return renderToBufferCb(pages, 'jpeg');
        });
        it('should render to tiff', function () {
            this.timeout(0);
            return renderToBufferCb(pages, 'tiff');
        });
        it('should pass errors asyncronously', function (done) {
            this.timeout(0);
            pages[0].renderToBuffer('jpg', 50, function (err, out) {
                a.equal(out, undefined);
                a.equal(err.message, 'Unsupported compression method');
                done();
            });
        });
    });

    describe('render to promise', function () {
        it('should render png to promise', function () {
            this.timeout(0);
            return renderToBufferAsync(pages, 'png');
        });
        it('should render jpeg to promise', function () {
            this.timeout(0);
            return renderToBufferAsync(pages, 'jpeg');
        });
        it('should render tiff to promise', function () {
            this.timeout(0);
            return renderToBufferAsync(pages, 'tiff');
        });
    });
});

describe('freeing', function () {
    before(function () {
        this.timeout(0);
        for (var i = docs.length - 1; i >= 0; i--) {
            delete docs[i];
            docs[i] = null;
        }
        gc();
    });

    it('page should throw if document deleted', function () {
        this.timeout(0);
        a.throws(function () {
            pages.forEach(function (x) {
                x.renderToBuffer('jpeg', 72);
            }, new RegExp('Document closed. You must delete this page'));
        });
    });
});
