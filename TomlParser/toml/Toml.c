#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crtdbg.h>
#include "../helper/Assertion.h"
#include "../helper/StringHash.h"
#include "../helper/Vec.h"
#include "../helper/Instance.h"
#include "Toml.h"
#include "TomlParseHelper.h"
#include "TomlBuffer.h"
#include "TomlParseString.h"
#include "TomlParseValue.h"

//-----------------------------------------------------------------------------
// �萔
//-----------------------------------------------------------------------------
/** �e�[�u���쐬�u���b�N���B */
#define TABLE_BLOCK_SIZE	8

//-----------------------------------------------------------------------------
// �\����
//-----------------------------------------------------------------------------
/**
 * Toml�h�L�������g�����B
 */
typedef struct TomlDocumentImpl
{
	// �L�[�^�l�n�b�V��
	TomlTable *		root;

	// �e�[�u���L���b�V��
	Instance	table_cache;

	// �e�[�u���z��L���b�V��
	Instance	tblarray_cache;

	// �z��L���b�V��
	Instance	array_cache;

	// �J�����g�e�[�u��
	TomlTable *		table;

	// ������e�[�u��
	const StringHash *	strings_cache;

	// �l�C���X�^���X
	Instance	value_cache;

	// ���t�C���X�^���X
	Instance	date_cache;

} TomlDocumentImpl;

/**
 * �e�[�u���B
 */
typedef struct _TomlTableImpl
{
	// �L�[�^�l�n�b�V��
	TomlTable table;

	// ��`�ς݂Ȃ�� 0�ȊO
	int		defined;

	// �Q�ƃ|�C���^
	TomlDocumentImpl * document;

} TomlTableImpl;

// �֐��v���g�^�C�v
static TomlResultSummary analisys_key_and_value(TomlDocumentImpl * impl,
												TomlTableImpl * table,
												TomlBuffer * buffer,
												size_t point,
												size_t * next_point,
												int last_nochk);

static int analisys_value(TomlDocumentImpl * impl,
						  TomlBuffer * buffer,
						  size_t point,
						  size_t * next_point,
						  TomlValue ** res_value,
						  TomlResultSummary * error);

/** ��l�B */
static TomlValue empty_value;

//-----------------------------------------------------------------------------
// ���̂��擾�A�ǉ�
//-----------------------------------------------------------------------------
/**
 * �e�[�u���L���b�V������e�[�u�����擾����B
 *
 * @param impl	Toml�h�L�������g�B
 * @return		�e�[�u���B
 */
static TomlTableImpl * create_table(TomlDocumentImpl * impl)
{
	TomlTableImpl * res = instance_pop(&impl->table_cache);
	res->table.hash = hash_initialize(sizeof(const char *));
	res->defined = 0;
	res->document = impl;
	return res;
}

/**
 * �e�[�u���̃f�X�g���N�^�A�N�V�����B
 *
 * @param target	�ΏۃC���X�^���X�B
 */
void delete_table_action(void * target)
{
	TomlTableImpl * table = (TomlTableImpl*)target;
	hash_delete(&table->table.hash);
	table->defined = 0;
	table->document = 0;
}

/**
 * �e�[�u���z��L���b�V������e�[�u���z����擾����B
 *
 * @param impl	Toml�h�L�������g�B
 * @return		�e�[�u���z��B
 */
static TomlTableArray * create_table_array(TomlDocumentImpl * impl)
{
	TomlTableArray * res = instance_pop(&impl->tblarray_cache);
	res->tables = vec_initialize(sizeof(TomlTable*));
	return res;
}

/**
 * �e�[�u���z��̃f�X�g���N�^�A�N�V�����B
 *
 * @param target	�ΏۃC���X�^���X�B
 */
static void delete_tablearray_action(void * target)
{
	TomlTableArray * table = (TomlTableArray*)target;
	vec_delete(&table->tables);
}

/**
 * �z��L���b�V������z����擾����B
 *
 * @param impl	Toml�h�L�������g�B
 * @return		�z��B
 */
static TomlArray * create_array(TomlDocumentImpl * impl)
{
	TomlArray * res = instance_pop(&impl->array_cache);
	res->array = vec_initialize(sizeof(TomlValue*));
	return res;
}

/**
 * �z��̃f�X�g���N�^�A�N�V�����B
 *
 * @param target	�ΏۃC���X�^���X�B
 */
static void delete_array_action(void * target)
{
	TomlArray * array = (TomlArray*)target;
	vec_delete(&array->array);
}

/**
 * �z����폜����B
 *
 * @param impl		Toml�h�L�������g�B
 * @param array		�폜����z��B
 */
static void delete_array(TomlDocumentImpl * impl, TomlArray * array)
{
	size_t		i;
	for (i = 0; i < array->array->length; ++i) {
		instance_push(&impl->value_cache, VEC_GET(TomlValue*, array->array, i));
	}
	instance_push_destructor(&impl->array_cache, array, delete_array_action);
}

//-----------------------------------------------------------------------------
// �R���X�g���N�^�A�f�X�g���N�^
//-----------------------------------------------------------------------------
/**
 * Toml�h�L�������g�𐶐�����B
 *
 * @return	Toml�h�L�������g�B
 */
TomlDocument * toml_initialize()
{
	// �C���X�^���X�쐬
	TomlDocumentImpl * res = (TomlDocumentImpl*)malloc(sizeof(TomlDocumentImpl));
	Assert(res != 0, "alloc error");
	_RPTN(_CRT_WARN, "toml_initialize malloc 0x%X\n", res);

	// �̈�𐶐�
	memset(res, 0, sizeof(TomlDocumentImpl));

	res->table_cache = instance_initialize(sizeof(TomlTableImpl), TABLE_BLOCK_SIZE);
	res->table = (TomlTable*)create_table(res);
	res->root = res->table;
	res->tblarray_cache = instance_initialize(sizeof(TomlTableArray), TABLE_BLOCK_SIZE);
	res->array_cache = instance_initialize(sizeof(TomlArray), TABLE_BLOCK_SIZE);
	res->date_cache = instance_initialize(sizeof(TomlDate), TABLE_BLOCK_SIZE);
	res->strings_cache = stringhash_initialize();
	res->value_cache = instance_initialize(sizeof(TomlValue), TABLE_BLOCK_SIZE);

	return (TomlDocument*)res;
}

