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
	TomlTable *		table;

	// テーブルキャッシュ
	Instance	table_cache;

	// テーブルキャッシュ（解放用）
	Vec *		table_list;

	// テーブル配列キャッシュ
	Instance	tblarray_cache;

	// テーブル配列キャッシュ（解放用）
	Vec *		tblarray_list;

	// 配列キャッシュ
	Instance	array_cache;

	// 配列キャッシュ（解放用）
	Vec *		array_list;

	// カレントテーブル
	TomlTable *		root;

	// 文字列テーブル
	const StringHash *	strings_cache;

	// キー／値インスタンス
	Instance	pair_cache;

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
						  TomlPair ** res_pair,
						  TomlResultSummary * error);

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
	vec_add(impl->table_list, &res);
	return res;
}

/**
 * テーブルキャッシュにテーブルを返す。
 *
 * @param impl		Tomlドキュメント。
 * @param table		削除するテーブル。
 */
static void delete_table(TomlDocumentImpl * impl, TomlTable * table)
{
	size_t		i;
	for (i = impl->table_list->length - 1; i >= 0 && i < impl->table_list->length; --i) {
		if (VEC_GET(TomlTable*, impl->table_list, i) == table) {
			vec_remove_at(impl->table_list, i);
			break;
		}
	}
	hash_delete(&table->hash);
	instance_push(&impl->table_cache, table);
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
	vec_add(impl->tblarray_list, &res);
	return res;
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
	res->array = vec_initialize(sizeof(TomlPair*));
	vec_add(impl->array_list, &res);
	return res;
}

/**
 * 配列キャッシュに配列を返す。
 *
 * @param impl		Tomlドキュメント。
 * @param array		削除する配列。
 */
