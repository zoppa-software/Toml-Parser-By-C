#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../helper/Assertion.h"
#include "../helper/StringHash.h"
#include "../helper/Vec.h"
#include "TomlParseHelper.h"

/** 空文字。 */
static TomlUtf8		empty_char;

/**
 * 指定位置の文字を取得する。
 *
 * @param utf8s		文字列。
 * @param point		指定位置。
 * @return			取得した文字。
 */
TomlUtf8 toml_get_char(Vec * utf8s, size_t point)
{
	if (point < utf8s->length) {
		return VEC_GET(TomlUtf8, utf8s, point);
	}
	else {
		return empty_char;
	}
}

/**
 * 空白文字を読み飛ばす。
 *
 * @param impl	tomlドキュメント。
 * @param i		読み込み位置。
 * @return		読み飛ばした位置。
 */
size_t toml_skip_space(Vec * utf8s, size_t i)
{
	TomlUtf8	c;
	for (; i < utf8s->length; ++i) {
		c = toml_get_char(utf8s, i);
		if (c.num != '\t' && c.num != ' ') {
			break;
		}
	}
	return i;
}

/**
 * エラーメッセージを取得する。
 *
 * @param utf8s		UTF8文字列。
 * @param word_dst	変換バッファ。
 * @preturn			エラーメッセージ。
 */
const char * toml_err_message(Vec *	utf8s, Vec * word_dst)
{
	size_t		i, j;
	TomlUtf8	c;

	vec_clear(word_dst);

	for (i = 0; i < utf8s->length; ++i) {
		c = toml_get_char(utf8s, i);
		for (j = 0; j < UTF8_CHARCTOR_SIZE && c.ch[j] != 0; ++j) {
			vec_add(word_dst, c.ch + j);
		}
	}
	vec_add(word_dst, &empty_char);

	return (const char *)word_dst->pointer;
}

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
size_t toml_check_start_line_token(Vec * utf8s,
								   size_t ptr,
								   TomlLineType * tokenType)
{
	TomlUtf8	c1, c2;

	// テーブル配列の開始か判定する
	if (utf8s->length - ptr >= 2) {
		c1 = toml_get_char(utf8s, ptr);
		c2 = toml_get_char(utf8s, ptr + 1);
		if (c1.num == '[' && c2.num == '[') {
			*tokenType = TomlTableArrayLine;
			return ptr + 2;
		}
	}

	// その他の開始を判定する
	//
	// 1. テーブルの開始か判定する
	// 2. コメントの開始か判定する
	// 4. キーの開始か判定する
	// 5. それ以外はエラー
	if (utf8s->length - ptr > 0) {
		c1 = toml_get_char(utf8s, ptr);
		switch (c1.num)
		{
		case '[':			// 1
			*tokenType = TomlTableLine;
			return ptr + 1;

		case '#':			// 2
			*tokenType = TomlCommenntLine;
			return ptr + 1;

		case '\r':			// 3
		case '\n':
		case '\0':
			*tokenType = TomlLineNone;
			return ptr;

		default:			// 4
			*tokenType = TomlKeyValueLine;
			return ptr;
		}
	}
	else {					// 5
		*tokenType = TomlLineNone;
		return ptr;
	}
}

/**
 * 空白を読み飛ばし、次が改行／終端／コメントならば真を返す。
 *
 * @param utf8s		対象文字列。
 * @param i			検索開始位置。
 * @return			改行／終端／コメントならば 0以外。
 */
int toml_skip_linefield(Vec * utf8s, size_t i)
{
	TomlUtf8	c;

	// 空白を読み飛ばす
	i = toml_skip_space(utf8s, i);

	// 文字を判定
	c = toml_get_char(utf8s, i++);
	return (c.num == '#' || c.num == '\r' || c.num == '\n' || c.num == 0);
}

/**
 * ']' の後、次が改行／終端／コメントならば真を返す。
 *
 * @param utf8s		対象文字列。
 * @param i			検索開始位置。
 * @return			改行／終端／コメントならば 0以外。
 */
int toml_close_table(Vec * utf8s, size_t i)
{
	TomlUtf8	c;

	// 空白を読み飛ばす
	i = toml_skip_space(utf8s, i);

	// ] の判定
	c = toml_get_char(utf8s, i++);
	if (c.num != ']') {
		return 0;
	}

	// 空白を読み飛ばす
	i = toml_skip_space(utf8s, i);

	// 文字を判定
	c = toml_get_char(utf8s, i++);
	return (c.num == '#' || c.num == '\r' || c.num == '\n' || c.num == 0);
}

/**
 * ']]' の後、次が改行／終端／コメントならば真を返す。
 *
 * @param utf8s		対象文字列。
 * @param i			検索開始位置。
 * @return			改行／終端／コメントならば 0以外。
 */
int toml_close_table_array(Vec * utf8s, size_t i)
{
	TomlUtf8	c;

	// 空白を読み飛ばす
	i = toml_skip_space(utf8s, i);

	// ]] の判定
	c = toml_get_char(utf8s, i++);
	if (c.num != ']') {
		return 0;
	}

	c = toml_get_char(utf8s, i++);
	if (c.num != ']') {
		return 0;
	}

	// 空白を読み飛ばす
	i = toml_skip_space(utf8s, i);

	// 文字を判定
	c = toml_get_char(utf8s, i++);
	return (c.num == '#' || c.num == '\r' || c.num == '\n' || c.num == 0);
}