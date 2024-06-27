#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <gbm.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>


// マクロ関数の定義 ---------------------------------------------------------

#define	lengthof(array)		(sizeof(array) / sizeof(array[0]))
#define my_assert(cond)		my_assert_impl((cond), #cond, __FILE__, __LINE__, __FUNCTION__)

// 型の定義 -----------------------------------------------------------------

// 頂点座標及びテクスチャ座標格納用のデータ型
typedef struct {
	GLfloat	vert_x;		// 頂点 X 座標
	GLfloat	vert_y;		// 頂点 Y 座標
	GLfloat	tex_x;		// テクスチャ X 座標
	GLfloat	tex_y;		// テクスチャ Y 座標
} VERT_TEX_COORD;

// 定数データの定義 ---------------------------------------------------------

// 頂点座標及びテクスチャ座標を格納するためのデータテーブル
static const VERT_TEX_COORD vertices[] = {
	// 頂点座標     テクスチャ座標
	// X   Y        U  V
    { -1,  1,		0, 1 },				// ①左上
    { -1, -1,		0, 0 },				// ②左下
    {  1,  1,		1, 1 },				// ③右上
    {  1, -1,		1, 0 },				// ④右下
};

// EGL フレームバッファ設定を選択するための属性情報リスト
static const EGLint	conf_attrib[] = {
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_DEPTH_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
};

// EGL レンダリングコンテキスト生成時に指定する属性情報リスト
static const EGLint ctx_attrib[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};

// ファイル内グローバル変数の定義 -------------------------------------------

// GBM デバイスオブジェクト
static struct gbm_device	*gbmDevice = NULL;

// GBM サーフェス
static struct gbm_surface	*gbmSurface = NULL;

// EGL ディスプレイコネクション
static EGLDisplay	eglDisplay = NULL;

// EGL レンダリングコンテキスト
static EGLContext	eglContext = NULL;

// EGL サーフェス
static EGLSurface	eglSurface = NULL;

// 非公開関数のプロトタイプ宣言 ---------------------------------------------

static void my_assert_impl(int cond, const char *cond_str, const char *file, int line, const char *func);
static void print_shader_info(const char *msg, GLuint shader);
static void gbm_clean();
static void egl_clean();
static double calc_diff_sec(struct timeval *start, struct timeval *end);


