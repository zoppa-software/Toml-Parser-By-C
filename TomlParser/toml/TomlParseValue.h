#ifndef __TOML_PARSE_VALUE_H__
#define __TOML_PARSE_VALUE_H__

// �w�����ő�l
#define EXPO_MAX_RANGE		308

// �w�����ŏ��l
#define EXPO_MIN_RANGE		-308

/**
 * ���l�i�����^�����^���t�j���擾����B
 *
 * @param buffer		�ǂݍ��ݗ̈�B
 * @param point			�J�n�ʒu�B
 * @param next_point	�I���ʒu�i�߂�l�j
 * @param tokenType		�擾�f�[�^�B
 * @param error			�G���[�ڍ׏��B
 * @return				�擾�ł����� 0�ȊO�B
 */
int toml_convert_value(TomlBuffer * buffer, size_t point, size_t * next_point,
								TomlValueType * tokenType, TomlResultSummary * error);

#endif /* __TOML_PARSE_VALUE_H__ */
