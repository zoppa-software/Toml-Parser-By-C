#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "toml/Toml.h"
#include "helper/Encoding.h"

static void suc_tml(const char * path);
static void test_tml(const char * path);
static void show_items(TomlTable * table);
static void show_item(TomlBucket item);
static void show_value(TomlValue * obj);

int main(int argc, const char ** argv)
{
#if 1
	TomlDocument * toml = toml_initialize();
	TomlValue	v;
	char buf[128];

	toml_read(toml, "test.toml");
	show_items(toml->table);

	v = toml_search_key(toml, toml->table, "title");
	printf_s("[ title = %s ]\n", encoding_utf8_to_cp932(v.value.string, buf, sizeof(buf)));

	toml_dispose(&toml);
#endif

#if 0
	suc_tml("bool.toml");
	suc_tml("float.toml");
	suc_tml("implicit-and-explicit-after.toml");
	suc_tml("implicit-and-explicit-before.toml");
	suc_tml("implicit-groups.toml");
	suc_tml("integer.toml");
	suc_tml("key-special-chars.toml");
	suc_tml("key-equals-nospace.toml");
	suc_tml("key-space.toml");
	suc_tml("long-float.toml");
	suc_tml("long-integer.toml");
	suc_tml("multiline-string.toml");
	suc_tml("raw-multiline-string.toml");
	suc_tml("raw-string.toml");
	suc_tml("string-empty.toml");
	suc_tml("string-escapes.toml");
	suc_tml("string-simple.toml");
	suc_tml("string-with-pound.toml");
	suc_tml("unicode-escape.toml");
	suc_tml("unicode-literal.toml");
	suc_tml("table-with-pound.toml");
	suc_tml("table-whitespace.toml");
	suc_tml("table-sub-empty.toml");
	suc_tml("table-empty.toml");
	suc_tml("datetime.toml");
	suc_tml("array-empty.toml");
	suc_tml("array-nospaces.toml");
	suc_tml("arrays.toml");
	suc_tml("arrays-hetergeneous.toml");
	suc_tml("arrays-nested.toml");
	suc_tml("comments-everywhere.toml");
	suc_tml("empty.toml");
	suc_tml("example.toml");
	suc_tml("table-array-implicit.toml");
	suc_tml("table-array-many.toml");
	suc_tml("table-array-nest.toml");
	suc_tml("table-array-one.toml");
#endif

#if 0
	test_tml("array-mixed-types-arrays-and-ints.toml");
	test_tml("array-mixed-types-ints-and-floats.toml");
	test_tml("array-mixed-types-strings-and-ints.toml");
	test_tml("datetime-malformed-no-leads.toml");
	test_tml("datetime-malformed-no-secs.toml");
	test_tml("datetime-malformed-no-t.toml");
	test_tml("datetime-malformed-with-milli.toml");
	test_tml("duplicate-keys.toml");
	test_tml("duplicate-key-table.toml");
	test_tml("duplicate-tables.toml");
	test_tml("empty-implicit-table.toml");
	test_tml("empty-table.toml");
	test_tml("float-no-leading-zero.toml");
	test_tml("float-no-trailing-digits.toml");
	test_tml("key-empty.toml");
	test_tml("key-hash.toml");
	test_tml("key-newline.toml");
	test_tml("key-open-bracket.toml");
	test_tml("key-single-open-bracket.toml");
	test_tml("key-space.toml");
	test_tml("key-start-bracket.toml");
	test_tml("key-two-equals.toml");
	test_tml("string-bad-byte-escape.toml");
	test_tml("string-bad-escape.toml");
	test_tml("string-byte-escapes.toml");
	test_tml("string-no-close.toml");
	test_tml("table-array-implicit.toml");
	test_tml("table-array-malformed-bracket.toml");
	test_tml("table-array-malformed-empty.toml");
	test_tml("table-empty.toml");
	test_tml("table-nested-brackets-close.toml");
	test_tml("table-nested-brackets-open.toml");
	test_tml("table-whitespace.toml");
	test_tml("table-with-pound.toml");
	test_tml("text-after-array-entries.toml");
	test_tml("text-after-integer.toml");
	test_tml("text-after-string.toml");
	test_tml("text-after-table.toml");
	test_tml("text-before-array-separator.toml");
	test_tml("text-in-array.toml");
#endif
}

