all: build/Release/poppler.node

build/Release/poppler.node: ./src/*.cc ./src/*.h
	npm install

test: all
	npm test

clean:
	npm run-script clean

debug: all
	npm run-script build-debug
	valgrind --trace-children=yes ./node_modules/mocha/bin/mocha -gc --timeout 0 --slow 250
	rm -rf ./build/Debug