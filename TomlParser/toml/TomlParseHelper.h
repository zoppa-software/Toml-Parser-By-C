#ifndef __TOML_PARSE_HELPER_H__
#define __TOML_PARSE_HELPER_H__

/**
 * 内部処理用のヘッダ、ヘルパ関数。
 *
 * Copyright (c) 2018 Takashi Zota
 * Released under the MIT license
 * https://opensource.org/licenses/mit-license.php
 */

#include "Toml.h"

/**
 * 行のデータ種類。
 */
typedef enum _TomlLineType
{
	// 空行。
	TomlLineNone,

	// コメント行。
	TomlCommenntLine,

	// テーブル行。
	TomlTableLine,

	// テーブル配列行。
	TomlTableArrayLine,

	// キー／値行。
	TomlKeyValueLine,

} TomlLineType;

/**
 * 指定位置の文字を取得する。
 *
 * @param utf8s		文字列。
 * @param point		指定位置。
 * @return			取得した文字。
 */
TomlUtf8 toml_get_char(Vec * utf8s, size_t point);

/**
 * 空白文字を読み飛ばす。
 *
 * @param impl	tomlドキュメント。
 * @param i		読み込み位置。
 * @return		読み飛ばした位置。
 */
size_t toml_skip_space(Vec * utf8s, size_t i);

/**
 * エラーメッセージを取得する。
 *
 * @param utf8s		UTF8文字列。
 * @param word_dst	変換バッファ。
 * @preturn			エラーメッセージ。
 */
const char * toml_err_message(Vec *	utf8s, Vec * word_dst);

//-----------------------------------------------------------------------------
// 文字判定
//-----------------------------------------------------------------------------
/**
 * 行の開始を判定する。
 *
 * @param utf8s		対象文字列。
 * @param i			読み込み位置。
 * @param tokenType	行のデータ種類。
 * @return			読み飛ばした位置。
 */
size_t toml_check_start_line_token(Vec * utf8s, size_t i, TomlLineType * tokenType);

/**
 * 空白を読み飛ばし、次が改行／終端／コメントならば真を返す。
 *
 * @param utf8s		対象文字列。
 * @param i			検索開始位置。
 * @return			改行／終端／コメントならば 0以外。
 */
int toml_skip_linefield(Vec * utf8s, size_t i);

/**
 * ']' の後、次が改行／終端／コメントならば真を返す。
 *
 * @param utf8s		対象文字列。
 * @param i			検索開始位置。
 * @return			改行／終端／コメントならば 0以外。
 */
int toml_close_table(Vec * utf8s, size_t i);

/**
 * ']]' の後、次が改行／終端／コメントならば真を返す。
 *
 * @param utf8s		対象文字列。
 * @param i			検索開始位置。
 * @return			改行／終端／コメントならば 0以外。
 */
int toml_close_table_array(Vec * utf8s, size_t i);

#endif