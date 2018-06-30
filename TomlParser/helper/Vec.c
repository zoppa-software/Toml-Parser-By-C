#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crtdbg.h>
#include "Assertion.h"
#include "Vec.h"

// 初期容量デフォルト値
#define DEFAULT_CAPACITY	16

/**
 * 可変長配列構造体、実体
 */
typedef struct _VecImpl
{
	// 可変長配列
	Vec		vector;

	// 要素のデータ長
	size_t	data_len;

	// 取得済み領域長
	size_t	capacity;

} VecImpl;

/**
 * 過去の領域を解放して、領域サイズを大きくする。
 *
 * @param vec		可変長配列。
 * @param capacity	新しい領域サイズ。
 */
static void vec_allocate(VecImpl * vec, size_t capacity)
{
	// 新しい領域を取得
	void * temp = malloc(capacity * vec->data_len);
	Assert(temp != 0, "vec allocate error");
	_RPTN(_CRT_WARN, "vec_allocate malloc 0x%X\n", temp);

	// 過去データを新領域へコピー
	memcpy(temp, vec->vector.pointer, vec->vector.length * vec->data_len);

	// 過去の領域を解放
	free(vec->vector.pointer);
	vec->vector.pointer = temp;
	vec->capacity = capacity;
}

//-----------------------------------------------------------------------------
// コンストラクタ、デストラクタ
//-----------------------------------------------------------------------------
/**
 * 可変長配列の初期化を行う。
 *
 * @param data_len		一個のデータ長。
 * @return				可変長配列。
 */
Vec * vec_initialize(size_t data_len)
{
	return vec_initialize_set_capacity(data_len, DEFAULT_CAPACITY);
}

/**
 * 可変長配列の初期化を行う（初期容量指定）
 *
 * @param data_len			一個のデータ長。
 * @param start_capacity	初期容量。
 * @return					可変長配列。
 */
Vec * vec_initialize_set_capacity(size_t data_len, size_t start_capacity)
{
	VecImpl * res = (VecImpl*)malloc(sizeof(VecImpl));
	Assert(res != 0, "vec allocate error");
	_RPTN(_CRT_WARN, "vec_initialize_set_capacity malloc 0x%X\n", res);

	// 可変長領域を設定
	memset(res, 0, sizeof(VecImpl));
	res->data_len = data_len;
	res->vector.pointer = malloc(start_capacity * data_len);
	Assert(res->vector.pointer != 0, "vec allocate error");
	_RPTN(_CRT_WARN, "vec_initialize_set_capacity malloc 0x%X\n", res->vector.pointer);
	res->capacity = start_capacity;

	return (Vec*)res;
}

/**
 * 可変長配列を削除する。
 *
 * @param ins_del		削除する可変長配列。
 */
void vec_delete(Vec ** ins_del)
{
	VecImpl * vec = (VecImpl*)(*ins_del);

	if (vec != 0) {
		free(vec->vector.pointer);
		free(vec);
		(*ins_del) = 0;
	}
}

//-----------------------------------------------------------------------------
// 参照機能
//-----------------------------------------------------------------------------
/**
 * 可変長配列より要素への参照を取得する
 *
 * @param vector		可変長配列。
 * @param index			要素位置。
 * @return				要素を参照するポインタ。
 */
void * vec_items(Vec * vector, size_t index)
{
	VecImpl * vec = (VecImpl*)vector;

	if (index < vec->vector.length) {
		return (void*)((char*)vec->vector.pointer + vec->data_len * index);
	}
	else {
		// 参照位置が範囲外
		Abort("out of range error");
	}
}

//-----------------------------------------------------------------------------
// 追加と削除
//-----------------------------------------------------------------------------
/**
 * 可変長配列を消去する。
 *
 * @param vector		可変長配列。
 */
void vec_clear(Vec * vector)
{
	VecImpl * vec = (VecImpl*)vector;
#if _DEBUG
	memset((char*)vec->vector.pointer, 0, vec->data_len * vec->capacity);
#endif
	vec->vector.length = 0;
}

/**
 * 可変長配列に要素を追加する。
 *
 * @param vector		可変長配列。
 * @param data			追加する要素。
 */
void vec_add(Vec * vector, void * data)
{
	VecImpl * vec = (VecImpl*)vector;
	size_t explen;

	// 追加容量が不足していたら追加
	if (vec->vector.length + 1 >= vec->capacity) {
		explen = (vec->capacity < VEC_EXPAND_LENGTH ? vec->capacity : VEC_EXPAND_LENGTH);
		vec_allocate(vec, vec->capacity + explen);
	}

	// データ書込み
	memcpy((char*)vec->vector.pointer + vec->data_len * vec->vector.length, data, vec->data_len);
	vec->vector.length++;
}

/**
 * 可変長配列の指定位置の要素を削除する。
 *
 * @param vector		可変長配列。
 * @param index			削除する位置。
 */
void vec_remove_at(Vec * vector, size_t index)
{
	VecImpl * vec = (VecImpl*)vector;
	if (index < vec->vector.length - 1) {
		// 途中の要素が削除されたならば、要素を寄せる
		size_t dst = vec->data_len * index;
		size_t src = dst + vec->data_len;
		size_t len = vec->data_len * (vec->vector.length - index - 1);
		memmove_s((char*)vec->vector.pointer + dst, vec->capacity - dst, (char*)vec->vector.pointer + src, len);
#if _DEBUG
		memset((char*)vec->vector.pointer + vec->data_len * (vec->vector.length - 1), 0, vec->data_len);
#endif
		vec->vector.length--;
	}
	else if (index == vec->vector.length - 1) {
		// 最後の要素のみ削除する
#if _DEBUG
		memset((char*)vec->vector.pointer + index, 0, vec->data_len);
#endif
		vec->vector.length--;
	}
	else {
		// 削除位置が範囲外
		Abort("out of range error");
	}
}