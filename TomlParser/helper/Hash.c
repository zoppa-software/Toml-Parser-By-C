#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <crtdbg.h>
#include "Assertion.h"
#include "Instance.h"
#include "Hash.h"

/** �L�[���ڐ����u���b�N�T�C�Y�B */
#define KEY_BLOCK_SIZE		32

/**
 * ���������\���́B
 *   �J�v�Z�����̂��ߔ���J
 */
typedef struct _HashImpl
{
	// ���J���̎���
	Hash				hash;

	// �L�[���̒���
	size_t				key_length;

	// �L�[���̎��̃��X�g
	Instance			key_instance;

	// �Ǘ�������e�[�u��
	HashPair *			tables;

	// �Ǘ�������e�[�u���̃T�C�Y
	size_t				capacity;

} HashImpl;

//-----------------------------------------------------------------------------
// �����@�\
//-----------------------------------------------------------------------------
/**
 * fnv1 �n�b�V���L�[�쐬�A���S���Y���B
 *
 * @param bytes		�L�[������B
 * @param length	�����񒷁B
 * @return			�n�b�V���l�B
 */
static size_t fnv_1_hash_32(const char * bytes, size_t length)
{
	size_t hash;
	const char * ptr;
	size_t i;

	hash = 2166136261U;
	for (i = 0, ptr = bytes; i < length; ++i, ++ptr) {
		hash = (16777619U * hash) ^ (*ptr);
	}

	return hash;
}

/**
 * �v�f�m�F�@�\�i���������j
 *
 * @param tables		�n�b�V���e�[�u���̈�B
 * @param table_size	�n�b�V���e�[�u���T�C�Y�B
 * @param key			�����L�[�B
 * @param key_len		�L�[�̃o�C�g���B
 * @param result		���ڂ����݂����Ƃ��A�L�[�^�l�̃y�A�i�߂�l�j
 * @param point			���ڂ����݂����Ƃ��A�o�^��̃e�[�u���ʒu�i�߂�l�j
 * @return				���ڂ����݂���� 0�ȊO�B
 */
static int hash_inner_contains(HashPair *	tables,
							   size_t		table_size,
							   const void *	key,
							   size_t		key_len,
							   HashPair *	result,
							   size_t *		point)
{
	size_t ptr;
	size_t hash_pos;
	HashPair find;

	// ������̃n�b�V���l���擾
	size_t hash_val = fnv_1_hash_32((char *)key, key_len);

	// �n�b�V���e�[�u���̑���
	//
	// 1. �e�[�u���ʒu�̌v�Z
	// 2. �v�f���擾
	//    2-1. �v�f���Ȃ���Ζ�����Ԃ�
	//    2-2. �v�f������A�����񂪈�v����Ȃ�ΗL���Ԃ�
	for (ptr = 0; ptr <= table_size / 2; ++ptr) {
		hash_pos = (hash_val + ptr * ptr) % table_size;		// 1

		find = tables[hash_pos];							// 2

		if (!find.key) {
			*point = hash_pos;								// 2-1
			return 0;
		}
		else {
			if (memcmp(find.key, (char *)key, key_len) == 0) {	// 2-2
				*result = find;
				*point = hash_pos;
				return 1;
			}
		}
	}

	return 0;
}

/**
 * �n�b�V���e�[�u���̍č\���B
 *
 * @param impl		�č\������n�b�V���e�[�u���B
 */
