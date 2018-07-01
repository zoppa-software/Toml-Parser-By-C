#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <math.h>
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
static void toml_append_char(Vec * word_dst,
							 unsigned char add_c)
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
static int toml_append_unicode(TomlBuffer * buffer,
							   size_t ptr,
							   size_t len)
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
int toml_get_key(TomlBuffer * buffer,
				 size_t point,
				 size_t * next_point)
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
 * '\'�G�X�P�[�v�������擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param ptr			�J�n�ʒu�B
 * @return				�擾�ł����� 0�ȊO�B
 */
static int toml_append_escape_char(TomlBuffer * buffer,
								   size_t * ptr)
{
	size_t		i = *ptr;
	TomlUtf8	c;

	if (i < buffer->utf8s->length - 1) {
		c = toml_get_char(buffer->utf8s, i + 1);

		switch (c.num) {
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
	*ptr = i;
	return 1;
}

/**
 * " �ň͂܂ꂽ��������擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @return				�擾�ł����� 0�ȊO�B
 */
int toml_get_string_value(TomlBuffer * buffer,
						  size_t start,
						  size_t * next_point)
{
	size_t		i, j;
	TomlUtf8	c;

	// ������o�b�t�@������
	vec_clear(buffer->word_dst);

	for (i = start; i < buffer->utf8s->length; ++i) {
		// �ꕶ���擾����
		c = toml_get_char(buffer->utf8s, i);

		switch (c.num) {
		case '"':
			// " ���擾�����當����I��
			toml_append_char(buffer->word_dst, '\0');
			*next_point = i + 1;
			return (buffer->word_dst->length > 1);

		case '\\':
			// \�̃G�X�P�[�v��������
			if (!toml_append_escape_char(buffer, &i)) {
				buffer->err_point = i;
				return 0;
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
 * ' �ň͂܂ꂽ��������擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @return				�擾�ł����� 0�ȊO�B
 */
static int toml_get_literal_string_value(TomlBuffer * buffer,
										 size_t start,
										 size_t * next_point)
{
	size_t		i, j;
	TomlUtf8	c;

	// ������o�b�t�@������
	vec_clear(buffer->word_dst);

	for (i = start; i < buffer->utf8s->length; ++i) {
		// �ꕶ���擾����
		c = toml_get_char(buffer->utf8s, i);

		switch (c.num) {
		case '\'':
			// ' ���擾�����當����I��
			toml_append_char(buffer->word_dst, '\0');
			*next_point = i + 1;
			return (buffer->word_dst->length > 1);

		default:
			// ��L�ȊO�͕��ʂ̕����Ƃ��Ď擾
			for (j = 0; j < UTF8_CHARCTOR_SIZE && c.ch[j] != 0; ++j) {
				vec_add(buffer->word_dst, c.ch + j);
			}
			break;
		}
	}

	// ' �ŏI���ł��Ȃ��������߃G���[
	buffer->err_point = buffer->utf8s->length - 1;
	return 0;
}

/**
 * �擪�̉��s����菜���B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @return				�ǂݔ�΂����ʒu�B
 */
static size_t remove_head_linefield(TomlBuffer * buffer,
									size_t start)
{
	size_t i = start;
	if (buffer->utf8s->length - i >= 2 &&
		toml_get_char(buffer->utf8s, i).num == '\r' &&
		toml_get_char(buffer->utf8s, i + 1).num == '\n') {
		i += 2;
	}
	if (buffer->utf8s->length - i >= 1 &&
		toml_get_char(buffer->utf8s, i).num == '\n') {
		i += 1;
	}
	return i;
}

/**
 * """ �ň͂܂ꂽ��������擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @return				�擾�ł����� 0�ȊO�B
 */
static int toml_get_multi_string_value(TomlBuffer * buffer,
									   size_t start,
									   size_t * next_point)
{
	size_t		i, j;
	TomlUtf8	c;
	char		last_c[2];
	int			skip_space = 0;
	unsigned int	nx_c;

	// ������o�b�t�@������
	vec_clear(buffer->word_dst);
	memset(last_c, 0, sizeof(last_c));

	// �擪�̉��s�͎�菜��
	i = remove_head_linefield(buffer, start);

	do {
		for (; i < buffer->utf8s->length; ++i) {
			// �ꕶ���擾����
			c = toml_get_char(buffer->utf8s, i);

			switch (c.num) {
			case '"':
				// " ���擾�����當����I��
				if (toml_get_char(buffer->utf8s, i + 2).num == '"' &&
					toml_get_char(buffer->utf8s, i + 1).num == '"') {
					toml_append_char(buffer->word_dst, '\0');
					*next_point = i + 3;
					return (buffer->word_dst->length > 1);
				}
				else {
					toml_append_char(buffer->word_dst, '"');
				}

			case '\\':
				// \�̃G�X�P�[�v��������
				nx_c = toml_get_char(buffer->utf8s, i + 1).num;
				if (nx_c == '\r' || nx_c == '\n') {
					skip_space = 1;
					i += 1;
				}
				else if (!toml_append_escape_char(buffer, &i)) {
					buffer->err_point = i;
					return 0;
				}
				break;

			case ' ':
			case '\t':
				// �󔒕�����ǉ�����
				vec_add(buffer->word_dst, c.ch);
				break;

			default:
				// ��L�ȊO�͕��ʂ̕����Ƃ��Ď擾
				if (c.num > 0x1f) {
					skip_space = 0;
					for (j = 0; j < UTF8_CHARCTOR_SIZE && c.ch[j] != 0; ++j) {
						vec_add(buffer->word_dst, c.ch + j);
					}
				}
				else {
					last_c[1] = last_c[0];
					last_c[0] = c.num & 0x1f;
				}
				break;
			}
		}

		// ���s�̓ǂݔ�΂��w�肪�Ȃ���Βǉ�����
		if (!skip_space) {
			if (last_c[1] == '\r' && last_c[0] == '\n') {
				toml_append_char(buffer->word_dst, '\r');
				toml_append_char(buffer->word_dst, '\n');
			}
			else if (last_c[0] == '\n') {
				toml_append_char(buffer->word_dst, '\n');
			}
		}
		toml_read_buffer(buffer);

	} while (!toml_end_of_file(buffer));

	// " �ŏI���ł��Ȃ��������߃G���[
	buffer->err_point = buffer->utf8s->length - 1;
	return 0;
}

/**
 * "'" �ň͂܂ꂽ��������擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @return				�擾�ł����� 0�ȊO�B
 */
static int toml_get_multi_literal_string_value(TomlBuffer * buffer,
											   size_t start,
											   size_t * next_point)
{
	size_t		i, j;
	TomlUtf8	c;

	// ������o�b�t�@������
	vec_clear(buffer->word_dst);

	// �擪�̉��s�͎�菜��
	i = remove_head_linefield(buffer, start);

	do {
		for (; i < buffer->utf8s->length; ++i) {
			// �ꕶ���擾����
			c = toml_get_char(buffer->utf8s, i);

			switch (c.num) {
			case '\'':
				// " ���擾�����當����I��
				if (toml_get_char(buffer->utf8s, i + 2).num == '\'' &&
					toml_get_char(buffer->utf8s, i + 1).num == '\'') {
					toml_append_char(buffer->word_dst, '\0');
					*next_point = i + 3;
					return (buffer->word_dst->length > 1);
				}
				else {
					toml_append_char(buffer->word_dst, '\'');
				}
				break;

			default:
				// ��L�ȊO�͕��ʂ̕����Ƃ��Ď擾
				for (j = 0; j < UTF8_CHARCTOR_SIZE && c.ch[j] != 0; ++j) {
					vec_add(buffer->word_dst, c.ch + j);
				}
				break;
			}
		}
		toml_read_buffer(buffer);

	} while (!toml_end_of_file(buffer));

	// " �ŏI���ł��Ȃ��������߃G���[
	buffer->err_point = buffer->utf8s->length - 1;
	return 0;
}

/**
 * �����̎w�������擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param point			�J�n�ʒu�B
 * @param digit			�����_�ʒu�B
 * @param expo			�w��(e)�B
 * @param res_expo		�w���l�i�߂�l�j
 * @param value_res		�擾���ʁB
 * @return				�ǂݍ��ݏI���ʒu�B
 */
static size_t exponent_convert(TomlBuffer * buffer,
							   size_t point,
							   int digit,
							   int expo,
							   int * res_expo,
							   TomlResult * value_res)
{
	size_t		i = point;
	int			ud = 0;
	TomlUtf8	c;
	int			sign = 0;
	int			exp_v = 0;

	*value_res = SUCCESS;

	if (expo >= 0) {
		// �������擾����
		c = toml_get_char(buffer->utf8s, i);
		if (c.num == '+') {
			sign = 0;
			i++;
		}
		else if (c.num == '-') {
			sign = 1;
			i++;
		}

		ud = 0;
		for (; i < buffer->utf8s->length; ++i) {
			// �ꕶ���擾����
			c = toml_get_char(buffer->utf8s, i);

			// 1. '_'�̘A���̔���
			// 2. �w�������v�Z����
			if (c.num == '_') {
				if (ud) {								// 1
					*value_res = UNDERBAR_CONTINUE_ERR;
					buffer->err_point = i;
					return i;
				}
				ud = 1;
			}
			else if (c.num >= '0' && c.num <= '9') {
				exp_v = exp_v * 10 + (c.num - '0');		// 2
				if (exp_v >= 308) {
					*value_res = DOUBLE_VALUE_ERR;
					return i;
				}
				ud = 0;
			}
			else {
				break;
			}
		}

		// �w���l���m�F����
		// 1. �w���l�� 0�ȉ��łȂ����Ƃ��m�F
		// 2. �w���l�� '0'�n�܂�łȂ����Ƃ��m�F
		if (exp_v <= 0) {
			*value_res = DOUBLE_VALUE_ERR;	// 1
		}
		else if (toml_get_char(buffer->utf8s, point).num == '0') {
			*value_res = ZERO_NUMBER_ERR;	// 2
		}
	} 

	// ������ݒ�
	if (sign) { exp_v = -exp_v; }

	// �����_�ʒu�ƃ}�[�W
	exp_v -= (digit > 0 ? digit : 0);
	if (exp_v > 308 || exp_v < -308) {
		*value_res = DOUBLE_VALUE_RANGE_ERR;
		return 0;
	}

	*res_expo = exp_v;
	return i;
}

/**
 * ���l�i�����^�����j���擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param tokenType		�擾�f�[�^�B
 * @param value_res		�擾���ʁB
 * @return				�擾�ł����� 0�ȊO�B
 */
int toml_get_number_value(TomlBuffer * buffer,
						  size_t point,
						  size_t * next_point,
						  TomlValueType * tokenType,
						  TomlResult * value_res)
{
 	unsigned long long int	v = 0;
	size_t			i, j;
	int				ud = 0;
	TomlUtf8		c;
	int				digit = -1;
	int				expo = -1;
	int				exp_v = -1;
	double long		dv;

	for (i = point; i < buffer->utf8s->length; ++i) {
		// �ꕶ���擾����
		c = toml_get_char(buffer->utf8s, i);

		// 1. '_'�̘A���̔���
		// 2. ���l�̌v�Z������
		// 3. �����_�ʒu���擾����
		// 4. �w�����ie�j���擾����
		if (c.num == '_') {
			if (ud) {							// 1
				*value_res = UNDERBAR_CONTINUE_ERR;
				buffer->err_point = i;
				return 0;
			}
			ud = 1;
		}
		else if (c.num >= '0' && c.num <= '9') {
			if (v < ULLONG_MAX / 10) {			// 2
				v = v * 10 + (c.num - '0');
				if (digit >= 0) {
					digit++;
				}
			}
			else {
				*value_res = INTEGER_VALUE_RANGE_ERR;
				buffer->err_point = i;
				return 0;
			}
			ud = 0;
		}
		else if (c.num == '.') {				// 3
			if (digit < 0) {
				digit = 0;
			}
			else {
				*value_res = MULTI_DECIMAL_ERR;
				buffer->err_point = i;
				return 0;
			}
		}
		else if (c.num == 'e' || c.num == 'E') {
			expo = 0;							// 4
			i++;
			break;
		}
		else {
			break;
		}
	}

	// ���������擾����
	i = exponent_convert(buffer, i, digit, expo, &exp_v, value_res);
	if (*value_res != SUCCESS) {
		return 0;
	}

	if (digit < 0 && expo < 0) {
		// �����l���擾����
		*tokenType = TomlIntegerValue;
		if (v <= LONG_MAX) {
			buffer->integer = (long long int)v;
		}
		else {
			*value_res = INTEGER_VALUE_RANGE_ERR;
			return 0;
		}
	}
	else {
		// �����l���擾����
		//
		// 1. ���̎w���Ȃ珜�Z
		// 2. ���̎w���Ȃ�ώZ
		// 3. 0�̎w���Ȃ�g�p���Ȃ�
		// 4. �l��ێ�
		*tokenType = TomlFloatValue;
		if (exp_v < 0) {
			double abs_e = 1;		// 1
			for (j = 0; j < abs(exp_v); ++j) {
				abs_e *= 10;
			}
			dv = (long double)v / abs_e;
		}
		else if (exp_v > 0) {
			double abs_e = 1;		// 2
			for (j = 0; j < abs(exp_v); ++j) {
				abs_e *= 10;
			}
			dv = (long double)v * abs_e;
		}
		else {
			dv = (long double)v;	// 3
		}
		if (dv <= DBL_MAX) {		// 4
			buffer->float_number = (double)dv;
		}
		else {
			*value_res = DOUBLE_VALUE_RANGE_ERR;
			return 0;
		}
	}

	*next_point = i;
	return 1;
}

/**
 * ���l�i���t�^�����^�����j���擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param point			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param tokenType		�擾�f�[�^�B
 * @param value_res		�擾���ʁB
 * @return				�擾�ł����� 0�ȊO�B
 */
int toml_get_number_or_date_value(TomlBuffer * buffer,
								  size_t point,
								  size_t * next_point,
								  TomlValueType * tokenType,
								  TomlResult * value_res)
{
	if (buffer->utf8s->length - point >= 5) {
		//if (toml_get_char(buffer->utf8s, point + 4).num == 'e' &&
		//	toml_get_char(buffer->utf8s, point + 3).num == 's' &&
		//	toml_get_char(buffer->utf8s, point + 2).num == 'l' &&
		//	toml_get_char(buffer->utf8s, point + 1).num == 'a' &&
		//	toml_get_char(buffer->utf8s, point).num == 'f') {
		//	buffer->boolean = 0;
		//	*next_point = point + 5;
		//	*tokenType = TomlBooleanValue;
		//	return 1;
		//}
	}

	if (buffer->utf8s->length - point >= 3) {
		//if (toml_get_char(buffer->utf8s, point + 4).num == 'e' &&
		//	toml_get_char(buffer->utf8s, point + 3).num == 's' &&
		//	toml_get_char(buffer->utf8s, point + 2).num == 'l' &&
		//	toml_get_char(buffer->utf8s, point + 1).num == 'a' &&
		//	toml_get_char(buffer->utf8s, point).num == 'f') {
		//	buffer->boolean = 0;
		//	*next_point = point + 5;
		//	*tokenType = TomlBooleanValue;
		//	return 1;
		//}
	}

	// ���l�i�����^�����j���擾����
	return toml_get_number_value(buffer, point, next_point, tokenType, value_res);
}

int toml_convert_value(TomlBuffer * buffer,
					   size_t point,
					   size_t * next_point,
					   TomlValueType * tokenType,
					   TomlResult * value_res)
{
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
		// �������C���������Ԃ�
		if (toml_get_char(buffer->utf8s, point + 2).num == '"' &&
			toml_get_char(buffer->utf8s, point + 1).num == '"' &&
			toml_get_char(buffer->utf8s, point).num == '"') {
			if (toml_get_multi_string_value(buffer, point + 3, next_point)) {
				*tokenType = TomlStringValue;
				return 1;
			}
			else {
				*value_res = MULTI_QUOAT_STRING_ERR;
				return 0;
			}
		}

		// �������C��������i���e�����j��Ԃ�
		if (toml_get_char(buffer->utf8s, point + 2).num == '\'' &&
			toml_get_char(buffer->utf8s, point + 1).num == '\'' &&
			toml_get_char(buffer->utf8s, point).num == '\'') {
			if (toml_get_multi_literal_string_value(buffer, point + 3, next_point)) {
				*tokenType = TomlStringValue;
				return 1;
			}
			else {
				*value_res = MULTI_LITERAL_STRING_ERR;
				return 0;
			}
		}
	}

	if (buffer->utf8s->length - point > 0) {
		c = toml_get_char(buffer->utf8s, point);
		switch (c.num)
		{
		case '"':
			// ��������擾����
			if (toml_get_string_value(buffer, point + 1, next_point)) {
				*tokenType = TomlStringValue;
				return 1;
			}
			else {
				*value_res = QUOAT_STRING_ERR;
				return 0;
			}

		case '\'':
			// ���e������������擾����
			if (toml_get_literal_string_value(buffer, point + 1, next_point)) {
				*tokenType = TomlStringValue;
				*value_res = LITERAL_STRING_ERR;
				return 1;
			}
			else {
				return 0;
			}

		case '#':
			// �R�����g���擾����
			*next_point = point + 1;
			return 1;

		case '+':
			// ���l���擾����
			if (toml_get_number_value(buffer, point + 1,
									  next_point, tokenType, value_res)) {
				return 1;
			}
			else {
				return 0;
			}

		case '-':
			// ���l���擾����
			//
			// 1. �����l�̕������t�]������
			// 2. �����l�̕������t�]������
			if (toml_get_number_value(buffer, point + 1,
									  next_point, tokenType, value_res)) {
				switch (*tokenType) {
				case TomlIntegerValue:		// 1
					if (buffer->integer < LONG_MAX) {
						buffer->integer = -buffer->integer;
						return 1;
					}
					else {
						*value_res = INTEGER_VALUE_RANGE_ERR;
						return 0;
					}

				case TomlFloatValue:		// 2
					buffer->float_number = -buffer->float_number;
					return 1;

				default:
					return 0;
				}
			}
			else {
				return 0;
			}
			break;

		default:
			// ���t�^���l���擾����
			return toml_get_number_or_date_value(buffer, point,
									next_point, tokenType, value_res);
		}
	}
	else {
		*value_res = INVALID_NAME_ERR;
		return 0;
	}
}