int main(int argc, char *argv[])
{
	int	fd, i, x, y, arg, loop, width = 0, height = 0;
	const char	*dev = "/dev/dri/card0";
	GLint	gl_status;
	GLuint	texture;
	FILE	*fp;
	char	*prog, *in_file = NULL, *p;
	char	out_file[40];
	GLuint	program, vert_shader, frag_shader, vbo;
	GLint	u_texture_loc, a_pos_loc, a_texpos_loc;
	EGLint	count, num_configs, visual_id;
	GLint	viewport[4];
	uint8_t	*rgba_buff = NULL, *rgb_buff = NULL, *src, *dest;
	size_t	rgba_data_size, rgb_data_size;
	double	msec;
	EGLConfig	*configs, egl_config;
	EGLBoolean	result;
	struct timeval	begin, end;

	// コマンドライン引数を評価する。
	prog = basename(argv[0]);
	arg = 1;
	if (arg < argc && strcmp(argv[arg], "-d") == 0) {
		arg++;
		if (arg < argc) dev = argv[arg++];
	}
	if (arg < argc) in_file = argv[arg++];
	if (in_file == NULL) {
		printf("usage: %s [-d <drm-device>] <input-raw-file>\n", prog);
		exit(2);
	}

	// 入力画像ファイルのファイル名から入力画像の画素数を得る。
	// ・想定しているファイル名の型式："xxxxxx-1920x1080.raw"
	p = strrchr(in_file, '.');
	my_assert(p != NULL);
	p--;
	while (isdigit(*p) || *p == 'x') {
		p--;
		my_assert(p >= in_file);
	}
	p++;
	sscanf(p, "%dx%d", &width, &height);
	my_assert(width > 0);
	my_assert(height > 0);

	//=======================================================================
	//  OpenGL ES デバイスの初期化
	//=======================================================================

	// DRM デバイス(GPU)をオープンする。
	fd = open(dev, O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		fprintf(stderr, "%s: cannnot open %s\n", prog, dev);
		exit(3);
	}

	// EGL ディスプレイコネクションを得る。
	gbmDevice = gbm_create_device(fd);
	atexit(gbm_clean);
	eglDisplay = eglGetDisplay(gbmDevice);
	my_assert(eglDisplay != EGL_NO_DISPLAY);
	atexit(egl_clean);

	// EGL を利用して OpenGL ES デバイスを初期化する。
	result = eglInitialize(eglDisplay, NULL, NULL);
	my_assert(result == EGL_TRUE);

	// 使用する OpenGL API を設定する。
	eglBindAPI(EGL_OPENGL_API);

	// 希望条件にマッチする EGL フレームバッファ設定リストを得る。
	eglGetConfigs(eglDisplay, NULL, 0, &count);
	configs = malloc(sizeof(configs) * count);
	result = eglChooseConfig(eglDisplay, conf_attrib, configs, count, &num_configs);
	my_assert(result == EGL_TRUE);

	// 32bit XRGB 形式のフレームバッファ設定を探す。
	for (i = 0; i < num_configs; i++) {
		result = eglGetConfigAttrib(eglDisplay, configs[i], EGL_NATIVE_VISUAL_ID, &visual_id);
		if (result != EGL_TRUE) continue;
		if (visual_id == GBM_FORMAT_XRGB8888) break;
    }
    my_assert(i < count);
    egl_config = configs[i];
	free(configs);
	configs = NULL;

	// レンダリングコンテキストを生成する。
	eglContext = eglCreateContext(eglDisplay, egl_config, EGL_NO_CONTEXT, ctx_attrib);
	my_assert(eglContext != EGL_NO_CONTEXT);

	// 描画先のサーフェス(= フレームバッファ = VRAM)を生成する。
	gbmSurface = gbm_surface_create(gbmDevice, width, height, GBM_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
	eglSurface = eglCreateWindowSurface(eglDisplay, egl_config, gbmSurface, NULL);
	my_assert(eglSurface != EGL_NO_SURFACE);

	// 生成したサーフェスを描画先に設定する。
	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);

	// 描画先の座標範囲(ビューポート)を設定する。
	glViewport(0, 0, width, height);

	// ビューポートが正しく設定されたことを確認する。
	glGetIntegerv(GL_VIEWPORT, viewport);
	my_assert(viewport[2] == width && viewport[3] == height);

	//=======================================================================
	//  GLSL シェーダープログラムの利用準備
	//=======================================================================

	// バーテックシェーダーオブジェクトを生成する。
	vert_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert_shader, 1, &vertex_shader_code, NULL);
	glCompileShader(vert_shader);
	glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &gl_status);
	my_assert(gl_status = GL_TRUE);
	print_shader_info("vertex_shader_code", vert_shader);

	// フラグメントシェーダーオブジェクトを生成する。
	frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag_shader, 1, &fragment_shader_code, NULL);
	glCompileShader(frag_shader);
	glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &gl_status);
	my_assert(gl_status = GL_TRUE);
	print_shader_info("fragment_shader_code", frag_shader);

	// プログラムオブジェクトを生成する。
	program = glCreateProgram();

	// プログラムオブジェクトにシェーダオブジェクトを登録する。
	glAttachShader(program, vert_shader);
	glAttachShader(program, frag_shader);

	// プログラムオブジェクトをリンクする。
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &gl_status);
	my_assert(gl_status = GL_TRUE);

	// リンクしたプログラムオブジェクトを使用する。
	glUseProgram(program);

	//=======================================================================
	//  GPU 描画の準備処理
	//=======================================================================

	// vbo (Vertex Buffer Object) を生成する。
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// テクスチャユニット０を選択する。
	glActiveTexture(GL_TEXTURE0);

	// テクスチャを生成する。
	glGenTextures(1, &texture);

	// 生成したテクスチャを操作対象に設定する。
	glBindTexture(GL_TEXTURE_2D, texture);

	// テクスチャ用の RAW 画像ファイルをメモリに読み込む。
	unsigned char *texture_buff;
	texture_buff = (unsigned char *)malloc(width * height * 3);
	fp = fopen(in_file, "r");
	if (fp == NULL) {
	    fprintf(stderr, "%s: cannot open %s\n", prog, in_file);
	    exit(2);
	}
	fread(texture_buff, 3, width * height, fp);
	fclose(fp);

	// テクスチャ用の画像データを VRAM にコピーする。
	glFinish();
	for (loop = 0; loop < 10; loop++) {
		// 処理開始前の時刻を得る。
		gettimeofday(&begin, NULL);

		// テクスチャデータを VRAM にコピーする。
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture_buff);

		// テクスチャデータのコピー処理が完了するのを待つ。
		glFinish();

		// テクスチャデータのコピーに掛かった時間を表示する。
		gettimeofday(&end, NULL);
		msec = calc_diff_sec(&begin, &end) * 1000.;
		printf("glTexImage2D(): time=%3.1fms\n", msec);
	}

	// テクスチャ読み込み用のメモリを解放する。
	free(texture_buff);

	// テクスチャ参照時の補間方法を指定する。
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// テクスチャの繰り返し方法を指定する。
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// シェーダープログラムに渡すユニフォーム変数のロケーション情報を得る。
	u_texture_loc = glGetUniformLocation(program, "u_texture");

	// シェーダープログラムに渡すアトリビュート変数のロケーション情報を得る。
	a_pos_loc = glGetAttribLocation(program, "a_pos");
	glEnableVertexAttribArray(a_pos_loc);
	a_texpos_loc = glGetAttribLocation(program, "a_texpos");
	glEnableVertexAttribArray(a_texpos_loc);

	//=======================================================================
	//  GPU 描画処理の実行
	//=======================================================================

	// 使用する vbo を選択する。
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	// シェーダーの引数 u_texture にテクスチャユニット番号を設定する。
	glUniform1i(u_texture_loc, 0);

	// アトリビュート変数に渡すデータ配列を定義する。
	glVertexAttribPointer(a_pos_loc, 2, GL_FLOAT, GL_FALSE, sizeof(VERT_TEX_COORD), (void *)0);
	glVertexAttribPointer(a_texpos_loc, 2, GL_FLOAT, GL_FALSE, sizeof(VERT_TEX_COORD), (void *)(sizeof(GLfloat) * 2));

	// 描画先のフレームバッファに四角形領域(三角形✕２)を描画する。
	glFinish();
	for (loop = 0; loop < 10; loop++) {
		// 処理開始前の時刻を得る。
		gettimeofday(&begin, NULL);

		// 描画先のフレームバッファをクリアする。
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 使用するテクスチャを選択する。
		glBindTexture(GL_TEXTURE_2D, texture);

		// フレームバッファに四角形領域(三角形✕２)を描画する。
		glDrawArrays(GL_TRIANGLE_STRIP, 0, lengthof(vertices));

		// 四角形に含まれる全ピクセルに対する描画処理が完了するのを待つ。
		glFinish();

		// 全ピクセルの描画時間を表示する。
		gettimeofday(&end, NULL);
		msec = calc_diff_sec(&begin, &end) * 1000.;
		printf("glDrawArrays(): time=%3.1fms\n", msec);
	}

	// テクスチャオブジェクトを削除する。
	glDeleteTextures(1, &texture);

	//=======================================================================
	//  GPU 処理結果のファイル保存処理
	//=======================================================================

	// フレームバッファから読み出した画像を保存するためのメモリバッファを
	// メインメモリ上に確保する。
	//   ・RGBA バッファ (フレームバッファからコピーしたデータ)
	//   ・RGB バッファ  (ファイルに書き出すデータ)
	rgba_data_size = width * height * 4;
	rgb_data_size = width * height * 3;
	rgba_buff = (uint8_t *)malloc(rgba_data_size);
	rgb_buff = (uint8_t *)malloc(rgb_data_size);

	// フレームバッファから描画後の画像データ(RGBA)を読み出す。
	glFinish();
	for (loop = 0; loop < 10; loop++) {
		// 処理開始前の時刻を得る。
		gettimeofday(&begin, NULL);

		// フレームバッファから画像データを読み出す。
	    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba_buff);

		// 画像読み出し処理が完了するのを待つ。
		glFinish();

		// 画像読み出しに掛かった時間を表示する。
		gettimeofday(&end, NULL);
		msec = calc_diff_sec(&begin, &end) * 1000.;
		printf("glReadPixels(): time=%3.1fms\n", msec);
	}

	// フレームバッファから読み出した RGBA データを RGB データに変換する。
	src = rgba_buff;
	dest = rgb_buff;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			*dest++ = *src++;			// Copy Red data
			*dest++ = *src++;			// Copy Green data
			*dest++ = *src++;			// Copy Blue data
			*src++;						// Skip Alpha channel
		}
	}

	// 変換した RGB データを RAW データファイルに書き出す。
	snprintf(out_file, sizeof(out_file), "%s%dx%d.raw", OUTPUT_FILE_PREFIX, width, height);
	fp = fopen(out_file, "w");
	if (fp == NULL) {
	    fprintf(stderr, "%s: cannot open %s for writing!\n", prog, out_file);
	    exit(2);
	}
    fwrite(rgb_buff, 1, rgb_data_size, fp);
    fclose(fp);
    printf("Saved to %s.\n", out_file);

	//=======================================================================
	//  終了処理
	//=======================================================================

	// 画像データ一時保存用のメモリバッファを解放する。
	free(rgba_buff);
	free(rgb_buff);

	// DRM デバイスをクローズする。
	close(fd);

	return 0;
}

