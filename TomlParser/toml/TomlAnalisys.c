#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crtdbg.h>
#include "../helper/Assertion.h"
#include "../helper/StringHash.h"
#include "../helper/Vec.h"
#include "Toml.h"
#include "TomlBuffer.h"
#include "TomlAnalisys.h"

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

/**
 * 1バイト文字を追加する。
 *
 * @param word_dst	文字バッファ。
 * @param add_c		追加する 1文字。
 */
static void toml_append_char(Vec * word_dst, unsigned char add_c)
{
	vec_add(word_dst, &add_c);
}

/**
 * UNICODEエスケープ判定。
 *
 * @param buffer	読み込み領域。
 * @param ptr		読み込み開始位置。
 * @param len		読み込む文字数。
 * @return			変換に成功したら 0以外。
 */
static int toml_append_unicode(TomlBuffer * buffer, size_t ptr, size_t len)
{
	unsigned int	val = 0;
	size_t	i;
	TomlUtf8	c;
	unsigned char	v;

	// 16進数文字を判定し、数値化
	for (i = 0; i < len; ++i) {
		c = toml_get_char(buffer->utf8s, ptr + i);
		val <<= 4;

		if (c.num >= '0' && c.num <= '9') {
			val |= c.num - '0';
		}
		else if (c.num >= 'a' && c.num <= 'f') {
			val |= 10 + c.num - 'a';
		}
		else if (c.num >= 'A' && c.num <= 'F') {
			val |= 10 + c.num - 'A';
		}
		else {
			buffer->err_point = i;
			return 0;
		}
	}

	// 数値化した値を文字として追加する
	if (len == 4) {
		v = (val >> 8) & 0xff;
		toml_append_char(buffer->word_dst, v);
		v = val & 0xff;
		toml_append_char(buffer->word_dst, v);
	}
	else {
		v = (val >> 24) & 0xff;
		toml_append_char(buffer->word_dst, v);
		v = (val >> 16) & 0xff;
		toml_append_char(buffer->word_dst, v);
		v = (val >> 8) & 0xff;
		toml_append_char(buffer->word_dst, v);
		v = val & 0xff;
		toml_append_char(buffer->word_dst, v);
	}
	return 1;
}

/**
 * キー文字列の取得。
 *
 * @param buffer	読み込み領域。
 * @param point		開始位置。
 * @param next_point	終了位置（戻り値）
 * @return			取得できたら 0以外。
 */
int toml_get_key(TomlBuffer * buffer, size_t point, size_t * next_point)
{
	size_t		i;
	TomlUtf8	c;

	// 文字列バッファを消去
	vec_clear(buffer->word_dst);

	// 空白を読み飛ばす
	i = toml_skip_space(buffer->utf8s, point);

	// 範囲の開始、終了位置で評価
	//
	// 1. 一文字取得する
	// 2. キー使用可能文字か判定する
	// 3. " なら文字列としてキー文字列を取得する
	for (; i < buffer->utf8s->length; ++i) {
		c = toml_get_char(buffer->utf8s, i);	// 1

		if ((c.num >= 'a' && c.num <= 'z') ||	// 2
			(c.num >= 'A' && c.num <= 'Z') ||
			(c.num >= '0' && c.num <= '9') ||
			c.num == '_' || c.num == '-') {
			vec_add(buffer->word_dst, c.ch);
			*next_point = i + 1;
		}
		else if (c.num == '"' &&				// 3
				 buffer->word_dst->length <= 0) {
			if (!toml_get_string_value(buffer, i + 1, next_point)) {
				return 0;
			}
			break;
		}
		else if (c.num == '\r' || c.num == '\n') {
			break;
		}
		else {
			buffer->err_point = i;
			return 1;
		}
	}

	// 終端文字を追加して終了
	toml_append_char(buffer->word_dst, '\0');
	return (buffer->word_dst->length > 1);
}

/**
 * " で囲まれた文字列を取得する。
 *
 * @param buffer		読み込み領域。
 * @param start			開始位置。
 * @param next_point	終了位置（戻り値）
 * @return				取得できたら 0以外。
 */
