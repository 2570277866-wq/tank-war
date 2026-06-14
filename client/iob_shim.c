// iob_shim.c — MinGW UCRT 兼容层
// 新版 UCRT 移除了 __iob_func，改用 __acrt_iob_func
// 旧版 EasyX（libeasyx.a）仍引用 __imp___iob_func
// 此 shim 提供过渡符号，配合 ld --defsym 将 imp 别名到本地定义

#include <stdio.h>
#include <stdlib.h>

FILE* __cdecl __iob_func(void) {
    static FILE* iob[3] = {NULL, NULL, NULL};
    if (!iob[0]) {
        iob[0] = (FILE*)__acrt_iob_func(0);
        iob[1] = (FILE*)__acrt_iob_func(1);
        iob[2] = (FILE*)__acrt_iob_func(2);
    }
    return (FILE*)iob;
}
