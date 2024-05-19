# DataFrame

DataFrameはC++におけるCSVファイルパーサクラスとして、
PythonにおけるPandasと似た利用方法がとれるライブラリとして開発されました。

## 1. 準備

DataFrameはC++11以降の環境で動作するシングルヘッダーのライブラリとして
開発してされています。(依存ライブラリ等はなく、includeするだけで使えます。)

## 2. 使い方

### 2.1 csvの読み取り

csvの読み取りはread_csvというメソッドを使用します。

``` cpp
auto df = DataFrame::read_csv("hoge.csv");
```

第2引数で以下のように読取オプションをキーバリューで指定できます。

``` cpp
auto df = DataFrame::read_csv(
        "hoge.csv",
        {
            { DataFrame::NEW_LINE, "\n"}, 
            { DataFrame::SEPARATOR, "\t"},
            { DataFrame::HEADER, false}
        }
    );
```

利用できるオプションと、指定しなかった場合のデフォルト値は以下の通りです。

|オプション名|意味|デフォルト値|
|--|--|--|
|NEW_LINE|改行コード|Windowsは"\r\n"(CRLF)、Linuxは"\n"(LF)|
|SEPARATOR|分割文字|","|
|HEADER|ヘッダー行を含んでいるか|true|
|AUTO_TRIME|各要素の前後の空白文字を自動削除するか|true|

読み取ったデータの内容はdescribeで確認することができます。

``` cpp
// hoge.csv
//
// month, tempature, precipitation
//     1,      10.5,           7.5  
//     2,       6.4,           0.0
//     3,       8.1,           5.5
//     4,      10.2,          10.2

auto df = DataFrame::read_csv("hoge.csv");
df.describe();

// header name : {month,tempature,precipitation}
//    row size : 4
// column size : 3
```

## 2.2 要素へのアクセス

要素へのアクセスについては読取のみ可能になっています。(代入不可)
列は列名を指定し、行は行番号を指定することで取得できます。
暗黙型変換をできるようにしているのでそのまま代入できます。(int/double/std::stringのみ)

行番号についてはpandasのように負の値をとった場合は末尾からのインデックスとして取り扱われます。

``` cpp
// hoge.csv
//
// month, tempature, precipitation
//     1,      10.5,           7.5  
//     2,       6.4,           0.0
//     3,       8.1,           5.5
//     4,      10.2,          10.2

auto df = DataFrame::read_csv("hoge.csv");
int i;
double d;
std::string s;

i = df["tempature"][3]; // 10
d = df["tempature"][3]; // 10.2
s = df["tempature"][3]; // "10.2"

// 以下の操作も同義です。
i = df["tempature"][-1]; // 10
d = df["tempature"][-1]; // 10.2
s = df["tempature"][-1]; // "10.2"

```

切り出して別のDataFrameインスタンスにすることもできます。

``` cpp
// hoge.csv
//
// month, tempature, precipitation
//     1,      10.5,           7.5
//     2,       6.4,           0.0
//     3,       8.1,           5.5
//     4,      10.2,          10.2

auto df = DataFrame::read_csv("hoge.csv");
auto df_1 = df[{"tempature", "precipitation"}][{1, 2, 3}];

// result
// tempature, precipitation 
//       6.4,           0.0
//       8.1,           5.5
//      10.2,          10.2

```

a以降b行目までという切り出し方をしたい場合はsliceメソッドを使用してください。

``` cpp
// hoge.csv
//
// month, tempature, precipitation
//     1,      10.5,           7.5
//     2,       6.4,           0.0
//     3,       8.1,           5.5
//     4,      10.2,          10.2

auto df = DataFrame::read_csv("hoge.csv");
auto df_1 = df[{"tempature", "precipitation"}].slice(1, 4);

// result
// tempature, precipitation 
//       6.4,           0.0
//       8.1,           5.5
//      10.2,          10.2

```

対象行または、対象列がすべて同じ型にキャスト可能である場合はvectorコンテナに変換するメソッドを用意しています。

``` cpp
// hoge.csv
//
// month, tempature, precipitation
//     1,      10.5,           7.5
//     2,       6.4,           0.0
//     3,       8.1,           5.5
//     4,      10.2,          10.2

auto df = DataFrame::read_csv("hoge.csv");
std::vector<double> row;
std::vector<double> col;

row = df[0].to_vector<double>(DataFrame::ROW); // { 1.0, 10.5, 7.5}
col = df["month"].to_vector<int>(DataFrame::COLUMN); // { 1, 2, 3, 4}

```

### 2.3 ファイルへの書込

ファイルへの書き込みはto_csvメソッドを使用します。

``` cpp
df.to_csv("piyo.csv");
```