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
// 定数
//-----------------------------------------------------------------------------
/** テーブル作成ブロック数。 */
#define TABLE_BLOCK_SIZE	8

//-----------------------------------------------------------------------------
// 構造体
//-----------------------------------------------------------------------------
/**
 * Tomlドキュメント実装。
 */
typedef struct TomlDocumentImpl
{
	// キー／値ハッシュ
	TomlTable *		root;

	// テーブルキャッシュ
	Instance	table_cache;

	// テーブル配列キャッシュ
	Instance	tblarray_cache;

	// 配列キャッシュ
	Instance	array_cache;

	// カレントテーブル
	TomlTable *		table;

	// 文字列テーブル
	const StringHash *	strings_cache;

	// 値インスタンス
	Instance	value_cache;

	// 日付インスタンス
	Instance	date_cache;

} TomlDocumentImpl;

/**
 * テーブル。
 */
typedef struct _TomlTableImpl
{
	// キー／値ハッシュ
	TomlTable table;

	// 定義済みならば 0以外
	int		defined;

	// 参照ポインタ
	TomlDocumentImpl * document;

} TomlTableImpl;

// 関数プロトタイプ
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

/** 空値。 */
static TomlValue empty_value;

//-----------------------------------------------------------------------------
// 実体を取得、追加
//-----------------------------------------------------------------------------
/**
 * テーブルキャッシュからテーブルを取得する。
 *
 * @param impl	Tomlドキュメント。
 * @return		テーブル。
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
 * テーブルのデストラクタアクション。
 *
 * @param target	対象インスタンス。
 */
void delete_table_action(void * target)
{
	TomlTableImpl * table = (TomlTableImpl*)target;
	hash_delete(&table->table.hash);
	table->defined = 0;
	table->document = 0;
}

/**
 * テーブル配列キャッシュからテーブル配列を取得する。
 *
 * @param impl	Tomlドキュメント。
 * @return		テーブル配列。
 */
static TomlTableArray * create_table_array(TomlDocumentImpl * impl)
{
	TomlTableArray * res = instance_pop(&impl->tblarray_cache);
	res->tables = vec_initialize(sizeof(TomlTable*));
	return res;
}

/**
 * テーブル配列のデストラクタアクション。
 *
 * @param target	対象インスタンス。
 */
static void delete_tablearray_action(void * target)
{
	TomlTableArray * table = (TomlTableArray*)target;
	vec_delete(&table->tables);
}

/**
 * 配列キャッシュから配列を取得する。
 *
 * @param impl	Tomlドキュメント。
 * @return		配列。
 */
static TomlArray * create_array(TomlDocumentImpl * impl)
{
	TomlArray * res = instance_pop(&impl->array_cache);
	res->array = vec_initialize(sizeof(TomlValue*));
	return res;
}

/**
 * 配列のデストラクタアクション。
 *
 * @param target	対象インスタンス。
 */
static void delete_array_action(void * target)
{
	TomlArray * array = (TomlArray*)target;
	vec_delete(&array->array);
}

/**
 * 配列を削除する。
 *
 * @param impl		Tomlドキュメント。
 * @param array		削除する配列。
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
 * Tomlドキュメントを解放する。
 *
 * @param document	解放する Tomlドキュメント。
 */
void toml_dispose(TomlDocument ** document)
{
	TomlDocumentImpl * impl = (TomlDocumentImpl*)*document;

	Assert(impl != 0, "null pointer del error");

	// 領域を解放する
	instance_delete_all_destructor(&impl->table_cache, delete_table_action);
	instance_delete_all_destructor(&impl->tblarray_cache, delete_tablearray_action);
	instance_delete_all_destructor(&impl->array_cache, delete_array_action);
	instance_delete(&impl->date_cache);
	stringhash_delete(&impl->strings_cache);
	instance_delete(&impl->value_cache);
	
	// インスタンスを解放する
	free(impl);
	*document = 0;
}

