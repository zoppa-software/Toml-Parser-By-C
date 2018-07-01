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
static void toml_append_char(Vec * word_dst,
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
 * @return			変換に成功したら 0以外。
 */
static int toml_append_unicode(TomlBuffer * buffer,
							   size_t ptr,
							   size_t len)
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
int toml_get_key(TomlBuffer * buffer,
				 size_t point,
				 size_t * next_point)
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
 * '\'エスケープ文字を取得する。
 *
 * @param buffer		読み込み領域。
 * @param ptr			開始位置。
 * @return				取得できたら 0以外。
 */
static int toml_append_escape_char(TomlBuffer * buffer,
								   size_t * ptr)
{
	size_t		i = *ptr;
	TomlUtf8	c;

	if (i < buffer->utf8s->length - 1) {
		c = toml_get_char(buffer->utf8s, i + 1);

		switch (c.num) {
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
	*ptr = i;
	return 1;
}

/**
 * " で囲まれた文字列を取得する。
 *
 * @param buffer		読み込み領域。
 * @param start			開始位置。
 * @param next_point	終了位置（戻り値）
 * @return				取得できたら 0以外。
 */
int toml_get_string_value(TomlBuffer * buffer,
						  size_t start,
						  size_t * next_point)
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
			toml_append_char(buffer->word_dst, '\0');
			*next_point = i + 1;
			return (buffer->word_dst->length > 1);

		case '\\':
			// \のエスケープ文字判定
			if (!toml_append_escape_char(buffer, &i)) {
				buffer->err_point = i;
				return 0;
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
 * ' で囲まれた文字列を取得する。
 *
 * @param buffer		読み込み領域。
 * @param start			開始位置。
 * @param next_point	終了位置（戻り値）
 * @return				取得できたら 0以外。
 */
static int toml_get_literal_string_value(TomlBuffer * buffer,
										 size_t start,
										 size_t * next_point)
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
			toml_append_char(buffer->word_dst, '\0');
			*next_point = i + 1;
			return (buffer->word_dst->length > 1);

		default:
			// 上記以外は普通の文字として取得
			for (j = 0; j < UTF8_CHARCTOR_SIZE && c.ch[j] != 0; ++j) {
				vec_add(buffer->word_dst, c.ch + j);
			}
			break;
		}
	}

	// ' で終了できなかったためエラー
	buffer->err_point = buffer->utf8s->length - 1;
	return 0;
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
 * """ で囲まれた文字列を取得する。
 *
 * @param buffer		読み込み領域。
 * @param start			開始位置。
 * @param next_point	終了位置（戻り値）
 * @return				取得できたら 0以外。
 */
static int toml_get_multi_string_value(TomlBuffer * buffer,
									   size_t start,
									   size_t * next_point)
{
	size_t		i, j;
	TomlUtf8	c;
	char		last_c[2];
	int			skip_space = 0;
	unsigned int	nx_c;

	// 文字列バッファを消去
	vec_clear(buffer->word_dst);
	memset(last_c, 0, sizeof(last_c));

	// 先頭の改行は取り除く
	i = remove_head_linefield(buffer, start);

	do {
		for (; i < buffer->utf8s->length; ++i) {
			// 一文字取得する
			c = toml_get_char(buffer->utf8s, i);

			switch (c.num) {
			case '"':
				// " を取得したら文字列終了
				if (toml_get_char(buffer->utf8s, i + 2).num == '"' &&
					toml_get_char(buffer->utf8s, i + 1).num == '"') {
					toml_append_char(buffer->word_dst, '\0');
					*next_point = i + 3;
					return (buffer->word_dst->length > 1);
				}
				else {
					toml_append_char(buffer->word_dst, '"');
				}

			case '\\':
				// \のエスケープ文字判定
				nx_c = toml_get_char(buffer->utf8s, i + 1).num;
				if (nx_c == '\r' || nx_c == '\n') {
					skip_space = 1;
					i += 1;
				}
				else if (!toml_append_escape_char(buffer, &i)) {
					buffer->err_point = i;
					return 0;
				}
				break;

			case ' ':
			case '\t':
				// 空白文字を追加する
				vec_add(buffer->word_dst, c.ch);
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
				toml_append_char(buffer->word_dst, '\r');
				toml_append_char(buffer->word_dst, '\n');
			}
			else if (last_c[0] == '\n') {
				toml_append_char(buffer->word_dst, '\n');
			}
		}
		toml_read_buffer(buffer);

	} while (!toml_end_of_file(buffer));

	// " で終了できなかったためエラー
	buffer->err_point = buffer->utf8s->length - 1;
	return 0;
}

/**
 * "'" で囲まれた文字列を取得する。
 *
 * @param buffer		読み込み領域。
 * @param start			開始位置。
 * @param next_point	終了位置（戻り値）
 * @return				取得できたら 0以外。
 */
static int toml_get_multi_literal_string_value(TomlBuffer * buffer,
											   size_t start,
											   size_t * next_point)
{
	size_t		i, j;
	TomlUtf8	c;

	// 文字列バッファを消去
	vec_clear(buffer->word_dst);

	// 先頭の改行は取り除く
	i = remove_head_linefield(buffer, start);

	do {
		for (; i < buffer->utf8s->length; ++i) {
			// 一文字取得する
			c = toml_get_char(buffer->utf8s, i);

			switch (c.num) {
			case '\'':
				// " を取得したら文字列終了
				if (toml_get_char(buffer->utf8s, i + 2).num == '\'' &&
					toml_get_char(buffer->utf8s, i + 1).num == '\'') {
					toml_append_char(buffer->word_dst, '\0');
					*next_point = i + 3;
					return (buffer->word_dst->length > 1);
				}
				else {
					toml_append_char(buffer->word_dst, '\'');
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

	} while (!toml_end_of_file(buffer));

	// " で終了できなかったためエラー
	buffer->err_point = buffer->utf8s->length - 1;
	return 0;
}

/**
 * 実数の指数部を取得する。
 *
 * @param buffer		読み込み領域。
 * @param point			開始位置。
 * @param digit			小数点位置。
 * @param expo			指数(e)。
 * @param res_expo		指数値（戻り値）
 * @param value_res		取得結果。
 * @return				読み込み終了位置。
 */
static size_t exponent_convert(TomlBuffer * buffer,
							   size_t point,
							   int digit,
							   int expo,
							   int * res_expo,
							   TomlResult * value_res)
{
	size_t		i = point;
	int			ud = 0;
	TomlUtf8	c;
	int			sign = 0;
	int			exp_v = 0;

	*value_res = SUCCESS;

	if (expo >= 0) {
		// 符号を取得する
		c = toml_get_char(buffer->utf8s, i);
		if (c.num == '+') {
			sign = 0;
			i++;
		}
		else if (c.num == '-') {
			sign = 1;
			i++;
		}

		ud = 0;
		for (; i < buffer->utf8s->length; ++i) {
			// 一文字取得する
			c = toml_get_char(buffer->utf8s, i);

			// 1. '_'の連続の判定
			// 2. 指数部を計算する
			if (c.num == '_') {
				if (ud) {								// 1
					*value_res = UNDERBAR_CONTINUE_ERR;
					buffer->err_point = i;
					return i;
				}
				ud = 1;
			}
			else if (c.num >= '0' && c.num <= '9') {
				exp_v = exp_v * 10 + (c.num - '0');		// 2
				if (exp_v >= 308) {
					*value_res = DOUBLE_VALUE_ERR;
					return i;
				}
				ud = 0;
			}
			else {
				break;
			}
		}

		// 指数値を確認する
		// 1. 指数値が 0以下でないことを確認
		// 2. 指数値が '0'始まりでないことを確認
		if (exp_v <= 0) {
			*value_res = DOUBLE_VALUE_ERR;	// 1
		}
		else if (toml_get_char(buffer->utf8s, point).num == '0') {
			*value_res = ZERO_NUMBER_ERR;	// 2
		}
	} 

	// 符号を設定
	if (sign) { exp_v = -exp_v; }

	// 小数点位置とマージ
	exp_v -= (digit > 0 ? digit : 0);
	if (exp_v > 308 || exp_v < -308) {
		*value_res = DOUBLE_VALUE_RANGE_ERR;
		return 0;
	}

	*res_expo = exp_v;
	return i;
}

/**
 * 数値（整数／実数）を取得する。
 *
 * @param buffer		読み込み領域。
 * @param start			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param tokenType		取得データ。
 * @param value_res		取得結果。
 * @return				取得できたら 0以外。
 */
int toml_get_number_value(TomlBuffer * buffer,
						  size_t point,
						  size_t * next_point,
						  TomlValueType * tokenType,
						  TomlResult * value_res)
{
 	unsigned long long int	v = 0;
	size_t			i, j;
	int				ud = 0;
	TomlUtf8		c;
	int				digit = -1;
	int				expo = -1;
	int				exp_v = -1;
	double long		dv;

	for (i = point; i < buffer->utf8s->length; ++i) {
		// 一文字取得する
		c = toml_get_char(buffer->utf8s, i);

		// 1. '_'の連続の判定
		// 2. 数値の計算をする
		// 3. 小数点位置を取得する
		// 4. 指数部（e）を取得する
		if (c.num == '_') {
			if (ud) {							// 1
				*value_res = UNDERBAR_CONTINUE_ERR;
				buffer->err_point = i;
				return 0;
			}
			ud = 1;
		}
		else if (c.num >= '0' && c.num <= '9') {
			if (v < ULLONG_MAX / 10) {			// 2
				v = v * 10 + (c.num - '0');
				if (digit >= 0) {
					digit++;
				}
			}
			else {
				*value_res = INTEGER_VALUE_RANGE_ERR;
				buffer->err_point = i;
				return 0;
			}
			ud = 0;
		}
		else if (c.num == '.') {				// 3
			if (digit < 0) {
				digit = 0;
			}
			else {
				*value_res = MULTI_DECIMAL_ERR;
				buffer->err_point = i;
				return 0;
			}
		}
		else if (c.num == 'e' || c.num == 'E') {
			expo = 0;							// 4
			i++;
			break;
		}
		else {
			break;
		}
	}

	// 仮数部を取得する
	i = exponent_convert(buffer, i, digit, expo, &exp_v, value_res);
	if (*value_res != SUCCESS) {
		return 0;
	}

	if (digit < 0 && expo < 0) {
		// 整数値を取得する
		*tokenType = TomlIntegerValue;
		if (v <= LONG_MAX) {
			buffer->integer = (long long int)v;
		}
		else {
			*value_res = INTEGER_VALUE_RANGE_ERR;
			return 0;
		}
	}
	else {
		// 実数値を取得する
		//
		// 1. 負の指数なら除算
		// 2. 正の指数なら積算
		// 3. 0の指数なら使用しない
		// 4. 値を保持
		*tokenType = TomlFloatValue;
		if (exp_v < 0) {
			double abs_e = 1;		// 1
			for (j = 0; j < abs(exp_v); ++j) {
				abs_e *= 10;
			}
			dv = (long double)v / abs_e;
		}
		else if (exp_v > 0) {
			double abs_e = 1;		// 2
			for (j = 0; j < abs(exp_v); ++j) {
				abs_e *= 10;
			}
			dv = (long double)v * abs_e;
		}
		else {
			dv = (long double)v;	// 3
		}
		if (dv <= DBL_MAX) {		// 4
			buffer->float_number = (double)dv;
		}
		else {
			*value_res = DOUBLE_VALUE_RANGE_ERR;
			return 0;
		}
	}

	*next_point = i;
	return 1;
}

/**
 * 数値（日付／整数／実数）を取得する。
 *
 * @param buffer		読み込み領域。
 * @param point			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param tokenType		取得データ。
 * @param value_res		取得結果。
 * @return				取得できたら 0以外。
 */
int toml_get_number_or_date_value(TomlBuffer * buffer,
								  size_t point,
								  size_t * next_point,
								  TomlValueType * tokenType,
								  TomlResult * value_res)
{
	if (buffer->utf8s->length - point >= 5) {
		//if (toml_get_char(buffer->utf8s, point + 4).num == 'e' &&
		//	toml_get_char(buffer->utf8s, point + 3).num == 's' &&
		//	toml_get_char(buffer->utf8s, point + 2).num == 'l' &&
		//	toml_get_char(buffer->utf8s, point + 1).num == 'a' &&
		//	toml_get_char(buffer->utf8s, point).num == 'f') {
		//	buffer->boolean = 0;
		//	*next_point = point + 5;
		//	*tokenType = TomlBooleanValue;
		//	return 1;
		//}
	}

	if (buffer->utf8s->length - point >= 3) {
		//if (toml_get_char(buffer->utf8s, point + 4).num == 'e' &&
		//	toml_get_char(buffer->utf8s, point + 3).num == 's' &&
		//	toml_get_char(buffer->utf8s, point + 2).num == 'l' &&
		//	toml_get_char(buffer->utf8s, point + 1).num == 'a' &&
		//	toml_get_char(buffer->utf8s, point).num == 'f') {
		//	buffer->boolean = 0;
		//	*next_point = point + 5;
		//	*tokenType = TomlBooleanValue;
		//	return 1;
		//}
	}

	// 数値（整数／実数）を取得する
	return toml_get_number_value(buffer, point, next_point, tokenType, value_res);
}

int toml_convert_value(TomlBuffer * buffer,
					   size_t point,
					   size_t * next_point,
					   TomlValueType * tokenType,
					   TomlResult * value_res)
{
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
		// 複数ライン文字列を返す
		if (toml_get_char(buffer->utf8s, point + 2).num == '"' &&
			toml_get_char(buffer->utf8s, point + 1).num == '"' &&
			toml_get_char(buffer->utf8s, point).num == '"') {
			if (toml_get_multi_string_value(buffer, point + 3, next_point)) {
				*tokenType = TomlStringValue;
				return 1;
			}
			else {
				*value_res = MULTI_QUOAT_STRING_ERR;
				return 0;
			}
		}

		// 複数ライン文字列（リテラル）を返す
		if (toml_get_char(buffer->utf8s, point + 2).num == '\'' &&
			toml_get_char(buffer->utf8s, point + 1).num == '\'' &&
			toml_get_char(buffer->utf8s, point).num == '\'') {
			if (toml_get_multi_literal_string_value(buffer, point + 3, next_point)) {
				*tokenType = TomlStringValue;
				return 1;
			}
			else {
				*value_res = MULTI_LITERAL_STRING_ERR;
				return 0;
			}
		}
	}

	if (buffer->utf8s->length - point > 0) {
		c = toml_get_char(buffer->utf8s, point);
		switch (c.num)
		{
		case '"':
			// 文字列を取得する
			if (toml_get_string_value(buffer, point + 1, next_point)) {
				*tokenType = TomlStringValue;
				return 1;
			}
			else {
				*value_res = QUOAT_STRING_ERR;
				return 0;
			}

		case '\'':
			// リテラル文字列を取得する
			if (toml_get_literal_string_value(buffer, point + 1, next_point)) {
				*tokenType = TomlStringValue;
				*value_res = LITERAL_STRING_ERR;
				return 1;
			}
			else {
				return 0;
			}

		case '#':
			// コメントを取得する
			*next_point = point + 1;
			return 1;

		case '+':
			// 数値を取得する
			if (toml_get_number_value(buffer, point + 1,
									  next_point, tokenType, value_res)) {
				return 1;
			}
			else {
				return 0;
			}

		case '-':
			// 数値を取得する
			//
			// 1. 整数値の符号を逆転させる
			// 2. 実数値の符号を逆転させる
			if (toml_get_number_value(buffer, point + 1,
									  next_point, tokenType, value_res)) {
				switch (*tokenType) {
				case TomlIntegerValue:		// 1
					if (buffer->integer < LONG_MAX) {
						buffer->integer = -buffer->integer;
						return 1;
					}
					else {
						*value_res = INTEGER_VALUE_RANGE_ERR;
						return 0;
					}

				case TomlFloatValue:		// 2
					buffer->float_number = -buffer->float_number;
					return 1;

				default:
					return 0;
				}
			}
			else {
				return 0;
			}
			break;

		default:
			// 日付／数値を取得する
			return toml_get_number_or_date_value(buffer, point,
									next_point, tokenType, value_res);
		}
	}
	else {
		*value_res = INVALID_NAME_ERR;
		return 0;
	}
}
