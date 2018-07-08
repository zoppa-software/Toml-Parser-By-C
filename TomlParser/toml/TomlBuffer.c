#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crtdbg.h>
#include "../helper/Assertion.h"
#include "../helper/Vec.h"
#include "TomlBuffer.h"

/**
 * �ǂݍ��ݗ̈�B
 */
typedef struct _TomlBufferImpl
{
	// ���J���̎Q��
	TomlBuffer	parent;

	// �t�@�C���|�C���^
	FILE *	fp;

	// �ǂݍ��݃o�b�t�@
	char	readbf[READ_BUF_SIZE];

	// �ǂݍ��ݍς݃o�C�g��
	size_t	readed;

	// �o�b�t�@�̎c��
	size_t	rest_buf;

} TomlBufferImpl;

/** �󕶎��B */
static TomlUtf8		empty_char;

/**
 * �t�@�C���X�g���[�����쐬����B
 *
 * @param path		�t�@�C���p�X�B
 * @return			�t�@�C���X�g���[���B
 */
TomlBuffer * toml_create_buffer(const char * path)
{
	TomlBufferImpl * res = (TomlBufferImpl*)malloc(sizeof(TomlBufferImpl));
	errno_t		err;

	memset(res, 0, sizeof(TomlBufferImpl));
	Assert(res != 0, "stream error");
	_RPTN(_CRT_WARN, "toml_create_stream malloc 0x%X\n", res);

	// �t�@�C���I�[�v��
	err = fopen_s(&res->fp, path, "rb");
	if (err == 0) {
		res->parent.utf8s = vec_initialize_set_capacity(sizeof(TomlUtf8), READ_BUF_SIZE);
		res->parent.word_dst = vec_initialize(sizeof(char));
		return (TomlBuffer*)res;
	}
	else {
		free(res);
		return 0;
	}
}

/**
 * �t�@�C���̓ǂݍ��݂��I�����Ă���� 0�ȊO��Ԃ��B
  *
 * @param buffer	�t�@�C���X�g���[���B
 */
int toml_end_of_file(TomlBuffer * buffer)
{
	TomlBufferImpl * impl = (TomlBufferImpl*)buffer;
	if (impl->rest_buf > 0) {
		return 0;
	}
	return (feof(impl->fp) || ferror(impl->fp));
}

/**
 * ���ɐi�߂�o�C�g�����擾����iUTF8�p�j
 *
 * @param caractor	�����B
 * @return			�i�߂�o�C�g���B
 */
static size_t toml_skip_func(unsigned char caractor)
{
	if ((caractor & 0xfc) == 0xfc) {
		return 6;
	}
	else if ((caractor & 0xf8) == 0xf8) {
		return 5;
	}
	else if ((caractor & 0xf0) == 0xf0) {
		return 4;
	}
	else if ((caractor & 0xe0) == 0xe0) {
		return 3;
	}
	else if ((caractor & 0xc0) == 0xc0) {
		return 2;
	}
	else {
		return 1;
	}
}

/**
 * �t�@�C������o�b�t�@�ɒl����荞�ށB
 *
 * @param buffer	�o�b�t�@�B
 * @return			�o�b�t�@�Ɏ�荞�܂ꂽ�o�C�g���B
 */
static size_t toml_read_from_file(TomlBufferImpl * buffer)
{
	char *	ptr;
	size_t	rst, len;

	// �o�b�t�@�̗]�T���|�C���g
	ptr = buffer->readbf + buffer->readed;
	rst = READ_BUF_SIZE - buffer->readed;

	// �o�b�t�@�Ɏ�荞��
	len = fread_s(ptr, rst, sizeof(char), rst - 1, buffer->fp);
	len += buffer->readed;

	// �ԕ��Ƃ��� 0�𖄂ߍ���
	buffer->readbf[len] = 0;
	buffer->readed = 0;
	return len;
}

/**
 * ��s���̕��������荞�ށB
 *
 * @param buffer	�t�@�C���X�g���[���B
 */
void toml_read_buffer(TomlBuffer * buffer)
{
	TomlBufferImpl * impl = (TomlBufferImpl*)buffer;
	char *		ptr;
	size_t		maxlen;
	size_t		i, skip;
	TomlUtf8	utf8;

	while (impl->rest_buf > 0 || !feof(impl->fp) && !ferror(impl->fp)) {
		// �o�b�t�@�ɕ��������荞��
		if (impl->rest_buf > 0) {
			maxlen = impl->rest_buf;
			impl->rest_buf = 0;
		}
		else {
			maxlen = toml_read_from_file(impl);
		}

		for (i = 0, ptr = impl->readbf;
					i < maxlen && *ptr != '\0'; ptr += skip, i += skip) {
			// �����̃o�C�g�����擾
			skip = toml_skip_func(*ptr);

			// UTF8�̕������o�b�t�@�ɏ�������
			//
			// 1. ����������荞��ł��邽�߃o�b�t�@�ɏ�������
			//    1-1. ���s�Ȃ�Ύ�荞�ݒ��~
			// 2. ��������荞�߂Ă��Ȃ����߁A������荞��
			if (i + skip < READ_BUF_SIZE) {
				memset(&utf8, 0, sizeof(utf8));		// 1
				memcpy(utf8.ch, ptr, skip);
				vec_add(impl->parent.utf8s, utf8.ch);

				if (utf8.num == '\n') {				// 1-1
					memmove_s(impl->readbf,
							  READ_BUF_SIZE,
							  impl->readbf + (i + 1),
							  READ_BUF_SIZE - (i + 1));
					impl->readed = maxlen - (i + 1);
					impl->rest_buf = impl->readed;
					impl->parent.loaded_line++;
					goto LOOP_END;
				}
			}
			else {
				memmove_s(impl->readbf,				// 2
						  READ_BUF_SIZE,
						  impl->readbf + i,
						  READ_BUF_SIZE - i);
				impl->readed = maxlen - i;
			}
		}
	}
LOOP_END:;
}

/**
 * �t�@�C���X�g���[�������B
 *
 * @param buffer	�t�@�C���X�g���[���B
 */
void toml_close_buffer(TomlBuffer * buffer)
{
	TomlBufferImpl * impl = (TomlBufferImpl*)buffer;

	fclose(impl->fp);
	vec_delete(&impl->parent.utf8s);
	vec_delete(&impl->parent.word_dst);
	free(buffer);
}