#ifndef __TOML_H__
#define __TOML_H__

/**
 * TOML�t�@�C���p�[�T�[�B
 *
 * #include "toml/Toml.h"
 * #include "helper/Encoding.h"
 *
 * TomlDocument * toml = toml_initialize();		// �@�\����
 * TomlValue	v;
 * char buf[128];
 * 
 * toml_read(toml, "test.toml");				// �t�@�C���ǂݍ���
 *
 * // �L�[���w�肵�Ēl���擾���܂��iTOML�t�@�C���� UTF8�Ȃ̂�s-jis�ϊ����ĕ�������o�͂��Ă��܂��j
 * v = toml_search_key(toml->table, "title");
 * printf_s("[ title = %s ]\n", encoding_utf8_to_cp932(v.value.string, buf, sizeof(buf)));
 *
 * toml_dispose(&toml);							// �@�\���
 *
 * Copyright (c) 2018 Takashi Zota
 * Released under the MIT license
 * https://opensource.org/licenses/mit-license.php
 */

#include "../helper/Hash.h"
#include "../helper/Vec.h"

/** UTF8�̕������B */
#define UTF8_CHARCTER_SIZE	6

/**
 * �l�̎�ނ�\���񋓁B
 */
typedef enum _TomlValueType
{
	// ��l
	TomlNoneValue,

	// �^�U�l
	TomlBooleanValue,

	// ������
	TomlStringValue,

	// �����l
	TomlIntegerValue,

	// �����l
	TomlFloatValue,

	// ���t
	TomlDateValue,

	// �z��
	TomlArrayValue,

	// �e�[�u��
	TomlTableValue,

	// �e�[�u���z��
	TomlTableArrayValue,

} TomlValueType;

/**
 * �Ǎ����ʗ񋓑�
 */
typedef enum _TomlResultCode
{
	// ����
	SUCCESS,

	// ���s
	FAIL,

	// �t�@�C���I�[�v���Ɏ��s
	FAIL_OPEN_ERR,

	// �L�[������̉�͂Ɏ��s
	KEY_ANALISYS_ERR,

	// �l�̉�͂Ɏ��s
	KEY_VALUE_ERR,

	// �e�[�u���������G���[
	TABLE_NAME_ANALISYS_ERR,

	// �e�[�u�����Ē�`�G���[
	TABLE_REDEFINITION_ERR,

	// �e�[�u���\���G���[
	TABLE_SYNTAX_ERR,

	// �e�[�u���z��\���G���[
	TABLE_ARRAY_SYNTAX_ERR,

	// �����N�H�[�e�[�V�����������`�G���[
	MULTI_QUOAT_STRING_ERR,

	// �������e�����������`�G���[
	MULTI_LITERAL_STRING_ERR,

	// �������`�G���[
	QUOAT_STRING_ERR,

	// ���e�����������`�G���[
	LITERAL_STRING_ERR,

	// �����l�͈̔̓G���[
	INTEGER_VALUE_RANGE_ERR,

	// ���l��`�ɘA�����ăA���_�[�o�[���g�p���ꂽ
	UNDERBAR_CONTINUE_ERR,

	// ���l�̐擪�ɖ����� 0������
	ZERO_NUMBER_ERR,

	// �L�[���Ē�`���ꂽ
	DEFINED_KEY_ERR,

	// �����Ȗ��̂��w�肳�ꂽ
	INVALID_NAME_ERR,

	// �����̏����_����`���ꂽ
	MULTI_DECIMAL_ERR,

	// �����̒�`�G���[
	DOUBLE_VALUE_ERR,

	// �����l�͈̔̓G���[
	DOUBLE_VALUE_RANGE_ERR,

	// ���t�����̐ݒ�G���[
	DATE_FORMAT_ERR,

	// ���j�R�[�h������`�G���[
	UNICODE_DEFINE_ERR,

	// �����ȃG�X�P�[�v�������w�肳�ꂽ
	INVALID_ESCAPE_CHAR_ERR,

	// �C�����C���e�[�u���������������Ă��Ȃ�
	INLINE_TABLE_NOT_CLOSE_ERR,

	// �z�񂪐����������Ă��Ȃ�
	ARRAY_NOT_CLOSE_ERR,

	// ��̃J���}������
	EMPTY_COMMA_ERR,

	// �z����̒l�̌^���قȂ�
	ARRAY_VALUE_DIFFERENT_ERR,

	// �����_�̑O�ɐ��l���Ȃ�
	NO_LEADING_ZERO_ERR,

	// �����_�̌�ɐ��l���Ȃ�
	NO_LAST_ZERO_ERR,

} TomlResultCode;

/**
 * ���t�`�����B
 */
typedef struct _TomlDate
{
	unsigned short		year;			// �N
	unsigned char		month;			// ��
	unsigned char		day;			// ��
	unsigned char		hour;			// ��
	unsigned char		minute;			// ��
	unsigned char		second;			// �b
	unsigned int		dec_second;		// �b�̏�����
	char				zone_hour;		// �����i���ԁj
	unsigned char		zone_minute;	// �����i���j

} TomlDate;

/**
 * UTF8�����B
 */
typedef union _TomlUtf8
{
	// �����o�C�g
	char	ch[UTF8_CHARCTER_SIZE];

	// ���l
	unsigned int	num;

} TomlUtf8;

