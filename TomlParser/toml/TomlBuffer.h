#ifndef __TOML__BUFFERH__
#define __TOML__BUFFERH__

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

	// �L�[������o�b�t�@
	Vec *	key_ptr;

	// �o�b�t�@
	Vec *	utf8s;

	// ��̓G���[�ʒu
	size_t	err_point;

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
