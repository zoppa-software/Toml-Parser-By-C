#ifndef __TOML_PARSE_HELPER_H__
#define __TOML_PARSE_HELPER_H__

/**
 * ���������p�̃w�b�_�A�w���p�֐��B
 *
 * Copyright (c) 2018 Takashi Zota
 * Released under the MIT license
 * https://opensource.org/licenses/mit-license.php
 */

#include "Toml.h"

/**
 * �s�̃f�[�^��ށB
 */
typedef enum _TomlLineType
{
	// ��s�B
	TomlLineNone,

	// �R�����g�s�B
	TomlCommenntLine,

	// �e�[�u���s�B
	TomlTableLine,

	// �e�[�u���z��s�B
	TomlTableArrayLine,

	// �L�[�^�l�s�B
	TomlKeyValueLine,

} TomlLineType;

/**
 * �w��ʒu�̕������擾����B
 *
 * @param utf8s		������B
 * @param point		�w��ʒu�B
 * @return			�擾���������B
 */
TomlUtf8 toml_get_char(Vec * utf8s, size_t point);

/**
 * �󔒕�����ǂݔ�΂��B
 *
 * @param impl	toml�h�L�������g�B
 * @param i		�ǂݍ��݈ʒu�B
 * @return		�ǂݔ�΂����ʒu�B
 */
size_t toml_skip_space(Vec * utf8s, size_t i);

/**
 * �G���[���b�Z�[�W���擾����B
 *
 * @param utf8s		UTF8������B
 * @param word_dst	�ϊ��o�b�t�@�B
 * @preturn			�G���[���b�Z�[�W�B
 */
const char * toml_err_message(Vec *	utf8s, Vec * word_dst);

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
size_t toml_check_start_line_token(Vec * utf8s, size_t i, TomlLineType * tokenType);

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

#endif