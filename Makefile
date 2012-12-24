all: build/default/poppler.node

build/default/poppler.node: ./src/*.cc ./src/*.h
	node-waf -v configure build

test: all
	mocha -gc 

clean:
	node-waf distclean