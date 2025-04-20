#ifndef __UTILS_ERROR_H__
#define __UTILS_ERROR_H__

void _throw(const char* reason, const char* file, int line, const char* func);

#define throw(msg) _throw(msg, __FILE__, __LINE__, __func__)

#endif