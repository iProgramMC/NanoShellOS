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

# As far as I can tell these will run all the time.

list: force
	@echo "Here is a list of projects managed by the mass application makefile."
	@echo $(PROJECTS)

help: force
	@echo "Help for NanoShell mass app makefile:"
	@echo "make cleanup  - runs 'make clean' on all projects"
	@echo "make all      - makes all projects"
	@echo "make update   - copies every compiled binary to the fs/Bin/ directory"
	@echo "make list     - lists all configured projects"

force: ;
