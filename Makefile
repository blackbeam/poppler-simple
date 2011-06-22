all: build
	node test.js

build: configure NodePopplerPage.cc NodePopplerPage.h NodePopplerDocument.cc NodePopplerDocument.h
	node-waf -v build

configure:
	node-waf -v configure

debug: build
	valgrind node test.js