static void suc_tml(const char * path)
{
	TomlDocument * toml;
	char	buf[256];
	printf_s("---- %s -----\n", path);
	sprintf_s(buf, sizeof(buf) - 1, "tests\\%s", path);

	toml = toml_initialize();
	toml_read(toml, buf);
	show_items(toml->table);
	toml_dispose(&toml);
}

static void test_tml(const char * path)
{
	TomlDocument * toml;
	char	buf[256];
	printf_s("---- %s -----\n", path);
	sprintf_s(buf, sizeof(buf) - 1, "invalid\\%s", path);

	toml = toml_initialize();
	toml_read(toml, buf);
	show_items(toml->table);
	toml_dispose(&toml);
}

static void show_items(TomlTable * table)
{
	size_t		i;
	TomlBuckets buckets = toml_collect_key_and_value(table);
	for (i = 0; i < buckets.length; ++i) {
		show_item(buckets.values[i]);
	}
	toml_delete_key_and_value(&buckets);
}

static void show_item(TomlBucket item)
{
	size_t	i;
	char	buf[256];

	switch (item.ref_value->value_type)
	{
	case TomlBooleanValue:
		printf_s("%s: %s\n", item.key_name, item.ref_value->value.boolean ? "true" : "false");
		break;
	case TomlStringValue:
		printf_s("%s: %s\n", item.key_name, encoding_utf8_to_cp932(item.ref_value->value.string, buf, sizeof(buf)));
		break;
	case TomlIntegerValue:
		printf_s("%s: %lld\n", item.key_name, item.ref_value->value.integer);
		break;
	case TomlFloatValue:
		printf_s("%s: %g\n", item.key_name, item.ref_value->value.float_number);
		break;
	case TomlDateValue:
		{
			TomlDate * date = item.ref_value->value.date;
			printf_s("%s: %04d-%02d-%02d %02d:%02d:%02d.%d %02d:%02d\n",
					item.key_name,
					date->year, date->month, date->day,
					date->hour, date->minute, date->second, date->dec_second,
					date->zone_hour, date->zone_minute);
		}
		break;
	case TomlTableValue:
		printf_s("%s: {\n", item.key_name);
		show_items(item.ref_value->value.table);
		printf_s("}\n");
		break;
	case TomlTableArrayValue:
		printf_s("%s: [\n", item.key_name);
		for (i = 0; i < item.ref_value->value.tbl_array->tables->length; ++i) {
			printf_s("{\n");
			show_items(VEC_GET(TomlTable*, item.ref_value->value.tbl_array->tables, i));
			printf_s("}\n");
		}
		printf_s("]\n");
		break;
	case TomlArrayValue:
		printf_s("%s: [\n", item.key_name);
		for (i = 0; i < item.ref_value->value.array->array->length; ++i) {
			show_value(VEC_GET(TomlValue*, item.ref_value->value.array->array, i));
			printf_s(", ");
		}
		printf_s("\n]\n");
		break;
	default:
		break;
	}
}

static void show_value(TomlValue * obj)
{
	size_t	i;
	TomlDate * date;
	char	buf[256];

	switch (obj->value_type)
	{
	case TomlBooleanValue:
		printf_s("%s", obj->value.boolean ? "true" : "false");
		break;
	case TomlStringValue:
		printf_s("%s", encoding_utf8_to_cp932(obj->value.string, buf, sizeof(buf)));
		break;
	case TomlIntegerValue:
		printf_s("%lld", obj->value.integer);
		break;
	case TomlFloatValue:
		printf_s("%g", obj->value.float_number);
		break;
	case TomlDateValue:
		date = obj->value.date;
		printf_s("%04d-%02d-%02d %02d:%02d:%02d.%d %02d:%02d",
				 date->year, date->month, date->day,
				 date->hour, date->minute, date->second, date->dec_second,
				 date->zone_hour, date->zone_minute);
		break;
	case TomlTableValue:
		printf_s("{\n");
		show_items(obj->value.table);
		printf_s("}\n");
		break;
	case TomlTableArrayValue:
		printf_s("[\n");
		for (i = 0; i < obj->value.tbl_array->tables->length; ++i) {
			printf_s("{\n");
			show_items(VEC_GET(TomlTable*, obj->value.tbl_array->tables, i));
			printf_s("}\n");
		}
		printf_s("]\n");
		break;
	case TomlArrayValue:
		printf_s("[");
		for (i = 0; i < obj->value.array->array->length; ++i) {
			show_value(VEC_GET(TomlValue*, obj->value.array->array, i));
			printf_s(", ");
		}
		printf_s("]\n");
		break;
	default:
		break;
	}
}