// 自前で作成した assert() 同等の関数
// [補足]
// ・OS 標準の assert() との違いは、assertion fail 時に atexit() で登録して
//   おいた終了処理関数が呼び出されること。
static void my_assert_impl(int cond, const char *cond_str, const char *file, int line, const char *func)
{
	if (!cond) {
		fprintf(stderr, "my_assert(): %s:%d:%s() Assertion \"%s\" failed.\n", file, line, func, cond_str);
		exit(2);
	}
}

// シェーダープログラムのコンパイル時のログメッセージを出力する関数。
static void print_shader_info(const char *msg, GLuint shader)
{
	GLint	info_size;
	char	*buff;

	// シェーダーコンパイル時のログメッセージの長さを得る。
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_size);
	if (info_size == 0) return;

	// シェーダーコンパイル時のログメッセージを出力する。
	buff = (char *)malloc(info_size + 1);
	glGetShaderInfoLog(shader, info_size + 1, NULL, buff);
	printf("== %s: compilation infomation ==\n%s", msg, buff);
	free(buff);
}

// GBM 関連の各種リソースを解放するためのクリーンナップ関数
static void gbm_clean()
{
//	printf("%s() called\n", __FUNCTION__);
	if (gbmSurface) {
//		printf("call gbm_surface_destroy(gbmSurface)\n");
		gbm_surface_destroy(gbmSurface);
	}
	if (gbmDevice) {
//		printf("call gbm_device_destroy(gbmDevice)\n");
		gbm_device_destroy(gbmDevice);
	}
}

// EGL 関連の各種リソースを解放するためのクリーンナップ関数
static void egl_clean()
{
//	printf("%s() called\n", __FUNCTION__);
	if (eglSurface) {
//		printf("call eglDestroySurface(eglDisplay, eglSurface)\n");
		eglDestroySurface(eglDisplay, eglSurface);
	}
	if (eglContext) {
//		printf("call eglDestroyContext(eglDisplay, eglContext)\n");
		eglDestroyContext(eglDisplay, eglContext);
	}
	if (eglDisplay) {
//		printf("call eglTerminate(eglDisplay)\n");
		eglTerminate(eglDisplay);
	}
}

// 経過時間を計算する関数
static double calc_diff_sec(struct timeval *start, struct timeval *end)
{
	if (end->tv_usec < start->tv_usec) {
		end->tv_usec += 1000000;
		end->tv_sec--;
	}
	return end->tv_sec - start->tv_sec + (end->tv_usec - start->tv_usec) / 1000000.;
}

