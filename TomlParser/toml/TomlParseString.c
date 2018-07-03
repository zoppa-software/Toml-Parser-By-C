#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <crtdbg.h>
#include "../helper/Assertion.h"
#include "../helper/StringHash.h"
#include "../helper/Vec.h"
#include "Toml.h"
#include "TomlParseHelper.h"
#include "TomlBuffer.h"
#include "TomlParseString.h"

//-----------------------------------------------------------------------------
// 文字解析
//-----------------------------------------------------------------------------
/**
 * 1バイト文字を追加する。
 *
 * @param word_dst	文字バッファ。
 * @param add_c		追加する 1文字。
 */
static void append_char(Vec * word_dst,
						unsigned char add_c)
{
	vec_add(word_dst, &add_c);
}

/**
 * UNICODEエスケープ判定。
 *
 * @param buffer	読み込み領域。
 * @param ptr		読み込み開始位置。
 * @param len		読み込む文字数。
 * @param error		エラー詳細情報。
 * @return			変換に成功したら 0以外。
 */
static int append_unicode(TomlBuffer * buffer,
						  size_t ptr,
						  size_t len,
						  TomlResultSummary * error)
{
	unsigned int	val = 0;
	size_t			i;
	TomlUtf8		c;
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
			error->code = UNICODE_DEFINE_ERR;
			error->column = i;
			error->row = buffer->loaded_line;
			return 0;
		}
	}

	// UTF8へ変換
	if (val < 0x80) {
		v = val & 0xff;
		append_char(buffer->word_dst, v);
	}
	else if (val < 0x800) {
		v = (unsigned char)(0xc0 | (val >> 6));
		append_char(buffer->word_dst, v);
		v = 0x80 | (val & 0x3f);
		append_char(buffer->word_dst, v);
	}
	else if (val < 0x10000) {
		v = (unsigned char)(0xe0 | (val >> 12));
		append_char(buffer->word_dst, v);
		v = 0x80 | ((val >> 6) & 0x3f);
		append_char(buffer->word_dst, v);
		v = 0x80 | (val & 0x3f);
		append_char(buffer->word_dst, v);
	}
	else if (val < 0x200000) {
		v = (unsigned char)(0xf0 | (val >> 18));
		append_char(buffer->word_dst, v);
		v = 0x80 | ((val >> 12) & 0x3f);
		append_char(buffer->word_dst, v);
		v = 0x80 | ((val >> 6) & 0x3f);
		append_char(buffer->word_dst, v);
		v = 0x80 | (val & 0x3f);
		append_char(buffer->word_dst, v);
	}
	else if (val < 0x4000000) {
		v = 0xf8 | (val >> 24);
		append_char(buffer->word_dst, v);
		v = 0x80 | ((val >> 18) & 0x3f);
		append_char(buffer->word_dst, v);
		v = 0x80 | ((val >> 12) & 0x3f);
		append_char(buffer->word_dst, v);
		v = 0x80 | ((val >> 6) & 0x3f);
		append_char(buffer->word_dst, v);
		v = 0x80 | (val & 0x3f);
		append_char(buffer->word_dst, v);
	}
	else {
		v = 0xfc | (val >> 30);
		append_char(buffer->word_dst, v);
		v = 0x80 | ((val >> 24) & 0x3f);
		append_char(buffer->word_dst, v);
		v = 0x80 | ((val >> 18) & 0x3f);
		append_char(buffer->word_dst, v);
		v = 0x80 | ((val >> 12) & 0x3f);
		append_char(buffer->word_dst, v);
		v = 0x80 | ((val >> 6) & 0x3f);
		append_char(buffer->word_dst, v);
		v = 0x80 | (val & 0x3f);
		append_char(buffer->word_dst, v);
	}
	return 1;
}

/**
 * '\'エスケープ文字を取得する。
 *
 * @param buffer	読み込み領域。
 * @param ptr		開始位置。
 * @param error		エラー詳細情報。
 * @return			取得できたら 0以外。
 */
static int append_escape_char(TomlBuffer * buffer,
							  size_t * ptr,
							  TomlResultSummary * error)
{
	size_t		i = *ptr;
	TomlUtf8	c;

	if (i < buffer->utf8s->length - 1) {
		c = toml_get_char(buffer->utf8s, i + 1);

		switch (c.num) {
		case 'b':
			append_char(buffer->word_dst, '\b');	i++;
			break;
		case 't':
			append_char(buffer->word_dst, '\t');	i++;
			break;
		case 'n':
			append_char(buffer->word_dst, '\n');	i++;
			break;
		case 'f':
			append_char(buffer->word_dst, '\f');	i++;
			break;
		case 'r':
			append_char(buffer->word_dst, '\r');	i++;
			break;
		case '"':
			append_char(buffer->word_dst, '"');	i++;
			break;
		case '/':
			append_char(buffer->word_dst, '/');	i++;
			break;
		case '\\':
			append_char(buffer->word_dst, '\\');	i++;
			break;
		case 'u':
			if (i + 5 < buffer->utf8s->length &&
				!append_unicode(buffer, i + 2, 4, error)) {
				return 0;
			}
			i += 5;
			break;
		case 'U':
			if (i + 9 < buffer->utf8s->length &&
				!append_unicode(buffer, i + 2, 8, error)) {
				return 0;
			}
			i += 9;
			break;
		default:
			error->code = INVALID_ESCAPE_CHAR_ERR;
			error->column = i;
			error->row = buffer->loaded_line;
			return 0;
		}
	}
	*ptr = i;
	return 1;
}

