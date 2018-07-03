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
	TomlTable *		table;

	// �e�[�u���L���b�V��
	Instance	table_cache;

	// �e�[�u���L���b�V���i����p�j
	Vec *		table_list;

	// �e�[�u���z��L���b�V��
	Instance	tblarray_cache;

	// �e�[�u���z��L���b�V���i����p�j
	Vec *		tblarray_list;

	// �J�����g�e�[�u��
	TomlTable *		root;

	// ������e�[�u��
	const StringHash *	strings_cache;

	// �L�[�^�l�C���X�^���X
	Instance	pair_cache;

	// ���t�C���X�^���X
	Instance	date_cache;

} TomlDocumentImpl;

// �֐��v���g�^�C�v
static TomlResultSummary toml_analisys_key_and_value(TomlDocumentImpl * impl,
													 TomlTable * table,
													 TomlBuffer * buffer,
													 size_t start_point,
													 size_t * next_point);

//-----------------------------------------------------------------------------
// ���̂��擾�A�ǉ�
//-----------------------------------------------------------------------------
/**
 * �e�[�u���L���b�V������e�[�u�����擾����B
 *
 * @param impl	Toml�h�L�������g�B
 * @return		�e�[�u���B
 */
static TomlTable * create_table(TomlDocumentImpl * impl)
{
	TomlTable * res = instance_pop(&impl->table_cache);
	res->hash = hash_initialize(sizeof(const char *));
	vec_add(impl->table_list, &res);
	return res;
}

/**
 * �e�[�u���L���b�V������e�[�u�����擾����B
 *
 * @param impl	Toml�h�L�������g�B
 * @return		�e�[�u���B
 */
static TomlTableArray * toml_create_table_array(TomlDocumentImpl * impl)
{
	TomlTableArray * res = instance_pop(&impl->tblarray_cache);
	res->tables = vec_initialize(sizeof(TomlTable*));
	vec_add(impl->tblarray_list, &res);
	return res;
}

/**
 * �������ێ�����B
 *
 * @param impl		Toml�h�L�������g�B
 * @param buffer	�ǂݍ��݃o�b�t�@�B
 * @return			������B
 */