int toml_get_string_value(TomlBuffer * buffer, size_t start, size_t * next_point)
{
	size_t	i, j;
	TomlUtf8	c, nc;
	char	empty = 0;

	// 文字列バッファを消去
	vec_clear(buffer->word_dst);

	for (i = start; i < buffer->utf8s->length; ++i) {
		// 一文字取得する
		c = toml_get_char(buffer->utf8s, i);

		switch (c.num) {
		case '"':
			// " を取得したら文字列終了
			vec_add(buffer->word_dst, &empty);
			*next_point = i + 1;
			return (buffer->word_dst->length > 1);

		case '\\':
			// \のエスケープ文字判定
			if (i < buffer->utf8s->length - 1) {
				nc = toml_get_char(buffer->utf8s, i + 1);
				switch (nc.num)
				{
				case 'b':
					toml_append_char(buffer->word_dst, '\b');	i++;
					break;
				case 't':
					toml_append_char(buffer->word_dst, '\t');	i++;
					break;
				case 'n':
					toml_append_char(buffer->word_dst, '\n');	i++;
					break;
				case 'f':
					toml_append_char(buffer->word_dst, '\f');	i++;
					break;
				case 'r':
					toml_append_char(buffer->word_dst, '\r');	i++;
					break;
				case '"':
					toml_append_char(buffer->word_dst, '"');	i++;
					break;
				case '/':
					toml_append_char(buffer->word_dst, '/');	i++;
					break;
				case '\\':
					toml_append_char(buffer->word_dst, '\\');	i++;
					break;
				case 'u':
					if (i + 5 < buffer->utf8s->length &&
						!toml_append_unicode(buffer, i + 2, 4)) {
						return 0;
					}
					i += 5;
					break;
				case 'U':
					if (i + 9 < buffer->utf8s->length &&
						!toml_append_unicode(buffer, i + 2, 8)) {
						return 0;
					}
					i += 9;
					break;

				default:
					return 0;
				}
			}

		default:
			// 上記以外は普通の文字として取得
			for (j = 0; j < UTF8_CHARCTOR_SIZE && c.ch[j] != 0; ++j) {
				vec_add(buffer->word_dst, c.ch + j);
			}
			break;
		}
	}

	// " で終了できなかったためエラー
	buffer->err_point = buffer->utf8s->length - 1;
	return 0;
}

/**
 * 数値（整数／実数／日付）を取得する。
 *
 * @param buffer		読み込み領域。
 * @param start			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param tokenType		取得データ。
 * @return				取得できたら 0以外。
 */
int toml_get_number_value(TomlBuffer * buffer, size_t point, size_t * next_point, TomlValueType * tokenType)
{

}

int toml_convert_value(TomlBuffer * buffer, size_t point, size_t * next_point, TomlValueType * tokenType)
{
	TomlValueType	res;
	TomlUtf8		c;

	// 真偽値（偽）を返す
	if (buffer->utf8s->length - point >= 5) {
		if (toml_get_char(buffer->utf8s, point + 4).num == 'e' &&
			toml_get_char(buffer->utf8s, point + 3).num == 's' &&
			toml_get_char(buffer->utf8s, point + 2).num == 'l' &&
			toml_get_char(buffer->utf8s, point + 1).num == 'a' &&
			toml_get_char(buffer->utf8s, point).num == 'f') {
			buffer->boolean = 0;
			*next_point = point + 5;
			*tokenType = TomlBooleanValue;
			return 1;
		}
	}

	// 真偽値（真）を返す
	if (buffer->utf8s->length - point >= 4) {
		if (toml_get_char(buffer->utf8s, point + 3).num == 'e' &&
			toml_get_char(buffer->utf8s, point + 2).num == 'u' &&
			toml_get_char(buffer->utf8s, point + 1).num == 'r' &&
			toml_get_char(buffer->utf8s, point).num == 't') {
			buffer->boolean = 1;
			*next_point = point + 4;
			*tokenType = TomlBooleanValue;
			return 1;
		}
	}

	if (buffer->utf8s->length - point >= 3) {
		if (toml_get_char(buffer->utf8s, point + 2).num == '"' &&
			toml_get_char(buffer->utf8s, point + 1).num == '"' &&
			toml_get_char(buffer->utf8s, point).num == '"') {
			//return StringMultiValue;
			return 1;
		}

		if (toml_get_char(buffer->utf8s, point + 2).num == '\'' &&
			toml_get_char(buffer->utf8s, point + 1).num == '\'' &&
			toml_get_char(buffer->utf8s, point).num == '\'') {
			//return StringMultiLiteral;
			return 1;
		}
	}

	if (buffer->utf8s->length - point > 0) {
		c = toml_get_char(buffer->utf8s, point);
		switch (c.num)
		{
		case '"':
			if (toml_get_string_value(buffer, point + 1, next_point)) {
				*tokenType = TomlStringValue;
				return 1;
			}

		case '\'':
			*tokenType = TomlStringValue;
			return 1;

		case '#':
			*next_point = point + 1;
			return 1;

		case '+':
			if (toml_get_number_value(buffer, point + 1, next_point, tokenType)) {
				return 1;
			}
			else {
				return 0;
			}

		case '-':
			if (toml_get_number_value(buffer, point + 1, next_point, tokenType)) {
				//if (res == TomlInteger) {
				//	buffer->integer = -buffer->integer;
				//	return IntegerValue;
				//}
				//else if (res == TomlFloat) {
				//	buffer->float_number = -buffer->float_number;
				//	return FloatValue;
				//}
				//else {
				//	return ErrorToken;
				//}
			}
			else {
				return 0;
			}

		default:
			return toml_get_number_value(buffer, point, next_point, tokenType);
		}
	}
	else {
		return 0;
	}
}