static void hash_realloc(HashImpl * impl)
{
	unsigned long long int	new_size = ((unsigned long long int)impl->capacity) * 2;
	unsigned long long int	div;
	HashPair * temp;
	HashPair * ptr;
	HashPair result;
	int hit = 0;
	size_t point;

	// �g�������T�C�Y�ȉ��̑f�������߂�
	for (; new_size < UINT_MAX; ++new_size) {
		if ((new_size & 1) == 1) {
			hit = 0;

			for (div = 3; (double)div <= sqrt((double)new_size); div += 2) {
				if (new_size % div == 0) {
					hit = 1;
					break;
				}
			}

			if (!hit) {
				break;
			}
		}
	}

	// ��L�T�C�Y�ŗ̈���č쐬
	temp = (HashPair*)malloc(sizeof(HashPair) * (size_t)new_size);
	Assert(temp != 0, "realloc error");
	_RPTN(_CRT_WARN, "hash_realloc malloc 0x%X\n", temp);
	memset((void*)temp, 0, sizeof(HashPair) * (size_t)new_size);

	// �o�^�ς݂̕���������Ɋi�[
	if (impl->tables) {
		ptr = impl->tables;
		for (ptr = impl->tables; ptr < impl->tables + impl->capacity; ++ptr) {
			if (ptr->key) {
				hash_inner_contains(temp, new_size, ptr->key, impl->key_length, &result, &point);
				temp[point].key = ptr->key;
				temp[point].value = ptr->value;
			}
		}
	}

	// �ߋ��̗̈�����
	free((void*)impl->tables);

	// �̈�̎Q�ƁA�T�C�Y���X�V
	impl->tables = temp;
	impl->capacity = new_size;
}

//-----------------------------------------------------------------------------
// �R���X�g���N�^�A�f�X�g���N�^
//-----------------------------------------------------------------------------
/**
 * �n�b�V���e�[�u���̏��������s���B
 *
 * @param key_length	�L�[�̃T�C�Y�i�o�C�g���j
 * @return				�C���X�^���X�B
 */
const Hash * hash_initialize(size_t key_length)
{
	HashImpl * res = (HashImpl*)malloc(sizeof(HashImpl));
	Assert(res != 0, "alloc error");
	_RPTN(_CRT_WARN, "hash_initialize malloc 0x%X\n", res);

	memset(res, 0, sizeof(HashImpl));
	res->capacity = 16;
	res->key_length = key_length;
	res->key_instance = instance_initialize(key_length, KEY_BLOCK_SIZE);
	hash_realloc(res);
	return (Hash*)res;
}

/**
 * �n�b�V���e�[�u�����폜����B
 *
 * @param ins_del		�폜����C���X�^���X�B
 */
void hash_delete(const Hash ** ins_del)
{
	HashImpl ** impl = (HashImpl**)ins_del;

	Assert(ins_del != 0, "null pointer del error");

	instance_delete(&(*impl)->key_instance);

	free((void*)(*impl)->tables);
	free(*impl);
	*ins_del = 0;
}

//-----------------------------------------------------------------------------
// �m�F�A�ǉ��A�폜
//-----------------------------------------------------------------------------
/**
 * ���ڂ̑��݊m�F���s���B
 *
 * @param instance		�Ώۃn�b�V���e�[�u���B
 * @param key			�m�F����L�[�B
 * @param result		���ڂ����݂����Ƃ��A���ݍ��ڂ̃|�C���^�i�߂�l�j
 * @return				���ڂ����݂���Ƃ� 0�ȊO��Ԃ��B
 */
int hash_contains(const Hash * instance, const void * key, HashPair * result)
{
	HashImpl * impl = (HashImpl*)instance;
	size_t point;
	return hash_inner_contains(impl->tables, impl->capacity, key, impl->key_length, result, &point);
}

/**
 * ���ڂ̒ǉ����s���B
 *
 * @param instance		�Ώۃn�b�V���e�[�u���B
 * @param key			�ǉ����鍀�ڂ̃L�[�B
 * @param value			�ǉ����鍀�ځB
 * @return				�n�b�V���e�[�u���ɒǉ��������ڃy�A�B
 */
