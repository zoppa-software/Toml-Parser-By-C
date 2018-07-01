#ifndef __TOML_ANALISYS_H__
#define __TOML_ANALISYS_H__

/**
 * 空白文字を読み飛ばす。
 *
 * @param impl	tomlドキュメント。
 * @param i		読み込み位置。
 * @return		読み飛ばした位置。
 */
size_t toml_skip_space(Vec * utf8s, size_t i);

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

/**
 * キー文字列の取得。
 *
 * @param buffer		読み込み領域。
 * @param point			開始位置。
 * @param next_point	終了位置（戻り値）
 * @return				取得できたら 0以外。
 */
int toml_get_key(TomlBuffer * buffer, size_t point, size_t * next_point);

/**
 * " で囲まれた文字列を取得する。
 *
 * @param buffer		読み込み領域。
 * @param start			開始位置。
 * @param next_point	終了位置（戻り値）
 * @return				取得できたら 0以外。
 */
int toml_get_string_value(TomlBuffer * buffer, size_t start, size_t * next_point);

///**
// * 数値（整数／実数／日付）を取得する。
// *
// * @param buffer		読み込み領域。
// * @param start			開始位置。
// * @param next_point	終了位置（戻り値）
// * @param tokenType		取得データ。
// * @return				取得できたら 0以外。
// */
//int toml_get_number_value(TomlBuffer * buffer, size_t point, size_t * next_point, TomlValueType * tokenType);

int toml_convert_value(TomlBuffer * buffer, size_t point, size_t * next_point, TomlValueType * tokenType, TomlResult * value_res);

#endif /* __TOML_ANALISYS_H__ */
