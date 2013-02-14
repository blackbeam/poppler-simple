## poppler-simple
A simple javascript interface to [poppler](http://poppler.freedesktop.org/) pdf library.

### Requirements:
1. iconv implementation (usually included in glibc).
2. Modern [poppler](http://poppler.freedesktop.org/) pdf library version (>= 0.20).

### Install:
```bash
npm install poppler-simple
```

### Load:
```javascript
var PopplerDocument = require('poppler-simple').PopplerDocument;
```

### Open document:
```javascript
var doc = new PopplerDocument('file://' + pathToPDFDocument);
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
