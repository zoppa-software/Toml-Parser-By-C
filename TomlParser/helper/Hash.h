#ifndef __HASH_H__
#define __HASH_H__

/**
 * キーのバイト長を指定したハッシュテーブル。
 *
 * const Hash * hash = hash_initialize(4);				// ハッシュテーブルの初期化
 * HashPair result;
 *
 * hash_add(hash, "ABCD", hash_value_of_integer(1));	// 項目追加
 * hash_add(hash, "EFG ", hash_value_of_integer(2));	// ※ キーは初期化時に指定した 4byte文字列
 * hash_add(hash, "HIJK", hash_value_of_integer(3));
 *
 * if (hash_contains(hash, "HIJK", &result)) {			// 項目の存在確認
 *		printf_s("hit %4.4s, %d\n", ((const char *)result.key), result.value.integer);
 * }
 * else {
 *		printf_s("not regist \"HIJK\"\n");
 * }
 *
 * hash_remove(hash, "HIJK");							// 項目の存在確認
 *
 * hash_delete(&hash);									// ハッシュテーブルの削除
 */

//-----------------------------------------------------------------------------
// 構造体
//-----------------------------------------------------------------------------
/**
 * 値構造体。
 */
typedef union _HashValue
{
	// 整数値。
	int			integer;

	// 実数値。
	double		float_number;

	// オブジェクト参照
	void *		object;

} HashValue;

/**
 * キーと値のペア構造体。
 */
typedef struct _HashPair
{
	// キー参照。
	const void *	key;

	// 値。
	HashValue		value;

} HashPair;

/**
 * 文字列ハッシュテーブル。
 */
typedef struct _Hash
{
	// 格納項目数
	size_t			count;

} Hash;

//-----------------------------------------------------------------------------
// コンストラクタ、デストラクタ
//-----------------------------------------------------------------------------
/**
 * ハッシュテーブルの初期化を行う。
 *
 * @param key_length	キーのサイズ（バイト長）
 * @return				インスタンス。
 */
const Hash * hash_initialize(size_t key_length);

/**
 * ハッシュテーブルを削除する。
 *
 * @param ins_del		削除するインスタンス。
 */
void hash_delete(const Hash ** ins_del);

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
int hash_contains(const Hash * instance, const void * key, HashPair * result);

/**
 * 項目の追加を行う。
 *
 * @param instance		対象ハッシュテーブル。
 * @param key			追加する項目のキー。
 * @param value			追加する項目。
 * @return				ハッシュテーブルに追加した項目ペア。
 */
HashPair hash_add(const Hash * instance, const void * key, HashValue value);

/**
 * ハッシュテーブルより、指定のキーの項目を削除する。
 *
 * @param instance		対象ハッシュテーブル。
 * @param key			削除する項目のキー。
 */
void hash_remove(const Hash * instance, const void * key);

//-----------------------------------------------------------------------------
// デバッグ
//-----------------------------------------------------------------------------
/**
 * 表示処理委譲関数ポインタ。
 *
 * @param pair	表示する項目。
 */
typedef void(*hash_show_function)(HashPair pair);

/**
 * 管理している項目を画面に出力する。
 *
 * @param instance			対象ハッシュテーブル。
 * @param show_function		処理移譲関数ポインタ。
 */
void hash_show(const Hash * instance, hash_show_function show_function);

/**
 * 繰返処理委譲関数ポインタ。
 *
 * @param pair	表示する項目。
 * @param param	パラメータ。
 */
typedef void(*hash_for_function)(HashPair pair, void * param);

/**
 * 管理している項目を順に評価する。
 *
 * @param instance			対象ハッシュテーブル。
 * @param for_function		処理移譲関数ポインタ。
 * @param param				パラメータ。
 */
void hash_foreach(const Hash * instance, hash_for_function for_function, void * param);

//-----------------------------------------------------------------------------
// 値変換
//-----------------------------------------------------------------------------
/**
 * ハッシュテーブルに登録する値を作成（整数値）
 *
 * @param value		登録する値。
 * @return			値構造体。
 */
HashValue hash_value_of_integer(int value);

/**
 * ハッシュテーブルに登録する値を作成（実数値）
 *
 * @param value		登録する値。
 * @return			値構造体。
 */
HashValue hash_value_of_float(double value);

/**
 * ハッシュテーブルに登録する値を作成（ポインタ）
 *
 * @param value		登録する値。
 * @return			値構造体。
 */
HashValue hash_value_of_pointer(void * value);

#endif /* __HASH_H__ */