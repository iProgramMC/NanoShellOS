# NanoShell mass app make file. This allows me to make all the applications at once.

PROJECTS :=         \
	Calc            \
	Chess           \
	Clock           \
	CrtDemo         \
	Doom            \
	Edit            \
	Finder          \
	Fireworks       \
	GameTest        \
	List            \
	Minesweeper     \
	NesEmu          \
	Notepad         \
	NyanCat         \
	Raycaster       \
	StickyNotes     \
	Tcc             \
	Tiny            \
	VBuilder        \
	WavePlayer      \
	WindowTest      \
	WorldMap


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
