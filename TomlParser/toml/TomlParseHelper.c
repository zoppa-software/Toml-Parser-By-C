#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../helper/Assertion.h"
#include "../helper/StringHash.h"
#include "../helper/Vec.h"
#include "TomlParseHelper.h"

/** �󕶎��B */
static TomlUtf8		empty_char;

/**
 * �w��ʒu�̕������擾����B
 *
 * @param utf8s		������B
 * @param point		�w��ʒu�B
 * @return			�擾���������B
 */
TomlUtf8 toml_get_char(Vec * utf8s, size_t point)
{
	if (point < utf8s->length) {
		return VEC_GET(TomlUtf8, utf8s, point);
	}
	else {
		return empty_char;
	}
}

/**
 * �󔒕�����ǂݔ�΂��B
 *
 * @param impl	toml�h�L�������g�B
 * @param i		�ǂݍ��݈ʒu�B
 * @return		�ǂݔ�΂����ʒu�B
 */
size_t toml_skip_space(Vec * utf8s, size_t i)
{
	TomlUtf8	c;
	for (; i < utf8s->length; ++i) {
		c = toml_get_char(utf8s, i);
		if (c.num != '\t' && c.num != ' ') {
			break;
		}
	}
	return i;
}

/**
 * �G���[���b�Z�[�W���擾����B
 *
 * @param utf8s		UTF8������B
 * @param word_dst	�ϊ��o�b�t�@�B
 * @preturn			�G���[���b�Z�[�W�B
 */
const char * toml_err_message(Vec *	utf8s, Vec * word_dst)
{
	size_t		i, j;
	TomlUtf8	c;

	vec_clear(word_dst);

	for (i = 0; i < utf8s->length; ++i) {
		c = toml_get_char(utf8s, i);
		for (j = 0; j < UTF8_CHARCTOR_SIZE && c.ch[j] != 0; ++j) {
			vec_add(word_dst, c.ch + j);
		}
	}
	vec_add(word_dst, &empty_char);

	return (const char *)word_dst->pointer;
}

//-----------------------------------------------------------------------------
// ��������
//-----------------------------------------------------------------------------
/**
 * �s�̊J�n�𔻒肷��B
 *
 * @param utf8s		�Ώە�����B
 * @param i			�ǂݍ��݈ʒu�B
 * @param tokenType	�s�̃f�[�^��ށB
 * @return			�ǂݔ�΂����ʒu�B
 */
size_t toml_check_start_line_token(Vec * utf8s,
								   size_t ptr,
								   TomlLineType * tokenType)
{
	TomlUtf8	c1, c2;

	// �e�[�u���z��̊J�n�����肷��
	if (utf8s->length - ptr >= 2) {
		c1 = toml_get_char(utf8s, ptr);
		c2 = toml_get_char(utf8s, ptr + 1);
		if (c1.num == '[' && c2.num == '[') {
			*tokenType = TomlTableArrayLine;
			return ptr + 2;
		}
	}

	// ���̑��̊J�n�𔻒肷��
	//
	// 1. �e�[�u���̊J�n�����肷��
	// 2. �R�����g�̊J�n�����肷��
	// 4. �L�[�̊J�n�����肷��
	// 5. ����ȊO�̓G���[
	if (utf8s->length - ptr > 0) {
		c1 = toml_get_char(utf8s, ptr);
		switch (c1.num)
		{
		case '[':			// 1
			*tokenType = TomlTableLine;
			return ptr + 1;

		case '#':			// 2
			*tokenType = TomlCommenntLine;
			return ptr + 1;

		case '\r':			// 3
		case '\n':
		case '\0':
			*tokenType = TomlLineNone;
			return ptr;

		default:			// 4
			*tokenType = TomlKeyValueLine;
			return ptr;
		}
	}
	else {					// 5
		*tokenType = TomlLineNone;
		return ptr;
	}
}

/**
 * �󔒂�ǂݔ�΂��A�������s�^�I�[�^�R�����g�Ȃ�ΐ^��Ԃ��B
 *
 * @param utf8s		�Ώە�����B
 * @param i			�����J�n�ʒu�B
 * @return			���s�^�I�[�^�R�����g�Ȃ�� 0�ȊO�B
 */
int toml_skip_linefield(Vec * utf8s, size_t i)
{
	TomlUtf8	c;

	// �󔒂�ǂݔ�΂�
	i = toml_skip_space(utf8s, i);

	// �����𔻒�
	c = toml_get_char(utf8s, i++);
	return (c.num == '#' || c.num == '\r' || c.num == '\n' || c.num == 0);
}

/**
 * ']' �̌�A�������s�^�I�[�^�R�����g�Ȃ�ΐ^��Ԃ��B
 *
 * @param utf8s		�Ώە�����B
 * @param i			�����J�n�ʒu�B
 * @return			���s�^�I�[�^�R�����g�Ȃ�� 0�ȊO�B
 */
int toml_close_table(Vec * utf8s, size_t i)
{
	TomlUtf8	c;

	// �󔒂�ǂݔ�΂�
	i = toml_skip_space(utf8s, i);

	// ] �̔���
	c = toml_get_char(utf8s, i++);
	if (c.num != ']') {
		return 0;
	}

	// �󔒂�ǂݔ�΂�
	i = toml_skip_space(utf8s, i);

	// �����𔻒�
	c = toml_get_char(utf8s, i++);
	return (c.num == '#' || c.num == '\r' || c.num == '\n' || c.num == 0);
}

/**
 * ']]' �̌�A�������s�^�I�[�^�R�����g�Ȃ�ΐ^��Ԃ��B
 *
 * @param utf8s		�Ώە�����B
 * @param i			�����J�n�ʒu�B
 * @return			���s�^�I�[�^�R�����g�Ȃ�� 0�ȊO�B
 */
int toml_close_table_array(Vec * utf8s, size_t i)
{
	TomlUtf8	c;

	// �󔒂�ǂݔ�΂�
	i = toml_skip_space(utf8s, i);

	// ]] �̔���
	c = toml_get_char(utf8s, i++);
	if (c.num != ']') {
		return 0;
	}

	c = toml_get_char(utf8s, i++);
	if (c.num != ']') {
		return 0;
	}

	// �󔒂�ǂݔ�΂�
	i = toml_skip_space(utf8s, i);

	// �����𔻒�
	c = toml_get_char(utf8s, i++);
	return (c.num == '#' || c.num == '\r' || c.num == '\n' || c.num == 0);
}