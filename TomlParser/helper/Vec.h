#ifndef __VEC_H__
#define __VEC_H__

/**
 * �ϒ��z��B
 *
 * Vec * vec = vec_initialize(sizeof(int));		// �ϒ��z��𐶐�
 *
 * for (i = 1; i <= 100; ++i) {					// �v�f��ǉ�
 *		vec_add(vec, &i);						// �� �ǉ��ł���v�f�̓|�C���^�ŎQ�Ƃ��邱�Ƃɒ���
 * }
 *
 * for (i = 0; i < 100; ++i) {
 *		printf_s("%d\n", VEC_GET(int, vec, i));	// �}�N���ŗv�f���Q��
 * }
 *
 * vec_remove_at(vec, 33);						// �폜������܂�
 * vec_remove_at(vec, 98);
 * 
 * vec_delete(&vec);							// �����Y�ꂸ��
 *
 * Copyright (c) 2018 Takashi Zota
 * Released under the MIT license
 * https://opensource.org/licenses/mit-license.php
 */

// �L������ő咷
#define VEC_EXPAND_LENGTH	1024

// ���ڎQ�ƃ}�N��
#define VEC_GET(T, name, index) (*((T*)vec_items(name, index)))

/**
 * �ϒ��z��\����
 */
typedef struct _Vec
{
	// �v�f��
	size_t		length;

	// ���̎Q��
	void *		pointer;

} Vec;

//-----------------------------------------------------------------------------
// �R���X�g���N�^�A�f�X�g���N�^
//-----------------------------------------------------------------------------
/**
 * �ϒ��z��̏��������s���B
 *
 * @param data_len		��̃f�[�^���B
 * @return				�ϒ��z��B
 */
Vec * vec_initialize(size_t data_len);

/**
 * �ϒ��z��̏��������s���i�����e�ʎw��j
 *
 * @param data_len			��̃f�[�^���B
 * @param start_capacity	�����e�ʁB
 * @return					�ϒ��z��B
 */
Vec * vec_initialize_set_capacity(size_t data_len, size_t start_capacity);

/**
 * �ϒ��z����폜����B
 *
 * @param ins_del		�폜����ϒ��z��B
 */
void vec_delete(Vec ** ins_del);

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
void * vec_items(Vec * vector, size_t index);

//-----------------------------------------------------------------------------
// �ǉ��ƍ폜
//-----------------------------------------------------------------------------
/**
 * �ϒ��z�����������B
 *
 * @param vector		�ϒ��z��B
 */
void vec_clear(Vec * vector);

/**
 * �ϒ��z��ɗv�f��ǉ�����B
 *
 * @param vector		�ϒ��z��B
 * @param data			�ǉ�����v�f�B
 */
void vec_add(Vec * vector, void * data);

/**
 * �ϒ��z��̎w��ʒu�̗v�f���폜����B
 *
 * @param vector		�ϒ��z��B
 * @param index			�폜����ʒu�B
 */
void vec_remove_at(Vec * vector, size_t index);

#endif /* __VEC_H__ */
