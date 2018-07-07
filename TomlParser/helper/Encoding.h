#ifndef __ENCODING_H__
#define __ENCODING_H__

/**
 * 文字コードを変換するための関数。
 *
 *	char	buf[8];
 *	char	ans[8];
 *
 *	buf[0] = 0xe5;
 *  buf[1] = 0xb3;
 *	buf[2] = 0xaf;
 *	buf[3] = 0;
 *
 *	printf_s("%s\n", encoding_utf8_to_cp932(buf, ans, sizeof(ans)));
 *
 * Copyright (c) 2018 Takashi Zota
 * Released under the MIT license
 * https://opensource.org/licenses/mit-license.php
 */

/**
 * 文字コードを変換する（UTF8 -> cp932）
 *
 * @param input			入力文字列。
 * @param buffer		変換用バッファ。
 * @param buffer_size	変換用バッファサイズ。
 */
const char * encoding_utf8_to_cp932(const char * input, char * buffer, size_t buffer_size);

#endif /* __ENCODING_H__ */