static char * toml_regist_string(TomlDocumentImpl * impl, TomlBuffer * buffer)
{
	static char empty[] = { 0 };
	if (buffer->word_dst->length > 1) {
		return (char*)stringhash_add(impl->strings_cache, (char*)buffer->word_dst->pointer);
	}
	else {
		return empty;
	}
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

	res->table_cache = instance_initialize(sizeof(TomlTable), TABLE_BLOCK_SIZE);
	res->table_list = vec_initialize_set_capacity(sizeof(TomlTable*), TABLE_BLOCK_SIZE);
	res->table = create_table(res);
	res->root = res->table;

	res->tblarray_cache = instance_initialize(sizeof(TomlTableArray), TABLE_BLOCK_SIZE);
	res->tblarray_list = vec_initialize_set_capacity(sizeof(TomlTableArray*), TABLE_BLOCK_SIZE);

	res->date_cache = instance_initialize(sizeof(TomlDate), TABLE_BLOCK_SIZE);

	res->strings_cache = stringhash_initialize();
	res->pair_cache = instance_initialize(sizeof(TomlPair), TABLE_BLOCK_SIZE);

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
	size_t	i;

	Assert(impl != 0, "null pointer del error");

	// �̈���������
	for (i = 0; i < impl->table_list->length; ++i) {
		TomlTable * tbl = VEC_GET(TomlTable*, impl->table_list, i);
		hash_delete(&tbl->hash);
	}
	vec_delete(&impl->table_list);
	instance_delete(&impl->table_cache);

	for (i = 0; i < impl->tblarray_list->length; ++i) {
		TomlTableArray * tbl = VEC_GET(TomlTableArray*, impl->tblarray_list, i);
		vec_delete(&tbl->tables);
	}
	vec_delete(&impl->tblarray_list);
	instance_delete(&impl->tblarray_cache);

	instance_delete(&impl->date_cache);

	stringhash_delete(&impl->strings_cache);
	instance_delete(&impl->pair_cache);
	
	// �C���X�^���X���������
	free(impl);
	*document = 0;
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
	size_t		i;

	while (!toml_end_of_file(buffer)) {
		// �o�b�t�@�ɕ����񂪕s��������ēǍ�
		if (point >= buffer->utf8s->length) {
			toml_read_buffer(buffer);
		}

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

		case '#':
			// �R�����g���擾������A���s�R�[�h�܂œǂݔ�΂�
			for (i = point + 1; i < buffer->utf8s->length; ++i) {
				if (toml_get_char(buffer->utf8s, i).num == '\n') {
					break;
				}
			}
			point = i + 1;
			break;

		case '\r':
		case '\n':
			// ���s�R�[�h�͓ǂݔ�΂�
			point++;
			break;

		default:
			// �C�����C���e�[�u���������Ă��Ȃ�
			error->code = INLINE_TABLE_NOT_CLOSE_ERR;
			error->column = point + 1;
			error->row = buffer->loaded_line;
			*next_point = point + 1;
			return 0;
		}
	}

	// �C�����C���e�[�u���������Ă��Ȃ�
	error->code = INLINE_TABLE_NOT_CLOSE_ERR;
	error->column = point + 1;
	error->row = buffer->loaded_line;
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
static int toml_get_inline_table(TomlDocumentImpl * impl,
								 TomlBuffer * buffer,
								 size_t point,
								 size_t * next_point,
								 TomlResultSummary * error)
{
	TomlTable *			table = create_table(impl);		// �� todo:�G���[���̓e�[�u����Ԃ��悤�ɂ��邩�i���݂̓G���[�ŏI�����邽�ߖ��Ȃ��j
	size_t				ptr = point;
	TomlResultSummary	res;

	while (ptr < buffer->utf8s->length) {
		// �󔒕��ǂݔ�΂�
		ptr = toml_skip_space(buffer->utf8s, ptr);

		// �L�[�^�l��������荞��
		res = toml_analisys_key_and_value(impl, table, buffer, ptr, next_point);
		if (res.code != SUCCESS) {
			*error = res;
			return 0;
		}

		// �󔒕��ǂݔ�΂�
		ptr = *next_point;
		ptr = toml_skip_space(buffer->utf8s, ptr);

		// �C�����C���e�[�u���������Ă��邩�m�F
		//
		// 1. �e�[�u���������Ă���
		// 2. ���̃L�[�^�l���擾
		// 3. �G���[
		switch (close_inline_table(buffer, ptr, next_point, error)) {
		case 1:							// 1
			buffer->table = table;
			return 1;

		case 2:							// 2
			ptr = *next_point;
			break;

		default:						// 3
			// �G���[�ڍׂ� close_inline_table�Őݒ�ς�
			return 0;
		}
	}

	// �C�����C���e�[�u���������Ă��Ȃ�
	error->code = INLINE_TABLE_NOT_CLOSE_ERR;
	error->column = buffer->utf8s->length;
	error->row = buffer->loaded_line;
	*next_point = buffer->utf8s->length;
	return 0;
}

/**
 * �L�[�^�l�̃y�A���擾����B
 *
 * @param impl			Toml�h�L�������g�B
 * @param table			�L�[�^�l��ێ�����e�[�u���B�B
 * @param buffer		�Ǎ��o�b�t�@�B
 * @param point			�Ǎ��J�n�ʒu�B
 * @param next_point	�Ǎ��I���ʒu�B
 * @return				�Ǎ����ʁB
 */
