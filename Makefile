MAKE := make
DOCKER := docker

all:
	$(MAKE) -C ./src

clean:
	$(MAKE) -C ./src clean

docker:
	$(DOCKER) build . -t piltools

docker-clean:
	$(DOCKER) rmi -f piltools
