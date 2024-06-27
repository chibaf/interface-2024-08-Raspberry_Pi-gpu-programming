// 定数の定義 ---------------------------------------------------------------

#define OUTPUT_FILE_PREFIX	"output-usm-gpu-"

// マクロ関数の定義 ---------------------------------------------------------

#define MAKE_STRING(x)		#x

// GLSL シェーダープログラムのソースコード ----------------------------------

// バーテックスシェーダープログラムのソースコード
const char *vertex_shader_code = MAKE_STRING(
    attribute vec3	a_pos;			// 引数：頂点座標
    attribute vec2	a_texpos;		// 引数：テクスチャ座標
    varying vec2	v_texref;		// 出力：テクスチャ参照座標

    void main()
    {
		// 受け取った頂点座標を描画用の頂点座標としてそのまま返す。
		gl_Position = vec4(a_pos, 1.0);

		// 受け取ったテクスチャ座標をテクスチャ参照座標としてそのまま返す。
		v_texref = a_texpos;
	}
);

// フラグメントシェーダープログラムのソースコード
const char *fragment_shader_code = MAKE_STRING(
	uniform sampler2D	u_texture;	// 引数：テクスチャオブジェクトの ID
	uniform vec2		u_texdel;	// 引数：テクスチャ１画素分の距離
	varying vec2		v_texref;	// 入力：描画対象画素に対応するテクスチャ参照座標

	void main()
	{
		float	threshold = 3.0 / 255.0;	// しきい値
		float	amount = 2.0;				// 適用量(200%)

		// 注目画素を得る。
		vec4 image = texture2D(u_texture, v_texref);

		// ガウシャンフィルタを適用して、ぼかし画像を得る。
		vec4 blurred =
			 image * 0.204
		  + (texture2D(u_texture, v_texref + vec2(-u_texdel.x, 0)) +
			 texture2D(u_texture, v_texref + vec2( u_texdel.x, 0)) +
			 texture2D(u_texture, v_texref + vec2(0, -u_texdel.y)) +
			 texture2D(u_texture, v_texref + vec2(0,  u_texdel.y))  ) * 0.124
		  + (texture2D(u_texture, v_texref + vec2(-u_texdel.x, -u_texdel.y)) +
			 texture2D(u_texture, v_texref + vec2( u_texdel.x, -u_texdel.y)) +
			 texture2D(u_texture, v_texref + vec2(-u_texdel.x,  u_texdel.y)) +
			 texture2D(u_texture, v_texref + vec2( u_texdel.x,  u_texdel.y))  ) * 0.075;

		// ([注目画素] - [ぼかし画像]) * [適用量] を計算して差分値を得る。
		vec4 diff = (image - blurred) * amount;

		// 差分値がしきい値を超える場合は、注目画素に差分値を加える。
#if 1
		if (abs(diff.r) > threshold) image.r += diff.r;
		if (abs(diff.g) > threshold) image.g += diff.g;
		if (abs(diff.b) > threshold) image.b += diff.b;
		image = clamp(image, 0.0, 1.0);
#else
		vec4 mask = vec4(greaterThan(abs(diff), vec4(threshold)));
		image += diff * mask;
		image = clamp(image, 0.0, 1.0);
#endif

		// アンシャープマスク処理結果を返す。
		gl_FragColor = image;
	}
);

