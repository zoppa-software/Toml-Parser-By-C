#ifndef __TOML__BUFFERH__
#define __TOML__BUFFERH__

/**
 * ���������p�̃w�b�_�A�t�@�C�����̓ǂݍ��݃o�b�t�@�B
 *
 * Copyright (c) 2018 Takashi Zota
 * Released under the MIT license
 * https://opensource.org/licenses/mit-license.php
 */

#include "Toml.h"

/** �ǂݍ��݃o�b�t�@�T�C�Y�B */
#define READ_BUF_SIZE	1024

/**
 * �ǂݍ��ݗ̈�B
 */
typedef struct _TomlBuffer
{
	// ������o�b�t�@�B
	Vec *	word_dst;

	// �����l�B
	long long int	integer;

	// �����l�B
	double	float_number;

	// �^�U�l�B
	int		boolean;

	// ���t���B
	TomlDate 	date;

	// �e�[�u���Q��
	TomlTable *	table;

	// �z��Q��
	TomlArray * array;

	// �o�b�t�@
	Vec *	utf8s;

	// �ǂݍ��ݍςݍs��
	size_t	loaded_line;

} TomlBuffer;

/**
 * �t�@�C���X�g���[�����쐬����B
 *
 * @param path		�t�@�C���p�X�B
 * @return			�t�@�C���X�g���[���B
 */
TomlBuffer * toml_create_buffer(const char * path);

/**
 * �t�@�C���̓ǂݍ��݂��I�����Ă���� 0�ȊO��Ԃ��B
  *
 * @param buffer	�t�@�C���X�g���[���B
 */
int toml_end_of_file(TomlBuffer * buffer);

/**
 * ��s���̕��������荞�ށB
 *
 * @param buffer	�t�@�C���X�g���[���B
 */
void toml_read_buffer(TomlBuffer * buffer);

/**
 * �t�@�C���X�g���[�������B
 *
 * @param buffer	�t�@�C���X�g���[���B
 */
void toml_close_buffer(TomlBuffer * buffer);

#endif /* __TOML__BUFFERH__ */
