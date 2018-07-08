#ifndef __TOML__BUFFERH__
#define __TOML__BUFFERH__

/**
 * 内部処理用のヘッダ、ファイルよりの読み込みバッファ。
 *
 * Copyright (c) 2018 Takashi Zota
 * Released under the MIT license
 * https://opensource.org/licenses/mit-license.php
 */

#include "Toml.h"

/** 読み込みバッファサイズ。 */
#define READ_BUF_SIZE	1024

/**
 * 読み込み領域。
 */
typedef struct _TomlBuffer
{
	// 文字列バッファ。
	Vec *	word_dst;

	// 整数値。
	long long int	integer;

	// 実数値。
	double	float_number;

	// 真偽値。
	int		boolean;

	// 日付情報。
	TomlDate 	date;

	// テーブル参照
	TomlTable *	table;

	// 配列参照
	TomlArray * array;

	// バッファ
	Vec *	utf8s;

	// 読み込み済み行数
	size_t	loaded_line;

} TomlBuffer;

/**
 * ファイルストリームを作成する。
 *
 * @param path		ファイルパス。
 * @return			ファイルストリーム。
 */
TomlBuffer * toml_create_buffer(const char * path);

/**
 * ファイルの読み込みが終了していれば 0以外を返す。
  *
 * @param buffer	ファイルストリーム。
 */
int toml_end_of_file(TomlBuffer * buffer);

/**
 * 一行分の文字列を取り込む。
 *
 * @param buffer	ファイルストリーム。
 */
void toml_read_buffer(TomlBuffer * buffer);

/**
 * ファイルストリームを閉じる。
 *
 * @param buffer	ファイルストリーム。
 */
void toml_close_buffer(TomlBuffer * buffer);

#endif /* __TOML__BUFFERH__ */