static TomlResultSummary toml_analisys_key_and_value(TomlDocumentImpl * impl,
													 TomlTable * table,
													 TomlBuffer * buffer,
													 size_t point,
													 size_t * next_point)
{
	size_t				ptr;
	TomlUtf8			c;
	TomlValueType		type;
	TomlPair *			pair;
	const  char *		key_str;
	HashPair			val_pair;
	TomlResultSummary	error;

	// �L�[��������擾����
	//
	// 1. �L�[��������擾
	// 2. �L�[�ȍ~�̋󔒂�ǂݔ�΂�
	// 3. = �ŘA�����Ă��邩�m�F
	// 4. �C�����C���e�[�u������͂���
	// 5. �l���擾����
	if (!toml_get_key(buffer, point, &ptr, &error)) {	// 1
		return error;
	}
	key_str = toml_regist_string(impl, buffer);

	ptr = toml_skip_space(buffer->utf8s, ptr);			// 2

	c = toml_get_char(buffer->utf8s, ptr);				// 3
	if (c.num != '=') {
		error.code = KEY_ANALISYS_ERR;
		error.column = ptr;
		error.row = buffer->loaded_line;
		*next_point = ptr;
		return error;
	}

	ptr = toml_skip_space(buffer->utf8s, ptr + 1);
	if (toml_get_char(buffer->utf8s, ptr).num == '{') {	// 4
		if (toml_get_inline_table(impl, buffer, ptr + 1, next_point, &error)) {
			type = TomlTableValue;
		}
		else {
			return error;
		}
	}
	else if (!toml_convert_value(buffer, ptr, next_point, &type, &error)) {
		return error;									// 5
	}

	// �L�[�^�l�̃y�A���i�[����̈���擾
	pair = instance_pop(&impl->pair_cache);

	// �l��ݒ�
	//
	// 1. �^�U�l
	// 2. ������l
	// 3. �����l
	// 4. �����l
	// 5. ���t
	// 6. �C�����C���e�[�u��
	switch (type) {
	case TomlBooleanValue:		// 1
		pair->value_type = TomlBooleanValue;
		pair->value.boolean = buffer->boolean;
		break;

	case TomlStringValue:		// 2
		pair->value_type = TomlStringValue;
		pair->value.string = toml_regist_string(impl, buffer);
		break;

	case TomlIntegerValue:		// 3
		pair->value_type = TomlIntegerValue;
		pair->value.integer = buffer->integer;
		break;

	case TomlFloatValue:		// 4
		pair->value_type = TomlFloatValue;
		pair->value.float_number = buffer->float_number;
		break;

	case TomlDateValue:			// 5
		pair->value_type = TomlDateValue;
		pair->value.date = (TomlDate*)instance_pop(&impl->date_cache);
		*pair->value.date = buffer->date;
		break;

	case TomlTableValue:		// 6
		pair->value_type = TomlTableValue;
		pair->value.table = buffer->table;
		break;

	default:
		break;
	}
		
	// �e�[�u���ɒǉ�
	if (!hash_contains(table->hash, &key_str, &val_pair)) {
		hash_add(table->hash, &key_str, hash_value_of_pointer(pair));
		error.code = SUCCESS;
		error.column = 0;
		error.row = buffer->loaded_line;
		
	}
	else {
		buffer->err_point = buffer->utf8s->length - 1;
		error.code = DEFINED_KEY_ERR;
		error.column = 0;
		error.row = buffer->loaded_line;
	}
	return error;
}

//-----------------------------------------------------------------------------
// �e�[�u���A�e�[�u���z��̒ǉ�
//-----------------------------------------------------------------------------
/**
 * �p�X�𑖍����āA�e�[�u�����擾����B
 *
 * @param table			�J�����g�e�[�u���B
 * @param key_str		�L�[������B
 * @param answer		�擾�����e�[�u���B
 * @return				0�A1�A2�B
 */
static int search_path_table(TomlTable * table,
							 const char * key_str,
							 TomlTable ** answer)
{
	HashPair		result;
	TomlPair *		pair;

	if (hash_contains(table->hash, &key_str, &result)) {
		// �w��̃L�[���o�^�ς݂Ȃ�΁A�R�t�����ڂ�Ԃ�
		//
		// 1. �e�[�u����Ԃ�
		// 2. �e�[�u���z��̃e�[�u����Ԃ�
		pair = (TomlPair*)result.value.object;
		switch (pair->value_type)
		{
		case TomlTableValue:			// 1
			*answer = pair->value.table;
			return 1;

		case TomlTableArrayValue:		// 2
			*answer = VEC_GET(TomlTable*,
							  pair->value.tbl_array->tables,
							  pair->value.tbl_array->tables->length - 1);
			return 1;

		default:
			return 0;
		}
	}
	else {
		return 2;
	}
}

