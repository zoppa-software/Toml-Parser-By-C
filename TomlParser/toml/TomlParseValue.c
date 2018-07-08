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
#include "TomlParseHelper.h"
#include "TomlBuffer.h"
#include "TomlParseString.h"
#include "TomlParseValue.h"

//-----------------------------------------------------------------------------
// ���l���
//-----------------------------------------------------------------------------
/**
 * �����̎w�������擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param point			�J�n�ʒu�B
 * @param next_point	�ǂݍ��ݏI���ʒu�i�߂�l�j
 * @param digit			�����_�ʒu�B
 * @param expo			�w��(e)�B
 * @param res_expo		�w���l�i�߂�l�j
 * @param error			�G���[�ڍ׏��B
 * @return				�擾�ł����� 0�ȊO�B
 */
static int exponent_convert(TomlBuffer * buffer,
							size_t point,
							size_t * next_point,
							int digit,
							int expo,
							int * res_expo,
							TomlResultSummary * error)
{
	size_t		i = point;
	int			ud = 0;
	TomlUtf8	c;
	int			sign = 0;
	int			exp_v = 0;

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
					*error = toml_res_ctor(UNDERBAR_CONTINUE_ERR, i, buffer->loaded_line);
					*next_point = i;
					return 0;
				}
				ud = 1;
			}
			else if (c.num >= '0' && c.num <= '9') {
				exp_v = exp_v * 10 + (c.num - '0');		// 2
				if (exp_v >= EXPO_MAX_RANGE) {
					*error = toml_res_ctor(DOUBLE_VALUE_ERR, i, buffer->loaded_line);
					*next_point = i;
					return 0;
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
		if (exp_v <= 0) {											// 1
			*error = toml_res_ctor(DOUBLE_VALUE_ERR, i, buffer->loaded_line);
			*next_point = i;
			return 0;
		}
		else if (toml_get_char(buffer->utf8s, point).num == '0') {	// 2
			*error = toml_res_ctor(ZERO_NUMBER_ERR, i, buffer->loaded_line);
			*next_point = i;
			return 0;
		}
	} 

	// ������ݒ�
	if (sign) { exp_v = -exp_v; }

	// �����_�ʒu�ƃ}�[�W
	exp_v -= (digit > 0 ? digit : 0);
	if (exp_v > EXPO_MAX_RANGE || exp_v < EXPO_MIN_RANGE) {
		*error = toml_res_ctor(DOUBLE_VALUE_RANGE_ERR, i, buffer->loaded_line);
		*next_point = i;
		return 0;
	}

	*res_expo = exp_v;
	*next_point = i;
	return 1;
}

/**
 * ���l�i�����^�����j���擾����B
 *
 * @param number_sign	�����B
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param token_type	�擾�f�[�^�B
 * @param error			�G���[�ڍ׏��B
 * @return				�擾�ł����� 0�ȊO�B
 */
