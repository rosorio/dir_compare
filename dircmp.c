#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdbool.h>
#include <libgen.h>
#include <string.h>
#include <getopt.h>


bool g_debug = false;

void print_usage(char * appname)
{
    printf("Usage: %s <dir1> <dir2>\n", appname);
}

int cmp_dirs(char *src, char *dst)
{
    DIR *d1, *d2;
    int ret     = 0;
    int count   = 0;

    if (g_debug) {
        printf("%s: Compare: %s %s\n",__func__, src,dst);
    }

    d1 = opendir(src);
    d2 = opendir(dst);
    if (d1 == NULL || d2 == NULL) {
        ret = -1;
        goto end;
    }

    while (readdir(d1) != NULL) {
        count++;
    }

    while (readdir(d2) != NULL) {
        count--;
    }

    ret = count;

end:
    if (d1) {
        closedir(d1);
    }

    if (d2) {
        closedir(d2);
    }

    return ret;
}

#define EVAL_ST(o, param) ((o[0].param == o[1].param) ? true : false)
#define EXPAND_ST(o, param) o[0].param, o[1].param

int cmp_files(bool flat_symlink, char *src, char *dst)
{
    struct stat st[2];
    int rc;

    if (g_debug) {
        printf("%s: Compare: %s %s\n",__func__, src,dst);
    }
    rc = (flat_symlink == true) ? stat(src, &(st[0])) : lstat(src, &(st[0]));
    if (rc != 0) {
        if (g_debug) {
            printf("%s: stat fail for %s\n", __func__, src);
        }
        return -1;
    }

    rc = stat(dst, &(st[1]));
    if (rc != 0) {
        if (g_debug) {
            printf("%s: stat fail for %s\n", __func__, dst);
        }
        return -1;
    }

    /* compare file content */

    if(EVAL_ST(st,st_uid) != true) {
        if (g_debug) {
            printf("uid test fail %d %d\n", EXPAND_ST(st,st_uid));
        }
        return -1;
    }

    if(EVAL_ST(st,st_gid) != true) {
        if (g_debug) {
            printf("gid test fail\n");
        }
        return -1;
    }

    if(EVAL_ST(st,st_mode) != true) {
        if (g_debug) {
            printf("mode test fail\n");
        }
        return -1;
    }
/*
    if(EVAL_ST(st,st_ctim) == true) {
        return -1;
    }
*/
    if(EVAL_ST(st,st_mtime) != true) {
        if (g_debug) {
            printf("mtime test fail\n");
        }
        return -1;
    }

    if (S_ISDIR(st[1].st_mode)) {
        return cmp_dirs(src, dst);
    }

    if(EVAL_ST(st,st_size) != true) {
        if (g_debug) {
            printf("size test fail\n");
        }
        return -1;
    }

    return 0;
}

int path_comp(bool flat_symlink, char * path1, char * path2) {
    DIR *dp;
    int ret = 0;
    char *p1, *p2;
    struct stat st;
    struct dirent *ent;

    if (g_debug) {
        printf("%s: %s %s\n", __func__, path1, path2);
    }

    dp = opendir(path1);
    if (dp == NULL) {
        return -1;
    }

    while ((ent = readdir(dp)) != NULL && ret == 0) {
        if(strcmp(ent->d_name,".") == 0  || strcmp(ent->d_name,"..") == 0) {
            continue;
        }

        asprintf(&p1,"%s/%s", path1, ent->d_name);
        asprintf(&p2,"%s/%s", path2, ent->d_name);

        ret = (flat_symlink == true) ? stat(p1, &st) : lstat(p1, &st);
        if (ret != 0) {
            return -1;
        }

        ret = cmp_files(flat_symlink, p1, p2);

        if (ret == 0 && S_ISDIR(st.st_mode)) {
           ret = path_comp(flat_symlink, p1, p2);
        }

        free(p1);
        free(p2);
    }
    closedir(dp);

    return ret;
}

int main(int argc, char * argv[])
{
    bool flat_symlink = false;
    char ch;
    char * appname = basename(argv[0]);

    while ((ch = getopt(argc, argv, "df")) != -1) {
        switch (ch) {
            case 'f':
                flat_symlink = true;
                break;
            case 'd':
                g_debug = true;
                break;
            default:
                printf(">>>%c\n", ch);
                print_usage(appname);
                exit (-1);
                break;
        }
    }
    argc -= optind;
    argv += optind;

    if(argc < 2) {
        printf("%d<<<\n", argc);
        print_usage(appname);
        exit (-1);
    }


    char * dir1 = argv[0];
    char * dir2 = argv[1];

    int ret = path_comp(flat_symlink, dir1, dir2);

    if(g_debug) {
        printf("Result: %s (%d)\n", ret?"fail":"success", ret);
    }

    return(ret);
}