/**
 * �e�[�u�����쐬����B
 *
 * @param impl			Toml�h�L�������g�B
 * @param table			�J�����g�e�[�u���B
 * @param buffer		�o�b�t�@�̈�B
 * @param start_point	�ǂݍ��݊J�n�ʒu�B
 * @param next_point	�ǂݍ��񂾈ʒu�i�߂�l�j
 * @return				�쐬���ʁB
 */
static TomlResultSummary toml_analisys_table(TomlDocumentImpl * impl,
									  TomlTable * table,
									  TomlBuffer * buffer,
									  size_t start_point,
									  size_t * next_point)
{
	TomlUtf8		c;
	size_t			i, nxt;
	TomlTable *		cur_table = table;
	TomlTable *		new_table;
	TomlPair *		pair;
	const char *	key_str;
	TomlResultSummary error;

	// 1. �󔒓ǂݎ̂�
	// 2. �e�[�u�������擾
	// 3. �i�[�e�[�u�����쐬
	// 4. '.' ������ΊK�w��ǂ�������
	for (i = start_point; i < buffer->utf8s->length; ) {
		i = toml_skip_space(buffer->utf8s, i);			// 1

		if (!toml_get_key(buffer, i, &nxt, &error)) {			// 2
			buffer->err_point = nxt;
			return error;
		}
		i = nxt;

		key_str = toml_regist_string(impl, buffer);		// 3
		switch (search_path_table(cur_table, key_str, &new_table)) {
		case 0:
			error.code = TABLE_REDEFINITION_ERR;
			error.column = 0;
			error.row = buffer->loaded_line;
			*next_point = i;
			return error;
		case 1:
			cur_table = new_table;
			break;
		default:
			new_table = create_table(impl);
			pair = instance_pop(&impl->pair_cache);
			pair->value_type = TomlTableValue;
			pair->value.table = new_table;
			hash_add(cur_table->hash, &key_str, hash_value_of_pointer(pair));
			cur_table = new_table;
			break;
		}

		c = toml_get_char(buffer->utf8s, i);			// 4
		if (c.num != '.') {
			break;
		}
		i++;
	}

	i = toml_skip_space(buffer->utf8s, i);
	*next_point = i;

	impl->table = cur_table;
	
	error.code = SUCCESS;
	error.column = 0;
	error.row = buffer->loaded_line;
	return error;
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
static TomlResultSummary toml_analisys_table_array(TomlDocumentImpl * impl,
												TomlTable * table,
												TomlBuffer * buffer,
												size_t start_point,
												size_t * next_point)
{
	TomlUtf8		c;
	size_t			i, j, nxt;
	TomlTable *		cur_table = table;
	TomlTable *		new_table;
	TomlPair *		pair;
	const char *	key_str;
	TomlTableArray* new_array;
	HashPair		result;
	TomlResultSummary error;

	vec_clear(buffer->key_ptr);

	// 1. �󔒓ǂݎ̂�
	// 2. �e�[�u�������擾
	// 3. �i�[�e�[�u�����쐬
	// 4. '.' ������ΊK�w��ǂ�������
	for (i = start_point; i < buffer->utf8s->length; ) {
		i = toml_skip_space(buffer->utf8s, i);			// 1

		if (!toml_get_key(buffer, i, &nxt, &error)) {			// 2
			buffer->err_point = nxt;
			return error;
		}
		i = nxt;

		key_str = toml_regist_string(impl, buffer);		// 3
		vec_add(buffer->key_ptr, (char*)&key_str);

		c = toml_get_char(buffer->utf8s, i);			// 4
		if (c.num != '.') {
			break;
		}
		i++;
	}

	for (j = 0; j < buffer->key_ptr->length - 1; ++j) {
		key_str = VEC_GET(const char*, buffer->key_ptr, j);
		switch (search_path_table(cur_table, key_str, &new_table)) {
		case 0:
			error.code = TABLE_REDEFINITION_ERR;
			error.column = 0;
			error.row = buffer->loaded_line;
			*next_point = i;
			return error;

		case 1:
			cur_table = new_table;
			break;

		default:
			new_table = create_table(impl);
			pair = instance_pop(&impl->pair_cache);
			pair->value_type = TomlTableValue;
			pair->value.table = new_table;
			hash_add(cur_table->hash, &key_str, hash_value_of_pointer(pair));
			cur_table = new_table;
			break;
		}
	}

	key_str = VEC_GET(const char*, buffer->key_ptr, buffer->key_ptr->length - 1);
	if (hash_contains(table->hash, &key_str, &result)) {
		pair = (TomlPair*)result.value.object;
		if (pair->value_type == TomlTableArrayValue) {
			new_table = create_table(impl);
			vec_add(pair->value.tbl_array->tables, &new_table);
		}
		else {
			error.code = TABLE_REDEFINITION_ERR;
			error.column = 0;
			error.row = buffer->loaded_line;
			*next_point = i;
			return error;
		}
	}
	else {
		new_array = toml_create_table_array(impl);
		pair = instance_pop(&impl->pair_cache);
		pair->value_type = TomlTableArrayValue;
		pair->value.tbl_array = new_array;
		hash_add(cur_table->hash, &key_str, hash_value_of_pointer(pair));

		new_table = create_table(impl);
		vec_add(new_array->tables, &new_table);
	}
	
	cur_table = new_table;

	i = toml_skip_space(buffer->utf8s, i);
	*next_point = i;

	impl->table = cur_table;

	error.code = SUCCESS;
	error.column = 0;
	error.row = buffer->loaded_line;
	return error;
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
TomlResultCode toml_appendline(TomlDocumentImpl * impl, TomlBuffer * buffer)
{
	size_t		i;
	size_t		rng_end;
	TomlLineType	tkn_type;
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
		res = toml_analisys_key_and_value(impl, impl->table, buffer, i, &rng_end);
		if (res.code == SUCCESS && !toml_skip_linefield(buffer->utf8s, rng_end)) {
			buffer->err_point = rng_end;
			res.code = KEY_VALUE_ERR;
		}
		break;

	case TomlTableLine:				// 3
		res = toml_analisys_table(impl, impl->root, buffer, i, &rng_end);
		if (res.code == SUCCESS &&
			!toml_close_table(buffer->utf8s, rng_end)) {
			buffer->err_point = rng_end;
			res.code = TABLE_SYNTAX_ERR;
		}
		break;

	case TomlTableArrayLine:			// 4
		res = toml_analisys_table_array(impl, impl->root, buffer, i, &rng_end);
		if (res.code == SUCCESS &&
			!toml_close_table_array(buffer->utf8s, rng_end)) {
			buffer->err_point = rng_end;
			res.code = TABLE_ARRAY_SYNTAX_ERR;
		}
		break;

	default:
		// �����
		break;
	}

	return res.code;
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
	TomlResultCode rescode;

	if (buffer == 0) {
		fprintf(stderr, "�t�@�C���I�[�v���G���[\n");
		return FAIL_OPEN_ERR;
	}

	do {
		rescode = toml_appendline(impl, buffer);
		if (rescode != SUCCESS) {
			const char * errmsg = toml_err_message(buffer->utf8s, buffer->word_dst);

			switch (rescode) {
			case FAIL_OPEN_ERR:
				fprintf(stderr, "�t�@�C���I�[�v���G���[\n");
				goto EXIT_ANALISYS;

			case KEY_ANALISYS_ERR:
				fprintf(stderr, "�L�[������̉�͂Ɏ��s : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case KEY_VALUE_ERR:
				fprintf(stderr, "�l�̉�͂Ɏ��s : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case TABLE_NAME_ANALISYS_ERR:
				fprintf(stderr, "�e�[�u���������Ɏ��s : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case TABLE_REDEFINITION_ERR:
				fprintf(stderr, "�e�[�u�������Ē�`���Ă��܂� : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case TABLE_SYNTAX_ERR:
				fprintf(stderr, "�e�[�u���\���̉�͂Ɏ��s : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case TABLE_ARRAY_SYNTAX_ERR:
				fprintf(stderr, "�e�[�u���z��\���̉�͂Ɏ��s : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case MULTI_QUOAT_STRING_ERR:
				fprintf(stderr, "�����N�H�[�e�[�V�����������`�G���[ : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case MULTI_LITERAL_STRING_ERR:
				fprintf(stderr, "�������e�����������`�G���[ : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case QUOAT_STRING_ERR:
				fprintf(stderr, "�������`�G���[ : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case LITERAL_STRING_ERR:
				fprintf(stderr, "���e�����������`�G���[ : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case INTEGER_VALUE_RANGE_ERR:
				fprintf(stderr, "�����l���L���͈͂𒴂��Ă��� : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case UNDERBAR_CONTINUE_ERR:
				fprintf(stderr, "���l��`�ɘA�����ăA���_�[�o�[���g�p���ꂽ : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case ZERO_NUMBER_ERR:
				fprintf(stderr, "���l�̐擪�ɖ����� 0������ : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case DEFINED_KEY_ERR:
				fprintf(stderr, "�L�[���Ē�`���ꂽ : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case MULTI_DECIMAL_ERR:
				fprintf(stderr, "�����̏����_����`���ꂽ : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case DOUBLE_VALUE_ERR:
				fprintf(stderr, "�����̕\�����͈͊O : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case DATE_FORMAT_ERR:
				fprintf(stderr, "���t����͂ł��Ȃ� : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case UNICODE_DEFINE_ERR:
				fprintf(stderr, "���j�R�[�h������`�̉�͂Ɏ��s : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case INVALID_ESCAPE_CHAR_ERR:
				fprintf(stderr, "�����ȃG�X�P�[�v�������w�肳�ꂽ : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case INLINE_TABLE_NOT_CLOSE_ERR:
				fprintf(stderr, "�C�����C���e�[�u���������������Ă��Ȃ� : %s\n", errmsg);
				goto EXIT_ANALISYS;
			}
		}

		vec_clear(buffer->utf8s);
		vec_clear(buffer->key_ptr);
	} while (!toml_end_of_file(buffer));

EXIT_ANALISYS:;
	toml_close_buffer(buffer);

	return SUCCESS;
}

//-----------------------------------------------------------------------------
// �f�o�b�O�o��
//-----------------------------------------------------------------------------
static void toml_show_table(TomlTable * table);

static void toml_show_table_pair(HashPair pair)
{
	TomlPair * obj = (TomlPair*)pair.value.object;
	size_t	i;
	TomlDate * date;

	switch (obj->value_type)
	{
	case TomlBooleanValue:
		printf_s("%s: %s\n", *(char**)pair.key, obj->value.boolean ? "true" : "false");
		break;
	case TomlStringValue:
		printf_s("%s: %s\n", *(char**)pair.key, obj->value.string);
		break;
	case TomlIntegerValue:
		printf_s("%s: %lld\n", *(char**)pair.key, obj->value.integer);
		break;
	case TomlFloatValue:
		printf_s("%s: %g\n", *(char**)pair.key, obj->value.float_number);
		break;
	case TomlDateValue:
		date = obj->value.date;
		printf_s("%s: %d-%d-%d %d:%d:%d.%d %d:%d\n",
				 *(char**)pair.key,
				 date->year, date->month, date->day,
				 date->hour, date->minute, date->second, date->dec_second,
				 date->zone_hour, date->zone_minute);
		break;
	case TomlTableValue:
		printf_s("%s: {\n", *(char**)pair.key);
		toml_show_table(obj->value.table);
		printf_s("}\n");
		break;
	case TomlTableArrayValue:
		printf_s("%s: [\n", *(char**)pair.key);
		for (i = 0; i < obj->value.tbl_array->tables->length; ++i) {
			printf_s("{\n");
			toml_show_table(VEC_GET(TomlTable*, obj->value.tbl_array->tables, i));
			printf_s("}\n");
		}
		printf_s("]\n");
		break;
	default:
		break;
	}
}

static void toml_show_table(TomlTable * table)
{
	hash_show(table->hash, toml_show_table_pair);
}

void toml_show(TomlDocument * document)
{
	TomlDocumentImpl * impl = (TomlDocumentImpl*)document;
	toml_show_table(impl->root);
}
