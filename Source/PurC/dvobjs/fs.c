/*
 * @file fs.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of file system dynamic variant object.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/edom.h"
#include "private/html.h"
#include "purc-variant.h"
#include "tools.h"

#include <unistd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysmacros.h>


static const char* get_work_dirctory (void)
{
    return "/home/gengyue";
}

static bool remove_dir (char * dir)
{
    char dir_name[PATH_MAX];
    DIR *dirp;
    struct dirent *dp;
    struct stat dir_stat;
    bool ret = true;

    if (access(dir, F_OK | R_OK) != 0) 
        return false;

    if (stat(dir, &dir_stat) < 0) 
        return false;

    if (S_ISREG(dir_stat.st_mode))
        remove(dir);
    else if (S_ISDIR(dir_stat.st_mode)) {
        dirp = opendir(dir);

        while ((dp=readdir(dirp)) != NULL) {
            if ((strcmp(dp->d_name, ".") == 0) || (strcmp(dp->d_name, "..") == 0)) 
                continue;
            sprintf(dir_name, "%s/%s", dir, dp->d_name);
            remove_dir(dir_name);
        }
        closedir(dirp);

        rmdir(dir);
    } 
    else 
        ret = false;
    
    return ret;
}

static purc_variant_t
list_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    char dir_name[PATH_MAX] = {0};
    char filename[PATH_MAX] = {0};
    const char *string_filename = NULL;
    purc_variant_t ret_var = NULL;
    const char *filter = NULL;
    char wildcard[10][16];
    int wildcard_num = 0;
    char au[10] = {0};
    int i = 0;

    if ((argv == NULL) || (nr_args < 1)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (*string_filename == '/') {
        strcpy (dir_name, string_filename);
    }
    else {
        strcpy (dir_name, get_work_dirctory ());
        strcat (dir_name, "/");
        strcat (dir_name, string_filename);
    }

    if (access(dir_name, F_OK | R_OK) == 0) 
        return purc_variant_make_boolean (false);

    // get the filter
    if ((argv[1] != NULL) && (!purc_variant_is_string (argv[1]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }
    if (argv[1] != NULL)
        filter = purc_variant_get_string_const (argv[1]);

    // get filter array
    if (filter) {
        size_t length = 0;
        const char *head = pcdvobjs_get_next_option (filter, ";", &length);
        while (head && (wildcard_num < 10)) {
            strncpy(wildcard[wildcard_num], head, length);
            wildcard[wildcard_num][length] = 0x00;
            pcdvobjs_remove_space (wildcard[wildcard_num]);
            wildcard_num ++;
            head = pcdvobjs_get_next_option (head + length + 1, ";", &length);
        }
    }


    // get the dirctory content
    DIR *dir = NULL;
    struct dirent *ptr = NULL;
    purc_variant_t obj_var = NULL;
    struct stat file_stat;

    if ((dir = opendir (dir_name)) == NULL) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    ret_var = purc_variant_make_array (0, PURC_VARIANT_INVALID);
    while ((ptr = readdir(dir)) != NULL)
    {
        if (strcmp (ptr->d_name,".") == 0 || strcmp(ptr->d_name, "..") == 0)
            continue;

        // use filter
        if (wildcard_num != 0) {
            for (i = 0; i < wildcard_num; i++) {
                if (wildcard_cmp (ptr->d_name, wildcard[i]))
                    break;
            }
            if (i == wildcard_num)
                continue;
        }

        obj_var = purc_variant_make_object (0, PURC_VARIANT_INVALID, 
                                                    PURC_VARIANT_INVALID);

        strcpy (filename, dir_name);
        strcat (filename, "/");
        strcat (filename, ptr->d_name);

        if (stat(filename, &file_stat) < 0) 
            continue;
        
        // name
        purc_variant_object_set_c (obj_var, "name",
                purc_variant_make_string (ptr->d_name, false));

        // dev
        purc_variant_object_set_c (obj_var, "name",
                purc_variant_make_number (file_stat.st_dev));
        
        // inode
        purc_variant_object_set_c (obj_var, "inode",
                purc_variant_make_number (ptr->d_ino));

        // type
        if (ptr->d_type == DT_BLK) {
            purc_variant_object_set_c (obj_var, "type",
                    purc_variant_make_string ("b", false));
        }
        else if(ptr->d_type == DT_CHR) {
            purc_variant_object_set_c (obj_var, "type",
                    purc_variant_make_string ("c", false));
        }
        else if(ptr->d_type == DT_DIR) {
            purc_variant_object_set_c (obj_var, "type",
                    purc_variant_make_string ("d", false));
        }
        else if(ptr->d_type == DT_FIFO) {
            purc_variant_object_set_c (obj_var, "type",
                    purc_variant_make_string ("f", false));
        }
        else if(ptr->d_type == DT_LNK) {
            purc_variant_object_set_c (obj_var, "type",
                    purc_variant_make_string ("l", false));
        }
        else if(ptr->d_type == DT_REG) {
            purc_variant_object_set_c (obj_var, "type",
                    purc_variant_make_string ("r", false));
        }
        else if(ptr->d_type == DT_SOCK) {
            purc_variant_object_set_c (obj_var, "type",
                    purc_variant_make_string ("s", false));
        }
        else if(ptr->d_type == DT_UNKNOWN) {
            purc_variant_object_set_c (obj_var, "type",
                    purc_variant_make_string ("u", false));
        }

        // mode
        purc_variant_object_set_c (obj_var, "mode",
                purc_variant_make_byte_sequence (&(file_stat.st_mode),
                                                    sizeof(unsigned long)));

        // mode_str
        for (i = 0; i < 3; i++) {
            if ((0x01 << (8 - 3 * i)) & file_stat.st_mode)
                au[i * 3 + 0] = 'r';
            else
                au[i * 3 + 0] = '-';
            if ((0x01 << (7 - 3 * i)) & file_stat.st_mode)
                au[i * 3 + 1] = 'w';
            else
                au[i * 3 + 1] = '-';
            if ((0x01 << (6 - 3 * i)) & file_stat.st_mode)
                au[i * 3 + 2] = 'x';
            else
                au[i * 3 + 2] = '-';
        }
        purc_variant_object_set_c (obj_var, "mode_str",
                purc_variant_make_string (au, false));

        // nlink
        purc_variant_object_set_c (obj_var, "nlink",
                purc_variant_make_number (file_stat.st_nlink));

        // uid
        purc_variant_object_set_c (obj_var, "uid",
                purc_variant_make_number (file_stat.st_uid));

        // gid
        purc_variant_object_set_c (obj_var, "gid",
                purc_variant_make_number (file_stat.st_gid));

        // rdev_major 
        purc_variant_object_set_c (obj_var, "rdev_major",
                purc_variant_make_number (major(file_stat.st_dev)));
        
        // rdev_minor
        purc_variant_object_set_c (obj_var, "rdev_minor",
                purc_variant_make_number (minor(file_stat.st_dev)));

        // size
        purc_variant_object_set_c (obj_var, "size",
                purc_variant_make_number (file_stat.st_size));

        // blksize
        purc_variant_object_set_c (obj_var, "blksize",
                purc_variant_make_number (file_stat.st_blksize));

        // blocks
        purc_variant_object_set_c (obj_var, "blocks",
                purc_variant_make_number (file_stat.st_blocks));

        // atime
        purc_variant_object_set_c (obj_var, "atime",
                purc_variant_make_string (ctime(&file_stat.st_atime), false));

        // mtime
        purc_variant_object_set_c (obj_var, "mtime",
                purc_variant_make_string (ctime(&file_stat.st_mtime), false));

        // ctime
        purc_variant_object_set_c (obj_var, "ctime",
                purc_variant_make_string (ctime(&file_stat.st_ctime), false));

        purc_variant_array_append (ret_var, obj_var);
    }

    closedir(dir);

    return ret_var;
}

#define DISPLAY_MODE    1
#define DISPLAY_NLINK   2 
#define DISPLAY_UID     3
#define DISPLAY_GID     4
#define DISPLAY_SIZE    5
#define DISPLAY_BLKSIZE 6
#define DISPLAY_ATIME   7
#define DISPLAY_CTIME   8
#define DISPLAY_MTIME   9
#define DISPLAY_NAME    10
#define DISPLAY_ALL     11
#define DISPLAY_DEFAULT 12

static purc_variant_t
list_prt_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    char dir_name[PATH_MAX] = {0};
    char filename[PATH_MAX] = {0};
    const char *string_filename = NULL;
    const char *filter = NULL;
    char wildcard[10][16];
    int wildcard_num = 0;
    const char *mode = NULL;
    int display[10] = {0};
    purc_variant_t ret_var = NULL;
    char au[10] = {0};
    int i = 0;

    if ((argv == NULL) || (nr_args < 1)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (*string_filename == '/') {
        strcpy (dir_name, string_filename);
    }
    else {
        strcpy (dir_name, get_work_dirctory ());
        strcat (dir_name, "/");
        strcat (dir_name, string_filename);
    }

    if (access(dir_name, F_OK | R_OK) == 0) 
        return purc_variant_make_boolean (false);

    // get the filter
    if ((argv[1] != NULL) && (!purc_variant_is_string (argv[1]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }
    if (argv[1] != NULL)
        filter = purc_variant_get_string_const (argv[1]);

    // get filter array
    if (filter) {
        size_t length = 0;
        const char *head = pcdvobjs_get_next_option (filter, ";", &length);
        while (head && (wildcard_num < 10)) {
            strncpy(wildcard[wildcard_num], head, length);
            wildcard[wildcard_num][length] = 0x00;
            pcdvobjs_remove_space (wildcard[wildcard_num]);
            wildcard_num ++;
            head = pcdvobjs_get_next_option (head + length + 1, ";", &length);
        }
    }

    // get the mode
    if ((argv[2] != NULL) && (!purc_variant_is_string (argv[2]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }
    if (argv[2] != NULL)
        mode = purc_variant_get_string_const (argv[1]);
    
    // get mode array
    i = 0;
    bool quit = false;
    size_t length = 0;
    const char * head = pcdvobjs_get_next_option (mode, " ", &length);
    while (head) {
        switch (* head)
        {
            case 'm':
            case 'M':
                if (strncasecmp (head, "mode", length) == 0) {
                    display[i] = DISPLAY_MODE;
                    i++;
                }
                else if (strncasecmp (head, "mtime", length) == 0) {
                    display[i] = DISPLAY_MTIME;
                    i++;
                }
                break;
            case 'n':
            case 'N':
                if (strncasecmp (head, "nlink", length) == 0) {
                    display[i] = DISPLAY_NLINK;
                    i++;
                }
                else if (strncasecmp (head, "name", length) == 0) {
                    display[i] = DISPLAY_NAME;
                    i++;
                }
                break;
            case 'u':
            case 'U':
                if (strncasecmp (head, "uid", length) == 0) {
                    display[i] = DISPLAY_UID;
                    i++;
                }
                break;
            case 'g':
            case 'G':
                if (strncasecmp (head, "gid", length) == 0) {
                    display[i] = DISPLAY_GID;
                    i++;
                }
                break;
            case 's':
            case 'S':
                if (strncasecmp (head, "size", length) == 0) {
                    display[i] = DISPLAY_SIZE;
                    i++;
                }
                break;
            case 'b':
            case 'B':
                if (strncasecmp (head, "blksize", length) == 0) {
                    display[i] = DISPLAY_BLKSIZE;
                    i++;
                }
                break;
            case 'a':
            case 'A':
                if (strncasecmp (head, "atime", length) == 0) {
                    display[i] = DISPLAY_ATIME;
                    i++;
                }
                else if (strncasecmp (head, "all", length) == 0) {
                    for (i = 0; i < 10; i++)
                        display[i] = i + 1;
                    quit = true;
                }
                break;
            case 'c':
            case 'C':
                if (strncasecmp (head, "ctime", length) == 0) {
                    display[i] = DISPLAY_CTIME;
                    i++;
                }
                break;
            case 'd':
            case 'D':
                if (strncasecmp (head, "default", length) == 0) {
                    for (i = 0; i < 10; i++)
                        display[i] = i + 1;
                    quit = true;
                }
                break;
        }

        if (quit)
            break;
        head = pcdvobjs_get_next_option (head + length + 1, " ", &length);
    }

    // get the dirctory content
    DIR *dir = NULL;
    struct dirent *ptr = NULL;
    struct stat file_stat;
    char info[512] = {0};

    if ((dir = opendir (dir_name)) == NULL) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    ret_var = purc_variant_make_array (0, PURC_VARIANT_INVALID);
    while ((ptr = readdir(dir)) != NULL)
    {
        if (strcmp (ptr->d_name,".") == 0 || strcmp(ptr->d_name, "..") == 0)
            continue;

        // use filter
        if (wildcard_num != 0) {
            for (i = 0; i < wildcard_num; i++) {
                if (wildcard_cmp (ptr->d_name, wildcard[i]))
                    break;
            }
            if (i == wildcard_num)
                continue;
        }

        info[0] = 0x00;

        strcpy (filename, dir_name);
        strcat (filename, "/");
        strcat (filename, ptr->d_name);

        if (stat(filename, &file_stat) < 0) 
            continue;
        
        for (i = 0; i < 10; i++) {
            switch (display[i]) {
                case DISPLAY_MODE:
                    // type
                    if (ptr->d_type == DT_BLK) {
                        sprintf (info + strlen (info), "b");
                    }
                    else if(ptr->d_type == DT_CHR) {
                        sprintf (info + strlen (info), "c");
                    }
                    else if(ptr->d_type == DT_DIR) {
                        sprintf (info + strlen (info), "d");
                    }
                    else if(ptr->d_type == DT_FIFO) {
                        sprintf (info + strlen (info), "f");
                    }
                    else if(ptr->d_type == DT_LNK) {
                        sprintf (info + strlen (info), "l");
                    }
                    else if(ptr->d_type == DT_REG) {
                        sprintf (info + strlen (info), "-");
                    }
                    else if(ptr->d_type == DT_SOCK) {
                        sprintf (info + strlen (info), "s");
                    }

                    // mode_str
                    for (i = 0; i < 3; i++) {
                        if ((0x01 << (8 - 3 * i)) & file_stat.st_mode)
                            au[i * 3 + 0] = 'r';
                        else
                            au[i * 3 + 0] = '-';
                        if ((0x01 << (7 - 3 * i)) & file_stat.st_mode)
                            au[i * 3 + 1] = 'w';
                        else
                            au[i * 3 + 1] = '-';
                        if ((0x01 << (6 - 3 * i)) & file_stat.st_mode)
                            au[i * 3 + 2] = 'x';
                        else
                            au[i * 3 + 2] = '-';
                    }
                    sprintf (info + strlen (info), "%s\t", au);
                    break;

                case DISPLAY_NLINK:
                    sprintf (info + strlen (info), "%ld\t", (long)file_stat.st_nlink);
                    break;
                
                case DISPLAY_UID:
                    sprintf (info + strlen (info), "%ld\t", (long)file_stat.st_uid);
                    break;

                case DISPLAY_GID:
                    sprintf (info + strlen (info), "%ld\t", (long)file_stat.st_gid);
                    break;

                case DISPLAY_SIZE:
                    sprintf (info + strlen (info), "%lld\t", (long long)file_stat.st_size);
                    break;

                case DISPLAY_BLKSIZE:
                    sprintf (info + strlen (info), "%ld\t", file_stat.st_blksize);
                    break;

                case DISPLAY_ATIME:
                    sprintf (info + strlen (info), "%s\t", ctime(&file_stat.st_atime));
                    break;

                case DISPLAY_CTIME:
                    sprintf (info + strlen (info), "%s\t", ctime(&file_stat.st_ctime));
                    break;

                case DISPLAY_MTIME:
                    sprintf (info + strlen (info), "%s\t", ctime(&file_stat.st_mtime));
                    break;

                case DISPLAY_NAME:
                    strcat (info, ptr->d_name);
                    strcat (info, "\t");
                    break;
            }
        }
        info[strlen (info) - 1] = 0x00;

        purc_variant_array_append (ret_var, 
                    purc_variant_make_string (info, false));
    }

    closedir(dir);

    return ret_var;
}

static purc_variant_t
mkdir_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    char filename[PATH_MAX] = {0,};
    const char* string_filename = NULL;
    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args != 1)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (*string_filename == '/') {
        strcpy (filename, string_filename);
    }
    else {
        strcpy (filename, get_work_dirctory ());
        strcat (filename, "/");
        strcat (filename, string_filename);
    }

    if (access(filename, F_OK | R_OK) == 0) 
        return purc_variant_make_boolean (false);

    if (mkdir (filename, 777) == 0)
        ret_var = purc_variant_make_boolean (false);
    else
        ret_var = purc_variant_make_boolean (true);

    return ret_var;
}


static purc_variant_t
rmdir_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    char filename[PATH_MAX] = {0,};
    const char* string_filename = NULL;
    DIR *dirp;
    struct dirent *dp;
    struct stat dir_stat;
    purc_variant_t ret_var = NULL;
    bool empty = false;

    if ((argv == NULL) || (nr_args != 1)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (*string_filename == '/') {
        strcpy (filename, string_filename);
    }
    else {
        strcpy (filename, get_work_dirctory ());
        strcat (filename, "/");
        strcat (filename, string_filename);
    }

    if (access(filename, F_OK | R_OK) != 0) 
        return purc_variant_make_boolean (false);

    if (stat(filename, &dir_stat) < 0) 
        return purc_variant_make_boolean (false);

    if (S_ISDIR(dir_stat.st_mode)) {
        dirp = opendir(filename);

        while ((dp=readdir(dirp)) != NULL) {
            if ((strcmp(dp->d_name, ".") == 0) || (strcmp(dp->d_name, "..") == 0)) 
                continue;
            else {
                break;
            }
        }
        closedir(dirp);

        if (empty) {
            if (rmdir(filename) == 0)
                empty = true;
        }
    } 

    if (empty)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
touch_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    char filename[PATH_MAX] = {0,};
    const char* string_filename = NULL;
    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args != 1)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (*string_filename == '/') {
        strcpy (filename, string_filename);
    }
    else {
        strcpy (filename, get_work_dirctory ());
        strcat (filename, "/");
        strcat (filename, string_filename);
    }

    return ret_var;
}


static purc_variant_t
unlink_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    char filename[PATH_MAX] = {0,};
    const char* string_filename = NULL;
    struct stat filestat;
    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args != 1)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (*string_filename == '/') {
        strcpy (filename, string_filename);
    }
    else {
        strcpy (filename, get_work_dirctory ());
        strcat (filename, "/");
        strcat (filename, string_filename);
    }

    if (access(filename, F_OK | R_OK) == 0) 
        return purc_variant_make_boolean (false);

    if (stat(filename, &filestat) < 0) 
        return purc_variant_make_boolean (false);

    if (S_ISREG(filestat.st_mode)) {
        if (unlink (filename) == 0)
            ret_var = purc_variant_make_boolean (false);
        else
            ret_var = purc_variant_make_boolean (true);
    }
    else
        ret_var = purc_variant_make_boolean (false); 

    return ret_var;
}

static purc_variant_t
rm_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    char filename[PATH_MAX] = {0,};
    const char* string_filename = NULL;
    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args != 1)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (*string_filename == '/') {
        strcpy (filename, string_filename);
    }
    else {
        strcpy (filename, get_work_dirctory ());
        strcat (filename, "/");
        strcat (filename, string_filename);
    }

    if (remove_dir (filename))
        ret_var = purc_variant_make_boolean (false);
    else
        ret_var = purc_variant_make_boolean (true);

    return ret_var;

}
// only for test now.
purc_variant_t pcdvojbs_get_fs (void)
{
    purc_variant_t fs = purc_variant_make_object_c (7,
            "list",      purc_variant_make_dynamic (list_getter, NULL),
            "list_prt",  purc_variant_make_dynamic (list_prt_getter, NULL),
            "mkdir",     purc_variant_make_dynamic (mkdir_getter, NULL),
            "rmdir",     purc_variant_make_dynamic (rmdir_getter, NULL),
            "touch",     purc_variant_make_dynamic (touch_getter, NULL),
            "unlink",    purc_variant_make_dynamic (unlink_getter, NULL),
            "rm",        purc_variant_make_dynamic (rm_getter, NULL)
       );
    return fs;
}
