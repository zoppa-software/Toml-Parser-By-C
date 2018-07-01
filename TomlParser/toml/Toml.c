#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crtdbg.h>
#include "../helper/Assertion.h"
#include "../helper/StringHash.h"
#include "../helper/Vec.h"
#include "../helper/Instance.h"
#include "Toml.h"
#include "TomlBuffer.h"
#include "TomlAnalisys.h"

//-----------------------------------------------------------------------------
// �萔
//-----------------------------------------------------------------------------
/** �e�[�u���쐬�u���b�N���B */
#define TABLE_BLOCK_SIZE	8

//-----------------------------------------------------------------------------
// �񋓑�
//-----------------------------------------------------------------------------
/**
 * �ǂݍ��݃g�[�N����ʁB
 */
typedef enum TokenType
{
	// �g�[�N������
	NoneToken,

	// �G���[�g�[�N��
	ErrorToken,

	// �R�����g�g�[�N��
	CommenntToken,

	// �e�[�u���g�[�N��
	TableToken,

	// �e�[�u���z��g�[�N��
	TableArrayToken,

	// �L�[�^�l�g�[�N��
	KeyValueToken,

} TokenType;

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

} TomlDocumentImpl;

//-----------------------------------------------------------------------------
// ���̂��擾�A�ǉ�
//-----------------------------------------------------------------------------
/**
 * �e�[�u���L���b�V������e�[�u�����擾����B
 *
 * @param impl	Toml�h�L�������g�B
 * @return		�e�[�u���B
 */
static TomlTable * toml_create_table(TomlDocumentImpl * impl)
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
	return (char*)stringhash_add(impl->strings_cache, (char*)buffer->word_dst->pointer);
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
	res->table = toml_create_table(res);
	res->root = res->table;

	res->tblarray_cache = instance_initialize(sizeof(TomlTableArray), TABLE_BLOCK_SIZE);
	res->tblarray_list = vec_initialize_set_capacity(sizeof(TomlTableArray*), TABLE_BLOCK_SIZE);

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

	stringhash_delete(&impl->strings_cache);
	instance_delete(&impl->pair_cache);
	
	// �C���X�^���X���������
	free(impl);
	*document = 0;
}

//-----------------------------------------------------------------------------
// �L�[�^�l�A�e�[�u���A�e�[�u���z��̒ǉ�
//-----------------------------------------------------------------------------
/**
 * �s�̊J�n�𔻒肷��B
 *
 * @param utf8s		�Ώە�����B
 * @param i			�ǂݍ��݈ʒu�B
 * @param tokenType	�s�̃f�[�^��ށB
 * @return			�ǂݔ�΂����ʒu�B
 */
static size_t toml_check_start_line_token(Vec * utf8s, size_t i, TokenType * tokenType)
{
	TomlUtf8	c1, c2;

	// �e�[�u���z��̊J�n�����肷��
	if (utf8s->length - i >= 2) {
		c1 = toml_get_char(utf8s, i);
		c2 = toml_get_char(utf8s, i + 1);
		if (c1.num == '[' && c2.num == '[') {
			*tokenType = TableArrayToken;
			return i + 2;
		}
	}

	// ���̑��̊J�n�𔻒肷��
	//
	// 1. �e�[�u���̊J�n�����肷��
	// 2. �R�����g�̊J�n�����肷��
	// 4. �L�[�̊J�n�����肷��
	// 5. ����ȊO�̓G���[
	if (utf8s->length - i > 0) {
		c1 = toml_get_char(utf8s, i);
		switch (c1.num)
		{
		case '[':			// 1
			*tokenType = TableToken;
			return i + 1;

		case '#':			// 2
			*tokenType = CommenntToken;
			return i + 1;

		case '\r':			// 3
		case '\n':
		case '\0':
			*tokenType = NoneToken;
			return i;

		default:			// 4
			*tokenType = KeyValueToken;
			return i;
		}
	}
	else {					// 5
		*tokenType = NoneToken;
		return i;
	}
}

/**
 * �L�[�^�l�̃y�A���擾����B
 *
 * @param impl			Toml�h�L�������g�B
 * @param table			�L�[�^�l��ێ�����e�[�u���B�B
 * @param buffer		�Ǎ��o�b�t�@�B
 * @param start_point	�Ǎ��J�n�ʒu�B
 * @param next_point	�Ǎ��I���ʒu�B
 * @return				�Ǎ����ʁB
 */