HashPair hash_add(const Hash * instance, const void * key, HashValue value)
{
	HashImpl * impl = (HashImpl*)instance;
	HashPair result;
	void * item;
	size_t point;

	// �ǉ�����
	//
	// 1. ���ɒǉ��ς݂Ȃ�Γo�^����Ă��鍀�ڂ�Ԃ�
	// 2. �o�^����Ă��Ȃ���Βǉ����s��
	//    2-1. �ǉ�����L�[���̃|�C���^���擾
	//    2-2. �L�[�����R�s�[���A�e�[�u���ɒǉ�
	//    2-3. �e�[�u������t�ɂȂ�����č\��
	if (hash_inner_contains(impl->tables, impl->capacity, key, impl->key_length, &result, &point)) {
		return result;									// 1
	}
	else {
		item = instance_pop(&impl->key_instance);		// 2-1
	
		memcpy(item, key, impl->key_length);			// 2-2
		result.key = item;
		result.value = value;
		impl->tables[point] = result;
		impl->hash.count++;
	
		if ((double)impl->hash.count > impl->capacity * 0.7) {
			hash_realloc(impl);							// 2-3
		}
	
		return result;
	}
}

/**
 * �n�b�V���e�[�u�����A�w��̃L�[�̍��ڂ��폜����B
 *
 * @param instance		�Ώۃn�b�V���e�[�u���B
 * @param key			�폜���鍀�ڂ̃L�[�B
 */
void hash_remove(const Hash * instance, const void * key)
{
	HashImpl *	impl = (HashImpl*)instance;
	HashPair	result;
	size_t		point;
	
	// ���ڂ����݂���΍폜����
	if (hash_inner_contains(impl->tables, impl->capacity, key, impl->key_length, &result, &point)) {
		instance_push(&impl->key_instance, (void*)impl->tables[point].key);

		memset(impl->tables + point, 0, sizeof(HashPair));
		impl->hash.count--;
	}
}

//-----------------------------------------------------------------------------
// �f�o�b�O
//-----------------------------------------------------------------------------
/**
 * �Ǘ����Ă��鍀�ڂ���ʂɏo�͂���B
 *
 * @param instance			�Ώۃn�b�V���e�[�u���B
 * @param show_function		�����ڏ��֐��|�C���^�B
 */
void hash_show(const Hash * instance,
			   hash_show_function show_function)
{
	HashImpl * impl = (HashImpl*)instance;
	HashPair * ptr;
	
	ptr = impl->tables;
	for (ptr = impl->tables;
		 ptr < impl->tables + impl->capacity; ++ptr) {
		if (ptr->key) {
			show_function(*ptr);
		}
	}
}

/**
 * �Ǘ����Ă��鍀�ڂ����ɕ]������B
 *
 * @param instance			�Ώۃn�b�V���e�[�u���B
 * @param for_function		�����ڏ��֐��|�C���^�B
 * @param param				�p�����[�^�B
 */
void hash_foreach(const Hash * instance,
				  hash_for_function for_function,
				  void * param)
{
	HashImpl * impl = (HashImpl*)instance;
	HashPair * ptr;
	
	ptr = impl->tables;
	for (ptr = impl->tables;
		 ptr < impl->tables + impl->capacity; ++ptr) {
		if (ptr->key) {
			for_function(*ptr, param);
		}
	}
}

//-----------------------------------------------------------------------------
// �l�ϊ�
//-----------------------------------------------------------------------------
/**
 * �n�b�V���e�[�u���ɓo�^����l���쐬�i�����l�j
 *
 * @param value		�o�^����l�B
 * @return			�l�\���́B
 */
HashValue hash_value_of_integer(int value)
{
	HashValue res;
	res.integer = value;
	return res;
}

/**
 * �n�b�V���e�[�u���ɓo�^����l���쐬�i�����l�j
 *
 * @param value		�o�^����l�B
 * @return			�l�\���́B
 */
HashValue hash_value_of_float(double value)
{
	HashValue res;
	res.float_number = value;
	return res;
}

/**
 * �n�b�V���e�[�u���ɓo�^����l���쐬�i�|�C���^�j
 *
 * @param value		�o�^����l�B
 * @return			�l�\���́B
 */
HashValue hash_value_of_pointer(void * value)
{
	HashValue res;
	res.object = value;
	return res;
}