/**
 * 先頭の改行を取り除く。
 *
 * @param buffer		読み込み領域。
 * @param start			開始位置。
 * @return				読み飛ばした位置。
 */
static size_t remove_head_linefield(TomlBuffer * buffer,
									size_t start)
{
	size_t i = start;
	if (buffer->utf8s->length - i >= 2 &&
		toml_get_char(buffer->utf8s, i).num == '\r' &&
		toml_get_char(buffer->utf8s, i + 1).num == '\n') {
		i += 2;
	}
	if (buffer->utf8s->length - i >= 1 &&
		toml_get_char(buffer->utf8s, i).num == '\n') {
		i += 1;
	}
	return i;
}

/**
 * キー文字列の取得。
 *
 * @param buffer		読み込み領域。
 * @param point			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param error			エラー詳細情報（戻り値）
 * @return				取得できたら 0以外。
 */
int toml_get_key(TomlBuffer * buffer,
				 size_t point,
				 size_t * next_point,
				 TomlResultSummary * error)
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
			if (!toml_get_string_value(buffer, i + 1, next_point, error) ||
				buffer->word_dst->length < 1) {
				error->code = KEY_ANALISYS_ERR;
				error->column = i;
				error->row = buffer->loaded_line;
				return 0;
			}
			break;
		}
		else {
			break;
		}
	}

	// 終端文字を追加して終了
	append_char(buffer->word_dst, '\0');
	if (buffer->word_dst->length > 1) {
		return 1;
	}
	else {
		error->code = KEY_ANALISYS_ERR;
		error->column = i;
		error->row = buffer->loaded_line;
		return 0;
	}
}

/**
 * " で囲まれた文字列を取得する。
 *
 * @param buffer		読み込み領域。
 * @param start			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param error			エラー詳細情報（戻り値）
 * @return				取得できたら 0以外。
 */
int toml_get_string_value(TomlBuffer * buffer,
						  size_t start,
						  size_t * next_point,
						  TomlResultSummary * error)
{
	size_t		i, j;
	TomlUtf8	c;

	// 文字列バッファを消去
	vec_clear(buffer->word_dst);

	for (i = start; i < buffer->utf8s->length; ++i) {
		// 一文字取得する
		c = toml_get_char(buffer->utf8s, i);

		switch (c.num) {
		case '"':
			// " を取得したら文字列終了
			append_char(buffer->word_dst, '\0');
			*next_point = i + 1;
			return 1;

		case '\\':
			// \のエスケープ文字判定
			if (!append_escape_char(buffer, &i, error)) {
				*next_point = i;
				return 0;
			}
			break;

		default:
			// 上記以外は普通の文字として取得
			for (j = 0; j < UTF8_CHARCTOR_SIZE && c.ch[j] != 0; ++j) {
				vec_add(buffer->word_dst, c.ch + j);
			}
			break;
		}
	}

	// " で終了できなかったためエラー
	error->code = QUOAT_STRING_ERR;
	error->column = i;
	error->row = buffer->loaded_line;
	*next_point = i;
	return 0;
}


/**
 * ' で囲まれた文字列を取得する。
 *
 * @param buffer		読み込み領域。
 * @param start			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param error			エラー詳細情報（戻り値）
 * @return				取得できたら 0以外。
 */
int get_literal_string_value(TomlBuffer * buffer,
							 size_t start,
							 size_t * next_point,
							 TomlResultSummary * error)
{
	size_t		i, j;
	TomlUtf8	c;

	// 文字列バッファを消去
	vec_clear(buffer->word_dst);

	for (i = start; i < buffer->utf8s->length; ++i) {
		// 一文字取得する
		c = toml_get_char(buffer->utf8s, i);

		switch (c.num) {
		case '\'':
			// ' を取得したら文字列終了
			append_char(buffer->word_dst, '\0');
			*next_point = i + 1;
			return 1;

		default:
			// 上記以外は普通の文字として取得
			for (j = 0; j < UTF8_CHARCTOR_SIZE && c.ch[j] != 0; ++j) {
				vec_add(buffer->word_dst, c.ch + j);
			}
			break;
		}
	}

	// ' で終了できなかったためエラー
	error->code = LITERAL_STRING_ERR;
	error->column = i;
	error->row = buffer->loaded_line;
	*next_point = i;
	return 0;
}