static void delete_array(TomlDocumentImpl * impl, TomlArray * array)
{
	size_t		i;
	for (i = impl->array_list->length - 1; i >= 0 && i < impl->array_list->length; --i) {
		if (VEC_GET(TomlArray*, impl->array_list, i) == array) {
			vec_remove_at(impl->array_list, i);
			break;
		}
	}
	vec_delete(&array->array);
	instance_push(&impl->array_cache, array);
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
	res->table_list = vec_initialize_set_capacity(sizeof(TomlTable*), TABLE_BLOCK_SIZE);
	res->table = create_table(res);
	res->root = res->table;

	res->tblarray_cache = instance_initialize(sizeof(TomlTableArray), TABLE_BLOCK_SIZE);
	res->tblarray_list = vec_initialize_set_capacity(sizeof(TomlTableArray*), TABLE_BLOCK_SIZE);

	res->array_cache = instance_initialize(sizeof(TomlArray), TABLE_BLOCK_SIZE);
	res->array_list = vec_initialize_set_capacity(sizeof(TomlArray*), TABLE_BLOCK_SIZE);

	res->date_cache = instance_initialize(sizeof(TomlDate), TABLE_BLOCK_SIZE);

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

	for (i = 0; i < impl->array_list->length; ++i) {
		TomlArray * tbl = VEC_GET(TomlArray*, impl->array_list, i);
		vec_delete(&tbl->array);
	}
	vec_delete(&impl->array_list);
	instance_delete(&impl->array_cache);

	instance_delete(&impl->date_cache);

	stringhash_delete(&impl->strings_cache);
	instance_delete(&impl->pair_cache);
	
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
			delete_table(impl, table);
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
			delete_table(impl, table);
			return 0;
		}
	}

	// インラインテーブルが閉じられていない
	delete_table(impl, table);
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
		type = VEC_GET(TomlPair*, array->array, 0)->value_type;

		for (i = 1; i < array->array->length; ++i) {
			if (type != VEC_GET(TomlPair*, array->array, i)->value_type) {
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
	// ※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※
	// ※　todo:リソース返し(pair)、TomlPairを使うべきかどうか
	// ※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※※
	TomlArray *	array = create_array(impl);
	size_t		ptr = point;
	TomlPair *	pair;

	while (ptr < buffer->utf8s->length) {
		// 改行、空白部を取り除く
		ptr = toml_skip_linefield_and_space(buffer, ptr);

		// 値を取り込む
		if (!analisys_value(impl, buffer, ptr, next_point, &pair, error)) {
			delete_array(impl, array);
			return 0;
		}

		// 空値以外は取り込む
		if (pair->value_type != TomlNoneValue) {
			vec_add(array->array, &pair);
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
			if (pair->value_type != TomlNoneValue) {
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
 * @param res_pair		取得した値（戻り値）
 * @param error			エラー詳細情報。
 * @return				正常に読み込めたならば 0以外。
 */
static int analisys_value(TomlDocumentImpl * impl,
						  TomlBuffer * buffer,
						  size_t point,
						  size_t * next_point,
						  TomlPair ** res_pair,
						  TomlResultSummary * error)
{
	TomlValueType		type;
	TomlPair *			pair;
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
	pair = instance_pop(&impl->pair_cache);

	// 値を設定
	switch (type) {
	case TomlBooleanValue:			// 真偽値
		pair->value_type = TomlBooleanValue;
		pair->value.boolean = buffer->boolean;
		break;

	case TomlStringValue:			// 文字列値
		pair->value_type = TomlStringValue;
		pair->value.string = regist_string(impl, buffer);
		break;

	case TomlIntegerValue:			// 整数値
		pair->value_type = TomlIntegerValue;
		pair->value.integer = buffer->integer;
		break;

	case TomlFloatValue:			// 実数値
		pair->value_type = TomlFloatValue;
		pair->value.float_number = buffer->float_number;
		break;

	case TomlDateValue:				// 日付
		pair->value_type = TomlDateValue;
		pair->value.date = (TomlDate*)instance_pop(&impl->date_cache);
		*pair->value.date = buffer->date;
		break;

	case TomlTableValue:			// インラインテーブル
		pair->value_type = TomlTableValue;
		pair->value.table = buffer->table;
		break;

	case TomlArrayValue:			// 配列
		pair->value_type = TomlArrayValue;
		pair->value.array = buffer->array;
		break;

	default:						// 無効値
		pair->value_type = TomlNoneValue;
		break;
	}

	*res_pair = pair;
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
	TomlPair *			pair;
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
	if (analisys_value(impl, buffer, ptr + 1, next_point, &pair, &res)) {	// 1
		if (pair->value_type == TomlNoneValue) {
			res.code = KEY_VALUE_ERR;
			res.column = ptr + 1;
			res.row = buffer->loaded_line;
		}
		else if (!hash_contains(table->hash, &key_str, &val_pair)) {		// 2
			hash_add(table->hash, &key_str, hash_value_of_pointer(pair));
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
	TomlPair *		pair;

	if (hash_contains(table->hash, &key_str, &result)) {
		// 指定のキーが登録済みならば、紐付く項目を返す
		//
		// 1. テーブルを返す
		// 2. テーブル配列のテーブルを返す
		// 3. テーブルの新規作成を依頼
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
	TomlPair *			pair;
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
			pair = instance_pop(&impl->pair_cache);
			pair->value_type = TomlTableValue;
			pair->value.table = new_table;
			hash_add(cur_table->hash, &key_str, hash_value_of_pointer(pair));
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
	
	// 戻り値を作成
	res.code = SUCCESS;
	res.column = 0;
	res.row = buffer->loaded_line;
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
											  TomlTable * table,
											  TomlBuffer * buffer,
											  size_t point,
											  size_t * next_point)
{
	TomlUtf8			c;
	size_t				i, j, nxt;
	TomlTable *			cur_table = table;
	TomlTable *			new_table;
	TomlPair *			pair;
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
			pair = instance_pop(&impl->pair_cache);
			pair->value_type = TomlTableValue;
			pair->value.table = new_table;
			hash_add(cur_table->hash, &key_str, hash_value_of_pointer(pair));
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
		pair = (TomlPair*)hash_pair.value.object;
		if (pair->value_type == TomlTableArrayValue) {					// 2-2
			new_table = create_table(impl);
			vec_add(pair->value.tbl_array->tables, &new_table);
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
		pair = instance_pop(&impl->pair_cache);
		pair->value_type = TomlTableArrayValue;
		pair->value.tbl_array = new_array;
		hash_add(cur_table->hash, &key_str, hash_value_of_pointer(pair));

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
// デバッグ出力
//-----------------------------------------------------------------------------
static void toml_show_table(TomlTable * table);

static void toml_show_pair(TomlPair * obj)
{
	size_t	i;
	TomlDate * date;

	switch (obj->value_type)
	{
	case TomlBooleanValue:
		printf_s("%s", obj->value.boolean ? "true" : "false");
		break;
	case TomlStringValue:
		printf_s("%s", obj->value.string);
		break;
	case TomlIntegerValue:
		printf_s("%lld", obj->value.integer);
		break;
	case TomlFloatValue:
		printf_s("%g", obj->value.float_number);
		break;
	case TomlDateValue:
		date = obj->value.date;
		printf_s("%d-%d-%d %d:%d:%d.%d %d:%d",
				 date->year, date->month, date->day,
				 date->hour, date->minute, date->second, date->dec_second,
				 date->zone_hour, date->zone_minute);
		break;
	case TomlTableValue:
		printf_s("{\n");
		toml_show_table(obj->value.table);
		printf_s("}\n");
		break;
	case TomlTableArrayValue:
		printf_s("[\n");
		for (i = 0; i < obj->value.tbl_array->tables->length; ++i) {
			printf_s("{\n");
			toml_show_table(VEC_GET(TomlTable*, obj->value.tbl_array->tables, i));
			printf_s("}\n");
		}
		printf_s("]\n");
		break;
	case TomlArrayValue:
		printf_s("[");
		for (i = 0; i < obj->value.array->array->length; ++i) {
			toml_show_pair(VEC_GET(TomlPair*, obj->value.array->array, i));
			printf_s(", ");
		}
		printf_s("]\n");
		break;
	default:
		break;
	}
}

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
	case TomlArrayValue:
		printf_s("%s: [\n", *(char**)pair.key);
		for (i = 0; i < obj->value.array->array->length; ++i) {
			toml_show_pair(VEC_GET(TomlPair*, obj->value.array->array, i));
			printf_s(", ");
		}
		printf_s("\n]\n");
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
