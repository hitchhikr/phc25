==============================================================================
                                                                              
 PHC-25 エミュレータ phc25.exe version 1.01 ソースコード                      
                                                                              
                                                      Programmed by たご      
                                                                              
==============================================================================

■はじめに
  これは、PHC-25 エミュレータのソースコードです。
  Visual C++ Ver.6、VisualStudio.NET2002、VisualStudio.NET2003 で
  コンパイルできる事を確認しています。

■使用条件
私の作った部分は悪事に使用しない限りそのまま使っても改変して使っても自由です。
以下の部分については、各作者の使用条件に従ってください。

source\common\ z80.cpp、z80.h

        武田さんが作成した CP/M Player for Win32 の物を少し変更して使っていま
        す。GPL 準拠とのことですので、copying.txt をご確認ください。

source\common\fmgen008 以下すべて

        cisc さんが作成した fmgen の PSG 部分のコードをそのまま使用しています。

source\win32 icon.ico、small.ico

        フルカラーアイコンは天丸さんが作成したアイコン画像をそのまま使用してい
        ます。16x16、16 色、256 色アイコンに関してはたごがリサイズおよび減色し
        ました。

■各作者のページ
武田さんのページ
http://www1.interq.or.jp/~t-takeda/mz2500/index.html
http://www1.interq.or.jp/~t-takeda/cpm/index.html
http://www1.interq.or.jp/~t-takeda/m5/index.html

cisc さんのページ
http://retropc.net/cisc/

天丸さんのページ
http://homepage2.nifty.com/tenpurako/index.html

■履歴
2004-10-23 Ver.1.01
・SCREEN 4 での多色表示に対応した。
・天丸さんのアイコンを使用した。
2004-06-14 Ver.1.00
・公開した。

■連絡先
名前   たご
E-MAIL sanyo_phc_25□□@yahoo.co.jp
URL    http://www.geocities.jp/sanyo_phc_25/
(□□は消してください。ウイルスメール対策です。)