/**
 * Toml�h�L�������g���������B
 *
 * @param document	������� Toml�h�L�������g�B
 */
void toml_dispose(TomlDocument ** document)
{
	TomlDocumentImpl * impl = (TomlDocumentImpl*)*document;

	Assert(impl != 0, "null pointer del error");

	// �̈���������
	instance_delete_all_destructor(&impl->table_cache, delete_table_action);
	instance_delete_all_destructor(&impl->tblarray_cache, delete_tablearray_action);
	instance_delete_all_destructor(&impl->array_cache, delete_array_action);
	instance_delete(&impl->date_cache);
	stringhash_delete(&impl->strings_cache);
	instance_delete(&impl->value_cache);
	
	// �C���X�^���X���������
	free(impl);
	*document = 0;
}

//-----------------------------------------------------------------------------
// �L�[���
//-----------------------------------------------------------------------------
/**
 * �������ێ�����B
 *
 * @param impl		Toml�h�L�������g�B
 * @param buffer	�ǂݍ��݃o�b�t�@�B
 * @return			������B
 */
static char * regist_string(TomlDocumentImpl * impl, TomlBuffer * buffer)
{
	static char empty[] = { 0 };
	if (buffer->word_dst->length > 1) {
		return (char*)stringhash_add(impl->strings_cache, (char*)buffer->word_dst->pointer);
	}
	else {
		return empty;
	}
}

/**
 * �L�[������̎擾�B
 *
 * @param impl			Toml�h�L�������g�B
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param key_ptr		�L�[�����񃊃X�g�B
 * @param point			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param error			�G���[�ڍ׏��i�߂�l�j
 * @return				�擾�ł����� 0�ȊO�B
 */
static int get_keys(TomlDocumentImpl * impl,
					TomlBuffer * buffer,
					Vec * key_ptr,
					size_t point,
					size_t * next_point,
					TomlResultSummary * error)
{
	size_t			i;
	TomlUtf8		c;
	const char *	key_str;

	vec_clear(key_ptr);

	for (i = point; i < buffer->utf8s->length; ) {
		i = toml_skip_space(buffer->utf8s, i);				// 1

		if (!toml_get_key(buffer, i, next_point, error)) {	// 2
			return 0;
		}
		i = *next_point;

		key_str = regist_string(impl, buffer);				// 3
		vec_add(key_ptr, (char*)&key_str);

		c = toml_get_char(buffer->utf8s, i);				// 4
		if (c.num != '.') {
			break;
		}
		i++;
	}
	return 1;
}

/**
 * �p�X�𑖍����āA�e�[�u�����擾����B
 *
 * @param table			�J�����g�e�[�u���B
 * @param key_str		�L�[������B
 * @param answer		�擾�����e�[�u���B
 * @return				0�̓G���[�A1�͍쐬�ς݂̃e�[�u���A2�͐V�K�쐬
 */
static int search_path_table(TomlTableImpl * table,
							 const char * key_str,
							 TomlTableImpl ** answer)
{
	HashPair		result;
	TomlValue *		value;

	if (hash_contains(table->table.hash, &key_str, &result)) {
		// �w��̃L�[���o�^�ς݂Ȃ�΁A�R�t�����ڂ�Ԃ�
		//
		// 1. �e�[�u����Ԃ�
		// 2. �e�[�u���z��̃e�[�u����Ԃ�
		// 3. �e�[�u���̐V�K�쐬���˗�
		value = (TomlValue*)result.value.object;
		switch (value->value_type)
		{
		case TomlTableValue:			// 1
			*answer = (TomlTableImpl*)value->value.table;
			return 1;

		case TomlTableArrayValue:		// 2
			*answer = VEC_GET(TomlTableImpl*,
							  value->value.tbl_array->tables,
							  value->value.tbl_array->tables->length - 1);
			return 1;

		default:
			return 0;
		}
	}
	else {
		return 2;						// 3
	}
}

//-----------------------------------------------------------------------------
// �L�[�^�l�A�C�����C���e�[�u���A�z��̒ǉ�
//-----------------------------------------------------------------------------
/**
 * �C�����C���e�[�u���������Ă��邱�Ƃ��m�F����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param point			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param error			�G���[�ڍ׏��B
 * @return				�����Ă����� 1�A���̃L�[�擾�Ȃ�� 2�A�G���[�� 0�B
 */
static int close_inline_table(TomlBuffer * buffer,
							  size_t point,
							  size_t * next_point,
							  TomlResultSummary * error)
{
	TomlUtf8	c;

	while (point < buffer->utf8s->length || !toml_end_of_file(buffer)) {
		// ���s�A�󔒕�����菜��
		point = toml_skip_linefield_and_space(buffer, point);

		// �ꕶ���擾
		c = toml_get_char(buffer->utf8s, point);
		
		// ��������
		switch (c.num) {
		case '}':
			// �C�����C���e�[�u��������ꂽ
			*next_point = point + 1;
			return 1;

		case ',':
			// �����Ď��̃L�[�^�l�����
			*next_point = point + 1;
			return 2;

		default:
			// �C�����C���e�[�u���������Ă��Ȃ�
			goto EXIT_WHILE;
		}
	}

EXIT_WHILE:
	// �C�����C���e�[�u���������Ă��Ȃ�
	*error = toml_res_ctor(INLINE_TABLE_NOT_CLOSE_ERR, point + 1, buffer->loaded_line);
	*next_point = point + 1;
	return 0;
}

