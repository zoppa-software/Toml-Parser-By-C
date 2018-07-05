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

// 関数プロトタイプ
static TomlResultSummary analisys_key_and_value(TomlDocumentImpl * impl,
												TomlTable * table,
												TomlBuffer * buffer,
												size_t start_point,
												size_t * next_point);

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
static TomlTable * create_table(TomlDocumentImpl * impl)
{
	TomlTable * res = instance_pop(&impl->table_cache);
	res->hash = hash_initialize(sizeof(const char *));
	res->defined = 0;
	return res;
}

/**
 * テーブルのデストラクタアクション。
 *
 * @param target	対象インスタンス。
 */
void delete_table_action(void * target)
{
	TomlTable * table = (TomlTable*)target;
	hash_delete(&table->hash);
	table->defined = 0;
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
	res->table = create_table(res);
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
			error->code = INLINE_TABLE_NOT_CLOSE_ERR;
			error->column = point + 1;
			error->row = buffer->loaded_line;
			*next_point = point + 1;
			return 0;
		}
	}

	// インラインテーブルが閉じられていない
	error->code = INLINE_TABLE_NOT_CLOSE_ERR;
	error->column = point + 1;
	error->row = buffer->loaded_line;
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
	TomlTable *			table = create_table(impl);
	size_t				ptr = point;
	TomlResultSummary	res;

	while (ptr < buffer->utf8s->length) {
		// 改行、空白部を取り除く
		ptr = toml_skip_linefield_and_space(buffer, ptr);

		// キー／値部分を取り込む
		res = analisys_key_and_value(impl, table, buffer, ptr, next_point);
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
			buffer->table = table;
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
	error->code = INLINE_TABLE_NOT_CLOSE_ERR;
	error->column = buffer->utf8s->length;
	error->row = buffer->loaded_line;
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
			error->code = ARRAY_NOT_CLOSE_ERR;
			error->column = point + 1;
			error->row = buffer->loaded_line;
			*next_point = point + 1;
			return 0;
		}
	}

	// 配列が閉じられていない
	error->code = ARRAY_NOT_CLOSE_ERR;
	error->column = point + 1;
	error->row = buffer->loaded_line;
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
				error->code = ARRAY_VALUE_DIFFERENT_ERR;
				error->column = *next_point;
				error->row = buffer->loaded_line;
				return 0;
			}

		case 2:							// 2
			if (value->value_type != TomlNoneValue) {
				ptr = *next_point;
				break;
			}
			else {
				delete_array(impl, array);
				error->code = EMPTY_COMMA_ERR;
				error->column = *next_point;
				error->row = buffer->loaded_line;
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
	error->code = ARRAY_NOT_CLOSE_ERR;
	error->column = buffer->utf8s->length;
	error->row = buffer->loaded_line;
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
	error->code = SUCCESS;
	error->column = 0;
	error->row = buffer->loaded_line;
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
 * @return				読込結果。
 */
static TomlResultSummary analisys_key_and_value(TomlDocumentImpl * impl,
												TomlTable * table,
												TomlBuffer * buffer,
												size_t point,
												size_t * next_point)
{
	size_t				ptr;
	TomlUtf8			c;
	TomlValue *			value;
	const  char *		key_str;
	HashPair			val_pair;
	TomlResultSummary	res;

	// キー文字列を取得する
	//
	// 1. キー文字列を取得
	// 2. キー以降の空白を読み飛ばす
	// 3. = で連結しているか確認
	if (!toml_get_key(buffer, point, &ptr, &res)) {			// 1
		return res;
	}
	key_str = regist_string(impl, buffer);

	ptr = toml_skip_space(buffer->utf8s, ptr);				// 2

	c = toml_get_char(buffer->utf8s, ptr);					// 3
	if (c.num != '=') {
		res.code = KEY_ANALISYS_ERR;
		res.column = ptr;
		res.row = buffer->loaded_line;
		*next_point = ptr;
		return res;
	}

	// 値を取得する
	//
	// 1. 値を解析する
	// 2. キーが登録済みか確認する
	if (analisys_value(impl, buffer, ptr + 1, next_point, &value, &res)) {	// 1
		if (value->value_type == TomlNoneValue) {
			res.code = KEY_VALUE_ERR;
			res.column = ptr + 1;
			res.row = buffer->loaded_line;
		}
		else if (!hash_contains(table->hash, &key_str, &val_pair)) {		// 2
			hash_add(table->hash, &key_str, hash_value_of_pointer(value));
			res.code = SUCCESS;
			res.column = 0;
			res.row = buffer->loaded_line;
		}
		else {
			buffer->err_point = buffer->utf8s->length - 1;
			res.code = DEFINED_KEY_ERR;
			res.column = 0;
			res.row = buffer->loaded_line;
		}
	}
	return res;
}

//-----------------------------------------------------------------------------
// テーブル、テーブル配列の追加
//-----------------------------------------------------------------------------
/**
 * パスを走査して、テーブルを取得する。
 *
 * @param table			カレントテーブル。
 * @param key_str		キー文字列。
 * @param answer		取得したテーブル。
 * @return				0はエラー、1は作成済みのテーブル、2は新規作成
 */
static int search_path_table(TomlTable * table,
							 const char * key_str,
							 TomlTable ** answer)
{
	HashPair		result;
	TomlValue *		value;

	if (hash_contains(table->hash, &key_str, &result)) {
		// 指定のキーが登録済みならば、紐付く項目を返す
		//
		// 1. テーブルを返す
		// 2. テーブル配列のテーブルを返す
		// 3. テーブルの新規作成を依頼
		value = (TomlValue*)result.value.object;
		switch (value->value_type)
		{
		case TomlTableValue:			// 1
			*answer = value->value.table;
			return 1;

		case TomlTableArrayValue:		// 2
			*answer = VEC_GET(TomlTable*,
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
										TomlTable * table,
										TomlBuffer * buffer,
										size_t point,
										size_t * next_point)
{
	TomlUtf8			c;
	size_t				i, nxt;
	TomlTable *			cur_table = table;
	TomlTable *			new_table;
	TomlValue *			value;
	const char *		key_str;
	TomlResultSummary	res;

	// 1. 空白読み捨て
	// 2. テーブル名を取得
	// 3. 格納テーブルを作成
	//    3-1. テーブル取得エラー
	//    3-2. 作成済みテーブルを取得できた
	//    3-3. 新規テーブルを作成する
	// 4. '.' があれば階層を追いかける
	for (i = point; i < buffer->utf8s->length; ) {
		i = toml_skip_space(buffer->utf8s, i);				// 1

		if (!toml_get_key(buffer, i, &nxt, &res)) {			// 2
			buffer->err_point = nxt;
			return res;
		}
		i = nxt;

		key_str = regist_string(impl, buffer);				// 3
		switch (search_path_table(cur_table, key_str, &new_table)) {
		case 0:
			res.code = TABLE_REDEFINITION_ERR;				// 3-1
			res.column = 0;
			res.row = buffer->loaded_line;
			*next_point = i;
			return res;
		case 1:
			cur_table = new_table;							// 3-2
			break;
		default:
			new_table = create_table(impl);					// 3-3
			value = instance_pop(&impl->value_cache);
			value->value_type = TomlTableValue;
			value->value.table = new_table;
			hash_add(cur_table->hash, &key_str, hash_value_of_pointer(value));
			cur_table = new_table;
			break;
		}

		c = toml_get_char(buffer->utf8s, i);				// 4
		if (c.num != '.') {
			break;
		}
		i++;
	}

	// 空白は読み捨てておく
	i = toml_skip_space(buffer->utf8s, i);
	*next_point = i;

	// カレントのテーブルを設定
	impl->table = cur_table;
	if (!impl->table->defined) {
		impl->table->defined = 1;
		res.code = SUCCESS;
		res.column = 0;
		res.row = buffer->loaded_line;
		return res;
	}
	else {
		res.code = DEFINED_KEY_ERR;
		res.column = 0;
		res.row = buffer->loaded_line;
		return res;
	}
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
											  TomlTable * table,
											  TomlBuffer * buffer,
											  size_t point,
											  size_t * next_point)
{
	TomlUtf8			c;
	size_t				i, j, nxt;
	TomlTable *			cur_table = table;
	TomlTable *			new_table;
	TomlValue *			value;
	const char *		key_str;
	TomlTableArray *	new_array;
	HashPair			hash_pair;
	TomlResultSummary	res;

	// '.' で区切られたテーブル名を事前に収集する
	//
	// 1. 
	// 1. 空白読み捨て
	// 2. テーブル名を取得
	// 3. 格納テーブルを作成
	// 4. '.' があれば階層を追いかける
	vec_clear(buffer->key_ptr);

	for (i = point; i < buffer->utf8s->length; ) {
		i = toml_skip_space(buffer->utf8s, i);				// 1

		if (!toml_get_key(buffer, i, &nxt, &res)) {			// 2
			buffer->err_point = nxt;
			return res;
		}
		i = nxt;

		key_str = regist_string(impl, buffer);				// 3
		vec_add(buffer->key_ptr, (char*)&key_str);

		c = toml_get_char(buffer->utf8s, i);				// 4
		if (c.num != '.') {
			break;
		}
		i++;
	}

	// 最下層のテーブル以外のテーブル参照を収集する
	//
	// 1. テーブル参照を取得
	// 2. エラーが有れば終了
	// 3. 既に作成済みならばカレントを変更
	// 4. 作成されていなければテーブルを作成し、カレントに設定
	for (j = 0; j < buffer->key_ptr->length - 1; ++j) {
		key_str = VEC_GET(const char*, buffer->key_ptr, j);

		switch (search_path_table(cur_table, key_str, &new_table)) {	// 1
		case 0:
			res.code = TABLE_REDEFINITION_ERR;							// 2
			res.column = 0;
			res.row = buffer->loaded_line;
			*next_point = i;
			return res;

		case 1:
			cur_table = new_table;										// 3
			break;

		default:
			new_table = create_table(impl);								// 4
			value = instance_pop(&impl->value_cache);
			value->value_type = TomlTableValue;
			value->value.table = new_table;
			hash_add(cur_table->hash, &key_str, hash_value_of_pointer(value));
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
	key_str = VEC_GET(const char*, buffer->key_ptr, buffer->key_ptr->length - 1);	// 1

	if (hash_contains(cur_table->hash, &key_str, &hash_pair)) {			// 2-1
		value = (TomlValue*)hash_pair.value.object;
		if (value->value_type == TomlTableArrayValue) {					// 2-2
			new_table = create_table(impl);
			vec_add(value->value.tbl_array->tables, &new_table);
		}
		else {
			res.code = TABLE_REDEFINITION_ERR;							// 2-3
			res.column = 0;
			res.row = buffer->loaded_line;
			*next_point = i;
			return res;
		}
	}
	else {
		new_array = create_table_array(impl);							// 3-1
		value = instance_pop(&impl->value_cache);
		value->value_type = TomlTableArrayValue;
		value->value.tbl_array = new_array;
		hash_add(cur_table->hash, &key_str, hash_value_of_pointer(value));

		new_table = create_table(impl);									// 3-2
		vec_add(new_array->tables, &new_table);
	}
	
	// カレントのテーブルを作成したテーブルに変更
	impl->table = new_table;

	// 空白読み飛ばし
	i = toml_skip_space(buffer->utf8s, i);
	*next_point = i;

	// 戻り値を作成して返す
	res.code = SUCCESS;
	res.column = 0;
	res.row = buffer->loaded_line;
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
TomlResultCode toml_appendline(TomlDocumentImpl * impl, TomlBuffer * buffer)
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
		res = analisys_key_and_value(impl, impl->table, buffer, i, &rng_end);
		if (res.code == SUCCESS && !toml_skip_linefield(buffer->utf8s, rng_end)) {
			buffer->err_point = rng_end;
			res.code = KEY_VALUE_ERR;
		}
		break;

	case TomlTableLine:					// 3
		res = analisys_table(impl, impl->root, buffer, i, &rng_end);
		if (res.code == SUCCESS &&
			!toml_close_table(buffer->utf8s, rng_end)) {
			buffer->err_point = rng_end;
			res.code = TABLE_SYNTAX_ERR;
		}
		break;

	case TomlTableArrayLine:			// 4
		res = analisys_table_array(impl, impl->root, buffer, i, &rng_end);
		if (res.code == SUCCESS &&
			!toml_close_table_array(buffer->utf8s, rng_end)) {
			buffer->err_point = rng_end;
			res.code = TABLE_ARRAY_SYNTAX_ERR;
		}
		break;

	default:
		// 空実装
		break;
	}

	return res.code;
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
	TomlResultCode rescode;

	if (buffer == 0) {
		fprintf(stderr, "ファイルオープンエラー\n");
		return FAIL_OPEN_ERR;
	}

	do {
		rescode = toml_appendline(impl, buffer);
		if (rescode != SUCCESS) {
			const char * errmsg = toml_err_message(buffer->utf8s, buffer->word_dst);

			switch (rescode) {
			case FAIL_OPEN_ERR:
				fprintf(stderr, "ファイルオープンエラー\n");
				goto EXIT_ANALISYS;

			case KEY_ANALISYS_ERR:
				fprintf(stderr, "キー文字列の解析に失敗 : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case KEY_VALUE_ERR:
				fprintf(stderr, "値の解析に失敗 : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case TABLE_NAME_ANALISYS_ERR:
				fprintf(stderr, "テーブル名解決に失敗 : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case TABLE_REDEFINITION_ERR:
				fprintf(stderr, "テーブル名を再定義しています : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case TABLE_SYNTAX_ERR:
				fprintf(stderr, "テーブル構文の解析に失敗 : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case TABLE_ARRAY_SYNTAX_ERR:
				fprintf(stderr, "テーブル配列構文の解析に失敗 : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case MULTI_QUOAT_STRING_ERR:
				fprintf(stderr, "複数クォーテーション文字列定義エラー : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case MULTI_LITERAL_STRING_ERR:
				fprintf(stderr, "複数リテラル文字列定義エラー : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case QUOAT_STRING_ERR:
				fprintf(stderr, "文字列定義エラー : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case LITERAL_STRING_ERR:
				fprintf(stderr, "リテラル文字列定義エラー : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case INTEGER_VALUE_RANGE_ERR:
				fprintf(stderr, "整数値が有効範囲を超えている : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case UNDERBAR_CONTINUE_ERR:
				fprintf(stderr, "数値定義に連続してアンダーバーが使用された : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case ZERO_NUMBER_ERR:
				fprintf(stderr, "数値の先頭に無効な 0がある : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case DEFINED_KEY_ERR:
				fprintf(stderr, "キーが再定義された : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case MULTI_DECIMAL_ERR:
				fprintf(stderr, "複数の小数点が定義された : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case DOUBLE_VALUE_ERR:
				fprintf(stderr, "実数の表現が範囲外 : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case DATE_FORMAT_ERR:
				fprintf(stderr, "日付が解析できない : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case UNICODE_DEFINE_ERR:
				fprintf(stderr, "ユニコード文字定義の解析に失敗 : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case INVALID_ESCAPE_CHAR_ERR:
				fprintf(stderr, "無効なエスケープ文字が指定された : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case INLINE_TABLE_NOT_CLOSE_ERR:
				fprintf(stderr, "インラインテーブルが正しく閉じられていない : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case ARRAY_NOT_CLOSE_ERR:
				fprintf(stderr, "配列が正しく閉じられていない : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case EMPTY_COMMA_ERR:
				fprintf(stderr, "カンマの前に値が定義されていない : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case ARRAY_VALUE_DIFFERENT_ERR:
				fprintf(stderr, "配列の値の型が異なる : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case NO_LEADING_ZERO_ERR:
				fprintf(stderr, "小数点の前に数値が入力されていない : %s\n", errmsg);
				goto EXIT_ANALISYS;

			case NO_LAST_ZERO_ERR:
				fprintf(stderr, "小数点の後に数値が入力されていない : %s\n", errmsg);
				goto EXIT_ANALISYS;
			}
		}

		vec_clear(buffer->utf8s);
		vec_clear(buffer->key_ptr);
	} while (!toml_end_of_file(buffer));

EXIT_ANALISYS:;
	toml_close_buffer(buffer);

	return rescode;
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
 * @param document		Tomlドキュメント。
 * @param table		検索するテーブル。
 * @param key		検索するキー。
 * @return			取得した値。
 */
TomlValue toml_search_key(TomlDocument * document, TomlTable * table, const char * key)
{
	TomlDocumentImpl * impl = (TomlDocumentImpl*)document;
	const char * reg_key = 0;
	HashPair	result;

	if (*key == 0) {
		return empty_value;
	}
	else {
		reg_key = stringhash_add(impl->strings_cache, key);
		if (hash_contains(table->hash, &reg_key, &result)) {
			return *((TomlValue*)result.value.object);
		}
		else {
			return empty_value;
		}
	}
}
