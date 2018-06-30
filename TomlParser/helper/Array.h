#ifndef __ARRAY_H__
#define __ARRAY_H__

/**
 * Cの配列をラップして領域越えアクセス時にアプリケーションをアボートさせる。
 *
 * ARRAY(int, num, num_src, 10);	// 宣言
 *
 * ARRAY_GET(int, num, 0) = 0;
 * ARRAY_GET(int, num, 1) = 1;
 * ARRAY_GET(int, num, 2) = 2;
 * ARRAY_GET(int, num, 3) = 3;
 * ARRAY_GET(int, num, 4) = 4;
 */

/**
 * 境界チェック配列構造体
 */
typedef struct _Array
{
	// 配列の最大長
	size_t	length;

	// データサイズ
	size_t	size;

	// 配列参照
	char *	data;

} Array;

/**
 * 境界チェック配列の生成マクロ
 */
#define ARRAY(T, name, src, length) \
	T src[length]; \
	Array name = array_attach(sizeof(T), src, sizeof(src) / sizeof(T));

/**
 * 配列チェック参照マクロ
 */
#define ARRAY_GET(T, name, index) (*((T*)array_get(&name, index)))

/**
 * 境界チェック配列構造体の作成
 *
 * @param size		データサイズ。
 * @param source	配列参照。
 * @param length	配列の要素数。
 * @return			境界チェック配列構造体。
 */
Array array_attach(size_t size, void * source, size_t length);

/**
 * 配列のポインタを取得する。
 *
 * @param array		境界チェック配列参照。
 * @param index		要素位置
 * @return			ポインタ。
 */
void * array_get(Array * array, size_t index);

#endif /* __ARRAY_H__ */