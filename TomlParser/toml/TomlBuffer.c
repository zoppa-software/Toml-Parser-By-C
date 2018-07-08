#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crtdbg.h>
#include "../helper/Assertion.h"
#include "../helper/Vec.h"
#include "TomlBuffer.h"

/**
 * 読み込み領域。
 */
typedef struct _TomlBufferImpl
{
	// 公開情報の参照
	TomlBuffer	parent;

	// ファイルポインタ
	FILE *	fp;

	// 読み込みバッファ
	char	readbf[READ_BUF_SIZE];

	// 読み込み済みバイト数
	size_t	readed;

	// バッファの残量
	size_t	rest_buf;

} TomlBufferImpl;

/** 空文字。 */
static TomlUtf8		empty_char;

/**
 * ファイルストリームを作成する。
 *
 * @param path		ファイルパス。
 * @return			ファイルストリーム。
 */
TomlBuffer * toml_create_buffer(const char * path)
{
	TomlBufferImpl * res = (TomlBufferImpl*)malloc(sizeof(TomlBufferImpl));
	errno_t		err;

	memset(res, 0, sizeof(TomlBufferImpl));
	Assert(res != 0, "stream error");
	_RPTN(_CRT_WARN, "toml_create_stream malloc 0x%X\n", res);

	// ファイルオープン
	err = fopen_s(&res->fp, path, "rb");
	if (err == 0) {
		res->parent.utf8s = vec_initialize_set_capacity(sizeof(TomlUtf8), READ_BUF_SIZE);
		res->parent.word_dst = vec_initialize(sizeof(char));
		return (TomlBuffer*)res;
	}
	else {
		free(res);
		return 0;
	}
}

/**
 * ファイルの読み込みが終了していれば 0以外を返す。
  *
 * @param buffer	ファイルストリーム。
 */
int toml_end_of_file(TomlBuffer * buffer)
{
	TomlBufferImpl * impl = (TomlBufferImpl*)buffer;
	if (impl->rest_buf > 0) {
		return 0;
	}
	return (feof(impl->fp) || ferror(impl->fp));
}

/**
 * 次に進めるバイト数を取得する（UTF8用）
 *
 * @param caractor	文字。
 * @return			進めるバイト数。
 */
static size_t toml_skip_func(unsigned char caractor)
{
	if ((caractor & 0xfc) == 0xfc) {
		return 6;
	}
	else if ((caractor & 0xf8) == 0xf8) {
		return 5;
	}
	else if ((caractor & 0xf0) == 0xf0) {
		return 4;
	}
	else if ((caractor & 0xe0) == 0xe0) {
		return 3;
	}
	else if ((caractor & 0xc0) == 0xc0) {
		return 2;
	}
	else {
		return 1;
	}
}

/**
 * ファイルからバッファに値を取り込む。
 *
 * @param buffer	バッファ。
 * @return			バッファに取り込まれたバイト数。
 */
static size_t toml_read_from_file(TomlBufferImpl * buffer)
{
	char *	ptr;
	size_t	rst, len;

	// バッファの余裕をポイント
	ptr = buffer->readbf + buffer->readed;
	rst = READ_BUF_SIZE - buffer->readed;

	// バッファに取り込む
	len = fread_s(ptr, rst, sizeof(char), rst - 1, buffer->fp);
	len += buffer->readed;

	// 番兵として 0を埋め込む
	buffer->readbf[len] = 0;
	buffer->readed = 0;
	return len;
}

/**
 * 一行分の文字列を取り込む。
 *
 * @param buffer	ファイルストリーム。
 */
void toml_read_buffer(TomlBuffer * buffer)
{
	TomlBufferImpl * impl = (TomlBufferImpl*)buffer;
	char *		ptr;
	size_t		maxlen;
	size_t		i, skip;
	TomlUtf8	utf8;

	while (impl->rest_buf > 0 || !feof(impl->fp) && !ferror(impl->fp)) {
		// バッファに文字列を取り込む
		if (impl->rest_buf > 0) {
			maxlen = impl->rest_buf;
			impl->rest_buf = 0;
		}
		else {
			maxlen = toml_read_from_file(impl);
		}

		for (i = 0, ptr = impl->readbf;
					i < maxlen && *ptr != '\0'; ptr += skip, i += skip) {
			// 文字のバイト数を取得
			skip = toml_skip_func(*ptr);

			// UTF8の文字をバッファに書き込む
			//
			// 1. 文字数分取り込んでいるためバッファに書き込む
			//    1-1. 改行ならば取り込み中止
			// 2. 文字数取り込めていないため、次を取り込む
			if (i + skip < READ_BUF_SIZE) {
				memset(&utf8, 0, sizeof(utf8));		// 1
				memcpy(utf8.ch, ptr, skip);
				vec_add(impl->parent.utf8s, utf8.ch);

				if (utf8.num == '\n') {				// 1-1
					memmove_s(impl->readbf,
							  READ_BUF_SIZE,
							  impl->readbf + (i + 1),
							  READ_BUF_SIZE - (i + 1));
					impl->readed = maxlen - (i + 1);
					impl->rest_buf = impl->readed;
					impl->parent.loaded_line++;
					goto LOOP_END;
				}
			}
			else {
				memmove_s(impl->readbf,				// 2
						  READ_BUF_SIZE,
						  impl->readbf + i,
						  READ_BUF_SIZE - i);
				impl->readed = maxlen - i;
			}
		}
	}
LOOP_END:;
}

/**
 * ファイルストリームを閉じる。
 *
 * @param buffer	ファイルストリーム。
 */
void toml_close_buffer(TomlBuffer * buffer)
{
	TomlBufferImpl * impl = (TomlBufferImpl*)buffer;

	fclose(impl->fp);
	vec_delete(&impl->parent.utf8s);
	vec_delete(&impl->parent.word_dst);
	free(buffer);
}