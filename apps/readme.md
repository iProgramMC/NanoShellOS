
To create a new application, simply copy one of the folders, rename it and change the makefile to output a .nse with that filename.

If you want to use a feature not currently supported (for example, use CC when you have CONSOLE and MEMORY), add it to the list of features.

If you are lazy, you can opt to use the following feature list (all the features), however do note that each feature takes up several kBs in your executable.
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