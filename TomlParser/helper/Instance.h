#ifndef __INSTANCE_H__
#define __INSTANCE_H__

/**
 * 構造体の実体を管理する機能。
 *
 * Instance instance = instance_initialize(sizeof(Data), 3);	// Data構造体を使用することを宣言
 *
 * Data * data = instance_pop(&instance);	// 管理領域から Data構造体への参照を取得
 *
 * instance_push(&instance, data);			// 使い終わった実体は管理機能に返すこと
 *
 * instance_delete(&instance);				// 最後に解放してください
 *
 * Copyright (c) 2018 Takashi Zota
 * Released under the MIT license
 * https://opensource.org/licenses/mit-license.php
 */

/**
 * インスタンス管理機能。
 */
typedef struct _Instance
{
	// 一個のデータ長
	size_t			data_len;

	// 一回に生成する個数
	size_t			block_size;

	// 実体管理領域
	//
	// 1. 実体ブロック個数
	// 2. 実体領域
	struct {
		int			block_count;	// 1
		void **		block;			// 2
	} entities;

	// スタック管理領域
	//
	// 1. スタック残個数
	// 2. スタックデータ
	struct {
		int			count;			// 1
		void **		cache;			// 2
	} stack;

} Instance;

/**
 * コンストラクタ／デストラクタアクション。
 *
 * @param target	対象インスタンス。
 */
typedef void(*instance_build_function)(void * target);

/**
 * インスタンス管理機能の初期化を行う。
 *
 * @param data_len		一個のデータ長。
 * @param block_size	一回に生成する個数。
 * @return				インスタンス管理機能。
 */
Instance instance_initialize(size_t data_len, size_t block_size);

/**
 * インスタンス管理機能を削除する。
 *
 * @param ins_del		削除する管理機能。
 */
void instance_delete(Instance * ins_del);

/**
 * インスタンス管理機能を削除する。
 *
 * @param ins_del		削除する管理機能。
 * @param build_action	デストラクタアクション。
 */
void instance_delete_all_destructor(Instance * ins_del, instance_build_function build_action);

/**
 * インスタンス管理機能にデータを返す。
 *
 * @param instance		インスタンス管理機能。
 * @param item			返すデータ。
 */
void instance_push(Instance * instance, void * item);

/**
 * インスタンス管理機能からデータを取得する。
 *
 * @param instance		インスタンス管理機能。
 * @return				データ。
 */
void * instance_pop(Instance * instance);

/**
 * インスタンス管理機能からデータを取得する（コンストラクタ実行）
 *
 * @param instance		インスタンス管理機能。
 * @param build_action	コンストラクタアクション。
 * @return				データ。
 */
void * instance_pop_constructor(Instance * instance, instance_build_function build_action);

/**
 * インスタンス管理機能にデータを返す（デストラクタ実行）
 *
 * @param instance		インスタンス管理機能。
 * @param build_action	デストラクタアクション。
 */
void instance_push_destructor(Instance * instance, void * item, instance_build_function build_action);

#endif /* __INSTANCE_H__ */