/**
 * �C�����C���e�[�u������͂���B
 *
 * @param impl			Toml�h�L�������g�B
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param point			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param error			�G���[�ڍ׏��B
 * @return				����ɓǂݍ��߂��Ȃ�� 0�ȊO�B
 */
static int get_inline_table(TomlDocumentImpl * impl,
							TomlBuffer * buffer,
							size_t point,
							size_t * next_point,
							TomlResultSummary * error)
{
	TomlTableImpl *		table = create_table(impl);
	size_t				ptr = point;
	TomlResultSummary	res;

	while (ptr < buffer->utf8s->length) {
		// ���s�A�󔒕�����菜��
		ptr = toml_skip_linefield_and_space(buffer, ptr);

		// �L�[�^�l��������荞��
		res = analisys_key_and_value(impl, table, buffer, ptr, next_point, 1);
		if (res.code != SUCCESS) {
			instance_push_destructor(&impl->table_cache, table, delete_table_action);
			*error = res;
			return 0;
		}

		// �C�����C���e�[�u���������Ă��邩�m�F
		//
		// 1. �e�[�u���������Ă���
		// 2. ���̃L�[�^�l���擾
		// 3. �G���[
		ptr = *next_point;
		switch (close_inline_table(buffer, ptr, next_point, error)) {
		case 1:							// 1
			buffer->table = (TomlTable*)table;
			return 1;

		case 2:							// 2
			ptr = *next_point;
			break;

		default:						// 3
			// �G���[�ڍׂ� close_inline_table�Őݒ�ς�
			instance_push_destructor(&impl->table_cache, table, delete_table_action);
			return 0;
		}
	}

	// �C�����C���e�[�u���������Ă��Ȃ�
	instance_push_destructor(&impl->table_cache, table, delete_table_action);
	*error = toml_res_ctor(INLINE_TABLE_NOT_CLOSE_ERR, buffer->utf8s->length, buffer->loaded_line);
	*next_point = buffer->utf8s->length;
	return 0;
}

/**
 * �z�񂪕����Ă��邱�Ƃ��m�F����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param point			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param error			�G���[�ڍ׏��B
 * @return				�����Ă����� 1�A���̃L�[�擾�Ȃ�� 2�A�G���[�� 0�B
 */
static int close_value_array(TomlBuffer * buffer,
							 size_t point,
							 size_t * next_point,
							 TomlResultSummary * error)
{
	TomlUtf8	c;

	while (point < buffer->utf8s->length || !toml_end_of_file(buffer)) {
		// ���s�A�󔒕�����菜��
		point = toml_skip_linefield_and_space(buffer, point);

		// �ꕶ���擾
		c = toml_get_char(buffer->utf8s, point);
		
		// ��������
		switch (c.num) {
		case ']':
			// �z�񂪕���ꂽ
			*next_point = point + 1;
			return 1;

		case ',':
			// �����Ď��̃L�[�^�l�����
			*next_point = point + 1;	
			return 2;

		default:
			// �z�񂪕����Ă��Ȃ�
			goto EXIT_WHILE;
		}
	}

EXIT_WHILE:
	// �z�񂪕����Ă��Ȃ�
	*error = toml_res_ctor(ARRAY_NOT_CLOSE_ERR, point + 1, buffer->loaded_line);
	*next_point = point + 1;
	return 0;
}

/**
 * �z����̒l�̌^���S�Ĉ�v���邩���肷��B
 *
 * @param array		���肷��z��Q�ƁB
 * @return			�G���[��������� 0�ȊO�B
 */
static int check_array_value_type(TomlArray * array)
{
	size_t			i;
	TomlValueType	type;

	if (array->array->length > 0) {
		type = VEC_GET(TomlValue*, array->array, 0)->value_type;

		for (i = 1; i < array->array->length; ++i) {
			if (type != VEC_GET(TomlValue*, array->array, i)->value_type) {
				return 0;
			}
		}
	}
	return 1;
}

/**
 * �z�����͂���B
 *
 * @param impl			Toml�h�L�������g�B
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param point			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param error			�G���[�ڍ׏��B
 * @return				����ɓǂݍ��߂��Ȃ�� 0�ȊO�B
 */
static int get_value_array(TomlDocumentImpl * impl,
						   TomlBuffer * buffer,
						   size_t point,
						   size_t * next_point,
						   TomlResultSummary * error)
{
	TomlArray *	array = create_array(impl);
	size_t		ptr = point;
	TomlValue *	value;

	while (ptr < buffer->utf8s->length) {
		// ���s�A�󔒕�����菜��
		ptr = toml_skip_linefield_and_space(buffer, ptr);

		// �l����荞��
		if (!analisys_value(impl, buffer, ptr, next_point, &value, error)) {
			delete_array(impl, array);
			return 0;
		}

		// ��l�ȊO�͎�荞��
		if (value->value_type != TomlNoneValue) {
			vec_add(array->array, &value);
		}

		// �z�񂪕����Ă��邩�m�F
		//
		// 1. �e�[�u���������Ă���
		// 2. ���̃L�[�^�l���擾
		// 3. �G���[
		ptr = *next_point;
		switch (close_value_array(buffer, ptr, next_point, error)) {
		case 1:							// 1
			if (check_array_value_type(array)) {
				buffer->array = array;
				return 1;
			}
			else {
				delete_array(impl, array);
				*error = toml_res_ctor(ARRAY_VALUE_DIFFERENT_ERR, *next_point, buffer->loaded_line);
				return 0;
			}

		case 2:							// 2
			if (value->value_type != TomlNoneValue) {
				ptr = *next_point;
				break;
			}
			else {
				delete_array(impl, array);
				*error = toml_res_ctor(EMPTY_COMMA_ERR, *next_point, buffer->loaded_line);
				return 0;
			}

		default:						// 3
			// �G���[�ڍׂ� close_value_array�Őݒ�ς�
			delete_array(impl, array);
			return 0;
		}
	}

	// �z�񂪕����Ă��Ȃ�
	delete_array(impl, array);
	*error = toml_res_ctor(ARRAY_NOT_CLOSE_ERR, buffer->utf8s->length, buffer->loaded_line);
	*next_point = buffer->utf8s->length;
	return 0;
}

