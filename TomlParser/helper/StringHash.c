#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <crtdbg.h>
#include "Assertion.h"
#include "StringHash.h"

/**
 * ���������\���́B
 *   �J�v�Z�����̂��ߔ���J
 */
typedef struct _StringHashImpl
{
	// ���J���̎���
	StringHash			hash;

	// �Ǘ�������e�[�u��
	stringhash_key *	tables;

	// �Ǘ�������e�[�u���̃T�C�Y
	size_t				capacity;

} StringHashImpl;

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
    for(i = 0, ptr = bytes; i < length; ++i, ++ptr) {
        hash = (16777619U * hash) ^ (*ptr);
    }

    return hash;
}

/**
 * �v�f�m�F�@�\�i���������j
 *
 * @param tables		�n�b�V���e�[�u���̈�B
 * @param table_size	�n�b�V���e�[�u���T�C�Y�B
 * @param key			����������B
 * @param result		�����񂪑��݂����Ƃ��A�o�^������̃|�C���^�i�߂�l�j
 * @param point			�����񂪑��݂����Ƃ��A�o�^������̃e�[�u���ʒu�i�߂�l�j
 * @return				���ڂ����݂���� 0�ȊO�B
 */
static int stringhash_inner_contains(stringhash_key * tables,
									 size_t table_size,
									 stringhash_key key,
									 stringhash_key * result,
									 size_t * point)
{
	size_t len = strlen(key);
	size_t ptr;
	size_t hash_pos;
	stringhash_key find;

	// ������̃n�b�V���l���擾
	size_t hash_val = fnv_1_hash_32(key, len);

	// �n�b�V���e�[�u���̑���
	//
	// 1. �e�[�u���ʒu�̌v�Z
	// 2. �v�f���擾
	//    2-1. �v�f���Ȃ���Ζ�����Ԃ�
	//    2-2. �v�f������A�����񂪈�v����Ȃ�ΗL���Ԃ�
	for (ptr = 0; ptr <= table_size / 2; ++ptr) {
		hash_pos = (hash_val + ptr * ptr) % table_size;		// 1

		find = tables[hash_pos];							// 2

		if (!find) {
			*point = hash_pos;								// 2-1
			return 0;
		}
		else {
			if (strcmp(find, key) == 0) {					// 2-2
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
static void stringhash_realloc(StringHashImpl * impl)
{
	unsigned long long int	new_size = ((unsigned long long int)impl->capacity) * 2;
	unsigned long long int	div;
	stringhash_key * temp;
	stringhash_key * ptr;
	stringhash_key result;
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
	temp = (const char **)malloc(sizeof(const char *) * (size_t)new_size);
	Assert(temp != 0, "realloc error");
	_RPTN(_CRT_WARN, "stringhash_realloc malloc 0x%X\n", temp);
	memset((void*)temp, 0, sizeof(const char *) * (size_t)new_size);

	// �o�^�ς݂̕���������Ɋi�[
	if (impl->tables) {
		ptr = impl->tables;
		for (ptr = impl->tables; ptr < impl->tables + impl->capacity; ++ptr) {
			if (*ptr) {
				stringhash_inner_contains(temp, new_size, *ptr, &result, &point);
				temp[point] = *ptr;
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
 * ������n�b�V���e�[�u���̏��������s���B
 *
 * @return				�C���X�^���X�B
 */
const StringHash * stringhash_initialize()
{
	StringHashImpl * res = (StringHashImpl*)malloc(sizeof(StringHashImpl));
	Assert(res != 0, "alloc error");
	_RPTN(_CRT_WARN, "stringhash_initialize malloc 0x%X\n", res);

	memset(res, 0, sizeof(StringHashImpl));
	res->capacity = 16;
	stringhash_realloc(res);
	return (StringHash*)res;
}

/**
 * ������n�b�V���e�[�u�����폜����B
 *
 * @param ins_del		�폜����C���X�^���X�B
 */
void stringhash_delete(const StringHash ** ins_del)
{
	StringHashImpl ** impl = (StringHashImpl**)ins_del;
	const char ** ptr;

	Assert(ins_del != 0, "null pointer del error");
	
	ptr = (*impl)->tables;
	for (ptr = (*impl)->tables;
			ptr < (*impl)->tables + (*impl)->capacity; ++ptr) {
		if (*ptr) {
			free((char*)*ptr);
		}
	}
	
	free((void*)(*impl)->tables);
	free(*impl);
	*ins_del = 0;
}

//-----------------------------------------------------------------------------
// �m�F�A�ǉ��A�폜
//-----------------------------------------------------------------------------
/**
 * ������̑��݊m�F���s���B
 *
 * @param instance		�Ώۃn�b�V���e�[�u���B
 * @param key			�m�F���镶����B
 * @param result		�����񂪑��݂����Ƃ��A���ݕ�����̃|�C���^�i�߂�l�j
 * @return				�����񂪑��݂���Ƃ� 0�ȊO��Ԃ��B
 */
int stringhash_contains(const StringHash * instance, stringhash_key key, stringhash_key * result)
{
	StringHashImpl * impl = (StringHashImpl*)instance;
	size_t point;
	return stringhash_inner_contains(impl->tables, impl->capacity, key, result, &point);
}

/**
 * ������̒ǉ����s���B
 *
 * @param instance		�Ώۃn�b�V���e�[�u���B
 * @param key			�ǉ����镶����B
 * @return				�n�b�V���e�[�u���ɒǉ����ꂽ������̃|�C���^�B
 */
stringhash_key stringhash_add(const StringHash * instance, stringhash_key key)
{
	StringHashImpl * impl = (StringHashImpl*)instance;
	const char * result;
	const char * item;
	size_t point;
	size_t len;

	// �ǉ�����
	//
	// 1. ���ɒǉ��ς݂Ȃ�Γo�^����Ă��镶�����Ԃ�
	// 2. �o�^����Ă��Ȃ���Βǉ����s��
	//    2-1. �ǉ����镶����̃|�C���^���쐬
	//    2-2. �L�[��������R�s�[���A�e�[�u���ɒǉ�
	//    2-3. �e�[�u������t�ɂȂ�����č\��
	if (stringhash_inner_contains(impl->tables, impl->capacity, key, &result, &point)) {
		return result;								// 1
	}
	else {
		len = strlen(key);							// 2-1
		item = (char*)malloc(len + 1);
		Assert(item != 0, "item new error");
		_RPTN(_CRT_WARN, "stringhash_add malloc 0x%X\n", item);

		strncpy_s((char*)item, len + 1, key, len);	// 2-2
		impl->tables[point] = item;
		impl->hash.count++;

		if ((double)impl->hash.count > impl->capacity * 0.7) {
			stringhash_realloc(impl);				// 2-3
		}

		return item;
	}
}

/**
 * ������̒ǉ����s���i������̕����j
 *
 * @param instance		�Ώۃn�b�V���e�[�u���B
 * @param key			�ǉ����镶����B
 * @param key_len		�����񒷁B
 * @return				�n�b�V���e�[�u���ɒǉ����ꂽ������̃|�C���^�B
 */
stringhash_key stringhash_add_partition(const StringHash * instance, stringhash_key key, size_t key_len)
{
	StringHashImpl * impl = (StringHashImpl*)instance;
	const char * result;
	size_t point;
	char 	key8[8];
	char 	key16[16];
	char 	key32[32];
	char *	new_key;
	int		created = 0;

	// ��������擾����
	if (key_len < 8) {
		memset(key8, 0, sizeof(key8));
		new_key = key8;
	}
	else if (key_len < 16) {
		memset(key16, 0, sizeof(key16));
		new_key = key16;
	}
	else if (key_len < 32) {
		memset(key32, 0, sizeof(key32));
		new_key = key32;
	}
	else {
		new_key = (char*)malloc(key_len + 1);
		Assert(new_key != 0, "item new error");
		_RPTN(_CRT_WARN, "stringhash_add_partition malloc 0x%X\n", new_key);
		created = 1;
	}
	memcpy(new_key, key, key_len);

	// �ǉ�����
	//
	// 1. ���ɒǉ��ς݂Ȃ�Γo�^����Ă��镶�����Ԃ�
	// 2. �o�^����Ă��Ȃ���Βǉ����s��
	//    2-1. �ǉ����镶����̃|�C���^���쐬
	//    2-2. �L�[��������R�s�[���A�e�[�u���ɒǉ�
	//    2-3. �e�[�u������t�ɂȂ�����č\��
	if (stringhash_inner_contains(impl->tables, impl->capacity, new_key, &result, &point)) {
		if (created) {
			free(new_key);
		}
		return result;									// 1
	}
	else {
		if (!created) {									// 2-1
			new_key = (char*)malloc(key_len + 1);
			Assert(new_key != 0, "item new error");
			_RPTN(_CRT_WARN, "stringhash_add_partition malloc 0x%X\n", new_key);

			strncpy_s(new_key, key_len + 1, key, key_len);	// 2-2
		}
		impl->tables[point] = new_key;
		impl->hash.count++;

		if ((double)impl->hash.count > impl->capacity * 0.7) {
			stringhash_realloc(impl);					// 2-3
		}

		return new_key;
	}
}

/**
* �n�b�V���e�[�u�����A�w��̕�������폜����B
*
* @param instance		�Ώۃn�b�V���e�[�u���B
* @param key			�폜���镶����B
*/
void stringhash_remove(const StringHash * instance, stringhash_key key)
{
	StringHashImpl * impl = (StringHashImpl*)instance;
	const char * result;
	size_t point;

	// ���ڂ����݂���΍폜����
	if (stringhash_inner_contains(impl->tables, impl->capacity, key, &result, &point)) {
		free((char*)impl->tables[point]);
		impl->tables[point] = 0;
		impl->hash.count--;
	}
}

/**
* �n�b�V���e�[�u�����A�w��̕�������폜����B
*
* @param instance		�Ώۃn�b�V���e�[�u���B
* @param key			�폜���镶����̐擪�|�C���^�B
* @param key_len		�폜���镶����̕����񒷁B
*/
void stringhash_remove_partition(const StringHash * instance, stringhash_key key, size_t key_len)
{
	// �L�[��������쐬
	char * new_key = (char*)malloc(key_len + 1);
	Assert(new_key != 0, "item new error");
	_RPTN(_CRT_WARN, "stringhash_remove_partition malloc 0x%X\n", new_key);

	// ��������폜
	memcpy(new_key, key, key_len);
	new_key[key_len] = 0;
	stringhash_remove(instance, new_key);

	// �L�[����������
	free(new_key);
}

//-----------------------------------------------------------------------------
// �f�o�b�O
//-----------------------------------------------------------------------------
/**
* �Ǘ����Ă��镶�������ʂɏo�͂���B
*
* @param instance	�ΏۃC���X�^���X�B
*/
void stringhash_show(const StringHash * instance)
{
	StringHashImpl * impl = (StringHashImpl*)instance;
	stringhash_key * ptr;
	
	ptr = impl->tables;
	for (ptr = impl->tables;
			ptr < impl->tables + impl->capacity; ++ptr) {
		if (*ptr) {
			fprintf(stdout, "%s\n", *ptr);
		}
	}
}