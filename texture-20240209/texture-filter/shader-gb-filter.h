// 定数の定義 ---------------------------------------------------------------

#define OUTPUT_FILE_PREFIX	"output-gb-filter-gpu-"

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
		// テクスチャの参照画素周辺の 3x3 画素にガウシャンフィルタを適用する。
		vec4 pix =
			 texture2D(u_texture, v_texref) * 0.204
		  + (texture2D(u_texture, v_texref + vec2(-u_texdel.x, 0)) +
			 texture2D(u_texture, v_texref + vec2( u_texdel.x, 0)) +
			 texture2D(u_texture, v_texref + vec2(0, -u_texdel.y)) +
			 texture2D(u_texture, v_texref + vec2(0,  u_texdel.y))  ) * 0.124
		  + (texture2D(u_texture, v_texref + vec2(-u_texdel.x, -u_texdel.y)) +
			 texture2D(u_texture, v_texref + vec2( u_texdel.x, -u_texdel.y)) +
			 texture2D(u_texture, v_texref + vec2(-u_texdel.x,  u_texdel.y)) +
			 texture2D(u_texture, v_texref + vec2( u_texdel.x,  u_texdel.y))  ) * 0.075;

		// ガウシャンフィルタ(ぼかし)画像を出力画素値として返す。
		gl_FragColor = pix;
	}
);

