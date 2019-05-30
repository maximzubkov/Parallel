//
// Created by Максим on 25.10.2018.
//

#ifndef LS_BETA_FLAG_PRINT_H
#define LS_BETA_FLAG_PRINT_H

#endif //LS_BETA_FLAG_PRINT_H

struct flag_struct // Структура, хранящая флаги
{
    int i_flag;
    int a_flag;
    int d_flag;
    int R_flag;
    int n_flag;
    int l_flag;
};

struct flag_struct * f_create()
{
    struct flag_struct * f;
    f = (struct flag_struct *)malloc(sizeof(struct flag_struct));
    f->d_flag = 0;
    f->l_flag = 0;
    f->a_flag = 0;
    f->i_flag = 0;
    f->n_flag = 0;
    f->R_flag = 0;
    return f;
}

void str_mode(mode_t mode, char *buf)
{
    const char chars[] = "rwxrwxrwx";
    for (int i = 0; i < 9; i++)
    {
        buf[i] = (mode & (1 << (8-i))) ? chars[i] : '-';
    }
    buf[9] = '\0';
}

int if_l (const char *start_dir, int n_flag)
{
    struct stat buf;
    if (stat(start_dir, &buf) < 0)
    {
        printf ("ERROR\n");
        return 0;
    }
    char mode[10];
    str_mode(buf.st_mode, mode);
    printf("-%s ", mode);
    printf("%hu ", buf.st_nlink);
    if (n_flag == 0) {
        struct passwd *pws;
        struct group *grp;
        pws = getpwuid(buf.st_uid);
        printf("%s  ", pws->pw_name);
        grp = getgrgid(buf.st_gid);
        printf("%s ",grp->gr_name);
    }
    printf("%lld ", buf.st_size);
    char * time = asctime(gmtime(&buf.st_mtime));
    time[sizeof(time)] = '\0';
    printf("%s", time);
    return 0;
}

int if_i  (const char *start_dir)
{
    struct stat buf;
    if (stat(start_dir, &buf) < 0)
    {
        printf ("ERROR\n");
        return 0;
    }
    printf("%llu ", buf.st_ino);
    return 0;
}

int if_n  (const char * start_dir)
{
    struct stat buf;
    if (stat(start_dir, &buf) < 0)
    {
        printf ("ERROR\n");
        return 0;
    }
    struct passwd *pws;
    struct group *grp;
    pws = getpwuid(buf.st_uid);
    printf("%s ", pws->pw_name);
    grp = getgrgid(buf.st_gid);
    printf("%s ", grp->gr_name);
    return 0;
}


void get_stat(char * name, struct flag_struct * f){  //Вывод статистики
    if (f->l_flag == 1) {
        if_l(name, f->n_flag);
    }
    if (f->n_flag == 1) {
        if_n(name);
    }
    if (f->i_flag == 1) {
        if_i(name);
    }
}