/**
 * �ǂݍ��݌��ʏڍׁB
 */
typedef struct _TomlResultSummary
{
	TomlResultCode	code;	// �ǂݍ��݌��ʃR�[�h
	Vec *	utf8s;			// UTF8������
	Vec *	word_dst;		// ������o�b�t�@
	size_t	column;			// ��ʒu
	size_t	row;			// �s�ʒu

} TomlResultSummary;

/**
 * �z��B
 */
typedef struct _TomlArray
{
	// �z��
	Vec *			array;

} TomlArray;

/**
 * �e�[�u���B
 */
typedef struct _TomlTable
{
	// �L�[�^�l�n�b�V��
	const Hash *	hash;

} TomlTable;

/**
 * �e�[�u���z��B
 */
typedef struct _TomlTableArray
{
	// �e�[�u��
	Vec *			tables;

} TomlTableArray;

/**
 * �l�ƒl�̌^�̍\���́B
 */
typedef struct _TomlValue
{
	// �l�̎��
	TomlValueType		value_type;

	// �l�Q��
	union {
		int					boolean;		// �^�U�l
		long long int		integer;		// �����l
		double				float_number;	// �����l
		const char *		string;			// ������
		TomlTable *			table;			// �e�[�u��
		TomlTableArray *	tbl_array;		// �e�[�u���z��
		TomlArray *			array;			// �z��
		TomlDate *			date;			// ���t
	} value;

} TomlValue;

/**
 * �L�[�ƒl�̃y�A���B
 */
typedef struct _TomlBucket
{
	// �L�[
	const char *	key_name;

	// �l
	const TomlValue *	ref_value;

} TomlBucket;

/**
 * �L�[�ƒl�̃y�A�����W���ʁB
 */
typedef struct _TomlBackets
{
	// ���W��
	size_t	length;

	// ���W���ʃ��X�g
	const TomlBucket *	values;

} TomlBuckets;

/**
 * Toml�h�L�������g�B
 */
typedef struct TomlDocument
{
	// �L�[�^�l�n�b�V��
	TomlTable *		table;

} TomlDocument;

//-----------------------------------------------------------------------------
// �R���X�g���N�^�A�f�X�g���N�^
//-----------------------------------------------------------------------------
/**
 * Toml�h�L�������g�𐶐�����B
 *
 * @return	Toml�h�L�������g�B
 */
TomlDocument * toml_initialize();

/**
 * Toml�h�L�������g���������B
 *
 * @param document	������� Toml�h�L�������g�B
 */
void toml_dispose(TomlDocument ** document);

//-----------------------------------------------------------------------------
// �Ǎ�
//-----------------------------------------------------------------------------
/**
 * TOML�`���̃t�@�C����ǂݍ��ށB
 *
 * @param document		Toml�h�L�������g�B
 * @param path			�t�@�C���p�X�B
 * @return				�Ǎ����ʁB
 */
TomlResultCode toml_read(TomlDocument * document, const char * path);

//-----------------------------------------------------------------------------
// �f�[�^�A�N�Z�X
//-----------------------------------------------------------------------------
/**
 * �w��e�[�u���̃L�[�ƒl�̃��X�g���擾����B
 *
 * @param table		�w��e�[�u���B
 * @return			�L�[�ƒl�̃��X�g�B
 */
TomlBuckets toml_collect_key_and_value(TomlTable * table);

/**
 * �w��e�[�u���̃L�[�ƒl�̃��X�g���폜����B
 *
 * @param list		�L�[�ƒl�̃��X�g�B
 */
void toml_delete_key_and_value(TomlBuckets * list);

/**
 * �w�肵���e�[�u������A�L�[�̒l���擾����B
 *
 * @param table		��������e�[�u���B
 * @param key		��������L�[�B
 * @return			�擾�����l�B
 */
TomlValue toml_search_key(TomlTable * table, const char * key);

/**
 * �w�肵���e�[�u���ɁA�w�肵���L�[�����݂��邩�m�F����B
 *
 * @param table		��������e�[�u���B
 * @param key		��������L�[�B
 * @return			���݂����� 0�ȊO��Ԃ��B
 */
int toml_contains_key(TomlTable * table, const char * key);

/**
 * �w�肵���z��̎w��C���f�b�N�X�̒l���擾����B
 *
 * @param array		�z��B
 * @param index		�C���f�b�N�X�B
 * @return			�l�B
 */
TomlValue toml_search_index(TomlArray * array, size_t index);

/**
 * �z��̗v�f�����擾����B
 *
 * @param array		�z��B
 * @return			�v�f���B
 */
size_t toml_get_array_count(TomlArray * array);

/**
 * �w�肵���e�[�u���z��̎w��C���f�b�N�X�̒l���擾����B
 *
 * @param tbl_array		�e�[�u���z��B
 * @param index			�C���f�b�N�X�B
 * @return				�e�[�u���B
 */
TomlTable * toml_search_table_index(TomlTableArray * tbl_array, size_t index);

/**
 * �e�[�u���z��̗v�f�����擾����B
 *
 * @param array		�e�[�u���z��B
 * @return			�v�f���B
 */
size_t toml_get_table_array_count(TomlTableArray * tbl_array);

#endif /*  __TOML_H__ */