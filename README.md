## poppler-simple [![Build Status](https://travis-ci.org/blackbeam/poppler-simple.svg)](https://travis-ci.org/blackbeam/poppler-simple)
A simple javascript interface to [poppler](http://poppler.freedesktop.org/) library.

### Requirements:
1. iconv implementation (usually included in glibc).
2. Modern [poppler](http://poppler.freedesktop.org/) library version (>= 0.18).

### Install:
```bash
npm install poppler-simple
```

#### CentOS

To install the poppler libraries on CentOS the following packages need to be installed:
```bash
sudo yum install poppler poppler-utils poppler-cpp-devel
```

#### Ubuntu

To install the poppler libraries on Ubuntu the following packages need to be installed:
```bash
sudo apt-get install poppler-utils libpoppler-cpp-dev libpoppler-private-dev
```

#### Install in Docker:
See `Dockerfile.example` for inspiration/information

### Documentation:
Documentation is available in form of [typescript definitions](lib/poppler.d.ts).

### Example:

```javascript
import { PopperDocument } from 'poppler-simple';

let doc = new PopplerDocument('file://' + pathToSomeDocument);
let page = doc.getPage(pageNum);
// renders page to a buffer in jpeg format with 75 quality and 120 DPI:
let result = page.renderToBuffer('jpeg', 120, {'quality': 75});
```
## License

Licensed under either of
 * Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE) or http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license ([LICENSE-MIT](LICENSE-MIT) or http://opensource.org/licenses/MIT)
at your option.

## Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in the work by you, as defined in the Apache-2.0 license, shall be dual licensed as above, without any
additional terms or conditions.

