#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <crtdbg.h>
#include "Assertion.h"
#include "Instance.h"
#include "Hash.h"

/** キー項目生成ブロックサイズ。 */
#define KEY_BLOCK_SIZE		32

/**
 * 内部実装構造体。
 *   カプセル化のため非公開
 */
typedef struct _HashImpl
{
	// 公開情報の実装
	Hash				hash;

	// キー情報の長さ
	size_t				key_length;

	// キー情報の実体リスト
	Instance			key_instance;

	// 管理文字列テーブル
	HashPair *			tables;

	// 管理文字列テーブルのサイズ
	size_t				capacity;

} HashImpl;

//-----------------------------------------------------------------------------
// 内部機能
//-----------------------------------------------------------------------------
/**
 * fnv1 ハッシュキー作成アルゴリズム。
 *
 * @param bytes		キー文字列。
 * @param length	文字列長。
 * @return			ハッシュ値。
 */
static size_t fnv_1_hash_32(const char * bytes, size_t length)
{
	size_t hash;
	const char * ptr;
	size_t i;

	hash = 2166136261U;
	for (i = 0, ptr = bytes; i < length; ++i, ++ptr) {
		hash = (16777619U * hash) ^ (*ptr);
	}

	return hash;
}

/**
 * 要素確認機能（内部実装）
 *
 * @param tables		ハッシュテーブル領域。
 * @param table_size	ハッシュテーブルサイズ。
 * @param key			検索キー。
 * @param key_len		キーのバイト長。
 * @param result		項目が存在したとき、キー／値のペア（戻り値）
 * @param point			項目が存在したとき、登録先のテーブル位置（戻り値）
 * @return				項目が存在すれば 0以外。
 */
static int hash_inner_contains(HashPair *	tables,
							   size_t		table_size,
							   const void *	key,
							   size_t		key_len,
							   HashPair *	result,
							   size_t *		point)
{
	size_t ptr;
	size_t hash_pos;
	HashPair find;

	// 文字列のハッシュ値を取得
	size_t hash_val = fnv_1_hash_32((char *)key, key_len);

	// ハッシュテーブルの走査
	//
	// 1. テーブル位置の計算
	// 2. 要素を取得
	//    2-1. 要素がなければ無しを返す
	//    2-2. 要素があり、文字列が一致するならば有りを返す
	for (ptr = 0; ptr <= table_size / 2; ++ptr) {
		hash_pos = (hash_val + ptr * ptr) % table_size;		// 1

		find = tables[hash_pos];							// 2

		if (!find.key) {
			*point = hash_pos;								// 2-1
			return 0;
		}
		else {
			if (memcmp(find.key, (char *)key, key_len) == 0) {	// 2-2
				*result = find;
				*point = hash_pos;
				return 1;
			}
		}
	}

	return 0;
}

/**
 * ハッシュテーブルの再構成。
 *
 * @param impl		再構成するハッシュテーブル。
 */
static void hash_realloc(HashImpl * impl)
{
	unsigned long long int	new_size = ((unsigned long long int)impl->capacity) * 2;
	unsigned long long int	div;
	HashPair * temp;
	HashPair * ptr;
	HashPair result;
	int hit = 0;
	size_t point;

	// 拡張したサイズ以下の素数を求める
	for (; new_size < UINT_MAX; ++new_size) {
		if ((new_size & 1) == 1) {
			hit = 0;

			for (div = 3; (double)div <= sqrt((double)new_size); div += 2) {
				if (new_size % div == 0) {
					hit = 1;
					break;
				}
			}

			if (!hit) {
				break;
			}
		}
	}

	// 上記サイズで領域を再作成
	temp = (HashPair*)malloc(sizeof(HashPair) * (size_t)new_size);
	Assert(temp != 0, "realloc error");
	_RPTN(_CRT_WARN, "hash_realloc malloc 0x%X\n", temp);
	memset((void*)temp, 0, sizeof(HashPair) * (size_t)new_size);

	// 登録済みの文字列を順に格納
	if (impl->tables) {
		ptr = impl->tables;
		for (ptr = impl->tables; ptr < impl->tables + impl->capacity; ++ptr) {
			if (ptr->key) {
				hash_inner_contains(temp, new_size, ptr->key, impl->key_length, &result, &point);
				temp[point].key = ptr->key;
				temp[point].value = ptr->value;
			}
		}
	}

	// 過去の領域を解放
	free((void*)impl->tables);

	// 領域の参照、サイズを更新
	impl->tables = temp;
	impl->capacity = new_size;
}

//-----------------------------------------------------------------------------
// コンストラクタ、デストラクタ
//-----------------------------------------------------------------------------
/**
 * ハッシュテーブルの初期化を行う。
 *
 * @param key_length	キーのサイズ（バイト長）
 * @return				インスタンス。
 */
const Hash * hash_initialize(size_t key_length)
{
	HashImpl * res = (HashImpl*)malloc(sizeof(HashImpl));
	Assert(res != 0, "alloc error");
	_RPTN(_CRT_WARN, "hash_initialize malloc 0x%X\n", res);

	memset(res, 0, sizeof(HashImpl));
	res->capacity = 16;
	res->key_length = key_length;
	res->key_instance = instance_initialize(key_length, KEY_BLOCK_SIZE);
	hash_realloc(res);
	return (Hash*)res;
}

