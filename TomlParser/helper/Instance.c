#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crtdbg.h>
#include "Assertion.h"
#include "Instance.h"

// 管理する実体領域を生成する
static void instance_append(Instance * instance);

/**
 * インスタンス管理機能の初期化を行う。
 *
 * @param data_len		一個のデータ長。
 * @param block_size	一回に生成する個数。
 * @return				インスタンス管理機能。
 */
Instance instance_initialize(size_t data_len, size_t block_size)
{
	Instance res;
	memset(&res, 0, sizeof(Instance));
	res.data_len = data_len;
	res.block_size = block_size;
	return res;
}

/**
 * インスタンス管理機能を削除する。
 *
 * @param ins_del		削除する管理機能。
 */
void instance_delete(Instance * ins_del)
{
	int		i;

	// 枝要素解放
	for (i = 0; i < ins_del->entities.block_count; ++i) {
		free(ins_del->entities.block[i]);
	}
	free(ins_del->entities.block);
	free(ins_del->stack.cache);
}

/**
 * インスタンス管理機能を削除する。
 *
 * @param ins_del		削除する管理機能。
 * @param build_action	デストラクタアクション。
 */
void instance_delete_all_destructor(Instance * ins_del,
									instance_build_function build_action)
{
	int		i, j;

	// 枝要素解放
	for (i = 0; i < ins_del->entities.block_count; ++i) {
		for (j = 0; j < ins_del->block_size; ++j) {
			build_action((char*)ins_del->entities.block[i] + (ins_del->data_len * j));
		}
		free(ins_del->entities.block[i]);
	}
	free(ins_del->entities.block);
	free(ins_del->stack.cache);
}

/**
* インスタンス管理機能にデータを返す。
*
* @param instance		インスタンス管理機能。
* @param item			返すデータ。
*/
void instance_push(Instance * instance, void * item)
{
	instance->stack.cache[instance->stack.count++] = item;
}

/**
* インスタンス管理機能からデータを取得する。
*
* @param instance		インスタンス管理機能。
* @return				データ。
*/
void * instance_pop(Instance * instance)
{
	return instance_pop_constructor(instance, 0);
}

/**
 * 管理する実体領域を生成する。
 *
 * @param instance		インスタンス管理機能。
 */
void instance_append(Instance * instance)
{
	char ** temp;
	char * newins;
	size_t i, len;

	// 要素の実体を生成
	//
	// 1. 過去分を退避
	// 2. 新規領域を追加
	// 3. 入替
	temp = (char**)malloc(sizeof(char*) * (instance->entities.block_count + 1));	// 1
	Assert(temp != 0, "instance add error");
	_RPTN(_CRT_WARN, "instance_append malloc 1 0x%X\n", temp);
	memcpy(temp, instance->entities.block, sizeof(char*) * instance->entities.block_count);

	newins = (char*)malloc(instance->data_len * instance->block_size);				// 2
	Assert(newins != 0, "instance add error");
	_RPTN(_CRT_WARN, "instance_append malloc 2 0x%X\n", newins);
	memset(newins, 0, instance->data_len * instance->block_size);
	temp[instance->entities.block_count] = newins;

	free(instance->entities.block);													// 3
	instance->entities.block = temp;
	instance->entities.block_count++;

	// 要素をスタックに積む
	//
	// 1. 過去分を含めて生成
	// 2. 入替
	len = instance->entities.block_count * instance->block_size;					// 1
	temp = (char**)malloc(sizeof(char*) * len);
	Assert(temp != 0, "instance add error");
	_RPTN(_CRT_WARN, "instance_append malloc 3 0x%X\n", temp);
	memset(temp, 0, sizeof(char*) * len);
	memcpy(temp, instance->stack.cache, sizeof(char*) * instance->stack.count);

	free(instance->stack.cache);													// 2
	instance->stack.cache = temp;

	// スタックに追加
	for (i = 0; i < instance->block_size; ++i) {
		instance_push(instance, newins + (instance->data_len * i));
	}
}

/**
 * インスタンス管理機能からデータを取得する（コンストラクタ実行）
 *
 * @param instance		インスタンス管理機能。
 * @param build_action	コンストラクタアクション。
 * @return				データ。
 */
void * instance_pop_constructor(Instance * instance, instance_build_function build_action)
{
	void * ans;

	// 余っていなければ枝要素を追加
	if (instance->stack.count <= 0) {
		instance_append(instance);
	}

	// スタックから取得
	instance->stack.count--;
	ans = instance->stack.cache[instance->stack.count];
	instance->stack.cache[instance->stack.count] = 0;

	// コンストラクタ実行
	if (build_action != 0) {
		build_action(ans);
	}

	return ans;
}

/**
 * インスタンス管理機能にデータを返す（デストラクタ実行）
 *
 * @param instance		インスタンス管理機能。
 * @param item			返すデータ。
 * @param build_action	デストラクタアクション。
 */
void instance_push_destructor(Instance * instance, void * item, instance_build_function build_action)
{
	build_action(item);
	instance->stack.cache[instance->stack.count++] = item;
}