DIRS = $(shell ls -d */)

all:
	@$(foreach dir,$(DIRS),						\
		echo $(dir);						\
		$(MAKE) -C $(dir) clean all > /dev/null;		\
	)
