.PHONY: docker

cwd := $(shell pwd)
DISTFILES := $(shell cat distribution.files.lst)

all: docker

docker:
	docker build -t openinverter-ui-test .

run:
	docker run -p5000:5000 -v $(cwd):/home/openinverter/www openinverter-ui-test

install:	
	@for distfile in $(shell cat distribution.files.lst); do \
		echo $${distfile} ; \
		curl -F "data=@$$distfile" http://${INVERTER_IP}/edit ; \
	done