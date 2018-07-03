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
#include "TomlParseValue.h"

//-----------------------------------------------------------------------------
// 数値解析
//-----------------------------------------------------------------------------
/**
 * 実数の指数部を取得する。
 *
 * @param buffer		読み込み領域。
 * @param point			開始位置。
 * @param next_point	読み込み終了位置（戻り値）
 * @param digit			小数点位置。
 * @param expo			指数(e)。
 * @param res_expo		指数値（戻り値）
 * @param error			エラー詳細情報。
 * @return				取得できたら 0以外。
 */
static int exponent_convert(TomlBuffer * buffer,
							size_t point,
							size_t * next_point,
							int digit,
							int expo,
							int * res_expo,
							TomlResultSummary * error)
{
	size_t		i = point;
	int			ud = 0;
	TomlUtf8	c;
	int			sign = 0;
	int			exp_v = 0;

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
					error->code = UNDERBAR_CONTINUE_ERR;
					error->column = i;
					error->row = buffer->loaded_line;
					*next_point = i;
					return 0;
				}
				ud = 1;
			}
			else if (c.num >= '0' && c.num <= '9') {
				exp_v = exp_v * 10 + (c.num - '0');		// 2
				if (exp_v >= EXPO_MAX_RANGE) {
					error->code = DOUBLE_VALUE_ERR;
					error->column = i;
					error->row = buffer->loaded_line;
					*next_point = i;
					return 0;
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
			error->code = DOUBLE_VALUE_ERR;	// 1
			error->column = i;
			error->row = buffer->loaded_line;
			*next_point = i;
			return 0;
		}
		else if (toml_get_char(buffer->utf8s, point).num == '0') {
			error->code = ZERO_NUMBER_ERR;	// 2
			error->column = i;
			error->row = buffer->loaded_line;
			*next_point = i;
			return 0;
		}
	} 

	// 符号を設定
	if (sign) { exp_v = -exp_v; }

	// 小数点位置とマージ
	exp_v -= (digit > 0 ? digit : 0);
	if (exp_v > EXPO_MAX_RANGE || exp_v < -EXPO_MIN_RANGE) {
		error->code = DOUBLE_VALUE_RANGE_ERR;
		error->column = i;
		error->row = buffer->loaded_line;
		*next_point = i;
		return 0;
	}

	*res_expo = exp_v;
	*next_point = i;
	return 1;
}

/**
 * 数値（整数／実数）を取得する。
 *
 * @param number_sign	符号。
 * @param buffer		読み込み領域。
 * @param start			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param tokenType		取得データ。
 * @param error			エラー詳細情報。
 * @return				取得できたら 0以外。
 */
