#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "toml/Toml.h"

int main(int argc, const char ** argv)
{
	TomlDocument * toml = toml_initialize();

	toml_read(toml, "test.toml");
	toml_show(toml);

	toml_dispose(&toml);
}
