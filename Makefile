
all:
	$(MAKE) -C src/
	mkdir -p bin/
	cp src/skype-dbb-read bin/

clean:
	-$(RM) bin/skype-dbb-read
	-rmdir bin
	$(MAKE) -C src/ clean

