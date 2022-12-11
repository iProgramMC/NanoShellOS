# NanoShell mass app make file. This allows me to make all the applications at once.

PROJECTS := Calc Clock CrtDemo GameTest List Minesweeper Notepad NyanCat Raycaster StickyNotes Tiny WindowTest WorldMap


cleanup:
	for dir in $(PROJECTS); do  \
		$(MAKE) -C $$dir clean; \
	done
	
all:
	for dir in $(PROJECTS); do \
		$(MAKE) -C $$dir;      \
	done

update:
	for dir in $(PROJECTS); do                   \
		cp $$dir/$$dir.nse  ../fs/Bin/$$dir.nse; \
	done