//-----------------------------------------------------------------------------
// キー解析
//-----------------------------------------------------------------------------
/**
 * 文字列を保持する。
 *
 * @param impl		Tomlドキュメント。
 * @param buffer	読み込みバッファ。
 * @return			文字列。
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
 * キー文字列の取得。
 *
 * @param impl			Tomlドキュメント。
 * @param buffer		読み込み領域。
 * @param key_ptr		キー文字列リスト。
 * @param point			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param error			エラー詳細情報（戻り値）
 * @return				取得できたら 0以外。
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
 * パスを走査して、テーブルを取得する。
 *
 * @param table			カレントテーブル。
 * @param key_str		キー文字列。
 * @param answer		取得したテーブル。
 * @return				0はエラー、1は作成済みのテーブル、2は新規作成
 */
static int search_path_table(TomlTableImpl * table,
							 const char * key_str,
							 TomlTableImpl ** answer)
{
	HashPair		result;
	TomlValue *		value;

	if (hash_contains(table->table.hash, &key_str, &result)) {
		// 指定のキーが登録済みならば、紐付く項目を返す
		//
		// 1. テーブルを返す
		// 2. テーブル配列のテーブルを返す
		// 3. テーブルの新規作成を依頼
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
// キー／値、インラインテーブル、配列の追加
//-----------------------------------------------------------------------------
/**
 * インラインテーブルが閉じられていることを確認する。
 *
 * @param buffer		読み込み領域。
 * @param point			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param error			エラー詳細情報。
 * @return				閉じられていたら 1、次のキー取得ならば 2、エラーは 0。
 */
static int close_inline_table(TomlBuffer * buffer,
							  size_t point,
							  size_t * next_point,
							  TomlResultSummary * error)
{
	TomlUtf8	c;

	while (point < buffer->utf8s->length || !toml_end_of_file(buffer)) {
		// 改行、空白部を取り除く
		point = toml_skip_linefield_and_space(buffer, point);

		// 一文字取得
		c = toml_get_char(buffer->utf8s, point);
		
		// 文字判定
		switch (c.num) {
		case '}':
			// インラインテーブルが閉じられた
			*next_point = point + 1;
			return 1;

		case ',':
			// 続けて次のキー／値判定へ
			*next_point = point + 1;
			return 2;

		default:
			// インラインテーブルが閉じられていない
			goto EXIT_WHILE;
		}
	}

EXIT_WHILE:
	// インラインテーブルが閉じられていない
	*error = toml_res_ctor(INLINE_TABLE_NOT_CLOSE_ERR, point + 1, buffer->loaded_line);
	*next_point = point + 1;
	return 0;
}

/**
 * インラインテーブルを解析する。
 *
 * @param impl			Tomlドキュメント。
 * @param buffer		読み込み領域。
 * @param point			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param error			エラー詳細情報。
 * @return				正常に読み込めたならば 0以外。
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
		// 改行、空白部を取り除く
		ptr = toml_skip_linefield_and_space(buffer, ptr);

		// キー／値部分を取り込む
		res = analisys_key_and_value(impl, table, buffer, ptr, next_point, 1);
		if (res.code != SUCCESS) {
			instance_push_destructor(&impl->table_cache, table, delete_table_action);
			*error = res;
			return 0;
		}

		// インラインテーブルが閉じられているか確認
		//
		// 1. テーブルが閉じられている
		// 2. 次のキー／値を取得
		// 3. エラー
		ptr = *next_point;
		switch (close_inline_table(buffer, ptr, next_point, error)) {
		case 1:							// 1
			buffer->table = (TomlTable*)table;
			return 1;

		case 2:							// 2
			ptr = *next_point;
			break;

		default:						// 3
			// エラー詳細は close_inline_tableで設定済み
			instance_push_destructor(&impl->table_cache, table, delete_table_action);
			return 0;
		}
	}

	// インラインテーブルが閉じられていない
	instance_push_destructor(&impl->table_cache, table, delete_table_action);
	*error = toml_res_ctor(INLINE_TABLE_NOT_CLOSE_ERR, buffer->utf8s->length, buffer->loaded_line);
	*next_point = buffer->utf8s->length;
	return 0;
}

/**
 * 配列が閉じられていることを確認する。
 *
 * @param buffer		読み込み領域。
 * @param point			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param error			エラー詳細情報。
 * @return				閉じられていたら 1、次のキー取得ならば 2、エラーは 0。
 */
static int close_value_array(TomlBuffer * buffer,
							 size_t point,
							 size_t * next_point,
							 TomlResultSummary * error)
{
	TomlUtf8	c;

	while (point < buffer->utf8s->length || !toml_end_of_file(buffer)) {
		// 改行、空白部を取り除く
		point = toml_skip_linefield_and_space(buffer, point);

		// 一文字取得
		c = toml_get_char(buffer->utf8s, point);
		
		// 文字判定
		switch (c.num) {
		case ']':
			// 配列が閉じられた
			*next_point = point + 1;
			return 1;

		case ',':
			// 続けて次のキー／値判定へ
			*next_point = point + 1;	
			return 2;

		default:
			// 配列が閉じられていない
			goto EXIT_WHILE;
		}
	}

EXIT_WHILE:
	// 配列が閉じられていない
	*error = toml_res_ctor(ARRAY_NOT_CLOSE_ERR, point + 1, buffer->loaded_line);
	*next_point = point + 1;
	return 0;
}

/**
 * 配列内の値の型が全て一致するか判定する。
 *
 * @param array		判定する配列参照。
 * @return			エラーが無ければ 0以外。
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
 * 配列を解析する。
 *
 * @param impl			Tomlドキュメント。
 * @param buffer		読み込み領域。
 * @param point			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param error			エラー詳細情報。
 * @return				正常に読み込めたならば 0以外。
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
		// 改行、空白部を取り除く
		ptr = toml_skip_linefield_and_space(buffer, ptr);

		// 値を取り込む
		if (!analisys_value(impl, buffer, ptr, next_point, &value, error)) {
			delete_array(impl, array);
			return 0;
		}

		// 空値以外は取り込む
		if (value->value_type != TomlNoneValue) {
			vec_add(array->array, &value);
		}

		// 配列が閉じられているか確認
		//
		// 1. テーブルが閉じられている
		// 2. 次のキー／値を取得
		// 3. エラー
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
			// エラー詳細は close_value_arrayで設定済み
			delete_array(impl, array);
			return 0;
		}
	}

	// 配列が閉じられていない
	delete_array(impl, array);
	*error = toml_res_ctor(ARRAY_NOT_CLOSE_ERR, buffer->utf8s->length, buffer->loaded_line);
	*next_point = buffer->utf8s->length;
	return 0;
}

/**
 * 値の解析を行う。
 *
 * @param impl			Tomlドキュメント。
 * @param buffer		読み込み領域。
 * @param point			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param res_value		取得した値（戻り値）
 * @param error			エラー詳細情報。
 * @return				正常に読み込めたならば 0以外。
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

	// インラインテーブル、配列、日付の確認
	//
	// 1. インラインテーブルを解析する
	// 2. 配列を解析する
	// 3. 値を取得する
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

	// キー／値のペアを格納する領域を取得
	value = instance_pop(&impl->value_cache);

	// 値を設定
	switch (type) {
	case TomlBooleanValue:			// 真偽値
		value->value_type = TomlBooleanValue;
		value->value.boolean = buffer->boolean;
		break;

	case TomlStringValue:			// 文字列値
		value->value_type = TomlStringValue;
		value->value.string = regist_string(impl, buffer);
		break;

	case TomlIntegerValue:			// 整数値
		value->value_type = TomlIntegerValue;
		value->value.integer = buffer->integer;
		break;

	case TomlFloatValue:			// 実数値
		value->value_type = TomlFloatValue;
		value->value.float_number = buffer->float_number;
		break;

	case TomlDateValue:				// 日付
		value->value_type = TomlDateValue;
		value->value.date = (TomlDate*)instance_pop(&impl->date_cache);
		*value->value.date = buffer->date;
		break;

	case TomlTableValue:			// インラインテーブル
		value->value_type = TomlTableValue;
		value->value.table = buffer->table;
		break;

	case TomlArrayValue:			// 配列
		value->value_type = TomlArrayValue;
		value->value.array = buffer->array;
		break;

	default:						// 無効値
		value->value_type = TomlNoneValue;
		break;
	}

	*res_value = value;
	*error = toml_res_ctor(SUCCESS, 0, buffer->loaded_line);
	return 1;
}

/**
 * キー／値のペアを取得する。
 *
 * @param impl			Tomlドキュメント。
 * @param table			キー／値を保持するテーブル。。
 * @param buffer		読込バッファ。
 * @param point			読込開始位置。
 * @param next_point	読込終了位置。
 * @param last_nochk	値の終了範囲チェックをしないなら 0以外
 * @return				読込結果。
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

	// キー文字列を取得する
	//
	// 1. キー文字列を取得
	// 2. キー以降の空白を読み飛ばす
	// 3. = で連結しているか確認
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

	// 値を取得する
	//
	// 1. 値が取得できるか
	// 2. 無効値であるか
	if (!analisys_value(impl, buffer,			// 1
						ptr + 1, next_point, &value, &res)) {
		goto EXIT_KEY_AND_VALUE;
	}
	if (value->value_type == TomlNoneValue) {	// 2
		res = toml_res_ctor(KEY_VALUE_ERR, ptr + 1, buffer->loaded_line);
		goto EXIT_KEY_AND_VALUE;
	}

	// 改行まで確認
	if (!last_nochk && !toml_skip_line_end(buffer->utf8s, *next_point)) {
		res = toml_res_ctor(KEY_VALUE_ERR, *next_point, buffer->loaded_line);
		goto EXIT_KEY_AND_VALUE;
	}

	// '.' で指定されたテーブル参照を収集する
	//
	// 1. テーブル参照を取得
	// 2. エラーが有れば終了
	// 3. 既に作成済みならばカレントを変更
	// 4. 作成されていなければテーブルを作成し、カレントに設定
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

	// 最終のテーブルに値を割り当てる
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
// テーブル、テーブル配列の追加
//-----------------------------------------------------------------------------
/**
 * テーブルを作成する。
 *
 * @param impl			Tomlドキュメント。
 * @param table			カレントテーブル。
 * @param buffer		バッファ領域。
 * @param point			読み込み開始位置。
 * @param next_point	読み込んだ位置（戻り値）
 * @return				作成結果。
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

	// '.' で区切られたテーブル名を事前に収集する
	key_ptr = vec_initialize(sizeof(char*));
	if (!get_keys(impl, buffer, key_ptr, point, &nxt, &res)) {
		goto EXIT_TABLE;
	}

	// テーブルが閉じられているか確認
	if (!toml_close_table(buffer->utf8s, nxt)) {
		res = toml_res_ctor(TABLE_SYNTAX_ERR, 0, buffer->loaded_line);
		*next_point = nxt;
		goto EXIT_TABLE;
	}

	// テーブルを作成する
	//
	// 1. テーブル参照を取得
	// 2. エラーが有れば終了
	// 3. 既に作成済みならばカレントを変更
	// 4. 作成されていなければテーブルを作成し、カレントに設定
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

	// 空白は読み捨てておく
	*next_point = toml_skip_space(buffer->utf8s, nxt);

	// カレントのテーブルを設定
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
 * テーブル配列を作成する。
 *
 * @param impl			Tomlドキュメント。
 * @param table			カレントテーブル。
 * @param buffer		バッファ領域。
 * @param start_point	読み込み開始位置。
 * @param next_point	読み込んだ位置（戻り値）
 * @return				作成結果。
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

	// '.' で区切られたテーブル名を事前に収集する
	key_ptr = vec_initialize(sizeof(char*));
	if (!get_keys(impl, buffer, key_ptr, point, &nxt, &res)) {
		goto EXIT_TABLE_ARRAY;
	}

	// テーブル配列が閉じられているか確認
	if (!toml_close_table_array(buffer->utf8s, nxt)) {
		res = toml_res_ctor(TABLE_ARRAY_SYNTAX_ERR, 0, buffer->loaded_line);
		*next_point = nxt;
		goto EXIT_TABLE_ARRAY;
	}

	// 最下層のテーブル以外のテーブル参照を収集する
	//
	// 1. テーブル参照を取得
	// 2. エラーが有れば終了
	// 3. 既に作成済みならばカレントを変更
	// 4. 作成されていなければテーブルを作成し、カレントに設定
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

	// 最下層のテーブルは新規作成となる
	//
	// 1. 登録する名前（キー文字列）を取得
	// 2. 親のテーブルに最下層のテーブル名が登録されている
	//    2-1. 登録されている名前のデータを取得する
	//    2-2. テーブル配列が登録されているならば、新規テーブルを作成し、カレントテーブルとする
	//    2-3. テーブル配列でないならばエラーとする
	// 3. 親のテーブルに最下層のテーブル名が登録されていない
	//    3-1. テーブル配列を作成
	//    3-2. テーブル配列にテーブルを追加、追加されたテーブルが次のカレントテーブルになる
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
	
	// カレントのテーブルを作成したテーブルに変更
	impl->table = (TomlTable*)new_table;

	// 空白読み飛ばし
	*next_point = toml_skip_space(buffer->utf8s, nxt);

	// 戻り値を作成して返す
	res = toml_res_ctor(SUCCESS, 0, buffer->loaded_line);

EXIT_TABLE_ARRAY:
	vec_delete(&key_ptr);
	return res;
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
TomlResultSummary append_line(TomlDocumentImpl * impl, TomlBuffer * buffer)
{
	size_t				i;
	size_t				rng_end;
	TomlLineType		tkn_type;
	TomlResultSummary	res;

	memset(&res, 0, sizeof(res));

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
		// 空実装
		break;
	}

	res.utf8s = buffer->utf8s;
	res.word_dst = buffer->word_dst;
	return res;
}

/**
 * エラー情報を出力する。
 *
 * @param rescode	エラー情報。
 */
static void show_message(TomlResultSummary rescode)
{
	const char * errmsg = 0;
	const char * errline = toml_err_message(rescode.utf8s, rescode.word_dst);

	switch (rescode.code) {
	case FAIL_OPEN_ERR:
		errmsg = "ファイルオープンエラー";
		break;
	case KEY_ANALISYS_ERR:
		errmsg = "キー文字列の解析に失敗";
		break;
	case KEY_VALUE_ERR:
		errmsg = "値の解析に失敗";
		break;
	case TABLE_NAME_ANALISYS_ERR:
		errmsg = "テーブル名解決に失敗";
		break;
	case TABLE_REDEFINITION_ERR:
		errmsg = "テーブル名を再定義しています";
		break;
	case TABLE_SYNTAX_ERR:
		errmsg = "テーブル構文の解析に失敗";
		break;
	case TABLE_ARRAY_SYNTAX_ERR:
		errmsg = "テーブル配列構文の解析に失敗";
		break;
	case MULTI_QUOAT_STRING_ERR:
		errmsg = "複数クォーテーション文字列定義エラー";
		break;
	case MULTI_LITERAL_STRING_ERR:
		errmsg = "複数リテラル文字列定義エラー";
		break;
	case QUOAT_STRING_ERR:
		errmsg = "文字列定義エラー";
		break;
	case LITERAL_STRING_ERR:
		errmsg = "リテラル文字列定義エラー";
		break;
	case INTEGER_VALUE_RANGE_ERR:
		errmsg = "整数値が有効範囲を超えている";
		break;
	case UNDERBAR_CONTINUE_ERR:
		errmsg = "数値定義に連続してアンダーバーが使用された";
		break;
	case ZERO_NUMBER_ERR:
		errmsg = "数値の先頭に無効な 0がある";
		break;
	case DEFINED_KEY_ERR:
		errmsg = "キーが再定義された";
		break;
	case MULTI_DECIMAL_ERR:
		errmsg = "複数の小数点が定義された";
		break;
	case DOUBLE_VALUE_ERR:
		errmsg = "実数の表現が範囲外";
		break;
	case DATE_FORMAT_ERR:
		errmsg = "日付が解析できない";
		break;
	case UNICODE_DEFINE_ERR:
		errmsg = "ユニコード文字定義の解析に失敗";
		break;
	case INVALID_ESCAPE_CHAR_ERR:
		errmsg = "無効なエスケープ文字が指定された";
		break;
	case INLINE_TABLE_NOT_CLOSE_ERR:
		errmsg = "インラインテーブルが正しく閉じられていない";
		break;
	case ARRAY_NOT_CLOSE_ERR:
		errmsg = "配列が正しく閉じられていない";
		break;
	case EMPTY_COMMA_ERR:
		errmsg = "カンマの前に値が定義されていない";
		break;
	case ARRAY_VALUE_DIFFERENT_ERR:
		errmsg = "配列の値の型が異なる";
		break;
	case NO_LEADING_ZERO_ERR:
		errmsg = "小数点の前に数値が入力されていない";
		break;
	case NO_LAST_ZERO_ERR:
		errmsg = "小数点の後に数値が入力されていない";
		break;	
	}

	fprintf(stderr, "%s\n", errmsg);
	fprintf(stderr, "行: %lld  位置: %lld\n", rescode.row, rescode.column);
	fprintf(stderr, "> %s\n", errline);
}

/**
 * TOML形式のファイルを読み込む。
 *
 * @param document		Tomlドキュメント。
 * @param path			ファイルパス。
 * @return				読込結果。
 */
TomlResultCode toml_read(TomlDocument * document, const char * path)
{
	TomlDocumentImpl * impl = (TomlDocumentImpl*)document;
	TomlBuffer * buffer = toml_create_buffer(path);
	TomlResultSummary rescode;

	if (buffer == 0) {
		fprintf(stderr, "ファイルオープンエラー\n");
		return FAIL_OPEN_ERR;
	}

	do {
		// 一行取り込み
		rescode = append_line(impl, buffer);
		if (rescode.code != SUCCESS) {
			show_message(rescode);
			goto EXIT_ANALISYS;
		}

		// バッファ消去
		vec_clear(buffer->utf8s);
	} while (!toml_end_of_file(buffer));

EXIT_ANALISYS:;
	toml_close_buffer(buffer);

	return rescode.code;
}

//-----------------------------------------------------------------------------
// データアクセス
//-----------------------------------------------------------------------------
/**
 * 繰返処理委譲関数ポインタ。
 *
 * @param pair	表示する項目。
 * @param param	パラメータ。
 */
void table_for_function(HashPair pair, void * param)
{
	TomlBucket	bucket;
	bucket.key_name = *((char**)pair.key);
	bucket.ref_value = (TomlValue*)pair.value.object;

	vec_add((Vec*)param, &bucket);
}

/**
 * 指定テーブルのキーと値のリストを取得する。
 *
 * @param document	Tomlドキュメント。
 * @param table		指定テーブル。
 * @return			キーと値のリスト。
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
 * 指定テーブルのキーと値のリストを削除する。
 *
 * @param list		キーと値のリスト。
 */
void toml_delete_key_and_value(TomlBuckets * list)
{
	if (list) {
		free((void*)list->values);
		list->values = 0;
	}
}

/**
 * 指定したテーブルから、キーの値を取得する。
 *
 * @param table		検索するテーブル。
 * @param key		検索するキー。
 * @return			取得した値。
 */
TomlValue toml_search_key(TomlTable * table,
						  const char * key)
{
	TomlTableImpl *	tbl_impl = (TomlTableImpl*)table;
	TomlDocumentImpl * impl = (TomlDocumentImpl*)tbl_impl->document;
	const char * reg_key = 0;
	HashPair	result;

	if (*key == 0) {
		// 空白文字を返す
		return empty_value;
	}
	else {
		// キー文字列を返す
		reg_key = stringhash_add(impl->strings_cache, key);

		// 値を返す
		if (hash_contains(table->hash, &reg_key, &result)) {
			return *((TomlValue*)result.value.object);
		}
		else {
			return empty_value;
		}
	}
}

/**
 * 指定したテーブルに、指定したキーが存在するか確認する。
 *
 * @param table		検索するテーブル。
 * @param key		検索するキー。
 * @return			存在したら 0以外を返す。
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
		// キー文字列を返す
		reg_key = stringhash_add(impl->strings_cache, key);

		// 存在したら 0以外を返す
		return hash_contains(table->hash, &reg_key, &result);
	}
}

/**
 * 指定した配列の指定インデックスの値を取得する。
 *
 * @param array		配列。
 * @param index		インデックス。
 * @return			値。
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
 * 配列の要素数を取得する。
 *
 * @param array		配列。
 * @return			要素数。
 */
size_t toml_get_array_count(TomlArray * array)
{
	return (array != 0 ? array->array->length : 0);
}

/**
 * 指定したテーブル配列の指定インデックスの値を取得する。
 *
 * @param tbl_array		テーブル配列。
 * @param index			インデックス。
 * @return				テーブル。
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
 * テーブル配列の要素数を取得する。
 *
 * @param array		テーブル配列。
 * @return			要素数。
 */
size_t toml_get_table_array_count(TomlTableArray * tbl_array)
{
	return (tbl_array != 0 ? tbl_array->tables->length : 0);
}
