#ifndef SNOW_FLAKE_H
#define SNOW_FLAKE_H
#ifdef __cplusplus
extern "C"
{
#endif

#define SNOW_FLAKE_LEN  25

/**
 * @brief 生成一个随机的SNOW_FLAKE_LEN位字符串
 * 
 * @return char* 需要手动free
 */
char* id_generate_string();

#ifdef __cplusplus
}
#endif
#endif // SNOW_FLAKE_H