static TomlResult toml_analisys_key_and_value(TomlDocumentImpl * impl,
											  TomlTable * table,
											  TomlBuffer * buffer,
											  size_t start_point,
											  size_t * next_point)
{
	size_t			ptr;
	TomlUtf8		c;
	TomlValueType	type;
	TomlPair *		pair;
	const  char *	key_str;
	TomlResult		value_res;
	HashPair		val_pair;

	// �L�[��������擾����
	//
	// 1. �L�[��������擾
	// 2. �L�[�ȍ~�̋󔒂�ǂݔ�΂�
	// 3. = �ŘA�����Ă��邩�m�F
	// 4. �l���擾����
	if (!toml_get_key(buffer, start_point, &ptr)) {	// 1
		buffer->err_point = ptr;
		return KEY_ANALISYS_ERR;
	}
	key_str = toml_regist_string(impl, buffer);

	ptr = toml_skip_space(buffer->utf8s, ptr);		// 2

	c = toml_get_char(buffer->utf8s, ptr);			// 3
	if (c.num != '=') {
		buffer->err_point = ptr;
		return KEY_ANALISYS_ERR;
	}

	ptr = toml_skip_space(buffer->utf8s, ptr + 1);
	if (!toml_convert_value(buffer, ptr,			// 4
							next_point, &type, &value_res)) {
		return value_res;
	}

	// �L�[�^�l�̃y�A���i�[����̈���擾
	pair = instance_pop(&impl->pair_cache);

	// �l��ݒ�
	//
	// 1. �^�U�l
	// 2. ������l
	// 3. �����l
	// 4. �����l
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

	default:
		break;
	}
		
	// �e�[�u���ɒǉ�
	if (!hash_contains(table->hash, &key_str, &val_pair)) {
		hash_add(table->hash, &key_str, hash_value_of_pointer(pair));
		return SUCCESS;
	}
	else {
		buffer->err_point = buffer->utf8s->length - 1;
		return DEFINED_KEY_ERR;
	}
}

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
static TomlResult toml_analisys_table(TomlDocumentImpl * impl,
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

	// 1. �󔒓ǂݎ̂�
	// 2. �e�[�u�������擾
	// 3. �i�[�e�[�u�����쐬
	// 4. '.' ������ΊK�w��ǂ�������
	for (i = start_point; i < buffer->utf8s->length; ) {
		i = toml_skip_space(buffer->utf8s, i);			// 1

		if (!toml_get_key(buffer, i, &nxt)) {			// 2
			buffer->err_point = nxt;
			return TABLE_NAME_ANALISYS_ERR;
		}
		i = nxt;

		key_str = toml_regist_string(impl, buffer);		// 3
		switch (search_path_table(cur_table, key_str, &new_table)) {
		case 0:
			return TABLE_REDEFINITION_ERR;
		case 1:
			cur_table = new_table;
			break;
		default:
			new_table = toml_create_table(impl);
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

	return SUCCESS;
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
static TomlResult toml_analisys_table_array(TomlDocumentImpl * impl,
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

	vec_clear(buffer->key_ptr);

	// 1. �󔒓ǂݎ̂�
	// 2. �e�[�u�������擾
	// 3. �i�[�e�[�u�����쐬
	// 4. '.' ������ΊK�w��ǂ�������
	for (i = start_point; i < buffer->utf8s->length; ) {
		i = toml_skip_space(buffer->utf8s, i);			// 1

		if (!toml_get_key(buffer, i, &nxt)) {			// 2
			buffer->err_point = nxt;
			return TABLE_NAME_ANALISYS_ERR;
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
			return TABLE_REDEFINITION_ERR;
		case 1:
			cur_table = new_table;
			break;
		default:
			new_table = toml_create_table(impl);
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
			new_table = toml_create_table(impl);
			vec_add(pair->value.tbl_array->tables, &new_table);
		}
		else {
			return TABLE_REDEFINITION_ERR;
		}
	}
	else {
		new_array = toml_create_table_array(impl);
		pair = instance_pop(&impl->pair_cache);
		pair->value_type = TomlTableArrayValue;
		pair->value.tbl_array = new_array;
		hash_add(cur_table->hash, &key_str, hash_value_of_pointer(pair));

		new_table = toml_create_table(impl);
		vec_add(new_array->tables, &new_table);
	}
	
	cur_table = new_table;

	i = toml_skip_space(buffer->utf8s, i);
	*next_point = i;

	impl->table = cur_table;

	return SUCCESS;
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
TomlResult toml_appendline(TomlDocumentImpl * impl, TomlBuffer * buffer)
{
	size_t		i;
	size_t		rng_end;
	TokenType	tkn_type;
	TomlResult	res = SUCCESS;

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
	case CommenntToken:				// 1
		break;

	case KeyValueToken:				// 2
		res = toml_analisys_key_and_value(impl, impl->table, buffer, i, &rng_end);
		if (res == SUCCESS && !toml_skip_linefield(buffer->utf8s, rng_end)) {
			buffer->err_point = rng_end;
			res = KEY_VALUE_ERR;
		}
		break;

	case TableToken:				// 3
		res = toml_analisys_table(impl, impl->root, buffer, i, &rng_end);
		if (res == SUCCESS &&
			!toml_close_table(buffer->utf8s, rng_end)) {
			buffer->err_point = rng_end;
			res = TABLE_SYNTAX_ERR;
		}
		break;

	case TableArrayToken:			// 4
		res = toml_analisys_table_array(impl, impl->root, buffer, i, &rng_end);
		if (res == SUCCESS &&
			!toml_close_table_array(buffer->utf8s, rng_end)) {
			buffer->err_point = rng_end;
			res = TABLE_ARRAY_SYNTAX_ERR;
		}
		break;

	default:
		// �����
		break;
	}

	return res;
}

TomlResult toml_read(TomlDocument * document, const char * path)
{
	TomlDocumentImpl * impl = (TomlDocumentImpl*)document;
	TomlBuffer * buffer = toml_create_buffer(path);

	if (buffer == 0) {
		fprintf(stderr, "�t�@�C���I�[�v���G���[\n");
		return FAIL_OPEN_ERR;
	}

	do {
		switch (toml_appendline(impl, buffer)) {
		case FAIL_OPEN_ERR:
			fprintf(stderr, "�t�@�C���I�[�v���G���[\n");
			goto EXIT_ANALISYS;

		case KEY_ANALISYS_ERR:
			fprintf(stderr, "�L�[������̉�͂Ɏ��s : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case KEY_VALUE_ERR:
			fprintf(stderr, "�l�̉�͂Ɏ��s : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case TABLE_NAME_ANALISYS_ERR:
			fprintf(stderr, "�e�[�u���������Ɏ��s : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case TABLE_REDEFINITION_ERR:
			fprintf(stderr, "�e�[�u�������Ē�`���Ă��܂� : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case TABLE_SYNTAX_ERR:
			fprintf(stderr, "�e�[�u���\���̉�͂Ɏ��s : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case TABLE_ARRAY_SYNTAX_ERR:
			fprintf(stderr, "�e�[�u���z��\���̉�͂Ɏ��s : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case MULTI_QUOAT_STRING_ERR:
			fprintf(stderr, "�����N�H�[�e�[�V�����������`�G���[ : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case MULTI_LITERAL_STRING_ERR:
			fprintf(stderr, "�������e�����������`�G���[ : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case QUOAT_STRING_ERR:
			fprintf(stderr, "�������`�G���[ : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case LITERAL_STRING_ERR:
			fprintf(stderr, "���e�����������`�G���[ : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case INTEGER_VALUE_RANGE_ERR:
			fprintf(stderr, "�����l���L���͈͂𒴂��Ă��� : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case UNDERBAR_CONTINUE_ERR:
			fprintf(stderr, "���l��`�ɘA�����ăA���_�[�o�[���g�p���ꂽ : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case ZERO_NUMBER_ERR:
			fprintf(stderr, "���l�̐擪�ɖ����� 0������ : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case DEFINED_KEY_ERR:
			fprintf(stderr, "�L�[���Ē�`���ꂽ : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case MULTI_DECIMAL_ERR:
			fprintf(stderr, "�����̏����_����`���ꂽ : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case DOUBLE_VALUE_ERR:
			fprintf(stderr, "�����̕\�����͈͊O : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;
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


