#ifndef __TOML_H__
#define __TOML_H__

#include "../helper/Hash.h"
#include "../helper/Vec.h"

/**
 * 値の種類を表す列挙。
 */
typedef enum _TomlValueType
{
	// 空値
	TomlNoneValue,

	// 真偽値
	TomlBooleanValue,

	// 文字列
	TomlStringValue,

	// 整数値
	TomlIntegerValue,

	// 実数値
	TomlFloatValue,

	// 日付
	TomlDateValue,

	// テーブル
	TomlTableValue,

	// テーブル配列
	TomlTableArrayValue,

} TomlValueType;

/**
 * 読込結果列挙体
 */
typedef enum _TomlResult
{
	// 成功
	SUCCESS,

	// 失敗
	FAIL,

	// ファイルオープンに失敗
	FAIL_OPEN_ERR,

	// キー文字列の解析に失敗
	KEY_ANALISYS_ERR,

	// 値の解析に失敗
	KEY_VALUE_ERR,

	// テーブル名解決エラー
	TABLE_NAME_ANALISYS_ERR,

	// テーブル名再定義エラー
	TABLE_REDEFINITION_ERR,

	// テーブル構文エラー
	TABLE_SYNTAX_ERR,

	// テーブル配列構文エラー
	TABLE_ARRAY_SYNTAX_ERR,

} TomlResult;

/**
 * テーブル。
 */
typedef struct _TomlTable
{
	// キー／値ハッシュ
	const Hash *	hash;

} TomlTable;

/**
 * テーブル配列。
 */
typedef struct _TomlTableArray
{
	// テーブル
	Vec *			tables;

} TomlTableArray;

/**
 * キー／値構造体。
 */
typedef struct _TomlPair
{
	// 値の種類
	TomlValueType		value_type;

	// 値参照
	// 1. 真偽値
	// 2. 文字列
	// 3. テーブル
	// 4. テーブル配列
	union {
		int					boolean;	// 1
		const char *		string;		// 2
		TomlTable *			table;		// 3
		TomlTableArray *	tbl_array;	// 4
	} value;

} TomlPair;

/**
 * Tomlドキュメント。
 */
typedef struct TomlDocument
{
	// キー／値ハッシュ
	TomlTable *		table;

} TomlDocument;

//-----------------------------------------------------------------------------
// コンストラクタ、デストラクタ
//-----------------------------------------------------------------------------
/**
 * Tomlドキュメントを生成する。
 *
 * @return	Tomlドキュメント。
 */
TomlDocument * toml_initialize();

/**
 * Tomlドキュメントを解放する。
 *
 * @param document	解放する Tomlドキュメント。
 */
void toml_dispose(TomlDocument ** document);

//-----------------------------------------------------------------------------
// 読込
//-----------------------------------------------------------------------------
///**
// * Tomlドキュメントとして 1行読み込む。
// *
// * @param document		Tomlドキュメント。
// * @param input			読込文字列。
// * @param input_len		読込文字列長。
// * @return				読込結果。
// */
//TomlReadState toml_readline(TomlDocument * document, const char * input, size_t input_len);
//
///**
// * Tomlドキュメントの読み残しを読み込む。
// *
// * @param document		Tomlドキュメント。
// * @param input			読込文字列。
// * @param input_len		読込文字列長。
// * @return				読込結果。
// */
//TomlReadState toml_appendline(TomlDocument * document, const char * input, size_t input_len);

TomlResult toml_read(TomlDocument * document, const char * path);

void toml_show(TomlDocument * document);

#endif /*  __TOML_H__ */