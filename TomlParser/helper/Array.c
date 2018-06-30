#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Assertion.h"
#include "Array.h"

/**
 * ���E�`�F�b�N�z��\���̂̍쐬
 *
 * @param size		�f�[�^�T�C�Y�B
 * @param source	�z��Q�ƁB
 * @param length	�z��̗v�f���B
 * @return			���E�`�F�b�N�z��\���́B
 */
Array array_attach(size_t size, void * source, size_t length)
{
	Array res;
	res.length = length;
	res.size = size;
	res.data = source;
	return res;
}

/**
 * �z��̃|�C���^���擾����B
 *
 * @param array		���E�`�F�b�N�z��Q�ƁB
 * @param index		�v�f�ʒu
 * @return			�|�C���^�B
 */
void * array_get(Array * array, size_t index)
{
	if (index < array->length) {
		return array->data + array->size * index;
	}
	else {
		Abort("out of index");
	}
}