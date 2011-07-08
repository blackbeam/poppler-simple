all: build/default/poppler.node test
	nodeunit test/test.js

build/default/poppler.node: *.cc *.h build/config.log
	node-waf -v build

build/config.log: wscript
	node-waf -v configure
