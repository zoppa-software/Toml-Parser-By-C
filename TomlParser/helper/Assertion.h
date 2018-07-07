#ifndef __ASSERTION_H__
#define __ASSERTION_H__

/**
 * �A�T�[�V�����`�F�b�N�p�̃}�N���Q
 *
 * Assert(expr, msg)	// expr ���^�Ŗ������ msg���G���[�o�͂��ď������A�{�[�g������
 * Abort(msg)			// msg���G���[�o�͂��ď������A�{�[�g������
 *
 * Copyright (c) 2018 Takashi Zota
 * Released under the MIT license
 * https://opensource.org/licenses/mit-license.php
 */

// �A�T�[�g�`�F�b�N
#define Assert(expr, msg) \
	if (!(expr)) {	\
		fprintf(stderr, "%s\n %s:%d\n", msg, __FILE__, __LINE__); \
		abort(); \
	};

// �ُ�I���}�N��
#define Abort(msg) \
	do {	\
		fprintf(stderr, "%s\n %s:%d\n", msg, __FILE__, __LINE__); \
		abort(); \
	} while(0);

#endif /* __ASSERTION_H__ */