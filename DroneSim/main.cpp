#include <DxLib.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include "MyMath.hpp"

using namespace std;

constexpr double Fps = 60; //1秒あたりのフレーム数
constexpr double Cycle = 1.0 / 60000; //微小時間
constexpr int ScreenWidth = 640; //画面の横幅
constexpr int ScreenHeight = 480; //画面の縦幅

DINPUT_JOYSTATE g_joyState; //ジョイパッドの生のデータ
int g_joypadInputState; //押されているボタン
int g_joypadPushedState; //押されていない状態から押されている状態になったボタン

Vector<6> g_x; //システムの状態

//システムのf(g_x)
Vector<6> f(const Vector<6> x)
{
	return Vector<6>{x[1], 0.0, x[3], -1.0, x[5], 0.0};
}

//システムのg(g_x)
Matrix<6, 2> g(const Vector<6> x)
{
	return Matrix<6, 2>{0.0, 0.0, -sin(x[4]), 0.0, 0.0, 0.0, cos(x[4]), 0.0, 0.0, 0.0, 0.0, 1.0};
}

//システムを初期化する
void initializeSystem()
{
	for(int i = 0; i < 6; ++i) {
		g_x[i] = 0.0;
	}
}

//スティックの入力を状態の入力uに変換
Vector<2> convertToInput(DINPUT_JOYSTATE joyState)
{
	return Vector<2>{
		static_cast<double>(joyState.Y) * -0.004,
			static_cast<double>(joyState.Rx) * -0.002,
	};
}

//機体を描画する
void drawDrone(Vector<6> x) {
	//状態を画面上の位置に変換
	//位置(横x[0], 縦x[2], 角度x[4])を
	//状態の長さ1が画面上の長さ100になるようにして
	//上下を反転(画面は下方向がプラスなので)して中心を Width/2, Height/2 にする
	double px = ScreenWidth/2 + x[0]*20, py = ScreenHeight/2 - x[2]*20; //縦、横位置
	double dx = px + cos(x[4]+M_PI/2)*20, dy = py - sin(x[4]+M_PI/2)*20; //進行方向を表す線の縦、横位置

	DrawCircle(static_cast<int>(px), static_cast<int>(py), 8, GetColor(128, 128, 255)); //機体の位置に円を描く
	DrawLine(static_cast<int>(px), static_cast<int>(py),
		static_cast<int>(dx), static_cast<int>(dy), GetColor(255, 255, 0)); //機体の進行方向を向く線を描く
}

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
	// 画面モードのセット
	SetGraphMode(ScreenWidth, ScreenHeight, 16);
	ChangeWindowMode(TRUE);
	if(DxLib_Init() == -1) return -1;
	// 描画先画面を裏画面にセット
	SetDrawScreen(DX_SCREEN_BACK);

	// ループ
	while(ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0) {
		clsDx(); //画面消去

		// コントローラ入力取得
		{
			GetJoypadDirectInputState(DX_INPUT_PAD1, &g_joyState);
			auto pjs = g_joypadInputState;
			g_joypadInputState = GetJoypadInputState(DX_INPUT_PAD1);
			//前フレームの入力(pjs)が0、かつ今回の入力(g_joypadInputState)が1のとき、1
			g_joypadPushedState = ~pjs & g_joypadInputState;
		}

		//Xボタンでリセット
		if(g_joypadPushedState & PAD_INPUT_3) initializeSystem();

		Vector<2> u; //入力

		for (int i = 0; i < 1 / Cycle / Fps; ++i) { //1フレームで1/60
			u = convertToInput(g_joyState); //スティックの入力を状態の入力uに変換

			//状態を更新
			g_x += Cycle * (f(g_x) + g(g_x)*u); //xの微分に微小時間Cycleをかけたものを加える
			g_x[4] = fmod(M_PI*2 + fmod(g_x[4], M_PI*2), M_PI*2); //角度を0~2πの範囲にする
		}

		//画面を初期化する
		ClearDrawScreen();

		// +++ 値を表示する +++
		printfDx("リセットはXボタン\n");
		//状態
		printfDx("g_x = { %4.2f, %4.2f, %4.2f, %4.2f, %4.2f, %4.2f}\n",
			g_x[0], g_x[1], g_x[2], g_x[3], g_x[4], g_x[5]);
		printfDx("u = { %4.2f, %4.2f}\n", u[0], u[1]); //入力
		//ジョイパッドの生の値
		printfDx("joypad = {  X:%5d,  Y:%5d,  Z:%5d,\n           Rx:%5d, Ry:%5d, Rz:%5d }\n",
			g_joyState.X, g_joyState.Y, g_joyState.Z, g_joyState.Rx, g_joyState.Ry, g_joyState.Rz);
		// --- 値を表示する ---

		//機体を描画する
		drawDrone(g_x);

		//裏画面の内容を表画面に反映させる
		ScreenFlip();
	}
	DxLib_End(); // ＤＸライブラリ使用の終了処理

	return 0; // ソフトの終了
}