/**
 * �l�̉�͂��s���B
 *
 * @param impl			Toml�h�L�������g�B
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param point			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param res_value		�擾�����l�i�߂�l�j
 * @param error			�G���[�ڍ׏��B
 * @return				����ɓǂݍ��߂��Ȃ�� 0�ȊO�B
 */
static int analisys_value(TomlDocumentImpl * impl,
						  TomlBuffer * buffer,
						  size_t point,
						  size_t * next_point,
						  TomlValue ** res_value,
						  TomlResultSummary * error)
{
	TomlValueType		type;
	TomlValue *			value;
	size_t				ptr;

	// �C�����C���e�[�u���A�z��A���t�̊m�F
	//
	// 1. �C�����C���e�[�u������͂���
	// 2. �z�����͂���
	// 3. �l���擾����
	ptr = toml_skip_space(buffer->utf8s, point);
	if (toml_get_char(buffer->utf8s, ptr).num == '{') {			// 1
		if (get_inline_table(impl, buffer, ptr + 1, next_point, error)) {
			type = TomlTableValue;
		}
		else {
			return 0;
		}
	}
	else if (toml_get_char(buffer->utf8s, ptr).num == '[') {	// 2
		if (get_value_array(impl, buffer, ptr + 1, next_point, error)) {
			type = TomlArrayValue;
		}
		else {
			return 0;
		}
	}
	else if (!toml_convert_value(buffer, ptr, next_point, &type, error)) {
		return 0;												// 3
	}

	// �L�[�^�l�̃y�A���i�[����̈���擾
	value = instance_pop(&impl->value_cache);

	// �l��ݒ�
	switch (type) {
	case TomlBooleanValue:			// �^�U�l
		value->value_type = TomlBooleanValue;
		value->value.boolean = buffer->boolean;
		break;

	case TomlStringValue:			// ������l
		value->value_type = TomlStringValue;
		value->value.string = regist_string(impl, buffer);
		break;

	case TomlIntegerValue:			// �����l
		value->value_type = TomlIntegerValue;
		value->value.integer = buffer->integer;
		break;

	case TomlFloatValue:			// �����l
		value->value_type = TomlFloatValue;
		value->value.float_number = buffer->float_number;
		break;

	case TomlDateValue:				// ���t
		value->value_type = TomlDateValue;
		value->value.date = (TomlDate*)instance_pop(&impl->date_cache);
		*value->value.date = buffer->date;
		break;

	case TomlTableValue:			// �C�����C���e�[�u��
		value->value_type = TomlTableValue;
		value->value.table = buffer->table;
		break;

	case TomlArrayValue:			// �z��
		value->value_type = TomlArrayValue;
		value->value.array = buffer->array;
		break;

	default:						// �����l
		value->value_type = TomlNoneValue;
		break;
	}

	*res_value = value;
	*error = toml_res_ctor(SUCCESS, 0, buffer->loaded_line);
	return 1;
}

/**
 * �L�[�^�l�̃y�A���擾����B
 *
 * @param impl			Toml�h�L�������g�B
 * @param table			�L�[�^�l��ێ�����e�[�u���B�B
 * @param buffer		�Ǎ��o�b�t�@�B
 * @param point			�Ǎ��J�n�ʒu�B
 * @param next_point	�Ǎ��I���ʒu�B
 * @param last_nochk	�l�̏I���͈̓`�F�b�N�����Ȃ��Ȃ� 0�ȊO
 * @return				�Ǎ����ʁB
 */
