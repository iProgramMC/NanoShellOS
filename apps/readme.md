## NanoShell Software Development Kit

To create a new application, simply copy one of the folders, rename it and change the makefile to output a .nse with that filename.
It's optional to use the same filename as the directory your application is in... unless you decide to use `make_all.sh` from the root,
which assumes that all executables are named the same as the directory's name, as in `apps/<AppName>/<AppName>.nse`.

If you want to use a feature not currently used (for example, use CC when you have CONSOLE and MEMORY), add it to the list of features,
`rm` the build directory, and build.

If you are lazy, you can opt to use the following feature list (all the features), however do note that each feature takes up several kBs
in your executable, which may or may not be useless.
```
FEATURES=\
	-DUSE_CONSOLE    \
	-DUSE_CC         \
	-DUSE_CLIP       \
	-DUSE_FILE       \
	-DUSE_MEMORY     \
	-DUSE_MISC       \
	-DUSE_VIDEO      \
	-DUSE_WINDOW
```