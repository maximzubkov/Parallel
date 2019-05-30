//
// Created by Максим on 25.10.2018.
//

#ifndef LS_BETA_COMPARE_H
#define LS_BETA_COMPARE_H

#endif //LS_BETA_COMPARE_H

int mycmp(const char *a, const char *b) {
    const char *cp1 = a, *cp2 = b;

    for (; (*cp1) == (*cp2); cp1++, cp2++)
        if (*cp1 == '\0')
            return 0;
    return (((*cp1) < (*cp2)) ? -1 : +1);
}

int cmp(const void *p1, const void *p2){
    return mycmp(* (char * const *) p1, * (char * const *) p2);
}