static TomlResultSummary analisys_key_and_value(TomlDocumentImpl * impl,
												TomlTableImpl * table,
												TomlBuffer * buffer,
												size_t point,
												size_t * next_point,
												int last_nochk)
{
	size_t				j, ptr;
	TomlUtf8			c;
	TomlValue *			value;
	TomlValue *			tbl_val;
	Vec *				key_ptr;
	const  char *		key_str;
	HashPair			val_pair;
	TomlResultSummary	res;
	TomlTableImpl *		cur_table;
	TomlTableImpl *		new_table;

	// �L�[��������擾����
	//
	// 1. �L�[��������擾
	// 2. �L�[�ȍ~�̋󔒂�ǂݔ�΂�
	// 3. = �ŘA�����Ă��邩�m�F
	key_ptr = vec_initialize(sizeof(char*));
	if (!get_keys(impl, buffer, key_ptr, point, &ptr, &res)) {	// 1
		goto EXIT_KEY_AND_VALUE;
	}

	ptr = toml_skip_space(buffer->utf8s, ptr);				// 2

	c = toml_get_char(buffer->utf8s, ptr);					// 3
	if (c.num != '=') {
		res = toml_res_ctor(KEY_ANALISYS_ERR, ptr, buffer->loaded_line);
		*next_point = ptr;
		goto EXIT_KEY_AND_VALUE;
	}

	// �l���擾����
	//
	// 1. �l���擾�ł��邩
	// 2. �����l�ł��邩
	if (!analisys_value(impl, buffer,			// 1
						ptr + 1, next_point, &value, &res)) {
		goto EXIT_KEY_AND_VALUE;
	}
	if (value->value_type == TomlNoneValue) {	// 2
		res = toml_res_ctor(KEY_VALUE_ERR, ptr + 1, buffer->loaded_line);
		goto EXIT_KEY_AND_VALUE;
	}

	// ���s�܂Ŋm�F
	if (!last_nochk && !toml_skip_line_end(buffer->utf8s, *next_point)) {
		res = toml_res_ctor(KEY_VALUE_ERR, *next_point, buffer->loaded_line);
		goto EXIT_KEY_AND_VALUE;
	}

	// '.' �Ŏw�肳�ꂽ�e�[�u���Q�Ƃ����W����
	//
	// 1. �e�[�u���Q�Ƃ��擾
	// 2. �G���[���L��ΏI��
	// 3. ���ɍ쐬�ς݂Ȃ�΃J�����g��ύX
	// 4. �쐬����Ă��Ȃ���΃e�[�u�����쐬���A�J�����g�ɐݒ�
	cur_table = table;
	for (j = 0; j < key_ptr->length - 1; ++j) {
		key_str = VEC_GET(const char*, key_ptr, j);

		switch (search_path_table(cur_table, key_str, &new_table)) {	// 1
		case 0:															// 2
			res = toml_res_ctor(TABLE_REDEFINITION_ERR, 0, buffer->loaded_line);
			*next_point = ptr;
			goto EXIT_KEY_AND_VALUE;
		case 1:
			cur_table = new_table;										// 3
			break;
		default:
			new_table = create_table(impl);								// 4
			tbl_val = instance_pop(&impl->value_cache);
			tbl_val->value_type = TomlTableValue;
			tbl_val->value.table = (TomlTable*)new_table;
			hash_add(cur_table->table.hash,
					 &key_str, hash_value_of_pointer(tbl_val));
			cur_table = new_table;
			break;
		}
	}

	// �ŏI�̃e�[�u���ɒl�����蓖�Ă�
	key_str = VEC_GET(const char*, key_ptr, key_ptr->length - 1);
	if (!hash_contains(cur_table->table.hash, &key_str, &val_pair)) {
		hash_add(cur_table->table.hash, &key_str, hash_value_of_pointer(value));
		res = toml_res_ctor(SUCCESS, 0, buffer->loaded_line);
	}
	else {
		res = toml_res_ctor(DEFINED_KEY_ERR, 0, buffer->loaded_line);
	}
EXIT_KEY_AND_VALUE:
	vec_delete(&key_ptr);
	return res;
}

//-----------------------------------------------------------------------------
// �e�[�u���A�e�[�u���z��̒ǉ�
//-----------------------------------------------------------------------------
/**
 * �e�[�u�����쐬����B
 *
 * @param impl			Toml�h�L�������g�B
 * @param table			�J�����g�e�[�u���B
 * @param buffer		�o�b�t�@�̈�B
 * @param point			�ǂݍ��݊J�n�ʒu�B
 * @param next_point	�ǂݍ��񂾈ʒu�i�߂�l�j
 * @return				�쐬���ʁB
 */
static TomlResultSummary analisys_table(TomlDocumentImpl * impl,
										TomlTableImpl * table,
										TomlBuffer * buffer,
										size_t point,
										size_t * next_point)
{
	size_t				j, nxt;
	TomlTableImpl *		cur_table = table;
	TomlTableImpl *		new_table;
	TomlValue *			value;
	Vec *				key_ptr;
	const char *		key_str;
	TomlResultSummary	res;

	// '.' �ŋ�؂�ꂽ�e�[�u���������O�Ɏ��W����
	key_ptr = vec_initialize(sizeof(char*));
	if (!get_keys(impl, buffer, key_ptr, point, &nxt, &res)) {
		goto EXIT_TABLE;
	}

	// �e�[�u���������Ă��邩�m�F
	if (!toml_close_table(buffer->utf8s, nxt)) {
		res = toml_res_ctor(TABLE_SYNTAX_ERR, 0, buffer->loaded_line);
		*next_point = nxt;
		goto EXIT_TABLE;
	}

	// �e�[�u�����쐬����
	//
	// 1. �e�[�u���Q�Ƃ��擾
	// 2. �G���[���L��ΏI��
	// 3. ���ɍ쐬�ς݂Ȃ�΃J�����g��ύX
	// 4. �쐬����Ă��Ȃ���΃e�[�u�����쐬���A�J�����g�ɐݒ�
	for (j = 0; j < key_ptr->length; ++j) {
		key_str = VEC_GET(const char*, key_ptr, j);

		switch (search_path_table(cur_table, key_str, &new_table)) {	// 1
		case 0:															// 2
			res = toml_res_ctor(TABLE_REDEFINITION_ERR, 0, buffer->loaded_line);
			*next_point = nxt;
			goto EXIT_TABLE;
		case 1:
			cur_table = new_table;										// 3
			break;
		default:
			new_table = create_table(impl);								// 4
			value = instance_pop(&impl->value_cache);
			value->value_type = TomlTableValue;
			value->value.table = (TomlTable*)new_table;
			hash_add(cur_table->table.hash,
					 &key_str, hash_value_of_pointer(value));
			cur_table = new_table;
			break;
		}
	}

	// �󔒂͓ǂݎ̂ĂĂ���
	*next_point = toml_skip_space(buffer->utf8s, nxt);

	// �J�����g�̃e�[�u����ݒ�
	impl->table = (TomlTable*)cur_table;
	if (!cur_table->defined) {
		cur_table->defined = 1;
		res = toml_res_ctor(SUCCESS, 0, buffer->loaded_line);
	}
	else {
		res = toml_res_ctor(DEFINED_KEY_ERR, 0, buffer->loaded_line);
	}
EXIT_TABLE:
	vec_delete(&key_ptr);
	return res;
}

