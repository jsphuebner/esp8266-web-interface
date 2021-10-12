
cwd := $(shell pwd)
DISTFILES := $(shell cat OpenInverterWeb/data/distribution.files.lst)

all: install

install:	
	@for distfile in $(shell cat OpenInverterWeb/data/distribution.files.lst); do \
		echo $${distfile} ; \
		curl -F "data=@$$distfile" http://${INVERTER_IP}/edit ; \
	done
