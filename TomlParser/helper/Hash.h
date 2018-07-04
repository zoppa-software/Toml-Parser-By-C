#ifndef __HASH_H__
#define __HASH_H__

/**
 * �L�[�̃o�C�g�����w�肵���n�b�V���e�[�u���B
 *
 * const Hash * hash = hash_initialize(4);				// �n�b�V���e�[�u���̏�����
 * HashPair result;
 *
 * hash_add(hash, "ABCD", hash_value_of_integer(1));	// ���ڒǉ�
 * hash_add(hash, "EFG ", hash_value_of_integer(2));	// �� �L�[�͏��������Ɏw�肵�� 4byte������
 * hash_add(hash, "HIJK", hash_value_of_integer(3));
 *
 * if (hash_contains(hash, "HIJK", &result)) {			// ���ڂ̑��݊m�F
 *		printf_s("hit %4.4s, %d\n", ((const char *)result.key), result.value.integer);
 * }
 * else {
 *		printf_s("not regist \"HIJK\"\n");
 * }
 *
 * hash_remove(hash, "HIJK");							// ���ڂ̑��݊m�F
 *
 * hash_delete(&hash);									// �n�b�V���e�[�u���̍폜
 */

//-----------------------------------------------------------------------------
// �\����
//-----------------------------------------------------------------------------
/**
 * �l�\���́B
 */
typedef union _HashValue
{
	// �����l�B
	int			integer;

	// �����l�B
	double		float_number;

	// �I�u�W�F�N�g�Q��
	void *		object;

} HashValue;

/**
 * �L�[�ƒl�̃y�A�\���́B
 */
typedef struct _HashPair
{
	// �L�[�Q�ƁB
	const void *	key;

	// �l�B
	HashValue		value;

} HashPair;

/**
 * ������n�b�V���e�[�u���B
 */
typedef struct _Hash
{
	// �i�[���ڐ�
	size_t			count;

} Hash;

//-----------------------------------------------------------------------------
// �R���X�g���N�^�A�f�X�g���N�^
//-----------------------------------------------------------------------------
/**
 * �n�b�V���e�[�u���̏��������s���B
 *
 * @param key_length	�L�[�̃T�C�Y�i�o�C�g���j
 * @return				�C���X�^���X�B
 */
const Hash * hash_initialize(size_t key_length);

/**
 * �n�b�V���e�[�u�����폜����B
 *
 * @param ins_del		�폜����C���X�^���X�B
 */
void hash_delete(const Hash ** ins_del);

//-----------------------------------------------------------------------------
// �m�F�A�ǉ��A�폜
//-----------------------------------------------------------------------------
/**
 * ���ڂ̑��݊m�F���s���B
 *
 * @param instance		�Ώۃn�b�V���e�[�u���B
 * @param key			�m�F����L�[�B
 * @param result		���ڂ����݂����Ƃ��A���ݍ��ڂ̃|�C���^�i�߂�l�j
 * @return				���ڂ����݂���Ƃ� 0�ȊO��Ԃ��B
 */
int hash_contains(const Hash * instance, const void * key, HashPair * result);

/**
 * ���ڂ̒ǉ����s���B
 *
 * @param instance		�Ώۃn�b�V���e�[�u���B
 * @param key			�ǉ����鍀�ڂ̃L�[�B
 * @param value			�ǉ����鍀�ځB
 * @return				�n�b�V���e�[�u���ɒǉ��������ڃy�A�B
 */
HashPair hash_add(const Hash * instance, const void * key, HashValue value);

/**
 * �n�b�V���e�[�u�����A�w��̃L�[�̍��ڂ��폜����B
 *
 * @param instance		�Ώۃn�b�V���e�[�u���B
 * @param key			�폜���鍀�ڂ̃L�[�B
 */
void hash_remove(const Hash * instance, const void * key);

//-----------------------------------------------------------------------------
// �f�o�b�O
//-----------------------------------------------------------------------------
/**
 * �\�������Ϗ��֐��|�C���^�B
 *
 * @param pair	�\�����鍀�ځB
 */
typedef void(*hash_show_function)(HashPair pair);

/**
 * �Ǘ����Ă��鍀�ڂ���ʂɏo�͂���B
 *
 * @param instance			�Ώۃn�b�V���e�[�u���B
 * @param show_function		�����ڏ��֐��|�C���^�B
 */
void hash_show(const Hash * instance, hash_show_function show_function);

/**
 * �J�ԏ����Ϗ��֐��|�C���^�B
 *
 * @param pair	�\�����鍀�ځB
 * @param param	�p�����[�^�B
 */
typedef void(*hash_for_function)(HashPair pair, void * param);

/**
 * �Ǘ����Ă��鍀�ڂ����ɕ]������B
 *
 * @param instance			�Ώۃn�b�V���e�[�u���B
 * @param for_function		�����ڏ��֐��|�C���^�B
 * @param param				�p�����[�^�B
 */
void hash_foreach(const Hash * instance, hash_for_function for_function, void * param);

//-----------------------------------------------------------------------------
// �l�ϊ�
//-----------------------------------------------------------------------------
/**
 * �n�b�V���e�[�u���ɓo�^����l���쐬�i�����l�j
 *
 * @param value		�o�^����l�B
 * @return			�l�\���́B
 */
HashValue hash_value_of_integer(int value);

/**
 * �n�b�V���e�[�u���ɓo�^����l���쐬�i�����l�j
 *
 * @param value		�o�^����l�B
 * @return			�l�\���́B
 */
HashValue hash_value_of_float(double value);

/**
 * �n�b�V���e�[�u���ɓo�^����l���쐬�i�|�C���^�j
 *
 * @param value		�o�^����l�B
 * @return			�l�\���́B
 */
HashValue hash_value_of_pointer(void * value);

#endif /* __HASH_H__ */