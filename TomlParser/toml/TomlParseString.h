#ifndef __TOML_PARSE_STRING_H__
#define __TOML_PARSE_STRING_H__

/**
 * �擪�̉��s����菜���B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @return				�ǂݔ�΂����ʒu�B
 */
size_t toml_skip_head_linefield(TomlBuffer * buffer, size_t start);

/**
 * ���s�A�󔒕���ǂݔ�΂��B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @return				�ǂݔ�΂����ʒu�B
 */
size_t toml_skip_linefield_and_space(TomlBuffer * buffer, size_t start);

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
int toml_get_literal_string_value(TomlBuffer * buffer,
		size_t start, size_t * next_point, TomlResultSummary * error);

/**
 * """ �ň͂܂ꂽ��������擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param error			�G���[�ڍ׏��i�߂�l�j
 * @return				�擾�ł����� 0�ȊO�B
 */
int toml_get_multi_string_value(TomlBuffer * buffer,
		size_t start, size_t * next_point, TomlResultSummary * error);

/**
 * "'" �ň͂܂ꂽ��������擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param error			�G���[�ڍ׏��i�߂�l�j
 * @return				�擾�ł����� 0�ȊO�B
 */
int toml_get_multi_literal_string_value(TomlBuffer * buffer,
		size_t start, size_t * next_point, TomlResultSummary * error);

#endif /* __TOML_PARSE_STRING_H__ */