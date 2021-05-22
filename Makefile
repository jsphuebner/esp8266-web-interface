.PHONY: docker

cwd := $(shell pwd)

all: docker

docker:
	docker build -t openinverter-ui-test .

run:
	docker run -p5000:5000 -v $(cwd):/home/openinverter/www openinverter-ui-test
