#ifndef __TOML_PARSE_STRING_H__
#define __TOML_PARSE_STRING_H__

/**
 * 先頭の改行を取り除く。
 *
 * @param buffer		読み込み領域。
 * @param start			開始位置。
 * @return				読み飛ばした位置。
 */
size_t toml_skip_head_linefield(TomlBuffer * buffer, size_t start);

/**
 * 改行、空白部を読み飛ばす。
 *
 * @param buffer		読み込み領域。
 * @param start			開始位置。
 * @return				読み飛ばした位置。
 */
size_t toml_skip_linefield_and_space(TomlBuffer * buffer, size_t start);

/**
 * キー文字列の取得。
 *
 * @param buffer		読み込み領域。
 * @param point			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param error			エラー詳細情報（戻り値）
 * @return				取得できたら 0以外。
 */
int toml_get_key(TomlBuffer * buffer, size_t point,
		size_t * next_point, TomlResultSummary * error);

/**
 * " で囲まれた文字列を取得する。
 *
 * @param buffer		読み込み領域。
 * @param start			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param error			エラー詳細情報（戻り値）
 * @return				取得できたら 0以外。
 */
int toml_get_string_value(TomlBuffer * buffer, size_t start,
		size_t * next_point, TomlResultSummary * error);

/**
 * ' で囲まれた文字列を取得する。
 *
 * @param buffer		読み込み領域。
 * @param start			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param error			エラー詳細情報（戻り値）
 * @return				取得できたら 0以外。
 */
int toml_get_literal_string_value(TomlBuffer * buffer,
		size_t start, size_t * next_point, TomlResultSummary * error);

/**
 * """ で囲まれた文字列を取得する。
 *
 * @param buffer		読み込み領域。
 * @param start			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param error			エラー詳細情報（戻り値）
 * @return				取得できたら 0以外。
 */
int toml_get_multi_string_value(TomlBuffer * buffer,
		size_t start, size_t * next_point, TomlResultSummary * error);

/**
 * "'" で囲まれた文字列を取得する。
 *
 * @param buffer		読み込み領域。
 * @param start			開始位置。
 * @param next_point	終了位置（戻り値）
 * @param error			エラー詳細情報（戻り値）
 * @return				取得できたら 0以外。
 */
int toml_get_multi_literal_string_value(TomlBuffer * buffer,
		size_t start, size_t * next_point, TomlResultSummary * error);

#endif /* __TOML_PARSE_STRING_H__ */