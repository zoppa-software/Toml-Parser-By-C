#ifndef __TOML_PARSE_VALUE_H__
#define __TOML_PARSE_VALUE_H__

/**
 * Copyright (c) 2018 Takashi Zota
 * Released under the MIT license
 * https://opensource.org/licenses/mit-license.php
 */

// 指数部最大値
#define EXPO_MAX_RANGE		308

// 指数部最小値
#define EXPO_MIN_RANGE		-308

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
int toml_convert_value(TomlBuffer * buffer, size_t point, size_t * next_point,
								TomlValueType * tokenType, TomlResultSummary * error);

#endif /* __TOML_PARSE_VALUE_H__ */
