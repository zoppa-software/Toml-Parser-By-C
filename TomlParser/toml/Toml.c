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
// 定数
//-----------------------------------------------------------------------------
/** テーブル作成ブロック数。 */
#define TABLE_BLOCK_SIZE	8

//-----------------------------------------------------------------------------
// 列挙体
//-----------------------------------------------------------------------------
/**
 * 読み込みトークン種別。
 */
typedef enum TokenType
{
	// トークン無し
	NoneToken,

	// エラートークン
	ErrorToken,

	// コメントトークン
	CommenntToken,

	// テーブルトークン
	TableToken,

	// テーブル配列トークン
	TableArrayToken,

	// キー／値トークン
	KeyValueToken,

} TokenType;

//-----------------------------------------------------------------------------
// 構造体
//-----------------------------------------------------------------------------
/**
 * Tomlドキュメント実装。
 */
typedef struct TomlDocumentImpl
{
	// キー／値ハッシュ
	TomlTable *		table;

	// テーブルキャッシュ
	Instance	table_cache;

	// テーブルキャッシュ（解放用）
	Vec *		table_list;

	// テーブル配列キャッシュ
	Instance	tblarray_cache;

	// テーブル配列キャッシュ（解放用）
	Vec *		tblarray_list;

	// カレントテーブル
	TomlTable *		root;

	// 文字列テーブル
	const StringHash *	strings_cache;

	// キー／値インスタンス
	Instance	pair_cache;

} TomlDocumentImpl;

//-----------------------------------------------------------------------------
// 実体を取得、追加
//-----------------------------------------------------------------------------
/**
 * テーブルキャッシュからテーブルを取得する。
 *
 * @param impl	Tomlドキュメント。
 * @return		テーブル。
 */
static TomlTable * toml_create_table(TomlDocumentImpl * impl)
{
	TomlTable * res = instance_pop(&impl->table_cache);
	res->hash = hash_initialize(sizeof(const char *));
	vec_add(impl->table_list, &res);
	return res;
}

/**
 * テーブルキャッシュからテーブルを取得する。
 *
 * @param impl	Tomlドキュメント。
 * @return		テーブル。
 */
static TomlTableArray * toml_create_table_array(TomlDocumentImpl * impl)
{
	TomlTableArray * res = instance_pop(&impl->tblarray_cache);
	res->tables = vec_initialize(sizeof(TomlTable*));
	vec_add(impl->tblarray_list, &res);
	return res;
}

/**
 * 文字列を保持する。
 *
 * @param impl		Tomlドキュメント。
 * @param buffer	読み込みバッファ。
 * @return			文字列。
 */
static char * toml_regist_string(TomlDocumentImpl * impl, TomlBuffer * buffer)
{
	return (char*)stringhash_add(impl->strings_cache, (char*)buffer->word_dst->pointer);
}

//-----------------------------------------------------------------------------
// コンストラクタ、デストラクタ
//-----------------------------------------------------------------------------
/**
 * Tomlドキュメントを生成する。
 *
 * @return	Tomlドキュメント。
 */