/**
 * �e�[�u���z����쐬����B
 *
 * @param impl			Toml�h�L�������g�B
 * @param table			�J�����g�e�[�u���B
 * @param buffer		�o�b�t�@�̈�B
 * @param start_point	�ǂݍ��݊J�n�ʒu�B
 * @param next_point	�ǂݍ��񂾈ʒu�i�߂�l�j
 * @return				�쐬���ʁB
 */
static TomlResultSummary analisys_table_array(TomlDocumentImpl * impl,
											  TomlTableImpl * table,
											  TomlBuffer * buffer,
											  size_t point,
											  size_t * next_point)
{
	size_t				j, nxt;
	TomlTableImpl *		cur_table = table;
	TomlTableImpl *		new_table;
	TomlValue *			value;
	Vec *				key_ptr = 0;
	const char *		key_str;
	TomlTableArray *	new_array;
	HashPair			hash_pair;
	TomlResultSummary	res;

	// '.' �ŋ�؂�ꂽ�e�[�u���������O�Ɏ��W����
	key_ptr = vec_initialize(sizeof(char*));
	if (!get_keys(impl, buffer, key_ptr, point, &nxt, &res)) {
		goto EXIT_TABLE_ARRAY;
	}

	// �e�[�u���z�񂪕����Ă��邩�m�F
	if (!toml_close_table_array(buffer->utf8s, nxt)) {
		res = toml_res_ctor(TABLE_ARRAY_SYNTAX_ERR, 0, buffer->loaded_line);
		*next_point = nxt;
		goto EXIT_TABLE_ARRAY;
	}

	// �ŉ��w�̃e�[�u���ȊO�̃e�[�u���Q�Ƃ����W����
	//
	// 1. �e�[�u���Q�Ƃ��擾
	// 2. �G���[���L��ΏI��
	// 3. ���ɍ쐬�ς݂Ȃ�΃J�����g��ύX
	// 4. �쐬����Ă��Ȃ���΃e�[�u�����쐬���A�J�����g�ɐݒ�
	for (j = 0; j < key_ptr->length - 1; ++j) {
		key_str = VEC_GET(const char*, key_ptr, j);

		switch (search_path_table(cur_table, key_str, &new_table)) {	// 1
		case 0:															// 2
			res = toml_res_ctor(TABLE_REDEFINITION_ERR, 0, buffer->loaded_line);
			*next_point = nxt;
			goto EXIT_TABLE_ARRAY;

		case 1:
			cur_table = new_table;										// 3
			break;

		default:
			new_table = create_table(impl);								// 4
			value = instance_pop(&impl->value_cache);
			value->value_type = TomlTableValue;
			value->value.table = (TomlTable*)new_table;
			hash_add(cur_table->table.hash,
					 &key_str, hash_value_of_pointer(value));
			cur_table = new_table;
			break;
		}
	}

	// �ŉ��w�̃e�[�u���͐V�K�쐬�ƂȂ�
	//
	// 1. �o�^���閼�O�i�L�[������j���擾
	// 2. �e�̃e�[�u���ɍŉ��w�̃e�[�u�������o�^����Ă���
	//    2-1. �o�^����Ă��閼�O�̃f�[�^���擾����
	//    2-2. �e�[�u���z�񂪓o�^����Ă���Ȃ�΁A�V�K�e�[�u�����쐬���A�J�����g�e�[�u���Ƃ���
	//    2-3. �e�[�u���z��łȂ��Ȃ�΃G���[�Ƃ���
	// 3. �e�̃e�[�u���ɍŉ��w�̃e�[�u�������o�^����Ă��Ȃ�
	//    3-1. �e�[�u���z����쐬
	//    3-2. �e�[�u���z��Ƀe�[�u����ǉ��A�ǉ����ꂽ�e�[�u�������̃J�����g�e�[�u���ɂȂ�
	key_str = VEC_GET(const char*, key_ptr, key_ptr->length - 1);		// 1

	if (hash_contains(cur_table->table.hash, &key_str, &hash_pair)) {	// 2-1
		value = (TomlValue*)hash_pair.value.object;
		if (value->value_type == TomlTableArrayValue) {					// 2-2
			new_table = create_table(impl);
			vec_add(value->value.tbl_array->tables, &new_table);
		}
		else {															// 2-3
			res = toml_res_ctor(TABLE_REDEFINITION_ERR, 0, buffer->loaded_line);
			*next_point = nxt;
			goto EXIT_TABLE_ARRAY;
		}
	}
	else {
		new_array = create_table_array(impl);							// 3-1
		value = instance_pop(&impl->value_cache);
		value->value_type = TomlTableArrayValue;
		value->value.tbl_array = new_array;
		hash_add(cur_table->table.hash,
				 &key_str, hash_value_of_pointer(value));

		new_table = create_table(impl);									// 3-2
		vec_add(new_array->tables, &new_table);
	}
	
	// �J�����g�̃e�[�u�����쐬�����e�[�u���ɕύX
	impl->table = (TomlTable*)new_table;

	// �󔒓ǂݔ�΂�
	*next_point = toml_skip_space(buffer->utf8s, nxt);

	// �߂�l���쐬���ĕԂ�
	res = toml_res_ctor(SUCCESS, 0, buffer->loaded_line);

EXIT_TABLE_ARRAY:
	vec_delete(&key_ptr);
	return res;
}

//-----------------------------------------------------------------------------
// �g�[�N�����
//-----------------------------------------------------------------------------
/**
 * Toml�h�L�������g�̓ǂݎc����ǂݍ��ށB
 *
 * @param impl			Toml�h�L�������g�B
 * @param input			�Ǎ�������B
 * @param input_len		�Ǎ������񒷁B
 * @return				�Ǎ����ʁB
 */
