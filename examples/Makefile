DIRS = $(shell ls -d */)

all:
	@$(foreach dir,$(DIRS),						\
		echo $(dir);						\
		$(MAKE) -C $(dir) all > /dev/null || exit;		\
	)

clean:
	@$(foreach dir,$(DIRS),						\
		echo $(dir);						\
		$(MAKE) -C $(dir) clean > /dev/null || exit;		\
	)
