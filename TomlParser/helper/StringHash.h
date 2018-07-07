#ifndef __STRING_HASH_H__
#define __STRING_HASH_H__

/**
 * ������iconst char *�j��ΏۂƂ����n�b�V���e�[�u���B
 *
 * const StringHash * hash = stringhash_initialize();	// �n�b�V���e�[�u���̍쐬
 *
 * stringhash_add(hash, "�k�C�� �D�y�s");					// �n�b�V���e�[�u���ɍ��ڂ�ǉ�
 *
 * const char * key;									// ���݊m�F
 * if (stringhash_contains(hash, "�F�{�� �쏬����", &key)) {
 *		printf_s("hit %s\n", key);
 * }
 *
 * stringhash_remove(hash, "�F�{�� �쏬����");			// ���ڍ폜
 *
 * stringhash_delete(&hash);							// �Ō�͉�����Ă�������
 *
 * Copyright (c) 2018 Takashi Zota
 * Released under the MIT license
 * https://opensource.org/licenses/mit-license.php
 */

/**
 * ������n�b�V���e�[�u���̃L�[�B
 */
typedef const char *	stringhash_key;

//-----------------------------------------------------------------------------
// �\����
//-----------------------------------------------------------------------------
/**
 * ������n�b�V���e�[�u���B
 */
typedef struct _StringHash
{
	// �i�[���ڐ�
	size_t			count;

} StringHash;

//-----------------------------------------------------------------------------
// �R���X�g���N�^�A�f�X�g���N�^
//-----------------------------------------------------------------------------
/**
 * ������n�b�V���e�[�u���̏��������s���B
 *
 * @return				�C���X�^���X�B
 */
const StringHash * stringhash_initialize();

/**
 * ������n�b�V���e�[�u�����폜����B
 *
 * @param ins_del		�폜����C���X�^���X�B
 */
void stringhash_delete(const StringHash ** ins_del);

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
int stringhash_contains(const StringHash * instance, stringhash_key key, stringhash_key * result);

/**
 * ������̒ǉ����s���B
 *
 * @param instance		�Ώۃn�b�V���e�[�u���B
 * @param key			�ǉ����镶����B
 * @return				�n�b�V���e�[�u���ɒǉ����ꂽ������̃|�C���^�B
 */
stringhash_key stringhash_add(const StringHash * instance, stringhash_key key);

/**
 * ������̒ǉ����s���i������̕����j
 *
 * @param instance		�Ώۃn�b�V���e�[�u���B
 * @param key			�ǉ����镶����B
 * @param key_len		�����񒷁B
 * @return				�n�b�V���e�[�u���ɒǉ����ꂽ������̃|�C���^�B
 */
stringhash_key stringhash_add_partition(const StringHash * instance, stringhash_key key, size_t key_len);

/**
 * �n�b�V���e�[�u�����A�w��̕�������폜����B
 *
 * @param instance		�Ώۃn�b�V���e�[�u���B
 * @param key			�폜���镶����B
 */
void stringhash_remove(const StringHash * instance, stringhash_key key);

/**
 * �n�b�V���e�[�u�����A�w��̕�������폜����B
 *
 * @param instance		�Ώۃn�b�V���e�[�u���B
 * @param key			�폜���镶����̐擪�|�C���^�B
 * @param key_len		�폜���镶����̕����񒷁B
 */
void stringhash_remove_partition(const StringHash * instance, stringhash_key key, size_t key_len);

//-----------------------------------------------------------------------------
// �f�o�b�O
//-----------------------------------------------------------------------------
/**
 * �Ǘ����Ă��镶�������ʂɏo�͂���B
 *
 * @param instance	�Ώۃn�b�V���e�[�u���B
 */
void stringhash_show(const StringHash * instance);

#endif /* __STRING_HASH_H__ */