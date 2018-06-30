#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Assertion.h"
#include "Array.h"

/**
 * 境界チェック配列構造体の作成
 *
 * @param size		データサイズ。
 * @param source	配列参照。
 * @param length	配列の要素数。
 * @return			境界チェック配列構造体。
 */
Array array_attach(size_t size, void * source, size_t length)
{
	Array res;
	res.length = length;
	res.size = size;
	res.data = source;
	return res;
}

/**
 * 配列のポインタを取得する。
 *
 * @param array		境界チェック配列参照。
 * @param index		要素位置
 * @return			ポインタ。
 */
void * array_get(Array * array, size_t index)
{
	if (index < array->length) {
		return array->data + array->size * index;
	}
	else {
		Abort("out of index");
	}
}