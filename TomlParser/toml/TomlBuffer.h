#ifndef __TOML__BUFFERH__
#define __TOML__BUFFERH__

/** 読み込みバッファサイズ。 */
#define READ_BUF_SIZE	8

/** UTF8の文字数。 */
#define UTF8_CHARCTOR_SIZE	6

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

	// キー文字列バッファ
	Vec *	key_ptr;

	// バッファ
	Vec *	utf8s;

	// 解析エラー位置
	size_t	err_point;

	// 読み込み済み行数
	size_t	loaded_line;

} TomlBuffer;

/**
 * UTF8文字。
 */
typedef union _TomlUtf8
{
	// 文字バイト
	char	ch[UTF8_CHARCTOR_SIZE];

	// 数値
	unsigned int	num;

} TomlUtf8;

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

/**
 * 指定位置の文字を取得する。
 *
 * @param utf8s		文字列。
 * @param point		指定位置。
 * @return			取得した文字。
 */
TomlUtf8 toml_get_char(Vec * utf8s, size_t point);

/**
 * エラーメッセージを取得する。
 *
 * @param buffer	読み込みバッファ。
 * @preturn			エラーメッセージ。
 */
const char * toml_err_message(TomlBuffer * buffer);

#endif /* __TOML__BUFFERH__ */
