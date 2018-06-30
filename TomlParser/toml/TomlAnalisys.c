#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crtdbg.h>
#include "../helper/Assertion.h"
#include "../helper/StringHash.h"
#include "../helper/Vec.h"
#include "Toml.h"
#include "TomlBuffer.h"
#include "TomlAnalisys.h"

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

/**
 * 1�o�C�g������ǉ�����B
 *
 * @param word_dst	�����o�b�t�@�B
 * @param add_c		�ǉ����� 1�����B
 */
static void toml_append_char(Vec * word_dst, unsigned char add_c)
{
	vec_add(word_dst, &add_c);
}

/**
 * UNICODE�G�X�P�[�v����B
 *
 * @param buffer	�ǂݍ��ݗ̈�B
 * @param ptr		�ǂݍ��݊J�n�ʒu�B
 * @param len		�ǂݍ��ޕ������B
 * @return			�ϊ��ɐ��������� 0�ȊO�B
 */
static int toml_append_unicode(TomlBuffer * buffer, size_t ptr, size_t len)
{
	unsigned int	val = 0;
	size_t	i;
	TomlUtf8	c;
	unsigned char	v;

	// 16�i�������𔻒肵�A���l��
	for (i = 0; i < len; ++i) {
		c = toml_get_char(buffer->utf8s, ptr + i);
		val <<= 4;

		if (c.num >= '0' && c.num <= '9') {
			val |= c.num - '0';
		}
		else if (c.num >= 'a' && c.num <= 'f') {
			val |= 10 + c.num - 'a';
		}
		else if (c.num >= 'A' && c.num <= 'F') {
			val |= 10 + c.num - 'A';
		}
		else {
			buffer->err_point = i;
			return 0;
		}
	}

	// ���l�������l�𕶎��Ƃ��Ēǉ�����
	if (len == 4) {
		v = (val >> 8) & 0xff;
		toml_append_char(buffer->word_dst, v);
		v = val & 0xff;
		toml_append_char(buffer->word_dst, v);
	}
	else {
		v = (val >> 24) & 0xff;
		toml_append_char(buffer->word_dst, v);
		v = (val >> 16) & 0xff;
		toml_append_char(buffer->word_dst, v);
		v = (val >> 8) & 0xff;
		toml_append_char(buffer->word_dst, v);
		v = val & 0xff;
		toml_append_char(buffer->word_dst, v);
	}
	return 1;
}

/**
 * �L�[������̎擾�B
 *
 * @param buffer	�ǂݍ��ݗ̈�B
 * @param point		�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @return			�擾�ł����� 0�ȊO�B
 */
int toml_get_key(TomlBuffer * buffer, size_t point, size_t * next_point)
{
	size_t		i;
	TomlUtf8	c;

	// ������o�b�t�@������
	vec_clear(buffer->word_dst);

	// �󔒂�ǂݔ�΂�
	i = toml_skip_space(buffer->utf8s, point);

	// �͈͂̊J�n�A�I���ʒu�ŕ]��
	//
	// 1. �ꕶ���擾����
	// 2. �L�[�g�p�\���������肷��
	// 3. " �Ȃ當����Ƃ��ăL�[��������擾����
	for (; i < buffer->utf8s->length; ++i) {
		c = toml_get_char(buffer->utf8s, i);	// 1

		if ((c.num >= 'a' && c.num <= 'z') ||	// 2
			(c.num >= 'A' && c.num <= 'Z') ||
			(c.num >= '0' && c.num <= '9') ||
			c.num == '_' || c.num == '-') {
			vec_add(buffer->word_dst, c.ch);
			*next_point = i + 1;
		}
		else if (c.num == '"' &&				// 3
				 buffer->word_dst->length <= 0) {
			if (!toml_get_string_value(buffer, i + 1, next_point)) {
				return 0;
			}
			break;
		}
		else if (c.num == '\r' || c.num == '\n') {
			break;
		}
		else {
			buffer->err_point = i;
			return 1;
		}
	}

	// �I�[������ǉ����ďI��
	toml_append_char(buffer->word_dst, '\0');
	return (buffer->word_dst->length > 1);
}

/**
 * " �ň͂܂ꂽ��������擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @return				�擾�ł����� 0�ȊO�B
 */
