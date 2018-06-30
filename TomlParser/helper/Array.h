#ifndef __ARRAY_H__
#define __ARRAY_H__

/**
 * C�̔z������b�v���ė̈�z���A�N�Z�X���ɃA�v���P�[�V�������A�{�[�g������B
 *
 * ARRAY(int, num, num_src, 10);	// �錾
 *
 * ARRAY_GET(int, num, 0) = 0;
 * ARRAY_GET(int, num, 1) = 1;
 * ARRAY_GET(int, num, 2) = 2;
 * ARRAY_GET(int, num, 3) = 3;
 * ARRAY_GET(int, num, 4) = 4;
 */

/**
 * ���E�`�F�b�N�z��\����
 */
typedef struct _Array
{
	// �z��̍ő咷
	size_t	length;

	// �f�[�^�T�C�Y
	size_t	size;

	// �z��Q��
	char *	data;

} Array;

/**
 * ���E�`�F�b�N�z��̐����}�N��
 */
#define ARRAY(T, name, src, length) \
	T src[length]; \
	Array name = array_attach(sizeof(T), src, sizeof(src) / sizeof(T));

/**
 * �z��`�F�b�N�Q�ƃ}�N��
 */
#define ARRAY_GET(T, name, index) (*((T*)array_get(&name, index)))

/**
 * ���E�`�F�b�N�z��\���̂̍쐬
 *
 * @param size		�f�[�^�T�C�Y�B
 * @param source	�z��Q�ƁB
 * @param length	�z��̗v�f���B
 * @return			���E�`�F�b�N�z��\���́B
 */
Array array_attach(size_t size, void * source, size_t length);

/**
 * �z��̃|�C���^���擾����B
 *
 * @param array		���E�`�F�b�N�z��Q�ƁB
 * @param index		�v�f�ʒu
 * @return			�|�C���^�B
 */
void * array_get(Array * array, size_t index);

#endif /* __ARRAY_H__ */