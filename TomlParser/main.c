#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "toml/Toml.h"

int main(int argc, const char ** argv)
{
	TomlDocument * toml;

	toml = toml_initialize();
	toml_read(toml, "test.toml");
	toml_show(toml);
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
