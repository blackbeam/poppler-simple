## poppler-simple [![Build Status](https://travis-ci.org/blackbeam/poppler-simple.svg)](https://travis-ci.org/blackbeam/poppler-simple)
A simple javascript interface to [poppler](http://poppler.freedesktop.org/) library.

### Requirements:
1. iconv implementation (usually included in glibc).
2. Modern [poppler](http://poppler.freedesktop.org/) library version (>= 0.18).

### Install:
```bash
npm install poppler-simple
```

## Install in Docker:
See `Dockerfile.example` for inspiration/information

### Load:
```javascript
var PopplerDocument = require('poppler-simple').PopplerDocument;
```

### Open document:
```javascript
var doc = new PopplerDocument('file://' + pathToDocument);
```

### Get a page:
```javascript
var page = doc.getPage(pageNum);
```

### Render page to a buffer in jpeg format with 75 quality and 120 DPI:
```javascript
var buf = page.renderToBuffer('jpeg', 120, {'quality': 75});
```


***
For more info see _test/test.js_

## License

Licensed under either of
 * Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE) or http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license ([LICENSE-MIT](LICENSE-MIT) or http://opensource.org/licenses/MIT)
at your option.

## Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in the work by you, as defined in the Apache-2.0 license, shall be dual licensed as above, without any
additional terms or conditions.

