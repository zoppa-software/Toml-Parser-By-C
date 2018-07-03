#ifndef __TOML_H__
#define __TOML_H__

#include "../helper/Hash.h"
#include "../helper/Vec.h"

/** UTF8�̕������B */
#define UTF8_CHARCTOR_SIZE	6

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
	char	ch[UTF8_CHARCTOR_SIZE];

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
 * �L�[�^�l�\���́B
 */
typedef struct _TomlPair
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
		TomlDate *			date;			// ���t
	} value;

} TomlPair;

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

void toml_show(TomlDocument * document);

#endif /*  __TOML_H__ */