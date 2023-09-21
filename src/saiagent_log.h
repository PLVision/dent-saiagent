#ifndef __SAIAGENT_LOG_H
#define __SAIAGENT_LOG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define LOG_INFO(...)\
	do {                                              \
		printf("SAIAGENT: ");        \
		printf(__VA_ARGS__);                   \
		printf("\n");                          \
	} while (0)

#define LOG_ERR(...)\
	do {                                              \
		printf("SAIAGENT: ");        \
		printf("[%s:%d] ",__func__, __LINE__);\
		printf(__VA_ARGS__);                   \
		printf("\n");                          \
	} while (0)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SAIAGENT_LOG_H */