/**
 * """ で囲まれた文字列を取得する。
 *
 * @param buffer		読み込み領域。
 * @param start			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param error			エラー詳細情報（戻り値）
 * @return				取得できたら 0以外。
 */
int get_multi_string_value(TomlBuffer * buffer,
						   size_t start,
						   size_t * next_point,
						   TomlResultSummary * error)
{
	size_t		i, j;
	TomlUtf8	c;
	char		last_c[2];
	int			skip_space = 0;
	unsigned int	nx_c;
	int			eof;

	// 文字列バッファを消去
	vec_clear(buffer->word_dst);
	memset(last_c, 0, sizeof(last_c));

	// 先頭の改行は取り除く
	i = remove_head_linefield(buffer, start);

	do {
		eof = toml_end_of_file(buffer);

		for (; i < buffer->utf8s->length; ++i) {
			// 一文字取得する
			c = toml_get_char(buffer->utf8s, i);

			switch (c.num) {
			case '"':
				// " を取得したら文字列終了
				if (toml_get_char(buffer->utf8s, i + 2).num == '"' &&
					toml_get_char(buffer->utf8s, i + 1).num == '"') {
					append_char(buffer->word_dst, '\0');
					*next_point = i + 3;
					return 1;
				}
				else {
					append_char(buffer->word_dst, '"');
				}

			case '\\':
				// \のエスケープ文字判定
				nx_c = toml_get_char(buffer->utf8s, i + 1).num;
				if (nx_c == '\r' || nx_c == '\n') {
					skip_space = 1;
					i += 1;
				}
				else if (!append_escape_char(buffer, &i, error)) {
					*next_point = i;
					return 0;
				}
				break;

			case ' ':
			case '\t':
				// 空白文字を追加する
				if (!skip_space) {
					vec_add(buffer->word_dst, c.ch);
				}
				break;

			default:
				// 上記以外は普通の文字として取得
				if (c.num > 0x1f) {
					skip_space = 0;
					for (j = 0; j < UTF8_CHARCTOR_SIZE && c.ch[j] != 0; ++j) {
						vec_add(buffer->word_dst, c.ch + j);
					}
				}
				else {
					last_c[1] = last_c[0];
					last_c[0] = c.num & 0x1f;
				}
				break;
			}
		}

		// 改行の読み飛ばし指定がなければ追加する
		if (!skip_space) {
			if (last_c[1] == '\r' && last_c[0] == '\n') {
				append_char(buffer->word_dst, '\r');
				append_char(buffer->word_dst, '\n');
			}
			else if (last_c[0] == '\n') {
				append_char(buffer->word_dst, '\n');
			}
		}
		toml_read_buffer(buffer);

	} while (!eof);

	// " で終了できなかったためエラー
	error->code = MULTI_QUOAT_STRING_ERR;
	error->column = i;
	error->row = buffer->loaded_line;
	*next_point = i;
	return 0;
}

/**
 * "'" で囲まれた文字列を取得する。
 *
 * @param buffer		読み込み領域。
 * @param start			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param error			エラー詳細情報（戻り値）
 * @return				取得できたら 0以外。
 */
int get_multi_literal_string_value(TomlBuffer * buffer,
								   size_t start,
								   size_t * next_point,
								   TomlResultSummary * error)
{
	size_t		i, j;
	TomlUtf8	c;
	int			eof;

	// 文字列バッファを消去
	vec_clear(buffer->word_dst);

	// 先頭の改行は取り除く
	i = remove_head_linefield(buffer, start);

	do {
		eof = toml_end_of_file(buffer);

		for (; i < buffer->utf8s->length; ++i) {
			// 一文字取得する
			c = toml_get_char(buffer->utf8s, i);

			switch (c.num) {
			case '\'':
				// " を取得したら文字列終了
				if (toml_get_char(buffer->utf8s, i + 2).num == '\'' &&
					toml_get_char(buffer->utf8s, i + 1).num == '\'') {
					append_char(buffer->word_dst, '\0');
					*next_point = i + 3;
					return 1;
				}
				else {
					append_char(buffer->word_dst, '\'');
				}
				break;

			default:
				// 上記以外は普通の文字として取得
				for (j = 0; j < UTF8_CHARCTOR_SIZE && c.ch[j] != 0; ++j) {
					vec_add(buffer->word_dst, c.ch + j);
				}
				break;
			}
		}
		toml_read_buffer(buffer);

	} while (!eof);

	// " で終了できなかったためエラー
	error->code = MULTI_LITERAL_STRING_ERR;
	error->column = i;
	error->row = buffer->loaded_line;
	*next_point = i;
	return 0;
}