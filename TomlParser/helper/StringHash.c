#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <crtdbg.h>
#include "Assertion.h"
#include "StringHash.h"

/**
 * 内部実装構造体。
 *   カプセル化のため非公開
 */
typedef struct _StringHashImpl
{
	// 公開情報の実装
	StringHash			hash;

	// 管理文字列テーブル
	stringhash_key *	tables;

	// 管理文字列テーブルのサイズ
	size_t				capacity;

} StringHashImpl;

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
    for(i = 0, ptr = bytes; i < length; ++i, ++ptr) {
        hash = (16777619U * hash) ^ (*ptr);
    }

    return hash;
}

/**
 * 要素確認機能（内部実装）
 *
 * @param tables		ハッシュテーブル領域。
 * @param table_size	ハッシュテーブルサイズ。
 * @param key			検索文字列。
 * @param result		文字列が存在したとき、登録文字列のポインタ（戻り値）
 * @param point			文字列が存在したとき、登録文字列のテーブル位置（戻り値）
 * @return				項目が存在すれば 0以外。
 */
static int stringhash_inner_contains(stringhash_key * tables,
									 size_t table_size,
									 stringhash_key key,
									 stringhash_key * result,
									 size_t * point)
{
	size_t len = strlen(key);
	size_t ptr;
	size_t hash_pos;
	stringhash_key find;

	// 文字列のハッシュ値を取得
	size_t hash_val = fnv_1_hash_32(key, len);

	// ハッシュテーブルの走査
	//
	// 1. テーブル位置の計算
	// 2. 要素を取得
	//    2-1. 要素がなければ無しを返す
	//    2-2. 要素があり、文字列が一致するならば有りを返す
	for (ptr = 0; ptr <= table_size / 2; ++ptr) {
		hash_pos = (hash_val + ptr * ptr) % table_size;		// 1

		find = tables[hash_pos];							// 2

		if (!find) {
			*point = hash_pos;								// 2-1
			return 0;
		}
		else {
			if (strcmp(find, key) == 0) {					// 2-2
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
static void stringhash_realloc(StringHashImpl * impl)
{
	unsigned long long int	new_size = ((unsigned long long int)impl->capacity) * 2;
	unsigned long long int	div;
	stringhash_key * temp;
	stringhash_key * ptr;
	stringhash_key result;
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
	temp = (const char **)malloc(sizeof(const char *) * (size_t)new_size);
	Assert(temp != 0, "realloc error");
	_RPTN(_CRT_WARN, "stringhash_realloc malloc 0x%X\n", temp);
	memset((void*)temp, 0, sizeof(const char *) * (size_t)new_size);

	// 登録済みの文字列を順に格納
	if (impl->tables) {
		ptr = impl->tables;
		for (ptr = impl->tables; ptr < impl->tables + impl->capacity; ++ptr) {
			if (*ptr) {
				stringhash_inner_contains(temp, new_size, *ptr, &result, &point);
				temp[point] = *ptr;
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
 * 文字列ハッシュテーブルの初期化を行う。
 *
 * @return				インスタンス。
 */
const StringHash * stringhash_initialize()
{
	StringHashImpl * res = (StringHashImpl*)malloc(sizeof(StringHashImpl));
	Assert(res != 0, "alloc error");
	_RPTN(_CRT_WARN, "stringhash_initialize malloc 0x%X\n", res);

	memset(res, 0, sizeof(StringHashImpl));
	res->capacity = 16;
	stringhash_realloc(res);
	return (StringHash*)res;
}

/**
 * 文字列ハッシュテーブルを削除する。
 *
 * @param ins_del		削除するインスタンス。
 */
void stringhash_delete(const StringHash ** ins_del)
{
	StringHashImpl ** impl = (StringHashImpl**)ins_del;
	const char ** ptr;

	Assert(ins_del != 0, "null pointer del error");
	
	ptr = (*impl)->tables;
	for (ptr = (*impl)->tables;
			ptr < (*impl)->tables + (*impl)->capacity; ++ptr) {
		if (*ptr) {
			free((char*)*ptr);
		}
	}
	
	free((void*)(*impl)->tables);
	free(*impl);
	*ins_del = 0;
}

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
int stringhash_contains(const StringHash * instance, stringhash_key key, stringhash_key * result)
{
	StringHashImpl * impl = (StringHashImpl*)instance;
	size_t point;
	return stringhash_inner_contains(impl->tables, impl->capacity, key, result, &point);
}

/**
 * 文字列の追加を行う。
 *
 * @param instance		対象ハッシュテーブル。
 * @param key			追加する文字列。
 * @return				ハッシュテーブルに追加された文字列のポインタ。
 */
stringhash_key stringhash_add(const StringHash * instance, stringhash_key key)
{
	StringHashImpl * impl = (StringHashImpl*)instance;
	const char * result;
	const char * item;
	size_t point;
	size_t len;

	// 追加処理
	//
	// 1. 既に追加済みならば登録されている文字列を返す
	// 2. 登録されていなければ追加を行う
	//    2-1. 追加する文字列のポインタを作成
	//    2-2. キー文字列をコピーし、テーブルに追加
	//    2-3. テーブルが一杯になったら再構成
	if (stringhash_inner_contains(impl->tables, impl->capacity, key, &result, &point)) {
		return result;								// 1
	}
	else {
		len = strlen(key);							// 2-1
		item = (char*)malloc(len + 1);
		Assert(item != 0, "item new error");
		_RPTN(_CRT_WARN, "stringhash_add malloc 0x%X\n", item);

		strncpy_s((char*)item, len + 1, key, len);	// 2-2
		impl->tables[point] = item;
		impl->hash.count++;

		if ((double)impl->hash.count > impl->capacity * 0.7) {
			stringhash_realloc(impl);				// 2-3
		}

		return item;
	}
}

/**
 * 文字列の追加を行う（文字列の部分）
 *
 * @param instance		対象ハッシュテーブル。
 * @param key			追加する文字列。
 * @param key_len		文字列長。
 * @return				ハッシュテーブルに追加された文字列のポインタ。
 */
stringhash_key stringhash_add_partition(const StringHash * instance, stringhash_key key, size_t key_len)
{
	StringHashImpl * impl = (StringHashImpl*)instance;
	const char * result;
	size_t point;
	char 	key8[8];
	char 	key16[16];
	char 	key32[32];
	char *	new_key;
	int		created = 0;

	// 文字列を取得する
	if (key_len < 8) {
		memset(key8, 0, sizeof(key8));
		new_key = key8;
	}
	else if (key_len < 16) {
		memset(key16, 0, sizeof(key16));
		new_key = key16;
	}
	else if (key_len < 32) {
		memset(key32, 0, sizeof(key32));
		new_key = key32;
	}
	else {
		new_key = (char*)malloc(key_len + 1);
		Assert(new_key != 0, "item new error");
		_RPTN(_CRT_WARN, "stringhash_add_partition malloc 0x%X\n", new_key);
		created = 1;
	}
	memcpy(new_key, key, key_len);

	// 追加処理
	//
	// 1. 既に追加済みならば登録されている文字列を返す
	// 2. 登録されていなければ追加を行う
	//    2-1. 追加する文字列のポインタを作成
	//    2-2. キー文字列をコピーし、テーブルに追加
	//    2-3. テーブルが一杯になったら再構成
	if (stringhash_inner_contains(impl->tables, impl->capacity, new_key, &result, &point)) {
		if (created) {
			free(new_key);
		}
		return result;									// 1
	}
	else {
		if (!created) {									// 2-1
			new_key = (char*)malloc(key_len + 1);
			Assert(new_key != 0, "item new error");
			_RPTN(_CRT_WARN, "stringhash_add_partition malloc 0x%X\n", new_key);

			strncpy_s(new_key, key_len + 1, key, key_len);	// 2-2
		}
		impl->tables[point] = new_key;
		impl->hash.count++;

		if ((double)impl->hash.count > impl->capacity * 0.7) {
			stringhash_realloc(impl);					// 2-3
		}

		return new_key;
	}
}

/**
* ハッシュテーブルより、指定の文字列を削除する。
*
* @param instance		対象ハッシュテーブル。
* @param key			削除する文字列。
*/
void stringhash_remove(const StringHash * instance, stringhash_key key)
{
	StringHashImpl * impl = (StringHashImpl*)instance;
	const char * result;
	size_t point;

	// 項目が存在すれば削除する
	if (stringhash_inner_contains(impl->tables, impl->capacity, key, &result, &point)) {
		free((char*)impl->tables[point]);
		impl->tables[point] = 0;
		impl->hash.count--;
	}
}

/**
* ハッシュテーブルより、指定の文字列を削除する。
*
* @param instance		対象ハッシュテーブル。
* @param key			削除する文字列の先頭ポインタ。
* @param key_len		削除する文字列の文字列長。
*/
void stringhash_remove_partition(const StringHash * instance, stringhash_key key, size_t key_len)
{
	// キー文字列を作成
	char * new_key = (char*)malloc(key_len + 1);
	Assert(new_key != 0, "item new error");
	_RPTN(_CRT_WARN, "stringhash_remove_partition malloc 0x%X\n", new_key);

	// 文字列を削除
	memcpy(new_key, key, key_len);
	new_key[key_len] = 0;
	stringhash_remove(instance, new_key);

	// キー文字列を解放
	free(new_key);
}

//-----------------------------------------------------------------------------
// デバッグ
//-----------------------------------------------------------------------------
/**
* 管理している文字列を画面に出力する。
*
* @param instance	対象インスタンス。
*/
void stringhash_show(const StringHash * instance)
{
	StringHashImpl * impl = (StringHashImpl*)instance;
	stringhash_key * ptr;
	
	ptr = impl->tables;
	for (ptr = impl->tables;
			ptr < impl->tables + impl->capacity; ++ptr) {
		if (*ptr) {
			fprintf(stdout, "%s\n", *ptr);
		}
	}
}