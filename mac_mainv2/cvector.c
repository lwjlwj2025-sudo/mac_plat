#include "config.h"
#include "stdint.h"
#include <stdlib.h>
#include "string.h"
#include "cvector.h"

void cvector_init(cvector *vec)
{
	vec->size = 0;
	vec->total_Data_len = 0;
	vec->head = NULL;
	vec->tail = NULL;
}

void cvector_destory(cvector *vec) // 释放内存
{
	uint32_t i;
	for (i = 0; i < vec->size; i++)
	{
		struct link_list *nlptr = (vec->head->next != NULL) ? vec->head->next : NULL;
		free(vec->head->data);
		free(vec->head);
		vec->head = nlptr;
	}
}

int cvector_pushback(cvector *vec, char *data, uint32_t data_len) // 将新节点添加到链表尾部，为新数据分配空间
{
	struct link_list *llptr = (struct link_list *)malloc(sizeof(struct link_list));
	if (llptr == NULL)
	{
		LOG("malloc failed.");
		return CVEFAILED;
	}
	uint8_t *ptr = (uint8_t *)malloc(data_len);
	if (ptr == NULL)
	{
		LOG("malloc failed.");
		free(llptr);
		return CVEFAILED;
	}
	memcpy(ptr, data, data_len);
	llptr->data = ptr;
	llptr->data_len = data_len;
	llptr->pre = vec->tail;
	llptr->next = NULL;
	if (vec->size > 0)
	{
		vec->tail->next = llptr;
	}
	else
	{
		vec->head = llptr;
	}
	vec->tail = llptr;
	vec->size += 1;
	// 累加vector中业务数据总长
	vec->total_Data_len += data_len;
	return CVESUCCESS;
}

struct link_list *cvector_llget(cvector *vec, uint32_t index) // 获取链表中指定索引index的节点
{
	uint32_t i;
	if (index >= vec->size)
	{
		LOG("%d>=%d", index, vec->size);
		return NULL;
	}
	struct link_list *llptr = vec->head;
	for (i = 0; i < index; i++)
	{
		if (llptr != NULL)
		{
			llptr = llptr->next; // 移动到下一个节点
		}
	}
	return llptr;
}

int cvector_get(cvector *vec, uint32_t index, char *buf, uint32_t buf_len) // 取出特定节点的数据
{
	struct link_list *cllptr = cvector_llget(vec, index);
	if (cllptr == NULL)
	{
		LOG("cllptr==NULL");
		return CVEFAILED;
	}
	if (buf_len < cllptr->data_len)
	{
		return CVEFAILED;
	}
	memcpy(buf, cllptr->data, cllptr->data_len);
	return cllptr->data_len;
}

int cvector_set(cvector *vec, uint32_t index, char *data, uint32_t data_len) // 设置特定节点的数据
{
	struct link_list *cllptr = cvector_llget(vec, index);
	if (cllptr == NULL)
	{
		return CVEFAILED;
	}
	free(cllptr->data);
	cllptr->data = (uint8_t *)malloc(data_len);
	if (cllptr->data == NULL)
	{
		return CVEFAILED;
	}
	cllptr->data_len = data_len;
	memcpy(cllptr->data, data, data_len);
	return CVESUCCESS;
}

int cvector_pophead(cvector *vec, char *buf, uint32_t buflen) // 将头节点推出去
{
	int rst = 0;
	if (vec->size >= 1)
	{
		struct link_list *llptr = vec->head->next;
		if (buflen >= vec->head->data_len && buf != NULL)
		{
			// vector中业务数据总长减去弹出的业务包的大小
			vec->total_Data_len -= vec->head->data_len;
			memcpy(buf, vec->head->data, vec->head->data_len);
			rst = vec->head->data_len;
		}
		free(vec->head->data);
		free(vec->head);
		if (vec->size > 1)
		{
			llptr->pre = NULL;
			vec->head = llptr;
			vec->size -= 1;
		}
		else
		{
			cvector_init(vec);
		}
	}
	return rst;
}