TomlResultSummary append_line(TomlDocumentImpl * impl, TomlBuffer * buffer)
{
	size_t				i;
	size_t				rng_end;
	TomlLineType		tkn_type;
	TomlResultSummary	res;

	memset(&res, 0, sizeof(res));

	// ��s�̃f�[�^���擾����
	toml_read_buffer(buffer);

	// �s�̃f�[�^�J�n�𔻒肷��
	i = toml_skip_space(buffer->utf8s, 0);
	i = toml_check_start_line_token(buffer->utf8s, i, &tkn_type);

	// �s�̃f�[�^��ނɂ���āA�e�[�u���^�L�[�^�R�����g�̔�����s��
	//
	// 1. �R�����g
	// 2. �L�[������
	// 3. �e�[�u��
	// 4. �e�[�u���z��
	switch (tkn_type)
	{
	case TomlCommenntLine:				// 1
		break;

	case TomlKeyValueLine:				// 2
		res = analisys_key_and_value(impl, (TomlTableImpl*)impl->table, buffer, i, &rng_end, 0);
		break;

	case TomlTableLine:					// 3
		res = analisys_table(impl, (TomlTableImpl*)impl->root, buffer, i, &rng_end);
		break;

	case TomlTableArrayLine:			// 4
		res = analisys_table_array(impl, (TomlTableImpl*)impl->root, buffer, i, &rng_end);
		break;

	default:
		// �����
		break;
	}

	res.utf8s = buffer->utf8s;
	res.word_dst = buffer->word_dst;
	return res;
}

/**
 * �G���[�����o�͂���B
 *
 * @param rescode	�G���[���B
 */
static void show_message(TomlResultSummary rescode)
{
	const char * errmsg = 0;
	const char * errline = toml_err_message(rescode.utf8s, rescode.word_dst);

	switch (rescode.code) {
	case FAIL_OPEN_ERR:
		errmsg = "�t�@�C���I�[�v���G���[";
		break;
	case KEY_ANALISYS_ERR:
		errmsg = "�L�[������̉�͂Ɏ��s";
		break;
	case KEY_VALUE_ERR:
		errmsg = "�l�̉�͂Ɏ��s";
		break;
	case TABLE_NAME_ANALISYS_ERR:
		errmsg = "�e�[�u���������Ɏ��s";
		break;
	case TABLE_REDEFINITION_ERR:
		errmsg = "�e�[�u�������Ē�`���Ă��܂�";
		break;
	case TABLE_SYNTAX_ERR:
		errmsg = "�e�[�u���\���̉�͂Ɏ��s";
		break;
	case TABLE_ARRAY_SYNTAX_ERR:
		errmsg = "�e�[�u���z��\���̉�͂Ɏ��s";
		break;
	case MULTI_QUOAT_STRING_ERR:
		errmsg = "�����N�H�[�e�[�V�����������`�G���[";
		break;
	case MULTI_LITERAL_STRING_ERR:
		errmsg = "�������e�����������`�G���[";
		break;
	case QUOAT_STRING_ERR:
		errmsg = "�������`�G���[";
		break;
	case LITERAL_STRING_ERR:
		errmsg = "���e�����������`�G���[";
		break;
	case INTEGER_VALUE_RANGE_ERR:
		errmsg = "�����l���L���͈͂𒴂��Ă���";
		break;
	case UNDERBAR_CONTINUE_ERR:
		errmsg = "���l��`�ɘA�����ăA���_�[�o�[���g�p���ꂽ";
		break;
	case ZERO_NUMBER_ERR:
		errmsg = "���l�̐擪�ɖ����� 0������";
		break;
	case DEFINED_KEY_ERR:
		errmsg = "�L�[���Ē�`���ꂽ";
		break;
	case MULTI_DECIMAL_ERR:
		errmsg = "�����̏����_����`���ꂽ";
		break;
	case DOUBLE_VALUE_ERR:
		errmsg = "�����̕\�����͈͊O";
		break;
	case DATE_FORMAT_ERR:
		errmsg = "���t����͂ł��Ȃ�";
		break;
	case UNICODE_DEFINE_ERR:
		errmsg = "���j�R�[�h������`�̉�͂Ɏ��s";
		break;
	case INVALID_ESCAPE_CHAR_ERR:
		errmsg = "�����ȃG�X�P�[�v�������w�肳�ꂽ";
		break;
	case INLINE_TABLE_NOT_CLOSE_ERR:
		errmsg = "�C�����C���e�[�u���������������Ă��Ȃ�";
		break;
	case ARRAY_NOT_CLOSE_ERR:
		errmsg = "�z�񂪐����������Ă��Ȃ�";
		break;
	case EMPTY_COMMA_ERR:
		errmsg = "�J���}�̑O�ɒl����`����Ă��Ȃ�";
		break;
	case ARRAY_VALUE_DIFFERENT_ERR:
		errmsg = "�z��̒l�̌^���قȂ�";
		break;
	case NO_LEADING_ZERO_ERR:
		errmsg = "�����_�̑O�ɐ��l�����͂���Ă��Ȃ�";
		break;
	case NO_LAST_ZERO_ERR:
		errmsg = "�����_�̌�ɐ��l�����͂���Ă��Ȃ�";
		break;	
	}

	fprintf(stderr, "%s\n", errmsg);
	fprintf(stderr, "�s: %lld  �ʒu: %lld\n", rescode.row, rescode.column);
	fprintf(stderr, "> %s\n", errline);
}

/**
 * TOML�`���̃t�@�C����ǂݍ��ށB
 *
 * @param document		Toml�h�L�������g�B
 * @param path			�t�@�C���p�X�B
 * @return				�Ǎ����ʁB
 */
