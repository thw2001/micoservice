#ifndef UINTPTR_TO_STRING_H
#define UINTPTR_TO_STRING_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <ctype.h>

void uintptr_to_string(uintptr_t addr, char* buff, size_t buff_size) {
    // 格式化输出：固定16字符宽度，前导零填充
    snprintf(buff, buff_size, "0x%016" PRIxPTR, addr);
}
int string_to_uintptr(const char* str, uintptr_t* result) {
    if (!str || !result) return -1;// 参数检查
    while (isspace(*str)) str++;// 跳过空白字符
    if (str[0] != '0' || (str[1] != 'x' && str[1] != 'X'))// 验证十六进制前缀
        return -1;
    str += 2;
    // 验证长度（16字符）
    const char* p = str;
    while (isxdigit(*p)) p++;
    if (p - str != 16 || *p != '\0')
        return -1;
    // 转换并检查范围
    char* endptr;
    errno = 0;
    unsigned long long val = strtoull(str, &endptr, 16);
    if (errno == ERANGE || val > UINTPTR_MAX)
        return -1;
    *result = (uintptr_t)val;
    return 0;
}
#endif
