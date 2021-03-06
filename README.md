# Toml-Parser-By-C

## Overview
TOMLファイルを　**C言語** で読み込み、値を取得するためのライブラリです。
私の Github公開のための学習用ライブラリです。開発は Visual Studio 2017で行っています。
ライセンスは MIT_license としています。

TOMLは initファイルと似た構文で設定情報を記述するための軽量な言語です。
以下に例を示します。詳細の仕様は[公式サイト](https://github.com/toml-lang/toml)を参照ください。

```
title = "TOMLサンプル"

[owner]
name = "Tom Preston-Werner"
dob = 1979-05-27T07:32:00-08:00 # First class dates

[database]
server = "192.168.1.1"
ports = [ 8001, 8001, 8002 ]
connection_max = 5000
enabled = true

[servers]

  # Indentation (tabs and/or spaces) is allowed but not required
  [servers.alpha]
  ip = "10.0.0.1"
  dc = "eqdc10"

  [servers.beta]
  ip = "10.0.0.2"
  dc = "eqdc10"

[clients]
data = [ ["gamma", "delta"], [1, 2] ]

# Line breaks are OK when inside arrays
hosts = [
  "alpha",
  "omega"
]
```

このライブラリでは以下のようにTOMLファイルを読み込みます。

``` C
#include "toml/Toml.h"
#include "helper/Encoding.h"

TomlDocument * toml = toml_initialize();
TomlValue	v;
char buf[128];

// TOMLファイルを読み込みます
toml_read(toml, "test.toml");

// キーを指定して値を取得します（TOMLファイルは UTF8なのでs-jis変換して文字列を出力しています）
v = toml_search_key(toml->table, "title");
printf_s("[ title = %s ]\n", encoding_utf8_to_cp932(v.value.string, buf, sizeof(buf)));

// TOMLファイルを開放します
toml_dispose(&toml);
```

## Description
学習用であるため標準ライブラリ以外は使わないように実装しています。
そのため、実務で使用するには冗長になり使用できないと考えています。

また、UTF8の文字を表現する構造体はリトルエンディアンが前提で実装しているため、ビックエンディアン環境では動作しないと思います。

## Licence
ライセンスは[MITライセンス](https://opensource.org/licenses/mit-license.php)としています。