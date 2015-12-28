SUBDIRS = cli module

.PHONY: subdirs $(SUBDIRS)

subdirs:
	for dir in $(SUBDIRS); do \
		cd $$dir; $(MAKE); cd ../; \
	done

all: subdirs

clean:
	for dir in $(SUBDIRS); do \
		cd $$dir; $(MAKE) clean; cd ../; \
	done