TomlDocument * toml_initialize()
{
	// インスタンス作成
	TomlDocumentImpl * res = (TomlDocumentImpl*)malloc(sizeof(TomlDocumentImpl));
	Assert(res != 0, "alloc error");
	_RPTN(_CRT_WARN, "toml_initialize malloc 0x%X\n", res);

	// 領域を生成
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
 * Tomlドキュメントを解放する。
 *
 * @param document	解放する Tomlドキュメント。
 */
void toml_dispose(TomlDocument ** document)
{
	TomlDocumentImpl * impl = (TomlDocumentImpl*)*document;
	size_t	i;

	Assert(impl != 0, "null pointer del error");

	// 領域を解放する
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
	
	// インスタンスを解放する
	free(impl);
	*document = 0;
}

//-----------------------------------------------------------------------------
// キー／値、テーブル、テーブル配列の追加
//-----------------------------------------------------------------------------
/**
 * 行の開始を判定する。
 *
 * @param utf8s		対象文字列。
 * @param i			読み込み位置。
 * @param tokenType	行のデータ種類。
 * @return			読み飛ばした位置。
 */
static size_t toml_check_start_line_token(Vec * utf8s, size_t i, TokenType * tokenType)
{
	TomlUtf8	c1, c2;

	// テーブル配列の開始か判定する
	if (utf8s->length - i >= 2) {
		c1 = toml_get_char(utf8s, i);
		c2 = toml_get_char(utf8s, i + 1);
		if (c1.num == '[' && c2.num == '[') {
			*tokenType = TableArrayToken;
			return i + 2;
		}
	}

	// その他の開始を判定する
	//
	// 1. テーブルの開始か判定する
	// 2. コメントの開始か判定する
	// 4. キーの開始か判定する
	// 5. それ以外はエラー
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
 * キー／値のペアを取得する。
 *
 * @param impl			Tomlドキュメント。
 * @param table			キー／値を保持するテーブル。。
 * @param buffer		読込バッファ。
 * @param start_point	読込開始位置。
 * @param next_point	読込終了位置。
 * @return				読込結果。
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

	// キー文字列を取得する
	//
	// 1. キー文字列を取得
	// 2. キー以降の空白を読み飛ばす
	// 3. = で連結しているか確認
	// 4. 値を取得する
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

	// キー／値のペアを格納する領域を取得
	pair = instance_pop(&impl->pair_cache);

	// 値を設定
	//
	// 1. 真偽値
	// 2. 文字列値
	// 3. 整数値
	// 4. 実数値
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
		
	// テーブルに追加
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
 * パスを走査して、テーブルを取得する。
 *
 * @param table			カレントテーブル。
 * @param key_str		キー文字列。
 * @param answer		取得したテーブル。
 * @return				0、1、2。
 */
static int search_path_table(TomlTable * table,
							 const char * key_str,
							 TomlTable ** answer)
{
	HashPair		result;
	TomlPair *		pair;

	if (hash_contains(table->hash, &key_str, &result)) {
		// 指定のキーが登録済みならば、紐付く項目を返す
		//
		// 1. テーブルを返す
		// 2. テーブル配列のテーブルを返す
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
 * テーブルを作成する。
 *
 * @param impl			Tomlドキュメント。
 * @param table			カレントテーブル。
 * @param buffer		バッファ領域。
 * @param start_point	読み込み開始位置。
 * @param next_point	読み込んだ位置（戻り値）
 * @return				作成結果。
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

	// 1. 空白読み捨て
	// 2. テーブル名を取得
	// 3. 格納テーブルを作成
	// 4. '.' があれば階層を追いかける
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
 * テーブル配列を作成する。
 *
 * @param impl			Tomlドキュメント。
 * @param table			カレントテーブル。
 * @param buffer		バッファ領域。
 * @param start_point	読み込み開始位置。
 * @param next_point	読み込んだ位置（戻り値）
 * @return				作成結果。
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

	// 1. 空白読み捨て
	// 2. テーブル名を取得
	// 3. 格納テーブルを作成
	// 4. '.' があれば階層を追いかける
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
// トークン解析
//-----------------------------------------------------------------------------
/**
 * Tomlドキュメントの読み残しを読み込む。
 *
 * @param impl			Tomlドキュメント。
 * @param input			読込文字列。
 * @param input_len		読込文字列長。
 * @return				読込結果。
 */
TomlResult toml_appendline(TomlDocumentImpl * impl, TomlBuffer * buffer)
{
	size_t		i;
	size_t		rng_end;
	TokenType	tkn_type;
	TomlResult	res = SUCCESS;

	// 一行のデータを取得する
	toml_read_buffer(buffer);

	// 行のデータ開始を判定する
	i = toml_skip_space(buffer->utf8s, 0);
	i = toml_check_start_line_token(buffer->utf8s, i, &tkn_type);

	// 行のデータ種類によって、テーブル／キー／コメントの判定を行う
	//
	// 1. コメント
	// 2. キー文字列
	// 3. テーブル
	// 4. テーブル配列
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
		// 空実装
		break;
	}

	return res;
}

TomlResult toml_read(TomlDocument * document, const char * path)
{
	TomlDocumentImpl * impl = (TomlDocumentImpl*)document;
	TomlBuffer * buffer = toml_create_buffer(path);

	if (buffer == 0) {
		fprintf(stderr, "ファイルオープンエラー\n");
		return FAIL_OPEN_ERR;
	}

	do {
		switch (toml_appendline(impl, buffer)) {
		case FAIL_OPEN_ERR:
			fprintf(stderr, "ファイルオープンエラー\n");
			goto EXIT_ANALISYS;

		case KEY_ANALISYS_ERR:
			fprintf(stderr, "キー文字列の解析に失敗 : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case KEY_VALUE_ERR:
			fprintf(stderr, "値の解析に失敗 : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case TABLE_NAME_ANALISYS_ERR:
			fprintf(stderr, "テーブル名解決に失敗 : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case TABLE_REDEFINITION_ERR:
			fprintf(stderr, "テーブル名を再定義しています : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case TABLE_SYNTAX_ERR:
			fprintf(stderr, "テーブル構文の解析に失敗 : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case TABLE_ARRAY_SYNTAX_ERR:
			fprintf(stderr, "テーブル配列構文の解析に失敗 : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case MULTI_QUOAT_STRING_ERR:
			fprintf(stderr, "複数クォーテーション文字列定義エラー : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case MULTI_LITERAL_STRING_ERR:
			fprintf(stderr, "複数リテラル文字列定義エラー : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case QUOAT_STRING_ERR:
			fprintf(stderr, "文字列定義エラー : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case LITERAL_STRING_ERR:
			fprintf(stderr, "リテラル文字列定義エラー : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case INTEGER_VALUE_RANGE_ERR:
			fprintf(stderr, "整数値が有効範囲を超えている : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case UNDERBAR_CONTINUE_ERR:
			fprintf(stderr, "数値定義に連続してアンダーバーが使用された : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case ZERO_NUMBER_ERR:
			fprintf(stderr, "数値の先頭に無効な 0がある : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case DEFINED_KEY_ERR:
			fprintf(stderr, "キーが再定義された : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case MULTI_DECIMAL_ERR:
			fprintf(stderr, "複数の小数点が定義された : %s\n", toml_err_message(buffer));
			goto EXIT_ANALISYS;

		case DOUBLE_VALUE_ERR:
			fprintf(stderr, "実数の表現が範囲外 : %s\n", toml_err_message(buffer));
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
// デバッグ出力
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


