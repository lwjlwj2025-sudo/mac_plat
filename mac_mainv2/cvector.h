#ifndef CVECTOR_H
#define CVECTOR_H
#include <stdint.h>

#define CVEFAILED -1
#define CVESUCCESS 0

struct link_list
{
	uint8_t *data;
	uint32_t data_len;
	struct link_list *pre;
	struct link_list *next;
};

typedef struct
{
	uint16_t size;
	uint32_t total_Data_len; // 用于记录vector中数据的总长
	struct link_list *head;
	struct link_list *tail;
} cvector;

void cvector_init(cvector *vec);
void cvector_destory(cvector *vec);
int cvector_pushback(cvector *vec, char *data, uint32_t data_len);
int cvector_get(cvector *vec, uint32_t index, char *buf, uint32_t buf_len);
int cvector_set(cvector *vec, uint32_t index, char *data, uint32_t data_len);
int cvector_pophead(cvector *vec, char *buf, uint32_t buflen);
int cvector_popback(cvector *vec, char *buf, uint32_t buflen);
int cvector_erase(cvector *vec, uint32_t index);
int cvector_insert(cvector *vec, uint32_t index, char *data, uint32_t data_len);
int cvector_swap(cvector *vec, uint32_t index_a, uint32_t index_b);
struct link_list *cvector_llget(cvector *vec, uint32_t index);
#endif