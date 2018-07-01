#ifndef __TOML_H__
#define __TOML_H__

#include "../helper/Hash.h"
#include "../helper/Vec.h"

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
typedef enum _TomlResult
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

} TomlResult;

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
	// 1. �^�U�l
	// 2. �����l
	// 3. �����l
	// 4. ������
	// 5. �e�[�u��
	// 6. �e�[�u���z��
	union {
		int					boolean;		// 1
		long long int		integer;		// 2
		double				float_number;	// 3
		const char *		string;			// 4
		TomlTable *			table;			// 5
		TomlTableArray *	tbl_array;		// 6
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
///**
// * Toml�h�L�������g�Ƃ��� 1�s�ǂݍ��ށB
// *
// * @param document		Toml�h�L�������g�B
// * @param input			�Ǎ�������B
// * @param input_len		�Ǎ������񒷁B
// * @return				�Ǎ����ʁB
// */
//TomlReadState toml_readline(TomlDocument * document, const char * input, size_t input_len);
//
///**
// * Toml�h�L�������g�̓ǂݎc����ǂݍ��ށB
// *
// * @param document		Toml�h�L�������g�B
// * @param input			�Ǎ�������B
// * @param input_len		�Ǎ������񒷁B
// * @return				�Ǎ����ʁB
// */
//TomlReadState toml_appendline(TomlDocument * document, const char * input, size_t input_len);

TomlResult toml_read(TomlDocument * document, const char * path);

void toml_show(TomlDocument * document);

#endif /*  __TOML_H__ */