/**
 * ハッシュテーブルを削除する。
 *
 * @param ins_del		削除するインスタンス。
 */
void hash_delete(const Hash ** ins_del)
{
	HashImpl ** impl = (HashImpl**)ins_del;

	Assert(ins_del != 0, "null pointer del error");

	instance_delete(&(*impl)->key_instance);

	free((void*)(*impl)->tables);
	free(*impl);
	*ins_del = 0;
}

//-----------------------------------------------------------------------------
// 確認、追加、削除
//-----------------------------------------------------------------------------
/**
 * 項目の存在確認を行う。
 *
 * @param instance		対象ハッシュテーブル。
 * @param key			確認するキー。
 * @param result		項目が存在したとき、存在項目のポインタ（戻り値）
 * @return				項目が存在するとき 0以外を返す。
 */
int hash_contains(const Hash * instance, const void * key, HashPair * result)
{
	HashImpl * impl = (HashImpl*)instance;
	size_t point;
	return hash_inner_contains(impl->tables, impl->capacity, key, impl->key_length, result, &point);
}

/**
 * 項目の追加を行う。
 *
 * @param instance		対象ハッシュテーブル。
 * @param key			追加する項目のキー。
 * @param value			追加する項目。
 * @return				ハッシュテーブルに追加した項目ペア。
 */
HashPair hash_add(const Hash * instance, const void * key, HashValue value)
{
	HashImpl * impl = (HashImpl*)instance;
	HashPair result;
	void * item;
	size_t point;

	// 追加処理
	//
	// 1. 既に追加済みならば登録されている項目を返す
	// 2. 登録されていなければ追加を行う
	//    2-1. 追加するキー情報のポインタを取得
	//    2-2. キー情報をコピーし、テーブルに追加
	//    2-3. テーブルが一杯になったら再構成
	if (hash_inner_contains(impl->tables, impl->capacity, key, impl->key_length, &result, &point)) {
		return result;									// 1
	}
	else {
		item = instance_pop(&impl->key_instance);		// 2-1
	
		memcpy(item, key, impl->key_length);			// 2-2
		result.key = item;
		result.value = value;
		impl->tables[point] = result;
		impl->hash.count++;
	
		if ((double)impl->hash.count > impl->capacity * 0.7) {
			hash_realloc(impl);							// 2-3
		}
	
		return result;
	}
}

/**
 * ハッシュテーブルより、指定のキーの項目を削除する。
 *
 * @param instance		対象ハッシュテーブル。
 * @param key			削除する項目のキー。
 */
void hash_remove(const Hash * instance, const void * key)
{
	HashImpl *	impl = (HashImpl*)instance;
	HashPair	result;
	size_t		point;
	
	// 項目が存在すれば削除する
	if (hash_inner_contains(impl->tables, impl->capacity, key, impl->key_length, &result, &point)) {
		instance_push(&impl->key_instance, (void*)impl->tables[point].key);

		memset(impl->tables + point, 0, sizeof(HashPair));
		impl->hash.count--;
	}
}

//-----------------------------------------------------------------------------
// デバッグ
//-----------------------------------------------------------------------------
/**
 * 管理している項目を画面に出力する。
 *
 * @param instance			対象ハッシュテーブル。
 * @param show_function		処理移譲関数ポインタ。
 */
void hash_show(const Hash * instance,
			   hash_show_function show_function)
{
	HashImpl * impl = (HashImpl*)instance;
	HashPair * ptr;
	
	ptr = impl->tables;
	for (ptr = impl->tables;
		 ptr < impl->tables + impl->capacity; ++ptr) {
		if (ptr->key) {
			show_function(*ptr);
		}
	}
}

/**
 * 管理している項目を順に評価する。
 *
 * @param instance			対象ハッシュテーブル。
 * @param for_function		処理移譲関数ポインタ。
 * @param param				パラメータ。
 */
void hash_foreach(const Hash * instance,
				  hash_for_function for_function,
				  void * param)
{
	HashImpl * impl = (HashImpl*)instance;
	HashPair * ptr;
	
	ptr = impl->tables;
	for (ptr = impl->tables;
		 ptr < impl->tables + impl->capacity; ++ptr) {
		if (ptr->key) {
			for_function(*ptr, param);
		}
	}
}

//-----------------------------------------------------------------------------
// 値変換
//-----------------------------------------------------------------------------
/**
 * ハッシュテーブルに登録する値を作成（整数値）
 *
 * @param value		登録する値。
 * @return			値構造体。
 */
HashValue hash_value_of_integer(int value)
{
	HashValue res;
	res.integer = value;
	return res;
}

/**
 * ハッシュテーブルに登録する値を作成（実数値）
 *
 * @param value		登録する値。
 * @return			値構造体。
 */
HashValue hash_value_of_float(double value)
{
	HashValue res;
	res.float_number = value;
	return res;
}

/**
 * ハッシュテーブルに登録する値を作成（ポインタ）
 *
 * @param value		登録する値。
 * @return			値構造体。
 */
HashValue hash_value_of_pointer(void * value)
{
	HashValue res;
	res.object = value;
	return res;
}