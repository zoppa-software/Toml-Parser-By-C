#ifndef __STRING_HASH_H__
#define __STRING_HASH_H__

/**
 * 文字列（const char *）を対象としたハッシュテーブル。
 *
 * const StringHash * hash = stringhash_initialize();	// ハッシュテーブルの作成
 *
 * stringhash_add(hash, "北海道 札幌市");					// ハッシュテーブルに項目を追加
 *
 * const char * key;									// 存在確認
 * if (stringhash_contains(hash, "熊本県 南小国町", &key)) {
 *		printf_s("hit %s\n", key);
 * }
 *
 * stringhash_remove(hash, "熊本県 南小国町");			// 項目削除
 *
 * stringhash_delete(&hash);							// 最後は解放してください
 *
 * Copyright (c) 2018 Takashi Zota
 * Released under the MIT license
 * https://opensource.org/licenses/mit-license.php
 */

/**
 * 文字列ハッシュテーブルのキー。
 */
typedef const char *	stringhash_key;

//-----------------------------------------------------------------------------
// 構造体
//-----------------------------------------------------------------------------
/**
 * 文字列ハッシュテーブル。
 */
typedef struct _StringHash
{
	// 格納項目数
	size_t			count;

} StringHash;

//-----------------------------------------------------------------------------
// コンストラクタ、デストラクタ
//-----------------------------------------------------------------------------
/**
 * 文字列ハッシュテーブルの初期化を行う。
 *
 * @return				インスタンス。
 */
const StringHash * stringhash_initialize();

/**
 * 文字列ハッシュテーブルを削除する。
 *
 * @param ins_del		削除するインスタンス。
 */
void stringhash_delete(const StringHash ** ins_del);

//-----------------------------------------------------------------------------
// 確認、追加、削除
//-----------------------------------------------------------------------------
/**
 * 文字列の存在確認を行う。
 *
 * @param instance		対象ハッシュテーブル。
 * @param key			確認する文字列。
 * @param result		文字列が存在したとき、存在文字列のポインタ（戻り値）
 * @return				文字列が存在するとき 0以外を返す。
 */
int stringhash_contains(const StringHash * instance, stringhash_key key, stringhash_key * result);

/**
 * 文字列の追加を行う。
 *
 * @param instance		対象ハッシュテーブル。
 * @param key			追加する文字列。
 * @return				ハッシュテーブルに追加された文字列のポインタ。
 */
stringhash_key stringhash_add(const StringHash * instance, stringhash_key key);

/**
 * 文字列の追加を行う（文字列の部分）
 *
 * @param instance		対象ハッシュテーブル。
 * @param key			追加する文字列。
 * @param key_len		文字列長。
 * @return				ハッシュテーブルに追加された文字列のポインタ。
 */
stringhash_key stringhash_add_partition(const StringHash * instance, stringhash_key key, size_t key_len);

/**
 * ハッシュテーブルより、指定の文字列を削除する。
 *
 * @param instance		対象ハッシュテーブル。
 * @param key			削除する文字列。
 */
void stringhash_remove(const StringHash * instance, stringhash_key key);

/**
 * ハッシュテーブルより、指定の文字列を削除する。
 *
 * @param instance		対象ハッシュテーブル。
 * @param key			削除する文字列の先頭ポインタ。
 * @param key_len		削除する文字列の文字列長。
 */
void stringhash_remove_partition(const StringHash * instance, stringhash_key key, size_t key_len);

//-----------------------------------------------------------------------------
// デバッグ
//-----------------------------------------------------------------------------
/**
 * 管理している文字列を画面に出力する。
 *
 * @param instance	対象ハッシュテーブル。
 */
void stringhash_show(const StringHash * instance);

#endif /* __STRING_HASH_H__ */