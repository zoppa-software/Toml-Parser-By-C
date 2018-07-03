#ifndef __TOML_PARSE_STRING_H__
#define __TOML_PARSE_STRING_H__

/**
 * �L�[������̎擾�B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param point			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param error			�G���[�ڍ׏��i�߂�l�j
 * @return				�擾�ł����� 0�ȊO�B
 */
int toml_get_key(TomlBuffer * buffer, size_t point,
		size_t * next_point, TomlResultSummary * error);

/**
 * " �ň͂܂ꂽ��������擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param error			�G���[�ڍ׏��i�߂�l�j
 * @return				�擾�ł����� 0�ȊO�B
 */
int toml_get_string_value(TomlBuffer * buffer, size_t start,
		size_t * next_point, TomlResultSummary * error);

/**
 * ' �ň͂܂ꂽ��������擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param error			�G���[�ڍ׏��i�߂�l�j
 * @return				�擾�ł����� 0�ȊO�B
 */
int get_literal_string_value(TomlBuffer * buffer, size_t start,
		size_t * next_point, TomlResultSummary * error);

/**
 * """ �ň͂܂ꂽ��������擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param error			�G���[�ڍ׏��i�߂�l�j
 * @return				�擾�ł����� 0�ȊO�B
 */
int get_multi_string_value(TomlBuffer * buffer, size_t start,
		size_t * next_point, TomlResultSummary * error);

/**
 * "'" �ň͂܂ꂽ��������擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param error			�G���[�ڍ׏��i�߂�l�j
 * @return				�擾�ł����� 0�ȊO�B
 */
int get_multi_literal_string_value(TomlBuffer * buffer,
		size_t start, size_t * next_point, TomlResultSummary * error);

#endif /* __TOML_PARSE_STRING_H__ */