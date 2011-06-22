var log = console.log;
var dir = console.dir;
var fs = require('fs');
var sys   = require('sys'),
    exec  = require('child_process').exec;
var Pixbuf = require('../node-pixbuf/build/default/pixbuf').Pixbuf
var assert = require('assert');
var target = 'file://' + __dirname + '/rsl01000000001.pdf';

var p = require('./build/default/poppler');
assert.notStrictEqual( p, undefined, "Can't load poppler module" );

var pd = new p.PopplerDocument( target );
assert.notStrictEqual( p, undefined, "Can't open document" );

assert.equal( pd.pageCount, 24, 'Page count does not match');

var page = new p.PopplerPage(pd, 1);
assert.equal( page.index, 1, 'Page index does not match');

assert.equal( page.width, 299, 'Page width does not match');
assert.equal( page.height, 572, 'Page height does not match');
assert.deepEqual( page.crop_box, { x1: 0, x2: 299, y1: 0, y2: 572 }, 'Page crop box does not match');
assert.deepEqual( page.images, [ { x1: 0, x2: 1246, y1: -0.15999999999996817, y2: 2383.84 } ], 'Page images does not match');

var render = page.render(90, true);
assert.equal( render.data.pixels.length, 800800, 'Raw pixbuf is damaged' );

render = page.render(90, false)
assert.equal( render.type, 'file', 'Render type is not `file`');
assert.equal( render.path.substr( render.path.length - 4 ), '.pbm', 'Render path not ends with `.pbm`' );

search_results = page.findText("Российская");
assert.equal(search_results.length, 1, 'Can\'t find existed text');

console.log('ok');
