#ifndef __VEC_H__
#define __VEC_H__

/**
 * 可変長配列。
 *
 * Vec * vec = vec_initialize(sizeof(int));		// 可変長配列を生成
 *
 * for (i = 1; i <= 100; ++i) {					// 要素を追加
 *		vec_add(vec, &i);						// ※ 追加できる要素はポインタで参照することに注意
 * }
 *
 * for (i = 0; i < 100; ++i) {
 *		printf_s("%d\n", VEC_GET(int, vec, i));	// マクロで要素を参照
 * }
 *
 * vec_remove_at(vec, 33);						// 削除もあります
 * vec_remove_at(vec, 98);
 * 
 * vec_delete(&vec);							// 解放を忘れずに
 *
 * Copyright (c) 2018 Takashi Zota
 * Released under the MIT license
 * https://opensource.org/licenses/mit-license.php
 */

// 伸張する最大長
#define VEC_EXPAND_LENGTH	1024

// 項目参照マクロ
#define VEC_GET(T, name, index) (*((T*)vec_items(name, index)))

/**
 * 可変長配列構造体
 */
typedef struct _Vec
{
	// 要素数
	size_t		length;

	// 実体参照
	void *		pointer;

} Vec;

//-----------------------------------------------------------------------------
// コンストラクタ、デストラクタ
//-----------------------------------------------------------------------------
/**
 * 可変長配列の初期化を行う。
 *
 * @param data_len		一個のデータ長。
 * @return				可変長配列。
 */
Vec * vec_initialize(size_t data_len);

/**
 * 可変長配列の初期化を行う（初期容量指定）
 *
 * @param data_len			一個のデータ長。
 * @param start_capacity	初期容量。
 * @return					可変長配列。
 */
Vec * vec_initialize_set_capacity(size_t data_len, size_t start_capacity);

/**
 * 可変長配列を削除する。
 *
 * @param ins_del		削除する可変長配列。
 */
void vec_delete(Vec ** ins_del);

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
void * vec_items(Vec * vector, size_t index);

//-----------------------------------------------------------------------------
// 追加と削除
//-----------------------------------------------------------------------------
/**
 * 可変長配列を消去する。
 *
 * @param vector		可変長配列。
 */
void vec_clear(Vec * vector);

/**
 * 可変長配列に要素を追加する。
 *
 * @param vector		可変長配列。
 * @param data			追加する要素。
 */
void vec_add(Vec * vector, void * data);

/**
 * 可変長配列の指定位置の要素を削除する。
 *
 * @param vector		可変長配列。
 * @param index			削除する位置。
 */
void vec_remove_at(Vec * vector, size_t index);

#endif /* __VEC_H__ */