int cvector_popback(cvector *vec, char *buf, uint32_t buflen)
{
	int rst = 0;
	if (vec->size >= 1)
	{
		struct link_list *llptr = vec->tail->pre;
		if (buflen >= vec->tail->data_len && buf != NULL)
		{
			memcpy(buf, vec->tail->data, vec->tail->data_len);
			rst = vec->tail->data_len;
		}
		free(vec->tail->data);
		free(vec->tail);
		if (vec->size > 1)
		{
			vec->tail = llptr;
			vec->size -= 1;
		}
		else
		{
			cvector_init(vec);
		}
	}
	return rst;
}

int cvector_erase(cvector *vec, uint32_t index) // 删除指定索引的数据
{
	if (index >= vec->size)
	{
		return CVEFAILED;
	}
	if (index == 0)
	{ // 移除头节点
		cvector_pophead(vec, NULL, 0);
	}
	else if (index == vec->size - 1)
	{ // 移除尾节点
		cvector_popback(vec, NULL, 0);
	}
	else
	{
		struct link_list *cllptr = cvector_llget(vec, index);
		cllptr->pre->next = cllptr->next;
		cllptr->next->pre = cllptr->pre; // 跳过删除的数据
		free(cllptr->data);
		free(cllptr);
		vec->size -= 1;
	}
	return CVESUCCESS;
}
/* 在索引前插入数据 */
int cvector_insert(cvector *vec, uint32_t index, char *data, uint32_t data_len)
{
	/* 索引超过实际链表长度，直接push */
	if (vec->size <= 0 || index >= vec->size)
	{
		return cvector_pushback(vec, data, data_len);
	}

	struct link_list *llptr = (struct link_list *)malloc(sizeof(struct link_list)); // 把这个插进去
	if (llptr == NULL)
	{
		LOG("malloc failed.");
		return CVEFAILED;
	}
	uint8_t *ptr = (uint8_t *)malloc(data_len);
	if (ptr == NULL)
	{
		LOG("malloc failed.");
		free(llptr);
		return CVEFAILED;
	}
	memcpy(ptr, data, data_len);
	llptr->data = ptr;
	llptr->data_len = data_len;

	struct link_list *rptr = cvector_llget(vec, index);
	struct link_list *lptr = rptr->pre;
	rptr->pre = llptr;
	llptr->pre = lptr;
	llptr->next = rptr;
	if (index == 0)
	{
		vec->head = llptr;
	}
	else
	{
		lptr->next = llptr;
	}
	vec->size += 1;

	return CVESUCCESS;
}

int cvector_swap(cvector *vec, uint32_t index_a, uint32_t index_b) // 更新节点间的指针关系
{
	if (index_a >= vec->size || index_b >= vec->size)
	{
		return CVEFAILED;
	}
	if (index_a == index_b)
		return CVESUCCESS;
	if (index_a > index_b)
	{ // 确保a始终比b小，a在b的前一个节点，交换两者的大小
		uint32_t tmp = index_a;
		index_a = index_b;
		index_b = tmp;
	}
	uint32_t i;
	struct link_list *llptr_a, *llptr_b, *llptr = vec->head;
	for (i = 0; i < vec->size; i++)
	{
		if (i == index_a)
			llptr_a = llptr;
		if (i == index_b)
		{
			llptr_b = llptr;
			break;
		}
		llptr = llptr->next;
	}

	struct link_list *llptr_al, *llptr_ar, *llptr_bl, *llptr_br;
	llptr_al = llptr_a->pre;
	llptr_ar = llptr_a->next;
	llptr_bl = llptr_b->pre;
	llptr_br = llptr_b->next;
	if (NULL != llptr_al)
	{
		llptr_al->next = llptr_b;
	}
	if (NULL != llptr_bl)
	{
		llptr_bl->next = llptr_a;
	}
	llptr_a->next = llptr_br;
	llptr_b->pre = llptr_al;
	if (index_b - index_a == 1)
	{
		llptr_a->pre = llptr_b;
		llptr_b->next = llptr_a;
	}
	else
	{
		llptr_a->pre = llptr_bl;
		llptr_b->next = llptr_ar;
	}
	if (llptr_br != NULL)
	{
		llptr_br->pre = llptr_a;
	}
	if (llptr_ar != NULL)
	{
		llptr_ar->pre = llptr_b;
	}
	if (index_a == 0)
		vec->head = llptr_b;
	if (index_b == vec->size - 1)
		vec->tail = llptr_a;
	return CVESUCCESS;
}
