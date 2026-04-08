CC=emcc

FLAGS=--use-port=sdl2 -I include -gsource-map -O3

SRC=$(wildcard src/*.c)
STATIC_DIST_FILES=$(wildcard static/*)

.PHONY: all
all: dist-firefox.zip dist-chrome.zip compile_flags.txt

dist-firefox.zip: dist
	- cat manifest/manifest.json manifest/firefox.json | jq -s "add" > dist/manifest.json
	- cd dist && zip -r ../dist-firefox .

dist-chrome.zip: dist
	- cat manifest/manifest.json manifest/chrome.json | jq -s "add" > dist/manifest.json
	- cd dist && zip -r ../dist-chrome .

dist: build
	- mkdir -p dist
	- cp -r build/ dist/build/
	- cp $(STATIC_DIST_FILES) dist/

build: $(SRC)
	- mkdir -p build
	$(CC) $(FLAGS) -o build/main.js $(SRC)

compile_flags.txt:
	$(CC) $(FLAGS) --cflags | sed 's/ /\n/g' > compile_flags.txt

.PHONY: clean_build
clean_build:
	- rm -rf build
	- rm -rf dist
	- rm dist-firefox.zip
	- rm dist-chrome.zip

.PHONY: clean
clean: clean_build
	- rm compile_flags.txt
