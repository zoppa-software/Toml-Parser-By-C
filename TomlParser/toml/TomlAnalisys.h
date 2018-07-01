#ifndef __TOML_ANALISYS_H__
#define __TOML_ANALISYS_H__

/**
 * �󔒕�����ǂݔ�΂��B
 *
 * @param impl	toml�h�L�������g�B
 * @param i		�ǂݍ��݈ʒu�B
 * @return		�ǂݔ�΂����ʒu�B
 */
size_t toml_skip_space(Vec * utf8s, size_t i);

/**
 * �󔒂�ǂݔ�΂��A�������s�^�I�[�^�R�����g�Ȃ�ΐ^��Ԃ��B
 *
 * @param utf8s		�Ώە�����B
 * @param i			�����J�n�ʒu�B
 * @return			���s�^�I�[�^�R�����g�Ȃ�� 0�ȊO�B
 */
int toml_skip_linefield(Vec * utf8s, size_t i);

/**
 * ']' �̌�A�������s�^�I�[�^�R�����g�Ȃ�ΐ^��Ԃ��B
 *
 * @param utf8s		�Ώە�����B
 * @param i			�����J�n�ʒu�B
 * @return			���s�^�I�[�^�R�����g�Ȃ�� 0�ȊO�B
 */
int toml_close_table(Vec * utf8s, size_t i);

/**
 * ']]' �̌�A�������s�^�I�[�^�R�����g�Ȃ�ΐ^��Ԃ��B
 *
 * @param utf8s		�Ώە�����B
 * @param i			�����J�n�ʒu�B
 * @return			���s�^�I�[�^�R�����g�Ȃ�� 0�ȊO�B
 */
int toml_close_table_array(Vec * utf8s, size_t i);

/**
 * �L�[������̎擾�B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param point			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @return				�擾�ł����� 0�ȊO�B
 */
int toml_get_key(TomlBuffer * buffer, size_t point, size_t * next_point);

/**
 * " �ň͂܂ꂽ��������擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param start			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @return				�擾�ł����� 0�ȊO�B
 */
int toml_get_string_value(TomlBuffer * buffer, size_t start, size_t * next_point);

///**
// * ���l�i�����^�����^���t�j���擾����B
// *
// * @param buffer		�ǂݍ��ݗ̈�B
// * @param start			�J�n�ʒu�B
// * @param next_point	�I���ʒu�i�߂�l�j
// * @param tokenType		�擾�f�[�^�B
// * @return				�擾�ł����� 0�ȊO�B
// */
//int toml_get_number_value(TomlBuffer * buffer, size_t point, size_t * next_point, TomlValueType * tokenType);

int toml_convert_value(TomlBuffer * buffer, size_t point, size_t * next_point, TomlValueType * tokenType, TomlResult * value_res);

#endif /* __TOML_ANALISYS_H__ */
