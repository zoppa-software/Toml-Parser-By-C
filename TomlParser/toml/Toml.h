#ifndef __TOML_H__
#define __TOML_H__

#include "../helper/Hash.h"
#include "../helper/Vec.h"

/** UTF8の文字数。 */
#define UTF8_CHARCTOR_SIZE	6

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
typedef enum _TomlResultCode
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

	// 複数クォーテーション文字列定義エラー
	MULTI_QUOAT_STRING_ERR,

	// 複数リテラル文字列定義エラー
	MULTI_LITERAL_STRING_ERR,

	// 文字列定義エラー
	QUOAT_STRING_ERR,

	// リテラル文字列定義エラー
	LITERAL_STRING_ERR,

	// 整数値の範囲エラー
	INTEGER_VALUE_RANGE_ERR,

	// 数値定義に連続してアンダーバーが使用された
	UNDERBAR_CONTINUE_ERR,

	// 数値の先頭に無効な 0がある
	ZERO_NUMBER_ERR,

	// キーが再定義された
	DEFINED_KEY_ERR,

	// 無効な名称が指定された
	INVALID_NAME_ERR,

	// 複数の小数点が定義された
	MULTI_DECIMAL_ERR,

	// 実数の定義エラー
	DOUBLE_VALUE_ERR,

	// 実数値の範囲エラー
	DOUBLE_VALUE_RANGE_ERR,

	// 日付書式の設定エラー
	DATE_FORMAT_ERR,

	// ユニコード文字定義エラー
	UNICODE_DEFINE_ERR,

	// 無効なエスケープ文字が指定された
	INVALID_ESCAPE_CHAR_ERR,

	// インラインテーブルが正しく閉じられていない
	INLINE_TABLE_NOT_CLOSE_ERR,

} TomlResultCode;

/**
 * 日付形式情報。
 */
typedef struct _TomlDate
{
	unsigned short		year;			// 年
	unsigned char		month;			// 月
	unsigned char		day;			// 日
	unsigned char		hour;			// 時
	unsigned char		minute;			// 分
	unsigned char		second;			// 秒
	unsigned int		dec_second;		// 秒の小数部
	char				zone_hour;		// 時差（時間）
	unsigned char		zone_minute;	// 時差（分）

} TomlDate;

/**
 * UTF8文字。
 */
typedef union _TomlUtf8
{
	// 文字バイト
	char	ch[UTF8_CHARCTOR_SIZE];

	// 数値
	unsigned int	num;

} TomlUtf8;

/**
 * 読み込み結果詳細。
 */
typedef struct _TomlResultSummary
{
	TomlResultCode	code;	// 読み込み結果コード
	Vec *	utf8s;			// UTF8文字列
	Vec *	word_dst;		// 文字列バッファ
	size_t	column;			// 列位置
	size_t	row;			// 行位置

} TomlResultSummary;

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
	union {
		int					boolean;		// 真偽値
		long long int		integer;		// 整数値
		double				float_number;	// 実数値
		const char *		string;			// 文字列
		TomlTable *			table;			// テーブル
		TomlTableArray *	tbl_array;		// テーブル配列
		TomlDate *			date;			// 日付
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
/**
 * TOML形式のファイルを読み込む。
 *
 * @param document		Tomlドキュメント。
 * @param path			ファイルパス。
 * @return				読込結果。
 */
TomlResultCode toml_read(TomlDocument * document, const char * path);

void toml_show(TomlDocument * document);

#endif /*  __TOML_H__ */