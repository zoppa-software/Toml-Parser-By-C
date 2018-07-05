#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crtdbg.h>
#include "Assertion.h"
#include "Instance.h"

// �Ǘ�������̗̈�𐶐�����
static void instance_append(Instance * instance);

/**
 * �C���X�^���X�Ǘ��@�\�̏��������s���B
 *
 * @param data_len		��̃f�[�^���B
 * @param block_size	���ɐ���������B
 * @return				�C���X�^���X�Ǘ��@�\�B
 */
Instance instance_initialize(size_t data_len, size_t block_size)
{
	Instance res;
	memset(&res, 0, sizeof(Instance));
	res.data_len = data_len;
	res.block_size = block_size;
	return res;
}

/**
 * �C���X�^���X�Ǘ��@�\���폜����B
 *
 * @param ins_del		�폜����Ǘ��@�\�B
 */
void instance_delete(Instance * ins_del)
{
	int		i;

	// �}�v�f���
	for (i = 0; i < ins_del->entities.block_count; ++i) {
		free(ins_del->entities.block[i]);
	}
	free(ins_del->entities.block);
	free(ins_del->stack.cache);
}

/**
 * �C���X�^���X�Ǘ��@�\���폜����B
 *
 * @param ins_del		�폜����Ǘ��@�\�B
 * @param build_action	�f�X�g���N�^�A�N�V�����B
 */
void instance_delete_all_destructor(Instance * ins_del,
									instance_build_function build_action)
{
	int		i, j;

	// �}�v�f���
	for (i = 0; i < ins_del->entities.block_count; ++i) {
		for (j = 0; j < ins_del->block_size; ++j) {
			build_action((char*)ins_del->entities.block[i] + (ins_del->data_len * j));
		}
		free(ins_del->entities.block[i]);
	}
	free(ins_del->entities.block);
	free(ins_del->stack.cache);
}

/**
* �C���X�^���X�Ǘ��@�\�Ƀf�[�^��Ԃ��B
*
* @param instance		�C���X�^���X�Ǘ��@�\�B
* @param item			�Ԃ��f�[�^�B
*/
void instance_push(Instance * instance, void * item)
{
	instance->stack.cache[instance->stack.count++] = item;
}

/**
* �C���X�^���X�Ǘ��@�\����f�[�^���擾����B
*
* @param instance		�C���X�^���X�Ǘ��@�\�B
* @return				�f�[�^�B
*/
void * instance_pop(Instance * instance)
{
	return instance_pop_constructor(instance, 0);
}

/**
 * �Ǘ�������̗̈�𐶐�����B
 *
 * @param instance		�C���X�^���X�Ǘ��@�\�B
 */
void instance_append(Instance * instance)
{
	char ** temp;
	char * newins;
	size_t i, len;

	// �v�f�̎��̂𐶐�
	//
	// 1. �ߋ�����ޔ�
	// 2. �V�K�̈��ǉ�
	// 3. ����
	temp = (char**)malloc(sizeof(char*) * (instance->entities.block_count + 1));	// 1
	Assert(temp != 0, "instance add error");
	_RPTN(_CRT_WARN, "instance_append malloc 1 0x%X\n", temp);
	memcpy(temp, instance->entities.block, sizeof(char*) * instance->entities.block_count);

	newins = (char*)malloc(instance->data_len * instance->block_size);				// 2
	Assert(newins != 0, "instance add error");
	_RPTN(_CRT_WARN, "instance_append malloc 2 0x%X\n", newins);
	memset(newins, 0, instance->data_len * instance->block_size);
	temp[instance->entities.block_count] = newins;

	free(instance->entities.block);													// 3
	instance->entities.block = temp;
	instance->entities.block_count++;

	// �v�f���X�^�b�N�ɐς�
	//
	// 1. �ߋ������܂߂Đ���
	// 2. ����
	len = instance->entities.block_count * instance->block_size;					// 1
	temp = (char**)malloc(sizeof(char*) * len);
	Assert(temp != 0, "instance add error");
	_RPTN(_CRT_WARN, "instance_append malloc 3 0x%X\n", temp);
	memset(temp, 0, sizeof(char*) * len);
	memcpy(temp, instance->stack.cache, sizeof(char*) * instance->stack.count);

	free(instance->stack.cache);													// 2
	instance->stack.cache = temp;

	// �X�^�b�N�ɒǉ�
	for (i = 0; i < instance->block_size; ++i) {
		instance_push(instance, newins + (instance->data_len * i));
	}
}

/**
 * �C���X�^���X�Ǘ��@�\����f�[�^���擾����i�R���X�g���N�^���s�j
 *
 * @param instance		�C���X�^���X�Ǘ��@�\�B
 * @param build_action	�R���X�g���N�^�A�N�V�����B
 * @return				�f�[�^�B
 */
void * instance_pop_constructor(Instance * instance, instance_build_function build_action)
{
	void * ans;

	// �]���Ă��Ȃ���Ύ}�v�f��ǉ�
	if (instance->stack.count <= 0) {
		instance_append(instance);
	}

	// �X�^�b�N����擾
	instance->stack.count--;
	ans = instance->stack.cache[instance->stack.count];
	instance->stack.cache[instance->stack.count] = 0;

	// �R���X�g���N�^���s
	if (build_action != 0) {
		build_action(ans);
	}

	return ans;
}

/**
 * �C���X�^���X�Ǘ��@�\�Ƀf�[�^��Ԃ��i�f�X�g���N�^���s�j
 *
 * @param instance		�C���X�^���X�Ǘ��@�\�B
 * @param item			�Ԃ��f�[�^�B
 * @param build_action	�f�X�g���N�^�A�N�V�����B
 */
void instance_push_destructor(Instance * instance, void * item, instance_build_function build_action)
{
	build_action(item);
	instance->stack.cache[instance->stack.count++] = item;
}