int toml_get_string_value(TomlBuffer * buffer, size_t start, size_t * next_point)
{
	size_t	i, j;
	TomlUtf8	c, nc;
	char	empty = 0;

	// ������o�b�t�@������
	vec_clear(buffer->word_dst);

	for (i = start; i < buffer->utf8s->length; ++i) {
		// �ꕶ���擾����
		c = toml_get_char(buffer->utf8s, i);

		switch (c.num) {
		case '"':
			// " ���擾�����當����I��
			vec_add(buffer->word_dst, &empty);
			*next_point = i + 1;
			return (buffer->word_dst->length > 1);

		case '\\':
			// \�̃G�X�P�[�v��������
			if (i < buffer->utf8s->length - 1) {
				nc = toml_get_char(buffer->utf8s, i + 1);
				switch (nc.num)
				{
				case 'b':
					toml_append_char(buffer->word_dst, '\b');	i++;
					break;
				case 't':
					toml_append_char(buffer->word_dst, '\t');	i++;
					break;
				case 'n':
					toml_append_char(buffer->word_dst, '\n');	i++;
					break;
				case 'f':
					toml_append_char(buffer->word_dst, '\f');	i++;
					break;
				case 'r':
					toml_append_char(buffer->word_dst, '\r');	i++;
					break;
				case '"':
					toml_append_char(buffer->word_dst, '"');	i++;
					break;
				case '/':
					toml_append_char(buffer->word_dst, '/');	i++;
					break;
				case '\\':
					toml_append_char(buffer->word_dst, '\\');	i++;
					break;
				case 'u':
					if (i + 5 < buffer->utf8s->length &&
						!toml_append_unicode(buffer, i + 2, 4)) {
						return 0;
					}
					i += 5;
					break;
				case 'U':
					if (i + 9 < buffer->utf8s->length &&
						!toml_append_unicode(buffer, i + 2, 8)) {
						return 0;
					}
					i += 9;
					break;

				default:
					return 0;
				}
			}

		default:
			// ��L�ȊO�͕��ʂ̕����Ƃ��Ď擾
			for (j = 0; j < UTF8_CHARCTOR_SIZE && c.ch[j] != 0; ++j) {
				vec_add(buffer->word_dst, c.ch + j);
			}
			break;
		}
	}

	// " �ŏI���ł��Ȃ��������߃G���[
	buffer->err_point = buffer->utf8s->length - 1;
	return 0;
}

/**
 * ���l�i�����^�����^���t�j���擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param tokenType		�擾�f�[�^�B
 * @return				�擾�ł����� 0�ȊO�B
 */
int toml_get_number_value(TomlBuffer * buffer, size_t point, size_t * next_point, TomlValueType * tokenType)
{

}

int toml_convert_value(TomlBuffer * buffer, size_t point, size_t * next_point, TomlValueType * tokenType)
{
	TomlValueType	res;
	TomlUtf8		c;

	// �^�U�l�i�U�j��Ԃ�
	if (buffer->utf8s->length - point >= 5) {
		if (toml_get_char(buffer->utf8s, point + 4).num == 'e' &&
			toml_get_char(buffer->utf8s, point + 3).num == 's' &&
			toml_get_char(buffer->utf8s, point + 2).num == 'l' &&
			toml_get_char(buffer->utf8s, point + 1).num == 'a' &&
			toml_get_char(buffer->utf8s, point).num == 'f') {
			buffer->boolean = 0;
			*next_point = point + 5;
			*tokenType = TomlBooleanValue;
			return 1;
		}
	}

	// �^�U�l�i�^�j��Ԃ�
	if (buffer->utf8s->length - point >= 4) {
		if (toml_get_char(buffer->utf8s, point + 3).num == 'e' &&
			toml_get_char(buffer->utf8s, point + 2).num == 'u' &&
			toml_get_char(buffer->utf8s, point + 1).num == 'r' &&
			toml_get_char(buffer->utf8s, point).num == 't') {
			buffer->boolean = 1;
			*next_point = point + 4;
			*tokenType = TomlBooleanValue;
			return 1;
		}
	}

	if (buffer->utf8s->length - point >= 3) {
		if (toml_get_char(buffer->utf8s, point + 2).num == '"' &&
			toml_get_char(buffer->utf8s, point + 1).num == '"' &&
			toml_get_char(buffer->utf8s, point).num == '"') {
			//return StringMultiValue;
			return 1;
		}

		if (toml_get_char(buffer->utf8s, point + 2).num == '\'' &&
			toml_get_char(buffer->utf8s, point + 1).num == '\'' &&
			toml_get_char(buffer->utf8s, point).num == '\'') {
			//return StringMultiLiteral;
			return 1;
		}
	}

	if (buffer->utf8s->length - point > 0) {
		c = toml_get_char(buffer->utf8s, point);
		switch (c.num)
		{
		case '"':
			if (toml_get_string_value(buffer, point + 1, next_point)) {
				*tokenType = TomlStringValue;
				return 1;
			}

		case '\'':
			*tokenType = TomlStringValue;
			return 1;

		case '#':
			*next_point = point + 1;
			return 1;

		case '+':
			if (toml_get_number_value(buffer, point + 1, next_point, tokenType)) {
				return 1;
			}
			else {
				return 0;
			}

		case '-':
			if (toml_get_number_value(buffer, point + 1, next_point, tokenType)) {
				//if (res == TomlInteger) {
				//	buffer->integer = -buffer->integer;
				//	return IntegerValue;
				//}
				//else if (res == TomlFloat) {
				//	buffer->float_number = -buffer->float_number;
				//	return FloatValue;
				//}
				//else {
				//	return ErrorToken;
				//}
			}
			else {
				return 0;
			}

		default:
			return toml_get_number_value(buffer, point, next_point, tokenType);
		}
	}
	else {
		return 0;
	}
}