TomlResultCode toml_read(TomlDocument * document, const char * path)
{
	TomlDocumentImpl * impl = (TomlDocumentImpl*)document;
	TomlBuffer * buffer = toml_create_buffer(path);
	TomlResultSummary rescode;

	if (buffer == 0) {
		fprintf(stderr, "�t�@�C���I�[�v���G���[\n");
		return FAIL_OPEN_ERR;
	}

	do {
		// ��s��荞��
		rescode = append_line(impl, buffer);
		if (rescode.code != SUCCESS) {
			show_message(rescode);
			goto EXIT_ANALISYS;
		}

		// �o�b�t�@����
		vec_clear(buffer->utf8s);
	} while (!toml_end_of_file(buffer));

EXIT_ANALISYS:;
	toml_close_buffer(buffer);

	return rescode.code;
}

//-----------------------------------------------------------------------------
// �f�[�^�A�N�Z�X
//-----------------------------------------------------------------------------
/**
 * �J�ԏ����Ϗ��֐��|�C���^�B
 *
 * @param pair	�\�����鍀�ځB
 * @param param	�p�����[�^�B
 */
void table_for_function(HashPair pair, void * param)
{
	TomlBucket	bucket;
	bucket.key_name = *((char**)pair.key);
	bucket.ref_value = (TomlValue*)pair.value.object;

	vec_add((Vec*)param, &bucket);
}

/**
 * �w��e�[�u���̃L�[�ƒl�̃��X�g���擾����B
 *
 * @param document	Toml�h�L�������g�B
 * @param table		�w��e�[�u���B
 * @return			�L�[�ƒl�̃��X�g�B
 */
TomlBuckets toml_collect_key_and_value(TomlTable * table)
{
	TomlBuckets		res;
	Vec *			vec = vec_initialize_set_capacity(sizeof(TomlBucket), TABLE_BLOCK_SIZE);

	hash_foreach(table->hash, table_for_function, vec);

	res.length = vec->length;
	res.values = (TomlBucket*)malloc(sizeof(TomlBucket) * res.length);
	Assert(res.values != 0, "alloc error");
	_RPTN(_CRT_WARN, "toml_collect_key_and_value malloc 0x%X\n", res);

	memcpy((void *)res.values, vec->pointer, sizeof(TomlBucket) * res.length);
	vec_delete(&vec);
	return res;
}

/**
 * �w��e�[�u���̃L�[�ƒl�̃��X�g���폜����B
 *
 * @param list		�L�[�ƒl�̃��X�g�B
 */
void toml_delete_key_and_value(TomlBuckets * list)
{
	if (list) {
		free((void*)list->values);
		list->values = 0;
	}
}

/**
 * �w�肵���e�[�u������A�L�[�̒l���擾����B
 *
 * @param table		��������e�[�u���B
 * @param key		��������L�[�B
 * @return			�擾�����l�B
 */
TomlValue toml_search_key(TomlTable * table,
						  const char * key)
{
	TomlTableImpl *	tbl_impl = (TomlTableImpl*)table;
	TomlDocumentImpl * impl = (TomlDocumentImpl*)tbl_impl->document;
	const char * reg_key = 0;
	HashPair	result;

	if (*key == 0) {
		// �󔒕�����Ԃ�
		return empty_value;
	}
	else {
		// �L�[�������Ԃ�
		reg_key = stringhash_add(impl->strings_cache, key);

		// �l��Ԃ�
		if (hash_contains(table->hash, &reg_key, &result)) {
			return *((TomlValue*)result.value.object);
		}
		else {
			return empty_value;
		}
	}
}

/**
 * �w�肵���e�[�u���ɁA�w�肵���L�[�����݂��邩�m�F����B
 *
 * @param table		��������e�[�u���B
 * @param key		��������L�[�B
 * @return			���݂����� 0�ȊO��Ԃ��B
 */
int toml_contains_key(TomlTable * table, const char * key)
{
	TomlTableImpl *	tbl_impl = (TomlTableImpl*)table;
	TomlDocumentImpl * impl = (TomlDocumentImpl*)tbl_impl->document;
	const char * reg_key = 0;
	HashPair	result;

	if (*key == 0) {
		return 0;
	}
	else {
		// �L�[�������Ԃ�
		reg_key = stringhash_add(impl->strings_cache, key);

		// ���݂����� 0�ȊO��Ԃ�
		return hash_contains(table->hash, &reg_key, &result);
	}
}

/**
 * �w�肵���z��̎w��C���f�b�N�X�̒l���擾����B
 *
 * @param array		�z��B
 * @param index		�C���f�b�N�X�B
 * @return			�l�B
 */
TomlValue toml_search_index(TomlArray * array, size_t index)
{
	if (array != 0) {
		if (index < array->array->length) {
			return *(VEC_GET(TomlValue*, array->array, index));
		}
		else {
			return empty_value;
		}
	}
	else {
		return empty_value;
	}
}

/**
 * �z��̗v�f�����擾����B
 *
 * @param array		�z��B
 * @return			�v�f���B
 */
size_t toml_get_array_count(TomlArray * array)
{
	return (array != 0 ? array->array->length : 0);
}

/**
 * �w�肵���e�[�u���z��̎w��C���f�b�N�X�̒l���擾����B
 *
 * @param tbl_array		�e�[�u���z��B
 * @param index			�C���f�b�N�X�B
 * @return				�e�[�u���B
 */
TomlTable * toml_search_table_index(TomlTableArray * tbl_array, size_t index)
{
	if (tbl_array != 0) {
		if (index < tbl_array->tables->length) {
			return (VEC_GET(TomlTable*, tbl_array->tables, index));
		}
		else {
			return 0;
		}
	}
	else {
		return 0;
	}
}

/**
 * �e�[�u���z��̗v�f�����擾����B
 *
 * @param array		�e�[�u���z��B
 * @return			�v�f���B
 */
size_t toml_get_table_array_count(TomlTableArray * tbl_array)
{
	return (tbl_array != 0 ? tbl_array->tables->length : 0);
}
