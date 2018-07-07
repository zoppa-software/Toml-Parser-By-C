#ifndef __ENCODING_H__
#define __ENCODING_H__

/**
 * �����R�[�h��ϊ����邽�߂̊֐��B
 *
 *	char	buf[8];
 *	char	ans[8];
 *
 *	buf[0] = 0xe5;
 *  buf[1] = 0xb3;
 *	buf[2] = 0xaf;
 *	buf[3] = 0;
 *
 *	printf_s("%s\n", encoding_utf8_to_cp932(buf, ans, sizeof(ans)));
 *
 * Copyright (c) 2018 Takashi Zota
 * Released under the MIT license
 * https://opensource.org/licenses/mit-license.php
 */

/**
 * �����R�[�h��ϊ�����iUTF8 -> cp932�j
 *
 * @param input			���͕�����B
 * @param buffer		�ϊ��p�o�b�t�@�B
 * @param buffer_size	�ϊ��p�o�b�t�@�T�C�Y�B
 */
const char * encoding_utf8_to_cp932(const char * input, char * buffer, size_t buffer_size);

#endif /* __ENCODING_H__ */