static int get_10number_value(int number_sign,
						      TomlBuffer * buffer,
						      size_t point,
						      size_t * next_point,
						      TomlValueType * token_type,
							  TomlResultSummary * error)
{
 	unsigned long long int	v = 0;
	size_t			i, j;
	int				ud = 0;
	TomlUtf8		c;
	int				digit = -1;
	int				expo = -1;
	int				exp_v = -1;
	double long		dv;
	int				ld_zero = 0, lst_zero = 0;

	// ���������擾����
	for (i = point; i < buffer->utf8s->length; ++i) {
		// �ꕶ���擾����
		c = toml_get_char(buffer->utf8s, i);

		// 1. '_'�̘A���̔���
		// 2. ���l�̌v�Z������
		//    2-1. ���l�̗L���͈͂𒴂���Ȃ�΃G���[
		// 3. �����_�ʒu���擾����
		// 4. �w�����ie�j���擾����
		if (c.num == '_') {
			if (ud) {							// 1
				*error = toml_res_ctor(UNDERBAR_CONTINUE_ERR, i, buffer->loaded_line);
				*next_point = i;
				return 0;
			}
			ud = 1;
		}
		else if (c.num >= '0' && c.num <= '9') {
			if (v < ULLONG_MAX / 10) {			// 2
				v = v * 10 + (c.num - '0');
				if (digit >= 0) {
					digit++;
					lst_zero = 1;
				}
				else {
					ld_zero = 1;
				}
			}
			else {								// 2-1
				*error = toml_res_ctor(INTEGER_VALUE_RANGE_ERR, i, buffer->loaded_line);
				*next_point = i;
				return 0;
			}
			ud = 0;
		}
		else if (c.num == '.') {				// 3
			if (ld_zero && digit < 0) {
				digit = 0;
			}
			else {
				*error = toml_res_ctor((ld_zero ? MULTI_DECIMAL_ERR : NO_LEADING_ZERO_ERR), i, buffer->loaded_line);
				*next_point = i;
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

	// �ꕶ���̐��l���Ȃ���� None�l��Ԃ�
	if (i == point) {
		*token_type = TomlNoneValue;
		*next_point = i;
		return 1;
	}

	// ���������擾����
	if (!exponent_convert(buffer, i, next_point,
						  digit, expo, &exp_v, error)) {
		return 0;
	}
	i = *next_point;

	if (digit < 0 && expo < 0) {
		// �����l���擾����
		//
		// 1. �����l��Ԃ����Ƃ��w��
		// 2. ���̐����ϊ�
		// 3. ���̐����ϊ�
		*token_type = TomlIntegerValue;			// 1
		if (number_sign) {
			if (v <= LLONG_MAX) {				// 2
				buffer->integer = -((long long int)v);
			}
			else {
				buffer->integer = (long long int)v;
				if (buffer->integer != LLONG_MIN) {
					*error = toml_res_ctor(INTEGER_VALUE_RANGE_ERR, i, buffer->loaded_line);
					return 0;
				}
			}
		}
		else {
			if (v <= LLONG_MAX) {				//3
				buffer->integer = (long long int)v;
			}
			else {
				*error = toml_res_ctor(INTEGER_VALUE_RANGE_ERR, i, buffer->loaded_line);
				return 0;
			}
		}
	}
	else {
		if (digit >= 0 && !lst_zero) {
			*error = toml_res_ctor(NO_LAST_ZERO_ERR, i, buffer->loaded_line);
			return 0;
		}

		// �����l���擾����
		//
		// 1. �����l��Ԃ����Ƃ��w��
		// 1. ���̎w���Ȃ珜�Z
		// 2. ���̎w���Ȃ�ώZ
		// 3. 0�̎w���Ȃ�g�p���Ȃ�
		// 4. �l��ێ�
		*token_type = TomlFloatValue;			// 1
		if (exp_v < 0) {
			double abs_e = 1;					// 2
			for (j = 0; j < abs(exp_v); ++j) {
				abs_e *= 10;
			}
			dv = (long double)v / abs_e;
		}
		else if (exp_v > 0) {
			double abs_e = 1;					// 3
			for (j = 0; j < abs(exp_v); ++j) {
				abs_e *= 10;
			}
			dv = (long double)v * abs_e;
		}
		else {
			dv = (long double)v;				// 4
		}
		if (dv <= DBL_MAX) {					// 5
			buffer->float_number = (number_sign ? (double)-dv : (double)dv);
		}
		else {
			*error = toml_res_ctor(DOUBLE_VALUE_RANGE_ERR, i, buffer->loaded_line);
			return 0;
		}
	}

	return 1;
}

/**
 * ���l�i16�i���j���擾����B
 *
 * @param number_sign	�����B
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param token_type	�擾�f�[�^�B
 * @param error			�G���[�ڍ׏��B
 * @return				�擾�ł����� 0�ȊO�B
 */
static int get_16number_value(TomlBuffer * buffer,
						      size_t point,
						      size_t * next_point,
						      TomlValueType * token_type,
							  TomlResultSummary * error)
{
 	unsigned long long int	v = 0;
	size_t			i;
	int				ud = 0;
	TomlUtf8		c;

	// ���l���擾����
	for (i = point; i < buffer->utf8s->length; ++i) {
		// �ꕶ���擾����
		c = toml_get_char(buffer->utf8s, i);

		// 1. '_'�̘A���̔���
		// 2. ���l�̌v�Z������
		//    2-1. ���l�̗L���͈͂𒴂���Ȃ�΃G���[
		if (c.num == '_') {
			if (ud) {							// 1
				*error = toml_res_ctor(UNDERBAR_CONTINUE_ERR, i, buffer->loaded_line);
				*next_point = i;
				return 0;
			}
			ud = 1;
		}
		else if (c.num >= '0' && c.num <= '9') {
			if (v < ULLONG_MAX / 16) {			// 2
				v = v * 16 + (c.num - '0');
			}
			else {								// 2-1
				*error = toml_res_ctor(INTEGER_VALUE_RANGE_ERR, i, buffer->loaded_line);
				*next_point = i;
				return 0;
			}
			ud = 0;
		}
		else if (c.num >= 'A' && c.num <= 'F') {
			if (v < ULLONG_MAX / 16) {			// 2
				v = v * 16 + (c.num - 'A') + 10;
			}
			else {								// 2-1
				*error = toml_res_ctor(INTEGER_VALUE_RANGE_ERR, i, buffer->loaded_line);
				*next_point = i;
				return 0;
			}
			ud = 0;
		}
		else if (c.num >= 'a' && c.num <= 'f') {
			if (v < ULLONG_MAX / 16) {			// 2
				v = v * 16 + (c.num - 'a') + 10;
			}
			else {								// 2-1
				*error = toml_res_ctor(INTEGER_VALUE_RANGE_ERR, i, buffer->loaded_line);
				*next_point = i;
				return 0;
			}
			ud = 0;
		}
		else {
			break;
		}
	}

	// �ꕶ���̐��l���Ȃ���� None�l��Ԃ�
	*next_point = i;
	if (i == point) {
		*token_type = TomlNoneValue;
		*next_point = i;
		return 1;
	}

	if (v <= LLONG_MAX) {
		*token_type = TomlIntegerValue;
		buffer->integer = (long long int)v;
		return 1;
	}
	else {
		*error = toml_res_ctor(INTEGER_VALUE_RANGE_ERR, i , buffer->loaded_line);
		return 0;
	}
}

/**
 * ���l�i8�i���j���擾����B
 *
 * @param number_sign	�����B
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param token_type	�擾�f�[�^�B
 * @param error			�G���[�ڍ׏��B
 * @return				�擾�ł����� 0�ȊO�B
 */
static int get_8number_value(TomlBuffer * buffer,
						     size_t point,
						     size_t * next_point,
						     TomlValueType * token_type,
							 TomlResultSummary * error)
{
 	unsigned long long int	v = 0;
	size_t			i;
	int				ud = 0;
	TomlUtf8		c;

	// ���l���擾����
	for (i = point; i < buffer->utf8s->length; ++i) {
		// �ꕶ���擾����
		c = toml_get_char(buffer->utf8s, i);

		// 1. '_'�̘A���̔���
		// 2. ���l�̌v�Z������
		//    2-1. ���l�̗L���͈͂𒴂���Ȃ�΃G���[
		if (c.num == '_') {
			if (ud) {							// 1
				*error = toml_res_ctor(UNDERBAR_CONTINUE_ERR, i, buffer->loaded_line);
				*next_point = i;
				return 0;
			}
			ud = 1;
		}
		else if (c.num >= '0' && c.num <= '7') {
			if (v < ULLONG_MAX / 8) {			// 2
				v = v * 8 + (c.num - '0');
			}
			else {								// 2-1
				*error = toml_res_ctor(INTEGER_VALUE_RANGE_ERR, i, buffer->loaded_line);
				*next_point = i;
				return 0;
			}
			ud = 0;
		}
		else {
			break;
		}
	}

	// �ꕶ���̐��l���Ȃ���� None�l��Ԃ�
	*next_point = i;
	if (i == point) {
		*token_type = TomlNoneValue;
		return 1;
	}

	if (v <= LLONG_MAX) {
		*token_type = TomlIntegerValue;
		buffer->integer = (long long int)v;
		return 1;
	}
	else {
		*error = toml_res_ctor(INTEGER_VALUE_RANGE_ERR, i, buffer->loaded_line);
		return 0;
	}
}

/**
 * ���l�i2�i���j���擾����B
 *
 * @param number_sign	�����B
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param token_type	�擾�f�[�^�B
 * @param error			�G���[�ڍ׏��B
 * @return				�擾�ł����� 0�ȊO�B
 */
static int get_2number_value(TomlBuffer * buffer,
						     size_t point,
						     size_t * next_point,
						     TomlValueType * token_type,
							 TomlResultSummary * error)
{
 	unsigned long long int	v = 0;
	size_t			i;
	int				ud = 0;
	TomlUtf8		c;

	// ���l���擾����
	for (i = point; i < buffer->utf8s->length; ++i) {
		// �ꕶ���擾����
		c = toml_get_char(buffer->utf8s, i);

		// 1. '_'�̘A���̔���
		// 2. ���l�̌v�Z������
		//    2-1. ���l�̗L���͈͂𒴂���Ȃ�΃G���[
		if (c.num == '_') {
			if (ud) {							// 1
				*error = toml_res_ctor(UNDERBAR_CONTINUE_ERR, i, buffer->loaded_line);
				*next_point = i;
				return 0;
			}
			ud = 1;
		}
		else if (c.num >= '0' && c.num <= '1') {
			if (v < ULLONG_MAX / 2) {			// 2
				v = v * 2 + (c.num - '0');
			}
			else {								// 2-1
				*error = toml_res_ctor(INTEGER_VALUE_RANGE_ERR, i, buffer->loaded_line);
				*next_point = i;
				return 0;
			}
			ud = 0;
		}
		else {
			break;
		}
	}

	// �ꕶ���̐��l���Ȃ���� None�l��Ԃ�
	*next_point = i;
	if (i == point) {
		*token_type = TomlNoneValue;
		return 1;
	}

	if (v <= LLONG_MAX) {
		*token_type = TomlIntegerValue;
		buffer->integer = (long long int)v;
		return 1;
	}
	else {
		*error = toml_res_ctor(INTEGER_VALUE_RANGE_ERR, i, buffer->loaded_line);
		return 0;
	}
}

/**
 * ���l�i�����^�����j���擾����B
 *
 * @param number_sign	�����B
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param token_type	�擾�f�[�^�B
 * @param error			�G���[�ڍ׏��B
 * @return				�擾�ł����� 0�ȊO�B
 */
static int get_number_value(int number_sign,
							TomlBuffer * buffer,
							size_t point,
							size_t * next_point,
							TomlValueType * token_type,
							TomlResultSummary * error)
{
	if (number_sign == 0 &&
		buffer->utf8s->length - point >= 3 &&
		toml_get_char(buffer->utf8s, point).num == '0') {
		switch (toml_get_char(buffer->utf8s, point + 1).num)
		{
		case 'x':
			return get_16number_value(buffer, point + 2, next_point, token_type, error);
		case 'o':
			return get_8number_value(buffer, point + 2, next_point, token_type, error);
		case 'b':
			return get_2number_value(buffer, point + 2, next_point, token_type, error);
		default:
			break;
		}
	}
	return get_10number_value(number_sign, buffer,
					point, next_point, token_type, error);
}

//-----------------------------------------------------------------------------
// ���t���
//-----------------------------------------------------------------------------
/**
 * ������𕔕��I�ɐ؂���A���l�\�����m�F�A�擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param digit			�K�v�����B
 * @param point			�J�n�ʒu�B
 * @param result		�Ǎ����ʁi�߂�l�j
 * @return				�擾�ł����� 0�ȊO�B
 */
static int convert_partition_number(TomlBuffer * buffer,
									size_t digit,
								    size_t point,
									int * result)
{
	size_t	i;
	int		v, num = 0;
	if (buffer->utf8s->length - point < digit) {
		// �w�茅�������̐��l�ł��邽�߃G���[
		return 0;
	}
	else {
		// �K�v���������[�v���A�S�Ă����l�\���ł��邱�Ƃ��m�F
		// 1. ���[�v
		// 2. ���l���肵�A���ʂ��쐬����
		// 3. ���ʂ�Ԃ�
		for (i = 0; i < digit; ++i) {			// 1
			v = toml_get_char(buffer->utf8s, point + i).num;
			if (v >= '0' && v <= '9') {			// 2
				num = num * 10 + (v - '0');
			}
			else {
				return 0;
			}
		}
		*result = num;							// 3
		return 1;
	}
}

/**
 * ���t�\���̎��ԕ�������͂���B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param point			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param hour			���l�B
 * @param error			�G���[�ڍ׏��B
 * @return				�擾�ł����� 0�ȊO�B
 */
static int convert_time_format(TomlBuffer * buffer,
							   size_t point,
							   size_t * next_point,
							   int hour,
							   TomlResultSummary * error)
{
	int		minute, second, z_hor, z_min;
	size_t	i;
	unsigned int dec_sec = 0, v;
	TomlUtf8 c;

	// ���A�b�̔���
	//
	// 1. �擾�ł�����i�[
	// 2. �擾�ł��Ȃ�������G���[��Ԃ�
	if (convert_partition_number(buffer, 2, point + 3, &minute) &&
		toml_get_char(buffer->utf8s, point + 5).num == ':' &&
		convert_partition_number(buffer, 2, point + 6, &second)) {
		buffer->date.hour = (unsigned char)hour;
		buffer->date.minute = (unsigned char)minute;
		buffer->date.second = (unsigned char)second;
	}
	else {
		*error = toml_res_ctor(DATE_FORMAT_ERR, point + 3, buffer->loaded_line);
		*next_point = point;
		return 0;
	}

	// �~���b�̉��
	//
	// 1. '.' ������΃~���b��͊J�n
	// 2. �~���b�l���v�Z
	// 3. �~���b��ێ�����
	point += 8;
	if (toml_get_char(buffer->utf8s, point).num == '.') {		// 1
		for (i = point + 1; i < buffer->utf8s->length; ++i) {
			v = toml_get_char(buffer->utf8s, i).num;			// 2
			if (v >= '0' && v <= '9') {
				dec_sec = dec_sec * 10 + (v - '0');
			}
			else {
				break;
			}
		}
		buffer->date.dec_second = dec_sec;						// 3
		point = i;
	}

	// �����̉��
	//
	// 1. UTC�w��Ȃ�ΏI��
	// 2. �����w��Ȃ�΁A��������荞��
	//    2-1. �����̏����ɖ�肪�Ȃ���ΏI��
	//    2-2. �����̏����ɖ�肪����΃G���[
	// 3. �����w��Ȃ��A����I��
	*next_point = point;
	c = toml_get_char(buffer->utf8s, point);
	if (c.num == 'Z' || c.num == 'z') {							// 1
		*next_point = point + 1;
		return 1;
	}
	else if (c.num == '+' || c.num == '-') {
		if (convert_partition_number(buffer, 2, point + 1, &z_hor) &&
			toml_get_char(buffer->utf8s, point + 3).num == ':' &&
			convert_partition_number(buffer, 2, point + 4, &z_min)) {
			buffer->date.zone_hour = (c.num == '+' ? (char)z_hor : (char)-z_hor);
			buffer->date.zone_minute = (unsigned char)z_min;	// 2-1
			*next_point = point + 6;
			return 1;
		}
		else {													// 2-2
			*error = toml_res_ctor(DATE_FORMAT_ERR, point, buffer->loaded_line);
			*next_point = point;
			return 0;
		}
	}
	else {
		return 1;												// 3
	}
}

/**
 * ���t�\���̓��ɂ���������͂���B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param point			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param year			�N�l�B
 * @param error			�G���[�ڍ׏��B
 * @return				�擾�ł����� 0�ȊO�B
 */
static int convert_date_format(TomlBuffer * buffer,
							   size_t point,
							   size_t * next_point,
							   int year,
							   TomlResultSummary * error)
{
	int		month, day, hour;
	TomlUtf8 c;

	// �����̔���
	//
	// 1. �擾�ł�����i�[
	// 2. �擾�ł��Ȃ�������G���[��Ԃ�
	if (convert_partition_number(buffer, 2, point + 5, &month) &&
		toml_get_char(buffer->utf8s, point + 7).num == '-' &&
		convert_partition_number(buffer, 2, point + 8, &day)) {
		buffer->date.year = (unsigned short)year;	// 1
		buffer->date.month = (unsigned char)month;
		buffer->date.day = (unsigned char)day;
	}
	else {											// 2
		*error = toml_res_ctor(DATE_FORMAT_ERR, point + 5, buffer->loaded_line);
		*next_point = point;
		return 0;
	}

	// 'T' �̎w�肪�Ȃ���Γ��ɂ��̂݁A�I��
	c = toml_get_char(buffer->utf8s, point + 10);
	*next_point = point + 10;
	if (c.num != 'T' && c.num != 't' && c.num != ' ' &&
		(c.num == '\t' || c.num == '#' || c.num == '\r' || c.num == '\n')) {
		return 1;
	}

	// ���ԏ��𔻒肵�ĕԂ�
	if (convert_partition_number(buffer, 2, point + 11, &hour) &&
		toml_get_char(buffer->utf8s, point + 13).num == ':') {
		return convert_time_format(buffer, point + 11, next_point, hour, error);
	}
	else if (c.num == ' ') {
		return 1;
	}
	else {
		*error = toml_res_ctor(DATE_FORMAT_ERR, point + 11, buffer->loaded_line);
		*next_point = point;
		return 0;
	}
}

//-----------------------------------------------------------------------------
// ���l�^���t�^��������
//-----------------------------------------------------------------------------
/**
 * ���l�i���t�^�����^�����j���擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param point			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param tokenType		�擾�f�[�^�B
 * @param error			�G���[�ڍ׏��B
 * @return				�擾�ł����� 0�ȊO�B
 */
static int get_number_or_date_value(TomlBuffer * buffer,
									size_t point,
									size_t * next_point,
									TomlValueType * tokenType,
									TomlResultSummary * error)
{
	int		year, hour;

	memset(&buffer->date, 0, sizeof(TomlDate));

	if (convert_partition_number(buffer, 4, point, &year) &&
		toml_get_char(buffer->utf8s, point + 4).num == '-') {
		// ���t�i�N�����j���擾����
		if (!convert_date_format(buffer, point, next_point, year, error)) {
			return 0;	// �G���[�ڍׂ͌Ăяo����Őݒ�
		}
		else {
			*tokenType = TomlDateValue;
			return 1;
		}
	}
	else if (convert_partition_number(buffer, 2, point, &hour) &&
			 toml_get_char(buffer->utf8s, point + 2).num == ':') {
		// ���t�i�����b�j���擾����
		if (!convert_time_format(buffer, point, next_point, hour, error)) {
			return 0;	// �G���[�ڍׂ͌Ăяo����Őݒ�
		}
		else {
			*tokenType = TomlDateValue;
			return 1;
		}
	}
	else {
		// ���l�i�����^�����j���擾����
		return get_number_value(0, buffer, point, next_point, tokenType, error);
	}
}

/**
 * ���l�i�萔�j���擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param point			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param token_type	�擾�f�[�^�B
 * @return				�擾�ł����� 0�ȊO�B
 */
static int analisys_keyword(TomlBuffer * buffer,
							size_t point,
							size_t * next_point,
							TomlValueType * token_type)
{
	// �^�U�l�i�U�j��Ԃ�
	if (buffer->utf8s->length - point >= 5) {
		if (toml_get_char(buffer->utf8s, point + 4).num == 'e' &&
			toml_get_char(buffer->utf8s, point + 3).num == 's' &&
			toml_get_char(buffer->utf8s, point + 2).num == 'l' &&
			toml_get_char(buffer->utf8s, point + 1).num == 'a' &&
			toml_get_char(buffer->utf8s, point).num == 'f') {
			buffer->boolean = 0;
			*next_point = point + 5;
			*token_type = TomlBooleanValue;
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
			*token_type = TomlBooleanValue;
			return 1;
		}
	}

	// �����l��Ԃ�
	if (buffer->utf8s->length - point >= 3) {
		if (toml_get_char(buffer->utf8s, point + 2).num == 'f' &&
			toml_get_char(buffer->utf8s, point + 1).num == 'n' &&
			toml_get_char(buffer->utf8s, point).num == 'i') {
			buffer->float_number = INFINITY;
			*next_point = point + 3;
			*token_type = TomlFloatValue;
			return 1;
		}
	}

	// ���̖����l��Ԃ�
	if (buffer->utf8s->length - point >= 4) {
		if (toml_get_char(buffer->utf8s, point + 3).num == 'f' &&
			toml_get_char(buffer->utf8s, point + 2).num == 'n' &&
			toml_get_char(buffer->utf8s, point + 1).num == 'i' &&
			toml_get_char(buffer->utf8s, point).num == '+') {
			buffer->float_number = INFINITY;
			*next_point = point + 4;
			*token_type = TomlFloatValue;
			return 1;
		}
	}

	// ���̖����l��Ԃ�
	if (buffer->utf8s->length - point >= 4) {
		if (toml_get_char(buffer->utf8s, point + 3).num == 'f' &&
			toml_get_char(buffer->utf8s, point + 2).num == 'n' &&
			toml_get_char(buffer->utf8s, point + 1).num == 'i' &&
			toml_get_char(buffer->utf8s, point).num == '-') {
			buffer->float_number = -INFINITY;
			*next_point = point + 4;
			*token_type = TomlFloatValue;
			return 1;
		}
	}

	// �񐔒l��Ԃ�
	if (buffer->utf8s->length - point >= 3) {
		if (toml_get_char(buffer->utf8s, point + 2).num == 'n' &&
			toml_get_char(buffer->utf8s, point + 1).num == 'a' &&
			toml_get_char(buffer->utf8s, point).num == 'n') {
			buffer->float_number = NAN;
			*next_point = point + 3;
			*token_type = TomlFloatValue;
			return 1;
		}
	}

	// ���̔񐔒l��Ԃ�
	if (buffer->utf8s->length - point >= 4) {
		if (toml_get_char(buffer->utf8s, point + 3).num == 'n' &&
			toml_get_char(buffer->utf8s, point + 2).num == 'a' &&
			toml_get_char(buffer->utf8s, point + 1).num == 'n' &&
			toml_get_char(buffer->utf8s, point).num == '+') {
			buffer->float_number = NAN;
			*next_point = point + 4;
			*token_type = TomlFloatValue;
			return 1;
		}
	}

	// ���̔񐔒l��Ԃ�
	if (buffer->utf8s->length - point >= 4) {
		if (toml_get_char(buffer->utf8s, point + 3).num == 'n' &&
			toml_get_char(buffer->utf8s, point + 2).num == 'a' &&
			toml_get_char(buffer->utf8s, point + 1).num == 'n' &&
			toml_get_char(buffer->utf8s, point).num == '-') {
			buffer->float_number = -NAN;
			*next_point = point + 4;
			*token_type = TomlFloatValue;
			return 1;
		}
	}
	return 0;
}

/**
 * ���l�i�����^�����^���t�j���擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param point			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param tokenType		�擾�f�[�^�B
 * @param error			�G���[�ڍ׏��B
 * @return				�擾�ł����� 0�ȊO�B
 */
int toml_convert_value(TomlBuffer * buffer,
					   size_t point,
					   size_t * next_point,
					   TomlValueType * token_type,
					   TomlResultSummary * error)
{
	TomlUtf8		c;

	// ���̔񐔒l��Ԃ�
	if (analisys_keyword(buffer, point, next_point, token_type)) {
		return 1;
	}

	// �������C���������Ԃ�
	if (buffer->utf8s->length - point >= 3) {
		// �������C���������Ԃ�
		if (toml_get_char(buffer->utf8s, point + 2).num == '"' &&
			toml_get_char(buffer->utf8s, point + 1).num == '"' &&
			toml_get_char(buffer->utf8s, point).num == '"') {
			if (toml_get_multi_string_value(buffer, point + 3, next_point, error)) {
				*token_type = TomlStringValue;
				return 1;
			}
			else {
				return 0;	// �G���[�ڍׂ͌Ăяo����Őݒ�
			}
		}

		// �������C��������i���e�����j��Ԃ�
		if (toml_get_char(buffer->utf8s, point + 2).num == '\'' &&
			toml_get_char(buffer->utf8s, point + 1).num == '\'' &&
			toml_get_char(buffer->utf8s, point).num == '\'') {
			if (toml_get_multi_literal_string_value(buffer, point + 3, next_point, error)) {
				*token_type = TomlStringValue;
				return 1;
			}
			else {
				return 0;	// �G���[�ڍׂ͌Ăяo����Őݒ�
			}
		}
	}

	// ���l�^���t�^������
	if (buffer->utf8s->length - point > 0) {
		c = toml_get_char(buffer->utf8s, point);
		switch (c.num)
		{
		case '"':
			// ��������擾����
			if (toml_get_string_value(buffer, point + 1, next_point, error)) {
				*token_type = TomlStringValue;
				return 1;
			}
			else {
				return 0;	// �G���[�ڍׂ͌Ăяo����Őݒ�
			}

		case '\'':
			// ���e������������擾����
			if (toml_get_literal_string_value(buffer, point + 1, next_point, error)) {
				*token_type = TomlStringValue;
				return 1;
			}
			else {
				return 0;	// �G���[�ڍׂ͌Ăяo����Őݒ�
			}

		case '#':
			// �R�����g���擾����
			*next_point = point + 1;
			return 1;

		case '+':
			// ���l���擾����
			if (get_number_value(0, buffer, point + 1,
								 next_point, token_type, error)) {
				return 1;
			}
			else {
				return 0;	// �G���[�ڍׂ͌Ăяo����Őݒ�
			}

		case '-':
			// ���l���擾����
			//
			// 1. �����l�̕������t�]������
			// 2. �����l�̕������t�]������
			if (get_number_value(1, buffer, point + 1,
								 next_point, token_type, error)) {
				return 1;
			}
			else {
				return 0;	// �G���[�ڍׂ͌Ăяo����Őݒ�
			}
			break;

		default:
			// ���t�^���l���擾����
			return get_number_or_date_value(buffer, point,
									next_point, token_type, error);
		}
	}
	else {
		// �l���擾�ł��Ȃ�����
		*error = toml_res_ctor(INVALID_NAME_ERR, point, buffer->loaded_line);
		*next_point = point;
		return 0;
	}
}
