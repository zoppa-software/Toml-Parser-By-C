#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "toml/Toml.h"

static void show_items(TomlTable * table);
static void show_item(TomlBucket item);
static void show_value(TomlValue * obj);

int main(int argc, const char ** argv)
{
	TomlDocument * toml;

	toml = toml_initialize();
	toml_read(toml, "test.toml");
	show_items(toml->table);
	toml_dispose(&toml);

#if 0
	printf_s("---- %s -----\n", "bool.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\bool.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "float.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\float.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "implicit-and-explicit-after.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\implicit-and-explicit-after.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "implicit-and-explicit-before.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\implicit-and-explicit-before.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "implicit-groups.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\implicit-groups.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "integer.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\integer.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "key-special-chars.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\key-special-chars.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "key-equals-nospace.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\key-equals-nospace.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "key-space.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\key-space.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "long-float.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\long-float.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "long-integer.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\long-integer.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "multiline-string.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\multiline-string.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "raw-multiline-string.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\raw-multiline-string.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "raw-string.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\raw-string.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "string-empty.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\string-empty.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "string-escapes.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\string-escapes.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "string-simple.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\string-simple.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "string-with-pound.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\string-with-pound.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "unicode-escape.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\unicode-escape.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "unicode-literal.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\unicode-literal.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "table-with-pound.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\table-with-pound.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "table-whitespace.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\table-whitespace.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "table-sub-empty.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\table-sub-empty.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "table-empty.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\table-empty.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "datetime.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\datetime.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "array-empty.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\array-empty.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "array-nospaces.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\array-nospaces.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "arrays.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\arrays.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "arrays-hetergeneous.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\arrays-hetergeneous.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "arrays-nested.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\arrays-nested.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "comments-everywhere.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\comments-everywhere.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "empty.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\empty.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "example.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\example.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "table-array-implicit.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\table-array-implicit.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "table-array-many.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\table-array-many.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "table-array-nest.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\table-array-nest.toml");
	toml_show(toml);
	toml_dispose(&toml);

	printf_s("---- %s -----\n", "table-array-one.toml");
	toml = toml_initialize();
	toml_read(toml, "tests\\table-array-one.toml");
	toml_show(toml);
	toml_dispose(&toml);
#endif
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

	switch (item.ref_value->value_type)
	{
	case TomlBooleanValue:
		printf_s("%s: %s\n", item.key_name, item.ref_value->value.boolean ? "true" : "false");
		break;
	case TomlStringValue:
		printf_s("%s: %s\n", item.key_name, item.ref_value->value.string);
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

	switch (obj->value_type)
	{
	case TomlBooleanValue:
		printf_s("%s", obj->value.boolean ? "true" : "false");
		break;
	case TomlStringValue:
		printf_s("%s", obj->value.string);
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