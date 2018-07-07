#ifndef __ASSERTION_H__
#define __ASSERTION_H__

/**
 * アサーションチェック用のマクロ群
 *
 * Assert(expr, msg)	// expr が真で無ければ msgをエラー出力して処理をアボートさせる
 * Abort(msg)			// msgをエラー出力して処理をアボートさせる
 *
 * Copyright (c) 2018 Takashi Zota
 * Released under the MIT license
 * https://opensource.org/licenses/mit-license.php
 */

// アサートチェック
#define Assert(expr, msg) \
	if (!(expr)) {	\
		fprintf(stderr, "%s\n %s:%d\n", msg, __FILE__, __LINE__); \
		abort(); \
	};

// 異常終了マクロ
#define Abort(msg) \
	do {	\
		fprintf(stderr, "%s\n %s:%d\n", msg, __FILE__, __LINE__); \
		abort(); \
	} while(0);

#endif /* __ASSERTION_H__ */