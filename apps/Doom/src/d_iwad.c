//
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//     Search for and locate an IWAD file, and initialize according
//     to the IWAD type.
//

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "config.h"
#include "deh_str.h"
#include "doomkeys.h"
#include "d_iwad.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_misc.h"
#include "w_wad.h"
#include "z_zone.h"

static const iwad_t iwads[] =
{
    { "doom1.wad",    doom,      shareware,  "Doom Shareware" },
    { "doom2.wad",    doom2,     commercial, "Doom II" },
    { "plutonia.wad", pack_plut, commercial, "Final Doom: Plutonia Experiment" },
    { "tnt.wad",      pack_tnt,  commercial, "Final Doom: TNT: Evilution" },
    { "doom.wad",     doom,      retail,     "Doom" },
    { "chex.wad",     pack_chex, shareware,  "Chex Quest" },
    { "hacx.wad",     pack_hacx, commercial, "Hacx" },
    { "freedm.wad",   doom2,     commercial, "FreeDM" },
    { "freedoom2.wad", doom2,    commercial, "Freedoom: Phase 2" },
    { "freedoom1.wad", doom,     retail,     "Freedoom: Phase 1" },
    { "heretic.wad",  heretic,   retail,     "Heretic" },
    { "heretic1.wad", heretic,   shareware,  "Heretic Shareware" },
    { "hexen.wad",    hexen,     commercial, "Hexen" },
    //{ "strife0.wad",  strife,    commercial, "Strife" }, // haleyjd: STRIFE-FIXME
    { "strife1.wad",  strife,    commercial, "Strife" },
};

// Array of locations to search for IWAD files
//
// "128 IWAD search directories should be enough for anybody".

#define MAX_IWAD_DIRS 128

static boolean iwad_dirs_built = false;
static const char *iwad_dirs[MAX_IWAD_DIRS];
static int num_iwad_dirs = 0;

static void AddIWADDir(const char *dir)
{
    if (num_iwad_dirs < MAX_IWAD_DIRS)
    {
        iwad_dirs[num_iwad_dirs] = dir;
        ++num_iwad_dirs;
    }
}

static boolean DirIsFile(const char *path, const char *filename)
{
    size_t path_len;
    size_t filename_len;

    path_len = strlen(path);
    filename_len = strlen(filename);

    return path_len >= filename_len + 1
        && path[path_len - filename_len - 1] == DIR_SEPARATOR
        && !strcasecmp(&path[path_len - filename_len], filename);
}

// Check if the specified directory contains the specified IWAD
// file, returning the full path to the IWAD if found, or NULL
// if not found.

static char *CheckDirectoryHasIWAD(const char *dir, const char *iwadname)
{
    char *filename; 

    // As a special case, the "directory" may refer directly to an
    // IWAD file if the path comes from DOOMWADDIR or DOOMWADPATH.

    if (DirIsFile(dir, iwadname) && M_FileExists(dir))
    {
        return strdup(dir);
    }

    // Construct the full path to the IWAD if it is located in
    // this directory, and check if it exists.

    if (!strcmp(dir, "."))
    {
        filename = strdup(iwadname);
    }
    else
    {
        filename = M_StringJoin(dir, DIR_SEPARATOR_S, iwadname, NULL);
    }


    if (M_FileExists(filename))
    {
        return filename;
    }

    free(filename);

    return NULL;
}

// Search a directory to try to find an IWAD
// Returns the location of the IWAD if found, otherwise NULL.
//TODO FIXME
static char *SearchDirectoryForIWAD(const char *dir, int mask, GameMission_t *mission)
{
	(void)dir; (void)mask;
    char *filename;
    //size_t i;

    //for (i=0; i<arrlen(iwads); ++i) 
    {
		const char* pName = "doom1.wad", *pDir = FiGetCwd();
        //if (((1 << iwads[i].mission) & mask) == 0)
        {
            //continue;
			//return NULL;
        }

        filename = CheckDirectoryHasIWAD(pDir, pName);

        if (filename != NULL)
        {
            *mission = doom;//iwads[i].mission;

            return filename;
        }
    }

    return NULL;
}

// When given an IWAD with the '-iwad' parameter,
// attempt to identify it by its name.

static GameMission_t IdentifyIWADByName(char *name, int mask)
{
    size_t i;
    GameMission_t mission;
    char *p;

    p = strrchr(name, DIR_SEPARATOR);

    if (p != NULL)
    {
        name = p + 1;
    }

    mission = none;

    for (i=0; i<arrlen(iwads); ++i)
    {
        // Check if the filename is this IWAD name.

        // Only use supported missions:

        if (((1 << iwads[i].mission) & mask) == 0)
            continue;

        // Check if it ends in this IWAD name.

        if (!strcasecmp(name, iwads[i].name))
        {
            mission = iwads[i].mission;
            break;
        }
    }

    return mission;
}

#if ORIGCODE
//
// Add directories from the list in the DOOMWADPATH environment variable.
//

