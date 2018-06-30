#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crtdbg.h>
#include "Assertion.h"
#include "Vec.h"

// �����e�ʃf�t�H���g�l
#define DEFAULT_CAPACITY	16

/**
 * �ϒ��z��\���́A����
 */
typedef struct _VecImpl
{
	// �ϒ��z��
	Vec		vector;

	// �v�f�̃f�[�^��
	size_t	data_len;

	// �擾�ςݗ̈撷
	size_t	capacity;

} VecImpl;

/**
 * �ߋ��̗̈��������āA�̈�T�C�Y��傫������B
 *
 * @param vec		�ϒ��z��B
 * @param capacity	�V�����̈�T�C�Y�B
 */
static void vec_allocate(VecImpl * vec, size_t capacity)
{
	// �V�����̈���擾
	void * temp = malloc(capacity * vec->data_len);
	Assert(temp != 0, "vec allocate error");
	_RPTN(_CRT_WARN, "vec_allocate malloc 0x%X\n", temp);

	// �ߋ��f�[�^��V�̈�փR�s�[
	memcpy(temp, vec->vector.pointer, vec->vector.length * vec->data_len);

	// �ߋ��̗̈�����
	free(vec->vector.pointer);
	vec->vector.pointer = temp;
	vec->capacity = capacity;
}

//-----------------------------------------------------------------------------
// �R���X�g���N�^�A�f�X�g���N�^
//-----------------------------------------------------------------------------
/**
 * �ϒ��z��̏��������s���B
 *
 * @param data_len		��̃f�[�^���B
 * @return				�ϒ��z��B
 */
Vec * vec_initialize(size_t data_len)
{
	return vec_initialize_set_capacity(data_len, DEFAULT_CAPACITY);
}

/**
 * �ϒ��z��̏��������s���i�����e�ʎw��j
 *
 * @param data_len			��̃f�[�^���B
 * @param start_capacity	�����e�ʁB
 * @return					�ϒ��z��B
 */
Vec * vec_initialize_set_capacity(size_t data_len, size_t start_capacity)
{
	VecImpl * res = (VecImpl*)malloc(sizeof(VecImpl));
	Assert(res != 0, "vec allocate error");
	_RPTN(_CRT_WARN, "vec_initialize_set_capacity malloc 0x%X\n", res);

	// �ϒ��̈��ݒ�
	memset(res, 0, sizeof(VecImpl));
	res->data_len = data_len;
	res->vector.pointer = malloc(start_capacity * data_len);
	Assert(res->vector.pointer != 0, "vec allocate error");
	_RPTN(_CRT_WARN, "vec_initialize_set_capacity malloc 0x%X\n", res->vector.pointer);
	res->capacity = start_capacity;

	return (Vec*)res;
}

/**
 * �ϒ��z����폜����B
 *
 * @param ins_del		�폜����ϒ��z��B
 */
void vec_delete(Vec ** ins_del)
{
	VecImpl * vec = (VecImpl*)(*ins_del);

	if (vec != 0) {
		free(vec->vector.pointer);
		free(vec);
		(*ins_del) = 0;
	}
}

//-----------------------------------------------------------------------------
// �Q�Ƌ@�\
//-----------------------------------------------------------------------------
/**
 * �ϒ��z����v�f�ւ̎Q�Ƃ��擾����
 *
 * @param vector		�ϒ��z��B
 * @param index			�v�f�ʒu�B
 * @return				�v�f���Q�Ƃ���|�C���^�B
 */
void * vec_items(Vec * vector, size_t index)
{
	VecImpl * vec = (VecImpl*)vector;

	if (index < vec->vector.length) {
		return (void*)((char*)vec->vector.pointer + vec->data_len * index);
	}
	else {
		// �Q�ƈʒu���͈͊O
		Abort("out of range error");
	}
}

//-----------------------------------------------------------------------------
// �ǉ��ƍ폜
//-----------------------------------------------------------------------------
/**
 * �ϒ��z�����������B
 *
 * @param vector		�ϒ��z��B
 */
void vec_clear(Vec * vector)
{
	VecImpl * vec = (VecImpl*)vector;
#if _DEBUG
	memset((char*)vec->vector.pointer, 0, vec->data_len * vec->capacity);
#endif
	vec->vector.length = 0;
}

/**
 * �ϒ��z��ɗv�f��ǉ�����B
 *
 * @param vector		�ϒ��z��B
 * @param data			�ǉ�����v�f�B
 */
void vec_add(Vec * vector, void * data)
{
	VecImpl * vec = (VecImpl*)vector;
	size_t explen;

	// �ǉ��e�ʂ��s�����Ă�����ǉ�
	if (vec->vector.length + 1 >= vec->capacity) {
		explen = (vec->capacity < VEC_EXPAND_LENGTH ? vec->capacity : VEC_EXPAND_LENGTH);
		vec_allocate(vec, vec->capacity + explen);
	}

	// �f�[�^������
	memcpy((char*)vec->vector.pointer + vec->data_len * vec->vector.length, data, vec->data_len);
	vec->vector.length++;
}

/**
 * �ϒ��z��̎w��ʒu�̗v�f���폜����B
 *
 * @param vector		�ϒ��z��B
 * @param index			�폜����ʒu�B
 */
void vec_remove_at(Vec * vector, size_t index)
{
	VecImpl * vec = (VecImpl*)vector;
	if (index < vec->vector.length - 1) {
		// �r���̗v�f���폜���ꂽ�Ȃ�΁A�v�f���񂹂�
		size_t dst = vec->data_len * index;
		size_t src = dst + vec->data_len;
		size_t len = vec->data_len * (vec->vector.length - index - 1);
		memmove_s((char*)vec->vector.pointer + dst, vec->capacity - dst, (char*)vec->vector.pointer + src, len);
#if _DEBUG
		memset((char*)vec->vector.pointer + vec->data_len * (vec->vector.length - 1), 0, vec->data_len);
#endif
		vec->vector.length--;
	}
	else if (index == vec->vector.length - 1) {
		// �Ō�̗v�f�̂ݍ폜����
#if _DEBUG
		memset((char*)vec->vector.pointer + index, 0, vec->data_len);
#endif
		vec->vector.length--;
	}
	else {
		// �폜�ʒu���͈͊O
		Abort("out of range error");
	}
}