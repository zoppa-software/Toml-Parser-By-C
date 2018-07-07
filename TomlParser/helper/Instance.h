#ifndef __INSTANCE_H__
#define __INSTANCE_H__

/**
 * �\���̂̎��̂��Ǘ�����@�\�B
 *
 * Instance instance = instance_initialize(sizeof(Data), 3);	// Data�\���̂��g�p���邱�Ƃ�錾
 *
 * Data * data = instance_pop(&instance);	// �Ǘ��̈悩�� Data�\���̂ւ̎Q�Ƃ��擾
 *
 * instance_push(&instance, data);			// �g���I��������̂͊Ǘ��@�\�ɕԂ�����
 *
 * instance_delete(&instance);				// �Ō�ɉ�����Ă�������
 *
 * Copyright (c) 2018 Takashi Zota
 * Released under the MIT license
 * https://opensource.org/licenses/mit-license.php
 */

/**
 * �C���X�^���X�Ǘ��@�\�B
 */
typedef struct _Instance
{
	// ��̃f�[�^��
	size_t			data_len;

	// ���ɐ��������
	size_t			block_size;

	// ���̊Ǘ��̈�
	//
	// 1. ���̃u���b�N��
	// 2. ���̗̈�
	struct {
		int			block_count;	// 1
		void **		block;			// 2
	} entities;

	// �X�^�b�N�Ǘ��̈�
	//
	// 1. �X�^�b�N�c��
	// 2. �X�^�b�N�f�[�^
	struct {
		int			count;			// 1
		void **		cache;			// 2
	} stack;

} Instance;

/**
 * �R���X�g���N�^�^�f�X�g���N�^�A�N�V�����B
 *
 * @param target	�ΏۃC���X�^���X�B
 */
typedef void(*instance_build_function)(void * target);

/**
 * �C���X�^���X�Ǘ��@�\�̏��������s���B
 *
 * @param data_len		��̃f�[�^���B
 * @param block_size	���ɐ���������B
 * @return				�C���X�^���X�Ǘ��@�\�B
 */
Instance instance_initialize(size_t data_len, size_t block_size);

/**
 * �C���X�^���X�Ǘ��@�\���폜����B
 *
 * @param ins_del		�폜����Ǘ��@�\�B
 */
void instance_delete(Instance * ins_del);

/**
 * �C���X�^���X�Ǘ��@�\���폜����B
 *
 * @param ins_del		�폜����Ǘ��@�\�B
 * @param build_action	�f�X�g���N�^�A�N�V�����B
 */
void instance_delete_all_destructor(Instance * ins_del, instance_build_function build_action);

/**
 * �C���X�^���X�Ǘ��@�\�Ƀf�[�^��Ԃ��B
 *
 * @param instance		�C���X�^���X�Ǘ��@�\�B
 * @param item			�Ԃ��f�[�^�B
 */
void instance_push(Instance * instance, void * item);

/**
 * �C���X�^���X�Ǘ��@�\����f�[�^���擾����B
 *
 * @param instance		�C���X�^���X�Ǘ��@�\�B
 * @return				�f�[�^�B
 */
void * instance_pop(Instance * instance);

/**
 * �C���X�^���X�Ǘ��@�\����f�[�^���擾����i�R���X�g���N�^���s�j
 *
 * @param instance		�C���X�^���X�Ǘ��@�\�B
 * @param build_action	�R���X�g���N�^�A�N�V�����B
 * @return				�f�[�^�B
 */
void * instance_pop_constructor(Instance * instance, instance_build_function build_action);

/**
 * �C���X�^���X�Ǘ��@�\�Ƀf�[�^��Ԃ��i�f�X�g���N�^���s�j
 *
 * @param instance		�C���X�^���X�Ǘ��@�\�B
 * @param build_action	�f�X�g���N�^�A�N�V�����B
 */
void instance_push_destructor(Instance * instance, void * item, instance_build_function build_action);

#endif /* __INSTANCE_H__ */