static int get_number_value(int number_sign,
						    TomlBuffer * buffer,
						    size_t point,
						    size_t * next_point,
						    TomlValueType * tokenType,
							TomlResultSummary * error)
{
 	unsigned long long int	v = 0;
	size_t			i, j;
	int				ud = 0;
	TomlUtf8		c;
	int				digit = -1;
	int				expo = -1;
	int				exp_v = -1;
	double long		dv;

	// 仮数部を取得する
	for (i = point; i < buffer->utf8s->length; ++i) {
		// 一文字取得する
		c = toml_get_char(buffer->utf8s, i);

		// 1. '_'の連続の判定
		// 2. 数値の計算をする
		//    2-1. 数値の有効範囲を超えるならばエラー
		// 3. 小数点位置を取得する
		// 4. 指数部（e）を取得する
		if (c.num == '_') {
			if (ud) {							// 1
				error->code = UNDERBAR_CONTINUE_ERR;
				error->column = i;
				error->row = buffer->loaded_line;
				*next_point = i;
				return 0;
			}
			ud = 1;
		}
		else if (c.num >= '0' && c.num <= '9') {
			if (v < ULLONG_MAX / 10) {			// 2
				v = v * 10 + (c.num - '0');
				if (digit >= 0) { digit++; }
			}
			else {								// 2-1
				error->code = INTEGER_VALUE_RANGE_ERR;
				error->column = i;
				error->row = buffer->loaded_line;
				*next_point = i;
				return 0;
			}
			ud = 0;
		}
		else if (c.num == '.') {				// 3
			if (digit < 0) {
				digit = 0;
			}
			else {
				error->code = MULTI_DECIMAL_ERR;
				error->column = i;
				error->row = buffer->loaded_line;
				*next_point = i;
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
	if (!exponent_convert(buffer, i, next_point,
						  digit, expo, &exp_v, error)) {
		return 0;
	}
	i = *next_point;

	if (digit < 0 && expo < 0) {
		// 整数値を取得する
		//
		// 1. 整数値を返すことを指定
		// 2. 負の整数変換
		// 3. 正の整数変換
		*tokenType = TomlIntegerValue;			// 1
		if (number_sign) {
			if (v <= LLONG_MAX) {				// 2
				buffer->integer = -((long long int)v);
			}
			else {
				buffer->integer = (long long int)v;
				if (buffer->integer != LLONG_MIN) {
					error->code = INTEGER_VALUE_RANGE_ERR;
					error->column = i;
					error->row = buffer->loaded_line;
					*next_point = i;
					return 0;
				}
			}
		}
		else {
			if (v <= LLONG_MAX) {				//3
				buffer->integer = (long long int)v;
			}
			else {
				error->code = INTEGER_VALUE_RANGE_ERR;
				error->column = i;
				error->row = buffer->loaded_line;
				*next_point = i;
				return 0;
			}
		}
	}
	else {
		// 実数値を取得する
		//
		// 1. 実数値を返すことを指定
		// 1. 負の指数なら除算
		// 2. 正の指数なら積算
		// 3. 0の指数なら使用しない
		// 4. 値を保持
		*tokenType = TomlFloatValue;			// 1
		if (exp_v < 0) {
			double abs_e = 1;					// 2
			for (j = 0; j < abs(exp_v); ++j) {
				abs_e *= 10;
			}
			dv = (long double)v / abs_e;
		}
		else if (exp_v > 0) {
			double abs_e = 1;					// 3
			for (j = 0; j < abs(exp_v); ++j) {
				abs_e *= 10;
			}
			dv = (long double)v * abs_e;
		}
		else {
			dv = (long double)v;				// 4
		}
		if (dv <= DBL_MAX) {					// 5
			buffer->float_number = (number_sign ? (double)-dv : (double)dv);
		}
		else {
			error->code = DOUBLE_VALUE_RANGE_ERR;
			error->column = i;
			error->row = buffer->loaded_line;
			*next_point = i;
			return 0;
		}
	}

	*next_point = i;
	return 1;
}

//-----------------------------------------------------------------------------
// 日付解析
//-----------------------------------------------------------------------------
/**
 * 文字列を部分的に切り取り、数値表現か確認、取得する。
 *
 * @param buffer		読み込み領域。
 * @param digit			必要桁数。
 * @param point			開始位置。
 * @param result		読込結果（戻り値）
 * @return				取得できたら 0以外。
 */
static int convert_partition_number(TomlBuffer * buffer,
									size_t digit,
								    size_t point,
									int * result)
{
	size_t	i;
	int		v, num = 0;
	if (buffer->utf8s->length - point < digit) {
		// 指定桁数未満の数値であるためエラー
		return 0;
	}
	else {
		// 必要桁数分ループし、全てが数値表現であることを確認
		// 1. ループ
		// 2. 数値判定し、結果を作成する
		// 3. 結果を返す
		for (i = 0; i < digit; ++i) {			// 1
			v = toml_get_char(buffer->utf8s, point + i).num;
			if (v >= '0' && v <= '9') {			// 2
				num = num * 10 + (v - '0');
			}
			else {
				return 0;
			}
		}
		*result = num;							// 3
		return 1;
	}
}

/**
 * 日付表現の時間部分を解析する。
 *
 * @param buffer		読み込み領域。
 * @param point			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param hour			時値。
 * @param error			エラー詳細情報。
 * @return				取得できたら 0以外。
 */
static int convert_time_format(TomlBuffer * buffer,
							   size_t point,
							   size_t * next_point,
							   int hour,
							   TomlResultSummary * error)
{
	int		minute, second, z_hor, z_min;
	size_t	i;
	unsigned int dec_sec = 0, v;
	TomlUtf8 c;

	// 分、秒の判定
	//
	// 1. 取得できたら格納
	// 2. 取得できなかったらエラーを返す
	if (convert_partition_number(buffer, 2, point + 3, &minute) &&
		toml_get_char(buffer->utf8s, point + 5).num == ':' &&
		convert_partition_number(buffer, 2, point + 6, &second)) {
		buffer->date.hour = (unsigned char)hour;
		buffer->date.minute = (unsigned char)minute;
		buffer->date.second = (unsigned char)second;
	}
	else {
		error->code = DATE_FORMAT_ERR;
		error->column = point + 3;
		error->row = buffer->loaded_line;
		*next_point = point;
		return 0;
	}

	// ミリ秒の解析
	//
	// 1. '.' があればミリ秒解析開始
	// 2. ミリ秒値を計算
	// 3. ミリ秒を保持する
	point += 8;
	if (toml_get_char(buffer->utf8s, point).num == '.') {		// 1
		for (i = point + 1; i < buffer->utf8s->length; ++i) {
			v = toml_get_char(buffer->utf8s, i).num;			// 2
			if (v >= '0' && v <= '9') {
				dec_sec = dec_sec * 10 + (v - '0');
			}
			else {
				break;
			}
		}
		buffer->date.dec_second = dec_sec;						// 3
		point = i;
	}

	// 時差の解析
	//
	// 1. UTC指定ならば終了
	// 2. 時差指定ならば、時刻を取り込む
	//    2-1. 時刻の書式に問題がなければ終了
	//    2-2. 時刻の書式に問題があればエラー
	// 3. 時差指定なし、正常終了
	*next_point = point;
	c = toml_get_char(buffer->utf8s, point);
	if (c.num == 'Z' || c.num == 'z') {							// 1
		*next_point = point + 1;
		return 1;
	}
	else if (c.num == '+' || c.num == '-') {
		if (convert_partition_number(buffer, 2, point + 1, &z_hor) &&
			toml_get_char(buffer->utf8s, point + 3).num == ':' &&
			convert_partition_number(buffer, 2, point + 4, &z_min)) {
			buffer->date.zone_hour = (c.num == '+' ? (char)z_hor : (char)-z_hor);
			buffer->date.zone_minute = (unsigned char)z_min;	// 2-1
			*next_point = point + 6;
			return 1;
		}
		else {
			error->code = DATE_FORMAT_ERR;						// 2-2
			error->column = point;
			error->row = buffer->loaded_line;
			*next_point = point;
			return 0;
		}
	}
	else {
		return 1;												// 3
	}
}

/**
 * 日付表現の日にち部分を解析する。
 *
 * @param buffer		読み込み領域。
 * @param point			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param year			年値。
 * @param error			エラー詳細情報。
 * @return				取得できたら 0以外。
 */
static int convert_date_format(TomlBuffer * buffer,
							   size_t point,
							   size_t * next_point,
							   int year,
							   TomlResultSummary * error)
{
	int		month, day, hour;
	TomlUtf8 c;

	// 月日の判定
	//
	// 1. 取得できたら格納
	// 2. 取得できなかったらエラーを返す
	if (convert_partition_number(buffer, 2, point + 5, &month) &&
		toml_get_char(buffer->utf8s, point + 7).num == '-' &&
		convert_partition_number(buffer, 2, point + 8, &day)) {
		buffer->date.year = (unsigned short)year;	// 1
		buffer->date.month = (unsigned char)month;
		buffer->date.day = (unsigned char)day;
	}
	else {
		error->code = DATE_FORMAT_ERR;				// 2
		error->column = point + 5;
		error->row = buffer->loaded_line;
		*next_point = point;
		return 0;
	}

	// 'T' の指定がなければ日にちのみ、終了
	c = toml_get_char(buffer->utf8s, point + 10);
	*next_point = point + 10;
	if (c.num != 'T' && c.num != 't') {
		return 1;
	}

	// 時間情報を判定して返す
	if (convert_partition_number(buffer, 2, point + 11, &hour) &&
		toml_get_char(buffer->utf8s, point + 13).num == ':') {
		return convert_time_format(buffer, point + 11, next_point, hour, error);
	}
	else {
		error->code = DATE_FORMAT_ERR;
		error->column = point + 11;
		error->row = buffer->loaded_line;
		*next_point = point;
		return 0;
	}
}

//-----------------------------------------------------------------------------
// 数値／日付／文字列解析
//-----------------------------------------------------------------------------
/**
 * 数値（日付／整数／実数）を取得する。
 *
 * @param buffer		読み込み領域。
 * @param point			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param tokenType		取得データ。
 * @param error			エラー詳細情報。
 * @return				取得できたら 0以外。
 */
static int get_number_or_date_value(TomlBuffer * buffer,
									size_t point,
									size_t * next_point,
									TomlValueType * tokenType,
									TomlResultSummary * error)
{
	int		year, hour;

	memset(&buffer->date, 0, sizeof(TomlDate));

	if (convert_partition_number(buffer, 4, point, &year) &&
		toml_get_char(buffer->utf8s, point + 4).num == '-') {
		// 日付（年月日）を取得する
		if (!convert_date_format(buffer, point, next_point, year, error)) {
			return 0;	// エラー詳細は呼び出し先で設定
		}
		else {
			*tokenType = TomlDateValue;
			return 1;
		}
	}
	else if (convert_partition_number(buffer, 2, point, &hour) &&
			 toml_get_char(buffer->utf8s, point + 2).num == ':') {
		// 日付（時分秒）を取得する
		if (!convert_time_format(buffer, point, next_point, hour, error)) {
			return 0;	// エラー詳細は呼び出し先で設定
		}
		else {
			*tokenType = TomlDateValue;
			return 1;
		}
	}
	else {
		// 数値（整数／実数）を取得する
		return get_number_value(0, buffer, point, next_point, tokenType, error);
	}
}

/**
 * 数値（整数／実数／日付）を取得する。
 *
 * @param buffer		読み込み領域。
 * @param point			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param tokenType		取得データ。
 * @param error			エラー詳細情報。
 * @return				取得できたら 0以外。
 */
int toml_convert_value(TomlBuffer * buffer,
					   size_t point,
					   size_t * next_point,
					   TomlValueType * tokenType,
					   TomlResultSummary * error)
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

	// 複数ライン文字列を返す
	if (buffer->utf8s->length - point >= 3) {
		// 複数ライン文字列を返す
		if (toml_get_char(buffer->utf8s, point + 2).num == '"' &&
			toml_get_char(buffer->utf8s, point + 1).num == '"' &&
			toml_get_char(buffer->utf8s, point).num == '"') {
			if (get_multi_string_value(buffer, point + 3, next_point, error)) {
				*tokenType = TomlStringValue;
				return 1;
			}
			else {
				return 0;	// エラー詳細は呼び出し先で設定
			}
		}

		// 複数ライン文字列（リテラル）を返す
		if (toml_get_char(buffer->utf8s, point + 2).num == '\'' &&
			toml_get_char(buffer->utf8s, point + 1).num == '\'' &&
			toml_get_char(buffer->utf8s, point).num == '\'') {
			if (get_multi_literal_string_value(buffer, point + 3, next_point, error)) {
				*tokenType = TomlStringValue;
				return 1;
			}
			else {
				return 0;	// エラー詳細は呼び出し先で設定
			}
		}
	}

	// 数値／日付／文字列
	if (buffer->utf8s->length - point > 0) {
		c = toml_get_char(buffer->utf8s, point);
		switch (c.num)
		{
		case '"':
			// 文字列を取得する
			if (toml_get_string_value(buffer, point + 1, next_point, error)) {
				*tokenType = TomlStringValue;
				return 1;
			}
			else {
				return 0;	// エラー詳細は呼び出し先で設定
			}

		case '\'':
			// リテラル文字列を取得する
			if (get_literal_string_value(buffer, point + 1, next_point, error)) {
				*tokenType = TomlStringValue;
				return 1;
			}
			else {
				return 0;	// エラー詳細は呼び出し先で設定
			}

		case '#':
			// コメントを取得する
			*next_point = point + 1;
			return 1;

		case '+':
			// 数値を取得する
			if (get_number_value(0, buffer, point + 1,
								 next_point, tokenType, error)) {
				return 1;
			}
			else {
				return 0;	// エラー詳細は呼び出し先で設定
			}

		case '-':
			// 数値を取得する
			//
			// 1. 整数値の符号を逆転させる
			// 2. 実数値の符号を逆転させる
			if (get_number_value(1, buffer, point + 1,
								 next_point, tokenType, error)) {
				return 1;
			}
			else {
				return 0;	// エラー詳細は呼び出し先で設定
			}
			break;

		default:
			// 日付／数値を取得する
			return get_number_or_date_value(buffer, point,
									next_point, tokenType, error);
		}
	}
	else {
		// 値が取得できなかった
		error->code = INVALID_NAME_ERR;
		error->column = point;
		error->row = buffer->loaded_line;
		*next_point = point;
		return 0;
	}
}