static void AddDoomWadPath(void)
{
    char *doomwadpath;
    char *p;

    // Check the DOOMWADPATH environment variable.

    doomwadpath = getenv("DOOMWADPATH");

    if (doomwadpath == NULL)
    {
        return;
    }

    doomwadpath = strdup(doomwadpath);

    // Add the initial directory

    AddIWADDir(doomwadpath);

    // Split into individual dirs within the list.

    p = doomwadpath;

    for (;;)
    {
        p = strchr(p, PATH_SEPARATOR);

        if (p != NULL)
        {
            // Break at the separator and store the right hand side
            // as another iwad dir
  
            *p = '\0';
            p += 1;

            AddIWADDir(p);
        }
        else
        {
            break;
        }
    }
}

#endif

//
// Build a list of IWAD files
//

static void BuildIWADDirList(void)
{
    AddIWADDir (FILES_DIR);

    // Don't run this function again.

    iwad_dirs_built = true;
}

//
// Searches WAD search paths for an WAD with a specific filename.
// 

char *D_FindWADByName(char *name)
{
    char *path;
    int i;
    
    // Absolute path?

    if (M_FileExists(name))
    {
        return name;
    }

    BuildIWADDirList();

    // Search through all IWAD paths for a file with the given name.

    for (i=0; i<num_iwad_dirs; ++i)
    {
        // As a special case, if this is in DOOMWADDIR or DOOMWADPATH,
        // the "directory" may actually refer directly to an IWAD
        // file.

        if (DirIsFile(iwad_dirs[i], name) && M_FileExists(iwad_dirs[i]))
        {
            return strdup(iwad_dirs[i]);
        }

        // Construct a string for the full path

        path = M_StringJoin(iwad_dirs[i], DIR_SEPARATOR_S, name, NULL);

        if (M_FileExists(path))
        {
            return path;
        }

        free(path);
    }

    // File not found

    return NULL;
}

//
// D_TryWADByName
//
// Searches for a WAD by its filename, or passes through the filename
// if not found.
//

char *D_TryFindWADByName(char *filename)
{
    char *result;

    result = D_FindWADByName(filename);

    if (result != NULL)
    {
        return result;
    }
    else
    {
        return filename;
    }
}

//
// FindIWAD
// Checks availability of IWAD files by name,
// to determine whether registered/commercial features
// should be executed (notably loading PWADs).
//

char *D_FindIWAD(int mask, GameMission_t *mission)
{
    char *result;
    char *iwadfile;
    int iwadparm;
    int i;

    // Check for the -iwad parameter

    //!
    // Specify an IWAD file to use.
    //
    // @arg <file>
    //

    iwadparm = M_CheckParmWithArgs("-iwad", 1);

    if (iwadparm)
    {
        // Search through IWAD dirs for an IWAD with the given name.

        iwadfile = myargv[iwadparm + 1];

        result = D_FindWADByName(iwadfile);

        if (result == NULL)
        {
            I_Error("IWAD file '%s' not found!", iwadfile);
        }
        
        *mission = IdentifyIWADByName(result, mask);
    }
    else
    {
        // Search through the list and look for an IWAD

        printf("-iwad not specified, trying a few iwad names\n");

        result = NULL;

        /*BuildIWADDirList();
    
        for (i=0; result == NULL && i<num_iwad_dirs; ++i)
        {
			LogMsg("TRYING: %s", iwad_dirs[i]);
            result = SearchDirectoryForIWAD(iwad_dirs[i], mask, mission);
        }*/
		mission = doom;
		result = "doom1.wad";
    }

    return result;
}

// Find all IWADs in the IWAD search path matching the given mask.

const iwad_t **D_FindAllIWADs(int mask)
{
    const iwad_t **result;
    int result_len;
    char *filename;
    size_t i;

    result = malloc(sizeof(iwad_t *) * (arrlen(iwads) + 1));
    result_len = 0;

    // Try to find all IWADs

    for (i=0; i<arrlen(iwads); ++i)
    {
        if (((1 << iwads[i].mission) & mask) == 0)
        {
            continue;
        }

        filename = D_FindWADByName(iwads[i].name);

        if (filename != NULL)
        {
            result[result_len] = &iwads[i];
            ++result_len;
        }
    }

    // End of list

    result[result_len] = NULL;

    return result;
}

//
// Get the IWAD name used for savegames.
//

char *D_SaveGameIWADName(GameMission_t gamemission)
{
    size_t i;

    // Determine the IWAD name to use for savegames.
    // This determines the directory the savegame files get put into.
    //
    // Note that we match on gamemission rather than on IWAD name.
    // This ensures that doom1.wad and doom.wad saves are stored
    // in the same place.

    for (i=0; i<arrlen(iwads); ++i)
    {
        if (gamemission == iwads[i].mission)
        {
            return iwads[i].name;
        }
    }

    // Default fallback:

    return "unknown.wad";
}

char *D_SuggestIWADName(GameMission_t mission, GameMode_t mode)
{
    size_t i;

    for (i = 0; i < arrlen(iwads); ++i)
    {
        if (iwads[i].mission == mission && iwads[i].mode == mode)
        {
            return iwads[i].name;
        }
    }

    return "unknown.wad";
}

char *D_SuggestGameName(GameMission_t mission, GameMode_t mode)
{
    size_t i;

    for (i = 0; i < arrlen(iwads); ++i)
    {
        if (iwads[i].mission == mission
         && (mode == indetermined || iwads[i].mode == mode))
        {
            return iwads[i].description;
        }
    }

    return "Unknown game?";
}

