//////////////////////////////////////////////////////////////////
// full.fx ver1.2+　設定パラメータv1.0　byカユテガ　2018/2/26   //
//////////////////////////////////////////////////////////////////
//※プログラム内は「//」が文の最初に付いている場所以外では全て半角英数字で
//　記入して下さい！！　特にスペースや数字を全角で書く間違いは発生しやすいです！

//▲ノーマルマップ関係/////////////////////////////////////////////////////////
//△ノーマルマップとして使用するテクスチャの枚数を指定します。（※使用したい材質数ではありません）
//１０枚まで可(0～10)
#define MAKE_NORMAL_TEX_COUNT 10

//△ノーマルマップの場所とノーマルマップを使用したい材質の指定
//指定の仕方(※間違えやすいので必ず編集前にファイルをバックアップしてから編集を！)
//はじめに上で指定したテクスチャの枚数分、「使用する材質番号」の項目の
//先頭から「//」を外します。
//※上で指定した枚数分、各行先頭の「//」が無い行の数が一致していないとエラーになります。
//次に使用する材質番号を「""」で囲われた中に半角数字で記入し、
//その材質で使用したいテクスチャの場所(ファイルパス)を記入します。
//この材質番号とテクスチャの場所の記入を上で指定したテクスチャの枚数分、記入して下さい。
//（※ノーマルマップを指定しなかった材質はfull.fxというMMD標準に近い材質になります）
//1枚目
#define USE_NORMAL_MAP_MATERIALS0 "17"         //使用する材質番号
#define MAP_PASS_MATERIALS0 "2b_skirt_01e_n.png"    //テクスチャの場所

//2枚目
#define USE_NORMAL_MAP_MATERIALS1 "9"         //使用する材質番号
#define MAP_PASS_MATERIALS1 "2b_chest_01_n.png"    //テクスチャの場所

//3枚目
#define USE_NORMAL_MAP_MATERIALS2 "10"         //使用する材質番号
#define MAP_PASS_MATERIALS2 "2b_arm_00_n.png"    //テクスチャの場所

//4枚目
#define USE_NORMAL_MAP_MATERIALS3 "22"         //使用する材質番号
#define MAP_PASS_MATERIALS3 "2b_hair_01_n.png"    //テクスチャの場所

//5枚目
#define USE_NORMAL_MAP_MATERIALS4 "20"         //使用する材質番号
#define MAP_PASS_MATERIALS4 "2b_Sashbelt_01_n.png"    //テクスチャの場所

//6枚目
#define USE_NORMAL_MAP_MATERIALS5 "15"         //使用する材質番号
#define MAP_PASS_MATERIALS5 "2b_thigh_high_boots_01_n.png"    //テクスチャの場所

//7枚目
#define USE_NORMAL_MAP_MATERIALS6 "25"         //使用する材質番号
#define MAP_PASS_MATERIALS6 "2b_front_01a_n.png"    //テクスチャの場所

//8枚目
#define USE_NORMAL_MAP_MATERIALS7 "24"         //使用する材質番号
#define MAP_PASS_MATERIALS7 "2b_back_03_n.png"    //テクスチャの場所

//9枚目
#define USE_NORMAL_MAP_MATERIALS8 "8"         //使用する材質番号
#define MAP_PASS_MATERIALS8 "2b_leotard_02_n.png"    //テクスチャの場所

//10枚目
#define USE_NORMAL_MAP_MATERIALS9 "18"         //使用する材質番号
#define MAP_PASS_MATERIALS9 "2b_skirt_01e_n.png"    //テクスチャの場所

//●テクスチャフィルタ一括設定////////////////////////////////////////////////////////////////////
//○異方向性フィルタ（なめらか表示＆遠くても綺麗　※TEXFILTER_MAXANISOTROPYも設定しましょう）
//#define TEXFILTER_SETTING ANISOTROPIC
//バイリニア（なめらか表示：標準）
#define TEXFILTER_SETTING LINEAR
//ニアレストネイバー（くっきり表示：テクスチャのドットが綺麗に出る）
//#define TEXFILTER_SETTING POINT

//○異方向性フィルタを使うときの倍率
//1:Off or 2の倍数で指定(大きいほどなめらか効果・負荷大)
//対応しているかどうかは、ハードウェア次第（RadeonHD機種なら16までは対応しているはず
#define TEXFILTER_MAXANISOTROPY 16

//異方向性フィルタについて
//テクスチャのMiplevelsがある程度あるとより綺麗になります。
//※テクスチャ読み込みもMMEで書く必要があります
//2の累乗サイズのテクスチャであると比較的古いハードウェアにも優しいです。

//■テクスチャアドレッシング一括設定//////////////////////////////////////////////////////////////
//テクスチャ別に設定したいときは、サンプラーステート記述部で行いましょう。
//□U軸ループ
#define TEXADDRESS_U WRAP 
//□U軸ミラー＆ループ
//#define TEXADDRESS_U MIRROR
//□U軸0to1制限：MMD標準
//#define TEXADDRESS_U CLAMP
//□U軸-1to1制限：-方向はミラーリング
//#define TEXADDRESS_U MIRRORONCE

//□V軸ループ
#define TEXADDRESS_V WRAP 
//□V軸ミラー＆ループ
//#define TEXADDRESS_V MIRROR
//□V軸0to1制限：MMD標準
//#define TEXADDRESS_V CLAMP
//□V軸-1to1制限：-方向はミラーリング
//#define TEXADDRESS_V MIRRORONCE

//★「full(NormalMap)_CORE.fx」のファイルパス(場所)を記入//////////////////////////////////////////
#include "full(NormalMap)_CORE.fx"