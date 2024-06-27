// 定数の定義 ---------------------------------------------------------------

#define OUTPUT_FILE_PREFIX	"output-invert-gpu-"

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
	varying vec2		v_texref;	// 入力：描画対象画素に対応するテクスチャ参照座標

	void main()
	{
		// テクスチャを参照して参照座標の画素値を取り出す。
		vec4 pix = texture2D(u_texture, v_texref);

		// 画素値を反転した値を出力画素値として返す。
		gl_FragColor.rgb = vec3(1) - pix.rgb;			// 変更箇所(☆)
	}
);

